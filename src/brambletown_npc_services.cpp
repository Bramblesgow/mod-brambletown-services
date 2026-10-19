/*
**  Written by MtgCore
**  Rewritten by Poszer & Talamortis https://github.com/poszer/ & https://github.com/talamortis/
**  AzerothCore +2019 http://www.azerothcore.org/
**  Cleaned and made into a module by Micrah https://github.com/milestorme/
*/

#include "brambletown_npc_services.h"
#include "brambletown_provider_summons.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "WorldSession.h"
#include "Map.h"
#include "ObjectGuid.h"
#include "GameObject.h"
#include "ScriptedCreature.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "SpellAuras.h"
#include "PlayerbotMgr.h"
#include "PlayerbotAI.h"
#include "ChatHelper.h"
#include "Event.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <initializer_list>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
using FeastClock = std::chrono::steady_clock;
// Prevent repeated Fish Feast summons through a single Steward while alive.
std::unordered_map<ObjectGuid, FeastClock::time_point> ServedFeasts;
std::mutex ServedFeastsMutex; // Guards one-feast-per-Steward across map threads.

// Rotate announcements across feast placements, not within a single feast.
// Atomic counter protects the rotation when Stewards occupy different maps.
constexpr std::array<std::string_view, 5> FeastAnnouncements{{
    "Fish Feast down! Get it while it's warm!",
    "Fresh Fish Feast! Come grab a plate!",
    "Feast is served! Everyone, dig in!",
    "Food's up! Don't let it go to waste!",
    "A Fish Feast for the crew! Help yourselves!"
}};
std::atomic<uint32> NextFeastAnnouncement{0};

// Caller holds ServedFeastsMutex.
void PurgeExpiredFeastEntries(FeastClock::time_point now)
{
    for (auto it = ServedFeasts.begin(); it != ServedFeasts.end();)
        if (it->second <= now)
            it = ServedFeasts.erase(it);
        else
            ++it;
}

// Enforce usage restrictions on both menu opening and action selection.
bool CanInteract(Player* player)
{
    if (!player || !player->GetSession() || player->GetSession()->IsHeadless())
        return false;

    if (!sConfigMgr->GetOption<bool>("BrambleServices.Enable", false))
        return false;

    // Applying the check at interaction-time prevents another player from
    // using a GM's already-summoned General Services NPC.
    if (sConfigMgr->GetOption<bool>("BrambleServices.Access.GMOnly", true) &&
        player->GetSession()->GetSecurity() < SEC_GAMEMASTER)
    {
        ChatHandler(player->GetSession()).SendNotification("Services are restricted to Game Masters.");
        return false;
    }

    if (!player->IsAlive())
    {
        ChatHandler(player->GetSession()).SendNotification("You must be alive to use services.");
        return false;
    }

    if (player->InArena() || player->InBattleground())
    {
        ChatHandler(player->GetSession()).SendNotification("Services cannot be used in PvP instances.");
        return false;
    }

    if (!sConfigMgr->GetOption<bool>("BrambleServices.AllowInInstances", false) &&
        player->GetMap() && player->GetMap()->Instanceable())
    {
        ChatHandler(player->GetSession()).SendNotification("Services cannot be used inside instances.");
        return false;
    }

    if (!sConfigMgr->GetOption<bool>("BrambleServices.Interaction.AllowInCombat", false) &&
        player->IsInCombat())
    {
        ChatHandler(player->GetSession()).SendNotification("Services cannot be used while in combat.");
        return false;
    }

    return true;
}

bool ServiceEnabled(uint32 action)
{
    switch (action)
    {
        case 1: return sConfigMgr->GetOption<bool>("BrambleServices.Service.Heal", true);
        case 2: return sConfigMgr->GetOption<bool>("BrambleServices.Service.ResetInstances", false);
        case 3: return sConfigMgr->GetOption<bool>("BrambleServices.Service.ResetCooldowns", false);
        case 4: return sConfigMgr->GetOption<bool>("BrambleServices.Service.LeaveCombat", false);
        case 5: return sConfigMgr->GetOption<bool>("BrambleServices.Service.RemoveSickness", true);
        case 6: return sConfigMgr->GetOption<bool>("BrambleServices.Service.Repair", true);
        case 7: return sConfigMgr->GetOption<bool>("BrambleServices.Service.ResetTalents", true);
        case 8: return sConfigMgr->GetOption<bool>("BrambleServices.Service.Bank", true);
        case 9: return sConfigMgr->GetOption<bool>("BrambleServices.Service.Mailbox", true);
        case 10: return sConfigMgr->GetOption<bool>("BrambleServices.Service.DualSpec", true);
        case 11: return sConfigMgr->GetOption<bool>("BrambleServices.Service.FishFeast", true);
        case 12:
        case 121:
        case 122:
        case 123:
            return sConfigMgr->GetOption<bool>("BrambleServices.Service.FlaskOfNorth", true);
        default: return false;
    }
}
}

