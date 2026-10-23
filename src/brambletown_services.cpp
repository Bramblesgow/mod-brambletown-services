/*
 * Brambletown Services - temporary NPC summon coordinator.
 * Copyright (C) 2026 Brambletown contributors.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Temporary NPC coordinator; no database writes.
 * Source inspirations: aditbaco/mod-npc-spawn (AGPL-3.0).
 */
#include "ScriptMgr.h"
#include "CommandScript.h"
#include "Chat.h"
#include "Configuration/Config.h"
#include "Player.h"
#include "WorldSession.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "Map.h"
#include "ObjectGuid.h"
#include "DBCStores.h"
#include "brambletown_provider_summons.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Acore::ChatCommands;

namespace
{
    using Clock = std::chrono::steady_clock;

    // Provider 1 is the Steward; 2-5 are summoned from its gossip menu.
    // The fixed upper bound must match that menu and the access-guarded NPCs.
    constexpr uint32 MaxProviders = 5;

    struct Provider
    {
        uint32 entry = 0;
        uint32 allianceEntry = 0;
        uint32 hordeEntry = 0;
        uint32 displayId = 0; // 0 preserves the creature template model.
        float xOffset = 0.0f;
        float yOffset = 0.0f;
    };

    struct ActiveGroup
    {
        Clock::time_point expiresAt;
        uint32 mapId = 0;
        uint32 instanceId = 0;
        ObjectGuid stewardGuid;
        std::vector<ObjectGuid> creatureGuids;
        // Keys are provider numbers; retaining keys prevents duplicate calls even
        // if a provider despawns or is killed before the Steward expires.
        std::unordered_set<uint32> requestedProviders;
    };

    // Command, gossip and logout callbacks execute on AzerothCore's map-thread
    // player path. Never store raw Player/Creature pointers between callbacks.
    std::unordered_map<uint32, ActiveGroup> ActiveGroups;
    std::mutex ActiveGroupsMutex; // Guards all accesses to ActiveGroups.

    uint32 GetDurationSeconds()
    {
        return std::clamp(
            sConfigMgr->GetOption<uint32>("BrambleServices.DurationSeconds", 120u),
            30u, 600u);
    }

    std::string DurationLabel(uint32 secs)
    {
        uint32 const minutes = secs / 60;
        uint32 const remainder = secs % 60;
        if (!minutes)
            return std::to_string(secs) + (secs == 1 ? " second" : " seconds");

        std::string result = std::to_string(minutes) + (minutes == 1 ? " minute" : " minutes");
        if (remainder)
            result += " " + std::to_string(remainder) + (remainder == 1 ? " second" : " seconds");
        return result;
    }

    // Only dereference a creature GUID through the player's current map;
    // changing maps may make the old map inaccessible. Timed despawn remains
    // the fallback for map transfers that do not invoke BeforeTeleport.
    void DespawnOwned(Player* player, ActiveGroup const& group)
    {
        if (!player || !player->GetMap() ||
            player->GetMapId() != group.mapId ||
            player->GetMap()->GetInstanceId() != group.instanceId)
            return;

        for (ObjectGuid const& guid : group.creatureGuids)
            if (Creature* creature = player->GetMap()->GetCreature(guid))
                creature->DespawnOrUnsummon();
    }

    // Caller holds ActiveGroupsMutex.
    void PurgeExpired(Clock::time_point now)
    {
        for (auto it = ActiveGroups.begin(); it != ActiveGroups.end();)
        {
            if (it->second.expiresAt <= now)
                it = ActiveGroups.erase(it);
            else
                ++it;
        }
    }

    bool CanSpawn(Player const* player, ChatHandler* handler)
    {
        if (!player || !player->IsInWorld())
        {
            handler->SendSysMessage("Services cannot be summoned: your character is not in the world.");
            return false;
        }

        if (!player->IsAlive())
        {
            handler->SendSysMessage("You must be alive to summon services.");
            return false;
        }

        if (player->InArena())
        {
            handler->SendSysMessage("Services cannot be summoned in arenas.");
            return false;
        }

        if (player->InBattleground())
        {
            handler->SendSysMessage("Services cannot be summoned in battlegrounds.");
            return false;
        }

        if (!sConfigMgr->GetOption<bool>("BrambleServices.AllowInCombat", false) &&
            player->IsInCombat())
        {
            handler->SendSysMessage("Services cannot be summoned while in combat.");
            return false;
        }

        if (!sConfigMgr->GetOption<bool>("BrambleServices.AllowInInstances", false))
        {
            Map const* map = player->GetMap();
            if (!map)
            {
                handler->SendSysMessage("Services cannot be summoned: map information is unavailable.");
                return false;
            }

            if (map->IsRaid())
            {
                handler->SendSysMessage("Services cannot be summoned inside raids.");
                return false;
            }

            if (map->IsDungeon())
            {
                handler->SendSysMessage("Services cannot be summoned inside dungeons.");
                return false;
            }

            if (map->Instanceable())
            {
                handler->SendSysMessage("Services cannot be summoned in this instance.");
                return false;
            }
        }

        return true;
    }

    bool ProviderEnabled(uint32 index)
    {
        if (!index || index > MaxProviders ||
            index > sConfigMgr->GetOption<uint32>("BrambleServices.ProviderCount", MaxProviders))
            return false;

        return sConfigMgr->GetOption<bool>(
            "BrambleServices.Provider." + std::to_string(index) + ".Enable", false);
    }

    // Loads one provider only. A disabled/broken optional provider cannot prevent
    // the Steward from being summoned.
    bool ReadProvider(uint32 index, Provider& provider)
    {
        if (!ProviderEnabled(index))
            return false;
        std::string const key = "BrambleServices.Provider." + std::to_string(index);
        provider.entry = sConfigMgr->GetOption<uint32>(key + ".Entry", 0u);
        if (index == 4)
        {
            provider.allianceEntry = sConfigMgr->GetOption<uint32>(key + ".AllianceEntry", 910104u);
            provider.hordeEntry = sConfigMgr->GetOption<uint32>(key + ".HordeEntry", 910105u);
        }
        // Brambletown auctioneer entries are fixed because the core native
        // interaction guard protects exactly these reviewed template IDs.
        if ((index == 1 && provider.entry != 910101u) ||
            (index == 2 && provider.entry != 910102u) ||
            (index == 3 && provider.entry != 910103u) ||
            (index == 4 && (provider.allianceEntry != 910104u || provider.hordeEntry != 910105u)) ||
            (index == 5 && provider.entry != 910106u))
            return false;
        provider.displayId = sConfigMgr->GetOption<uint32>(key + ".DisplayId", 0u);
        provider.xOffset = sConfigMgr->GetOption<float>(key + ".OffsetX", 0.0f);
        provider.yOffset = sConfigMgr->GetOption<float>(key + ".OffsetY", 0.0f);

        return (index == 4 ? (provider.allianceEntry && provider.hordeEntry) : bool(provider.entry)) &&
            (!provider.displayId || sCreatureDisplayInfoStore.LookupEntry(provider.displayId)) &&
            std::isfinite(provider.xOffset) && std::isfinite(provider.yOffset) &&
            std::abs(provider.xOffset) <= 8.0f && std::abs(provider.yOffset) <= 8.0f;
    }

    Creature* SpawnAt(Player* player, WorldObject const* anchor, Provider const& provider, uint32 millis)
    {
        float const angle = anchor->GetOrientation();
        float const x = anchor->GetPositionX() + provider.xOffset * std::cos(angle) -
            provider.yOffset * std::sin(angle);
        float const y = anchor->GetPositionY() + provider.xOffset * std::sin(angle) +
            provider.yOffset * std::cos(angle);

        // SummonCreature performs the core's normal summon validation. The
        // configured offset is limited above to avoid distant placements.
        // Choose the faction-specific *NPC*, never an auction-house ID.
        // AzerothCore itself routes this NPC to neutral/shared or factional
        // auction pools according to AllowTwoSide.Interaction.Auction.
        uint32 selectedEntry = provider.entry;
        if (provider.allianceEntry && provider.hordeEntry)
        {
            if (player->GetTeamId() == TEAM_ALLIANCE)
                selectedEntry = provider.allianceEntry;
            else if (player->GetTeamId() == TEAM_HORDE)
                selectedEntry = provider.hordeEntry;
            else
                return nullptr;
        }
        Creature* creature = player->SummonCreature(selectedEntry, x, y,
            anchor->GetPositionZ(), angle, TEMPSUMMON_TIMED_DESPAWN, millis);
        if (creature && provider.displayId)
            creature->SetDisplayId(provider.displayId);
        return creature;
    }

    // Returns a pointer into ActiveGroups: caller must hold ActiveGroupsMutex
    // throughout use of the returned group.
    ActiveGroup* FindCurrentGroup(Player* player, Creature* steward)
    {
        if (!player || !steward || !steward->IsInWorld() || !player->GetMap() ||
            steward->GetMap() != player->GetMap() || !player->IsInWorld())
            return nullptr;

        auto it = ActiveGroups.find(player->GetGUID().GetCounter());
        if (it == ActiveGroups.end() || it->second.expiresAt <= Clock::now() ||
            it->second.stewardGuid != steward->GetGUID() ||
            it->second.mapId != player->GetMapId() ||
            it->second.instanceId != player->GetMap()->GetInstanceId())
            return nullptr;
        return &it->second;
    }