// Fish Feast bot coordinator is intentionally *inside Brambletown Services*.
// It calls the installed Playerbots "use" / "go" actions without changing or
// extending any Playerbots code or using privileged fake player commands.
// The Steward owns this small, map-thread-only update window so that the GO
// and bots are always resolved by GUID in the correct map (no stale pointers).
struct BrambletownStewardAI : ScriptedAI
{
    explicit BrambletownStewardAI(Creature* creature) : ScriptedAI(creature) { }

    // Native 3.3.5a chain: clicking Fish Feast (GO 192675) casts 57397,
    // spell_scripts applies 57398 (10-second eating aura), whose 10-second
    // periodic tick applies the actual Well Fed bonus 57399.
    static constexpr uint32 FishFeastEntry = 192675;
    static constexpr uint32 EatingAura = 57398;
    static constexpr uint32 WellFedAura = 57399;

    struct PendingBot
    {
        ObjectGuid guid;
        bool moveRequested = false;
        uint8 moveAttempts = 0;
        uint32 moveRetryDelayMs = 0;
        bool useRequested = false;
        bool foodStarted = false;
        bool fallbackCast = false;
        bool completed = false;
        bool receivedWellFed = false;
        // Snapshot the buff while waiting to eat. A previously present buff
        // is NOT proof of a new meal; an upward duration reset is.
        int32 previousWellFedDurationMs = 0;
        uint32 clickAgeMs = 0;
        uint32 eatingAgeMs = 0;
    };

    ObjectGuid feastGuid;
    ObjectGuid requesterGuid;
    std::vector<PendingBot> bots;
    uint32 windowMs = 0;
    uint32 tickMs = 0;

    void ReportResult()
    {
        if (requesterGuid && !bots.empty())
        {
            uint32 const received = static_cast<uint32>(std::count_if(
                bots.begin(), bots.end(), [](PendingBot const& bot) { return bot.receivedWellFed; }));
            if (Player* requester = ObjectAccessor::GetPlayer(*me, requesterGuid))
                if (requester->GetSession() && !requester->GetSession()->IsHeadless())
                    ChatHandler(requester->GetSession()).PSendSysMessage(
                        "Fish Feast: {} of {} invited bots refreshed Well Fed.", received, bots.size());
        }
        bots.clear();
        feastGuid.Clear();
        requesterGuid.Clear();
        windowMs = 0;
        tickMs = 0;
    }

    uint32 OfferFeastToNearbyBots(GameObject* feast, Player* requester)
    {
        // Called once per actual feast placement; normal countdown/expiry
        // continues even if the owner leaves the zone or logs out.
        ReportResult();
        if (!feast || !feast->isSpawned() || !requester ||
            !sConfigMgr->GetOption<bool>("BrambleServices.Service.FishFeast.AutoBots", true) ||
            !me->GetMap())
            return 0;

        float const radius = std::clamp(
            sConfigMgr->GetOption<float>("BrambleServices.Service.FishFeast.BotRadius", 12.0f),
            3.0f, 25.0f);
        uint32 const botLimit = std::clamp(
            sConfigMgr->GetOption<uint32>("BrambleServices.Service.FishFeast.BotMax", 25u),
            1u, 25u);
        bots.reserve(botLimit);

        for (auto const& ref : me->GetMap()->GetPlayers())
        {
            Player* bot = ref.GetSource();
            if (!bot || !bot->IsAlive() || bot->IsInCombat() || bot->IsInFlight() ||
                bot->IsMounted() ||
                !bot->InSamePhase(feast) || !feast->IsWithinDistInMap(bot, radius) ||
                !sPlayerbotsMgr.GetPlayerbotAI(bot))
                continue;

            PendingBot pending;
            pending.guid = bot->GetGUID();
            if (Aura const* buff = bot->GetAura(WellFedAura))
                pending.previousWellFedDurationMs = buff->GetDuration();
            bots.push_back(pending);
            if (bots.size() >= botLimit)
                break;
        }

        if (bots.empty())
            return 0;
        feastGuid = feast->GetGUID();
        requesterGuid = requester->GetGUID();
        windowMs = 28000; // 12-yard approach + 10-second meal + verification
        tickMs = 400;
        return static_cast<uint32>(bots.size());
    }