    bool SummonServices(ChatHandler* handler)
    {
        // ChatHandler prints a generic USAGE message if a command returns
        // false. These cases all provide their own user-facing rejection or
        // cooldown message, so they are *handled* command results (true).
        // CanSpawn() still returns false as an internal eligibility predicate.
        if (!sConfigMgr->GetOption<bool>("BrambleServices.Enable", false))
        {
            handler->SendSysMessage("Services are currently unavailable.");
            return true;
        }

        WorldSession* session = handler->GetSession();
        if (!session || session->IsHeadless())
        {
            handler->SendSysMessage("Only real player characters can summon services.");
            return true;
        }

        // Registered at SEC_PLAYER so .reload config can switch GM-only mode
        // without rebuilding command registrations or restarting worldserver.
        if (sConfigMgr->GetOption<bool>("BrambleServices.Access.GMOnly", true) &&
            session->GetSecurity() < SEC_GAMEMASTER)
        {
            handler->SendSysMessage("Services are currently restricted to Game Masters.");
            return true;
        }

        Player* player = session->GetPlayer();
        if (!CanSpawn(player, handler))
            return true;
        if (!player->GetMap())
        {
            handler->SendSysMessage("Services cannot be summoned: map information is unavailable.");
            return true;
        }
        if (!ProviderEnabled(1))
        {
            handler->SendSysMessage("The Steward service is disabled.");
            return true;
        }
        Provider stewardProvider;
        if (!ReadProvider(1, stewardProvider))
        {
            handler->SendSysMessage("Services are unavailable due to a server configuration issue.");
            return true;
        }

        // Reserve the cooldown while holding the registry lock, then perform
        // world operations without holding a global lock across core callbacks.
        auto const now = Clock::now();
        uint32 const ownerKey = player->GetGUID().GetCounter();
        std::optional<ActiveGroup> expiredGroup;
        uint32 cooldownSeconds = 0;
        bool onCooldown = false;
        uint32 const duration = GetDurationSeconds();
        {
            std::lock_guard<std::mutex> lock(ActiveGroupsMutex);
            auto expired = ActiveGroups.find(ownerKey);
            if (expired != ActiveGroups.end() && expired->second.expiresAt <= now)
            {
                expiredGroup = std::move(expired->second);
                ActiveGroups.erase(expired);
            }
            PurgeExpired(now);
            auto existing = ActiveGroups.find(ownerKey);
            if (existing != ActiveGroups.end())
            {
                auto const remainingMs =
                    std::chrono::duration_cast<std::chrono::milliseconds>(existing->second.expiresAt - now).count();
                cooldownSeconds = static_cast<uint32>((remainingMs + 999) / 1000);
                onCooldown = true;
            }
            else
            {
                ActiveGroup pending;
                pending.expiresAt = now + std::chrono::seconds(duration);
                pending.mapId = player->GetMapId();
                pending.instanceId = player->GetMap()->GetInstanceId();
                pending.creatureGuids.reserve(MaxProviders);
                pending.requestedProviders.insert(1);
                ActiveGroups.emplace(ownerKey, std::move(pending));
            }
        }
        if (expiredGroup)
            DespawnOwned(player, *expiredGroup);
        if (onCooldown)
        {
            handler->PSendSysMessage("{} seconds until services can be called.", cooldownSeconds);
            return true;
        }

        // The initial command summons only Provider 1. The reserved record
        // blocks repeated commands while its world summon is in flight.
        Creature* steward = SpawnAt(player, player, stewardProvider, duration * 1000u);
        bool committed = false;
        {
            std::lock_guard<std::mutex> lock(ActiveGroupsMutex);
            auto it = ActiveGroups.find(ownerKey);
            if (it != ActiveGroups.end() && it->second.expiresAt == now + std::chrono::seconds(duration) &&
                !it->second.stewardGuid && it->second.mapId == player->GetMapId() &&
                it->second.instanceId == player->GetMap()->GetInstanceId())
            {
                if (steward)
                {
                    it->second.stewardGuid = steward->GetGUID();
                    it->second.creatureGuids.push_back(it->second.stewardGuid);
                    committed = true;
                }
                else
                    ActiveGroups.erase(it); // Summon failed: no cooldown.
            }
        }
        if (!committed)
        {
            if (steward)
                steward->DespawnOrUnsummon();
            handler->SendSysMessage("The Steward could not be summoned. Contact a server administrator.");
            return true;
        }
        handler->PSendSysMessage("Steward summoned for {}. Ask the Steward to call other providers.", DurationLabel(duration));
        return true;
    }
}

namespace BrambletownServices
{
    bool IsProviderEnabled(uint32 index)
    {
        return index >= 2 && ProviderEnabled(index);
    }

    bool IsStewardOwner(Player* player, Creature* steward)
    {
        std::lock_guard<std::mutex> lock(ActiveGroupsMutex);
        return FindCurrentGroup(player, steward) != nullptr;
    }