    void UpdateAI(uint32 diff) override
    {
        if (!windowMs || !feastGuid)
            return;

        if (diff >= windowMs)
        {
            ReportResult();
            return;
        }
        windowMs -= diff;
        if (tickMs > diff)
        {
            tickMs -= diff;
            return;
        }
        tickMs = 1000;

        GameObject* feast = me->GetMap() ? me->GetMap()->GetGameObject(feastGuid) : nullptr;
        if (!feast || !feast->isSpawned() || feast->GetEntry() != FishFeastEntry)
        {
            ReportResult();
            return;
        }

        std::string const link = ChatHelper::FormatGameobject(feast);
        // Never fall through between bot states. A finished/failed eating
        // attempt must NOT issue a fresh use/go request on the same tick.
        // This explicit completion flag preserves the field-tested flow.
        bool allDone = true;
        for (PendingBot& pending : bots)
        {
            if (pending.completed)
                continue;

            Player* bot = ObjectAccessor::GetPlayer(*me, pending.guid);
            if (!bot || !bot->IsInWorld() || !bot->IsAlive() ||
                !bot->InSamePhase(feast))
            {
                pending.completed = true;
                continue;
            }
            PlayerbotAI* ai = sPlayerbotsMgr.GetPlayerbotAI(bot);
            if (!ai)
            {
                pending.completed = true;
                continue;
            }

            // Never hold a bot's AI idle if combat starts while eating.
            if (bot->IsInCombat() || bot->IsInFlight() || bot->IsMounted())
            {
                if (pending.foodStarted)
                    ai->ResetActionDuration();
                pending.completed = true;
                continue;
            }
            // Bots must eat for EVERY new feast, even if they already had the
            // hour-long Well Fed buff. Compare aura duration across updates:
            // a normal food tick refreshes that duration back to ~one hour.
            // Merely having the old aura must never complete the new meal.
            Aura const* wellFed = bot->GetAura(WellFedAura);
            int32 const remainingWellFedMs = wellFed ? wellFed->GetDuration() : 0;
            bool const refreshed = pending.foodStarted && wellFed &&
                (pending.previousWellFedDurationMs == 0 ||
                 remainingWellFedMs > pending.previousWellFedDurationMs + 2000);
            pending.previousWellFedDurationMs = remainingWellFedMs;
            if (refreshed)
            {
                pending.receivedWellFed = true;
                pending.completed = true;
                ai->ResetActionDuration();
                continue;
            }

            // Sending CMSG_GAMEOBJ_USE only confirms a click, not the 10s
            // eating aura / Well Fed result. Keep bots sitting long enough
            // for the real Food Feast trigger (57398 -> 57399) to finish.
            if (bot->HasAura(EatingAura))
            {
                if (!pending.foodStarted)
                {
                    bot->StopMoving();
                    bot->SetStandState(UNIT_STAND_STATE_SIT);
                    ai->YieldThread(bot, 12000);
                    pending.foodStarted = true;
                    pending.eatingAgeMs = 0;
                }
                pending.eatingAgeMs += 1000;
                allDone = false;
                continue;
            }

            if (pending.foodStarted)
            {
                pending.eatingAgeMs += 1000;
                // Don't claim success without seeing spell 57399. If a food
                // spell was interrupted or tick failed, allow the bot's normal
                // AI to resume rather than freezing for the full window.
                if (pending.eatingAgeMs >= 13000)
                {
                    ai->ResetActionDuration();
                    pending.completed = true;
                }
                else
                    allDone = false;
                continue;
            }

            if (pending.useRequested)
            {
                pending.clickAgeMs += 1000;
                if (pending.clickAgeMs >= 2000 && !pending.fallbackCast)
                {
                    pending.fallbackCast = true;
                    // Native Playerbots successfully sent a click but no
                    // eating aura appeared. Apply the genuine 10-second food
                    // aura through the Services module, not the final buff.
                    // This covers a suppressed GO script/phase edge case.
                    if (feast->IsWithinDistInMap(bot, feast->GetInteractionDistance()))
                        bot->CastSpell(bot, EatingAura, true);
                }
                if (pending.clickAgeMs >= 4000 && !bot->HasAura(EatingAura))
                {
                    ai->ResetActionDuration();
                    pending.completed = true;
                }
                else
                    allDone = false;
                continue;
            }

            if (feast->IsWithinDistInMap(bot, feast->GetInteractionDistance()))
            {
                if (ai->DoSpecificAction("use", Event("use", link), true))
                {
                    pending.useRequested = true;
                    pending.clickAgeMs = 0;
                    bot->StopMoving();
                    bot->SetStandState(UNIT_STAND_STATE_SIT);
                    ai->YieldThread(bot, 14500); // allow two-second fallback and ten-second meal
                }
                allDone = false;
            }
            else if (!pending.moveRequested && ai->GetMaster() && pending.moveAttempts < 3)
            {
                // A rejected "go" request must not be treated as movement in progress.
                // Retry at most three times, separated by three coordinator ticks.
                if (pending.moveRetryDelayMs > 0)
                    pending.moveRetryDelayMs = pending.moveRetryDelayMs > 1000 ? pending.moveRetryDelayMs - 1000 : 0;
                else
                {
                    ++pending.moveAttempts;
                    pending.moveRequested = ai->DoSpecificAction("go", Event("go", link), true);
                    if (!pending.moveRequested)
                        pending.moveRetryDelayMs = 3000;
                }
                allDone = false;
            }
            else if (!pending.moveRequested)
            {
                // No bot master or all attempts rejected: end cleanly, never loop forever.
                pending.completed = true;
            }
            else
                allDone = false;
        }

        if (allDone)
            ReportResult();
    }
};

CreatureAI* BrambletownNpcServices::GetAI(Creature* creature) const
{
    return new BrambletownStewardAI(creature);
}

bool BrambletownNpcServices::OnGossipHello(Player* player, Creature* creature)
{
    if (!creature || creature->GetEntry() != 910101 || !CanInteract(player))
    {
        if (player)
            CloseGossipMenuFor(player);
        return false;
    }

    // Only the Steward's owner may call additional NPCs. Other eligible
    // players may still access the general services permitted by config.
    if (BrambletownServices::IsStewardOwner(player, creature) &&
        (BrambletownServices::IsProviderEnabled(2) ||
         BrambletownServices::IsProviderEnabled(3) ||
         BrambletownServices::IsProviderEnabled(4) ||
         BrambletownServices::IsProviderEnabled(5)))
        AddGossipItemFor(player, 10, "|TInterface/Icons/Spell_Holy_SummonChampion:32:32:-14|t Call another service provider...", GOSSIP_SENDER_MAIN, 2000);

    // Restore Health and Mana
    if (ServiceEnabled(1))
        AddGossipItemFor(player, 10, "|TInterface\\icons\\Spell_Nature_Regenerate:32:32:-14|t Restore HP and MP", GOSSIP_SENDER_MAIN, 1);
    // Reset Instances
    if (ServiceEnabled(2))
        AddGossipItemFor(player, 10, "|TInterface\\icons\\Achievement_BG_winAB_underXminutes:32:32:-14|t Reset Instances", GOSSIP_SENDER_MAIN, 2);
    // Reset Cooldowns
    if (ServiceEnabled(3))
        AddGossipItemFor(player, 10, "|TInterface\\icons\\SPELL_HOLY_BORROWEDTIME:32:32:-14|t Reset Cooldowns", GOSSIP_SENDER_MAIN, 3);
    // Leave Combat
    if (ServiceEnabled(4))
        AddGossipItemFor(player, 10, "|TInterface\\icons\\Achievement_BG_AB_defendflags:32:32:-14|t Reset Combat", GOSSIP_SENDER_MAIN, 4);
    // Remove Sickness
    if (ServiceEnabled(5))
        AddGossipItemFor(player, 10, "|TInterface\\icons\\Spell_Shadow_DeathScream:32:32:-14|t Remove Sickness", GOSSIP_SENDER_MAIN, 5);
    // Repair Items
    if (ServiceEnabled(6))
        AddGossipItemFor(player, 10, "|TInterface\\icons\\INV_Hammer_24:32:32:-14|t Repair Items", GOSSIP_SENDER_MAIN, 6);
    // Reset Talents
    if (ServiceEnabled(7))
        AddGossipItemFor(player, 10, "|TInterface\\icons\\Achievement_WorldEvent_Lunar:32:32:-14|t Reset Talents", GOSSIP_SENDER_MAIN, 7);
    // Open Bank
    if (ServiceEnabled(8))
        AddGossipItemFor(player, 10, "|TInterface/Icons/INV_Misc_Bag_07:32:32:-14|t Bank", GOSSIP_SENDER_MAIN, 8);
    // Open Mailbox
    if (ServiceEnabled(9))
        AddGossipItemFor(player, 10, "|TInterface/Icons/INV_Letter_11:32:32:-14|t Mail", GOSSIP_SENDER_MAIN, 9);
    // Learn Dualspec
    if (ServiceEnabled(10))
        AddGossipItemFor(player, 10, "|TInterface/Icons/achievement_general:32:32:-14|t Learn Dual Talents", GOSSIP_SENDER_MAIN, 10);
    // One communal Fish Feast per temporary Steward; normal feast duration applies.
    if (ServiceEnabled(11))
        AddGossipItemFor(player, 10, "|TInterface/Icons/INV_Misc_Fish_51:32:32:-14|t Prepare Fish Feast", GOSSIP_SENDER_MAIN, 11);
    // Select which of the three Flask of the North buffs to receive.
    if (ServiceEnabled(12))
        AddGossipItemFor(player, 10, "|TInterface/Icons/INV_Alchemy_Elixir_Empty:32:32:-14|t Apply Flask of the North", GOSSIP_SENDER_MAIN, 12);

    SendGossipMenuFor(player, 1, creature->GetGUID());
    return true;
}