    bool SummonRequestedProvider(Player* player, Creature* steward, uint32 index)
    {
        if (!player || !player->GetSession() || player->GetSession()->IsHeadless())
            return false;
        ChatHandler handler(player->GetSession());
        if (index < 2 || index > MaxProviders ||
            !sConfigMgr->GetOption<bool>("BrambleServices.Enable", false) ||
            !ProviderEnabled(index))
        {
            handler.SendNotification("That provider is not available.");
            return false;
        }
        // Enforced independently of gossip-menu visibility and the Steward's
        // own action guards to reject forged or stale provider requests.
        if ((sConfigMgr->GetOption<bool>("BrambleServices.Access.GMOnly", true) &&
            player->GetSession()->GetSecurity() < SEC_GAMEMASTER) ||
            !player->IsAlive() || player->InArena() || player->InBattleground() ||
            (player->IsInCombat() && !sConfigMgr->GetOption<bool>("BrambleServices.Interaction.AllowInCombat", false)) ||
            (!sConfigMgr->GetOption<bool>("BrambleServices.AllowInInstances", false) &&
             player->GetMap() && player->GetMap()->Instanceable()))
            return false;

        Provider provider;
        if (!ReadProvider(index, provider))
        {
            handler.SendNotification("Provider configuration is invalid. Contact a server administrator.");
            return false;
        }

        uint32 remainingMs = 0;
        ObjectGuid stewardGuid;
        {
            std::lock_guard<std::mutex> lock(ActiveGroupsMutex);
            ActiveGroup* group = FindCurrentGroup(player, steward);
            if (!group)
            {
                handler.SendNotification("Only the player who called this Steward can summon providers.");
                return false;
            }
            if (group->requestedProviders.contains(index))
            {
                handler.SendNotification("This provider was already summoned by your Steward.");
                return false;
            }
            auto const remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                group->expiresAt - Clock::now()).count();
            if (remaining <= 0)
            {
                handler.SendNotification("This Steward's service period has expired.");
                return false;
            }
            remainingMs = static_cast<uint32>(remaining);
            stewardGuid = group->stewardGuid;
            group->requestedProviders.insert(index); // Reserve once, before unlocking.
        }

        Creature* summoned = SpawnAt(player, steward, provider, remainingMs);
        bool committed = false;
        {
            std::lock_guard<std::mutex> lock(ActiveGroupsMutex);
            ActiveGroup* group = FindCurrentGroup(player, steward);
            if (group && group->stewardGuid == stewardGuid)
            {
                if (summoned)
                {
                    group->creatureGuids.push_back(summoned->GetGUID());
                    committed = true;
                }
                else
                    group->requestedProviders.erase(index); // Failed summon can be retried.
            }
        }
        if (!committed)
        {
            if (summoned)
                summoned->DespawnOrUnsummon();
            handler.SendNotification("Provider summon failed. Contact a server administrator.");
            return false;
        }
        handler.PSendSysMessage("Provider {} summoned for the remainder of the Steward's service period.", index);
        return true;
    }
}

class BrambletownServicesCommands : public CommandScript
{
public:
    BrambletownServicesCommands() : CommandScript("BrambletownServicesCommands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable table = {
            {"services", SummonServices, SEC_PLAYER, Console::No}
        };
        return table;
    }
};

// Cleanup while the player still has a valid world/map reference.
// Cooldown remains linked to the original group lifetime even if NPCs vanish early.
class BrambletownServicesPlayerScript : public PlayerScript
{
public:
    BrambletownServicesPlayerScript()
        : PlayerScript("BrambletownServicesPlayerScript",
            {PLAYERHOOK_ON_BEFORE_LOGOUT, PLAYERHOOK_ON_BEFORE_TELEPORT}) { }

    void OnPlayerBeforeLogout(Player* player) override
    {
        if (player)
            DespawnPlayerGroup(player);
    }

    bool OnPlayerBeforeTeleport(Player* player, uint32 /*mapid*/, float /*x*/, float /*y*/,
        float /*z*/, float /*orientation*/, uint32 /*options*/, Unit* /*target*/) override
    {
        if (player)
            DespawnPlayerGroup(player);
        return true;
    }

private:
    static void DespawnPlayerGroup(Player* player)
    {
        std::optional<ActiveGroup> group;
        {
            std::lock_guard<std::mutex> lock(ActiveGroupsMutex);
            auto it = ActiveGroups.find(player->GetGUID().GetCounter());
            if (it != ActiveGroups.end())
                group = it->second; // Cooldown survives logout / teleport as before.
        }
        if (group)
            DespawnOwned(player, *group);
    }
};

void AddBrambletownServicesScripts()
{
    new BrambletownServicesCommands();
    new BrambletownServicesPlayerScript();
}