bool BrambletownNpcServices::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (!creature || creature->GetEntry() != 910101 ||
        sender != GOSSIP_SENDER_MAIN || !CanInteract(player))
    {
        if (player)
            CloseGossipMenuFor(player);
        return false;
    }

    player->PlayerTalkClass->ClearMenus();

    // Steward-mediated summons use a distinct action range. Do not forward
    // these actions to the existing general-services selector.
    if (action == 2000)
    {
        if (!BrambletownServices::IsStewardOwner(player, creature))
        {
            CloseGossipMenuFor(player);
            return false;
        }
        if (BrambletownServices::IsProviderEnabled(2))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Summon Enchanter", GOSSIP_SENDER_MAIN, 2002);
        if (BrambletownServices::IsProviderEnabled(3))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Summon Scribe (Glyphs)", GOSSIP_SENDER_MAIN, 2003);
        if (BrambletownServices::IsProviderEnabled(4))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Summon Faction Auctioneer", GOSSIP_SENDER_MAIN, 2004);
        if (BrambletownServices::IsProviderEnabled(5))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Summon General Goods Vendor", GOSSIP_SENDER_MAIN, 2005);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back to Steward services", GOSSIP_SENDER_MAIN, 2001);
        SendGossipMenuFor(player, 1, creature->GetGUID());
        return true;
    }
    if (action == 2001)
        return OnGossipHello(player, creature);

    if (action >= 2002 && action <= 2005)
    {
        // Provider index is the final digit; the summon coordinator enforces
        // ownership, valid Steward, enable switch, and the remaining lifetime.
        BrambletownServices::SummonRequestedProvider(player, creature, action - 2000);
        CloseGossipMenuFor(player);
        return true;
    }

    // Reject forged/stale gossip selections, not just hidden menu entries.
    if (!ServiceEnabled(action))
    {
        CloseGossipMenuFor(player);
        return false;
    }

    switch (action)
    {
        case 12: // Open the Flask of the North buff selection menu.
        {
            if (!creature || !creature->IsInWorld() || creature->GetMap() != player->GetMap())
                return false;

            AddGossipItemFor(player, 10, "+40 Strength", GOSSIP_SENDER_MAIN, 121);
            AddGossipItemFor(player, 10, "+80 Attack Power", GOSSIP_SENDER_MAIN, 122);
            AddGossipItemFor(player, 10, "+47 Spell Power", GOSSIP_SENDER_MAIN, 123);
            SendGossipMenuFor(player, 1, creature->GetGUID());
            return true;
        }

        case 121: // Strength
        case 122: // Attack Power
        case 123: // Spell Power
        {
            CloseGossipMenuFor(player);
            if (!creature || !creature->IsInWorld() || creature->GetMap() != player->GetMap())
                return false;

            // Direct buff spell effects, rather than the Alchemy-restricted
            // Flask of the North item (47499) or its item-use spell (67019).
            uint32 const spellId = action == 121 ? 67018u : (action == 122 ? 67017u : 67016u);
            SpellCastResult const result = player->CastSpell(player, spellId, true);
            if (result != SPELL_CAST_OK)
            {
                ChatHandler(player->GetSession()).SendNotification("Flask of the North could not be applied.");
                return false;
            }

            // A character may choose a different effect later, but must not
            // retain more than one Flask of the North variant at once.
            for (uint32 const otherSpell : {67016u, 67017u, 67018u})
                if (otherSpell != spellId)
                    player->RemoveAurasDueToSpell(otherSpell);

            char const* stat = action == 121 ? "Strength" :
                (action == 122 ? "Attack Power" : "Spell Power");
            ChatHandler(player->GetSession()).PSendSysMessage(
                "Flask of the North applied: {}.", stat);
            return true;
        }

        case 11: // Prepare a communal Fish Feast just ahead of the Steward.
        {
            CloseGossipMenuFor(player);
            // This Steward's AI runs bot eating for up to 28 seconds. Near
            // despawn, reject the request instead of stranding bots mid-meal.
            // Reserve two seconds for the world update/despawn boundary.
            if (sConfigMgr->GetOption<bool>("BrambleServices.Service.FishFeast.AutoBots", true))
            {
                TempSummon* summon = creature ? creature->ToTempSummon() : nullptr;
                if (!summon || summon->GetTimer() <= 30000u)
                {
                    ChatHandler(player->GetSession()).SendNotification(
                        "This Steward is expiring too soon to coordinate a Fish Feast. Call a new Steward.");
                    return false;
                }
            }
            if (player->IsInCombat())
            {
                ChatHandler(player->GetSession()).SendNotification("Fish Feast cannot be prepared during combat.");
                return false;
            }
            if (!creature || !creature->IsInWorld() || creature->GetMap() != player->GetMap())
                return false;

            uint32 const requiredLevel = std::clamp(
                sConfigMgr->GetOption<uint32>("BrambleServices.Service.FishFeast.MinLevel", 1u), 1u, 80u);
            if (player->GetLevel() < requiredLevel)
            {
                ChatHandler(player->GetSession()).PSendSysMessage(
                    "Fish Feast is available from level {}.", requiredLevel);
                return false;
            }

            GameObject* feast = nullptr;
            {
                // Check + spawn + mark must be indivisible: two players cannot
                // both order the same Steward to prepare a feast concurrently.
                std::lock_guard<std::mutex> lock(ServedFeastsMutex);
                auto const now = FeastClock::now();
                PurgeExpiredFeastEntries(now);
                ObjectGuid const npcKey = creature->GetGUID();
                if (ServedFeasts.contains(npcKey))
                {
                    ChatHandler(player->GetSession()).SendNotification(
                        "This Steward has already prepared a Fish Feast.");
                    return false;
                }

                // GetNearPosition considers the Steward's facing and terrain.
                // The feast persists independently of the Steward.
                Position const position = creature->GetNearPosition(2.0f, 0.0f);
                feast = creature->SummonGameObject(
                    192675, position.GetPositionX(), position.GetPositionY(),
                    position.GetPositionZ(), position.GetOrientation(),
                    0.0f, 0.0f, 0.0f, 1.0f, 180u, true, GO_SUMMON_TIMED_DESPAWN);
                if (!feast)
                {
                    ChatHandler(player->GetSession()).SendNotification(
                        "Fish Feast could not be prepared here.");
                    return false;
                }

                // A feast lasts 180s; the once-per-Steward restriction lasts
                // for at least the Steward's entire configured lifetime.
                uint32 const npcDuration = std::clamp(
                    sConfigMgr->GetOption<uint32>("BrambleServices.DurationSeconds", 120u), 30u, 600u);
                ServedFeasts.emplace(npcKey,
                    now + std::chrono::seconds(std::max(180u, npcDuration)));
            }
            ChatHandler(player->GetSession()).SendNotification("Fish Feast prepared in front of the Steward.");
            if (auto* ai = static_cast<BrambletownStewardAI*>(creature->AI()))
            {
                uint32 const eligibleBots = ai->OfferFeastToNearbyBots(feast, player);
                if (eligibleBots)
                    ChatHandler(player->GetSession()).PSendSysMessage(
                        "Inviting {} nearby playerbots to eat the feast automatically.", eligibleBots);
                else if (sConfigMgr->GetOption<bool>("BrambleServices.Service.FishFeast.AutoBots", true))
                    ChatHandler(player->GetSession()).SendSysMessage(
                        "No eligible playerbots nearby. Mounted, dead, in-combat, out-of-phase, "
                        "or out-of-range bots cannot eat; existing Well Fed buffs do not exclude bots.");
            }

            // The Steward announces the feast in world chat after asking the
            // coordinator to notify bots. The announcement also plays when no
            // bots are nearby; no extra gossip command is required.
            uint32 const announcementIndex =
                NextFeastAnnouncement.fetch_add(1, std::memory_order_relaxed) % FeastAnnouncements.size();
            creature->Say(FeastAnnouncements[announcementIndex], LANG_UNIVERSAL);
            return true;
        }

        case 1: // Restore HP and MP
            CloseGossipMenuFor(player);
            if (player->IsInCombat())
            {
                ChatHandler(player->GetSession()).SendNotification("You are in combat!");
                return false;
            }
            else if (player->getPowerType() == POWER_MANA)
                player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));

            player->SetHealth(player->GetMaxHealth());
            ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFHP & MP successfully restored!");
            player->CastSpell(player, 31726);
            break;

        case 2: // Reset Instances
            CloseGossipMenuFor(player);
            for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
            {
                BoundInstancesMap const& m_boundInstances = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), Difficulty(i));
                for (BoundInstancesMap::const_iterator itr = m_boundInstances.begin(); itr != m_boundInstances.end();)
                {
                    if (itr->first != player->GetMapId())
                    {
                        sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), itr->first, Difficulty(i), true, player);
                        itr = m_boundInstances.begin();
                    }
                    else
                        ++itr;
                }
            }

           ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFInstances successfully reset!");
            player->CastSpell(player, 59908);
            return true;

        case 3: // Reset Cooldowns
            CloseGossipMenuFor(player);
            if (player->IsInCombat())
            {
                ChatHandler(player->GetSession()).SendNotification("You are in combat!");
                return false;
            }

            player->RemoveAllSpellCooldown();
            ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFCooldowns successfully reset!");
            player->CastSpell(player, 31726);
            break;

        case 4: // Leave Combat
            CloseGossipMenuFor(player);
            player->CombatStop();
            ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFCombat successfully removed!");
            player->CastSpell(player, 31726);
            break;

        case 5: // Remove Sickness
            CloseGossipMenuFor(player);
            if (player->HasAura(15007))
                player->RemoveAura(15007);
            ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFSickness successfully removed!");
            player->CastSpell(player, 31726);
            break;

        case 6: // Repair Items
            CloseGossipMenuFor(player);
            player->DurabilityRepairAll(false, 0, false);
            ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFItems repaired successfully!");
            player->CastSpell(player, 31726);
            break;

        case 7: // Reset Talents
            CloseGossipMenuFor(player);
            player->resetTalents(true);
            player->SendTalentsInfoData(false);
            ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFTalents reset successfully!");
            player->CastSpell(player, 31726);
            break;

        case 8: // Bank
            CloseGossipMenuFor(player);
            player->GetSession()->SendShowBank(player->GetGUID());
            break;

        case 9: // Mailbox
            CloseGossipMenuFor(player);
            player->GetSession()->SendShowMailBox(creature->GetGUID());
            break;

        case 10: // Learn Dual Talent Specialization
            CloseGossipMenuFor(player);
            if (player->IsInCombat())
            {
                ChatHandler(player->GetSession()).SendNotification("You are in combat!");
                return false;
            }

            player->learnSpell(63644);
            player->CastSpell(player, 31726);
            player->CastSpell(player, 63624);
            player->learnSpell(63645);
            player->UpdateSpecCount(2);
            ChatHandler(player->GetSession()).SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFDual Talents Learned successfully!");
            return true;

    }
    return true;
}

void AddBrambletownNpcServices()
{
    new BrambletownNpcServices();
}
