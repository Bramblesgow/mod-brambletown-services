// Brambletown Scribe: player-selected class -> native WotLK vendor window.
// Copyright (C) 2026 Brambletown contributors. AGPL-3.0-or-later.
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Configuration/Config.h"
#include "Player.h"
#include "WorldSession.h"
#include "Creature.h"
#include "Map.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"

#include <array>
#include <mutex>
#include <unordered_map>

namespace
{
constexpr uint32 ScribeEntry = 910103;
constexpr uint32 ClassVendorBase = 910120;

struct ClassChoice
{
    uint8 classId;
    char const* name;
};

constexpr std::array<ClassChoice, 10> GlyphClasses{{
    {1, "Warrior"}, {2, "Paladin"}, {3, "Hunter"}, {4, "Rogue"},
    {5, "Priest"}, {6, "Death Knight"}, {7, "Shaman"},
    {8, "Mage"}, {9, "Warlock"}, {11, "Druid"}
}};

struct SelectedCatalog
{
    ObjectGuid vendorGuid;
    uint32 templateId;
};

// Each character's selected catalog is bound to its summoned Scribe GUID.
// These hooks run on the player/world update path; logout clears the record.
std::unordered_map<uint32, SelectedCatalog> Selections;
std::mutex SelectionsMutex;

uint32 CatalogForClass(uint32 classId)
{
    for (ClassChoice const& cls : GlyphClasses)
        if (cls.classId == classId)
            return ClassVendorBase + classId;
    return 0;
}

bool IsCatalog(uint32 entry)
{
    return entry > ClassVendorBase && CatalogForClass(entry - ClassVendorBase) == entry;
}

bool CanUseScribe(Player* player)
{
    if (!player || !player->GetSession() || player->GetSession()->IsHeadless() ||
        !player->IsAlive() || player->InArena() || player->InBattleground())
        return false;
    if (!sConfigMgr->GetOption<bool>("BrambleServices.Enable", false) ||
        !sConfigMgr->GetOption<bool>("BrambleServices.Provider.3.Enable", false) ||
        sConfigMgr->GetOption<uint32>("BrambleServices.ProviderCount", 5u) < 3u)
        return false;
    if (sConfigMgr->GetOption<bool>("BrambleServices.Access.GMOnly", true) &&
        player->GetSession()->GetSecurity() < SEC_GAMEMASTER)
        return false;
    if (!sConfigMgr->GetOption<bool>("BrambleServices.Interaction.AllowInCombat", false) &&
        player->IsInCombat())
        return false;
    if (!sConfigMgr->GetOption<bool>("BrambleServices.AllowInInstances", false) &&
        player->GetMap() && player->GetMap()->Instanceable())
        return false;
    return true;
}

bool HasValidSelection(Player* player, ObjectGuid vendorGuid, uint32* templateId = nullptr)
{
    if (!CanUseScribe(player))
        return false;
    SelectedCatalog selected{};
    {
        std::lock_guard<std::mutex> lock(SelectionsMutex);
        auto it = Selections.find(player->GetGUID().GetCounter());
        if (it == Selections.end())
            return false;
        selected = it->second;
    }
    if (selected.vendorGuid != vendorGuid || !IsCatalog(selected.templateId))
        return false;
    Creature* creature = player->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
    if (!creature || creature->GetEntry() != ScribeEntry)
        return false;
    if (templateId)
        *templateId = selected.templateId;
    return true;
}
}

class BrambletownScribe : public CreatureScript
{
public:
    BrambletownScribe() : CreatureScript("brambletown_scribe") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!CanUseScribe(player) || !creature || creature->GetEntry() != ScribeEntry)
        {
            if (player)
                CloseGossipMenuFor(player);
            return true;
        }

        // Force a fresh class selection each time the player opens gossip.
        {
            std::lock_guard<std::mutex> lock(SelectionsMutex);
            Selections.erase(player->GetGUID().GetCounter());
        }

        // All ten classes are selectable, independent of the buyer's class.
        // One class is shown at a time to respect client vendor limits.
        for (ClassChoice const& cls : GlyphClasses)
        {
            uint32 const catalog = CatalogForClass(cls.classId);
            if (sObjectMgr->GetNpcVendorItemList(catalog))
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, cls.name, GOSSIP_SENDER_MAIN, 100 + cls.classId);
        }

        SendGossipMenuFor(player, 1, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (!CanUseScribe(player) || !creature || creature->GetEntry() != ScribeEntry ||
            sender != GOSSIP_SENDER_MAIN)
        {
            if (player)
                CloseGossipMenuFor(player);
            return true;
        }
        uint32 const catalog = action >= 100 ? CatalogForClass(action - 100) : 0;
        if (!catalog || !sObjectMgr->GetNpcVendorItemList(catalog))
        {
            CloseGossipMenuFor(player);
            return true;
        }

        {
            std::lock_guard<std::mutex> lock(SelectionsMutex);
            Selections[player->GetGUID().GetCounter()] = {creature->GetGUID(), catalog};
        }
        CloseGossipMenuFor(player);
        // The PlayerScript hook enforces the selected catalog on both the
        // first inventory response and later CMSG_LIST_INVENTORY refreshes.
        player->GetSession()->SendListInventory(creature->GetGUID(), catalog);
        return true;
    }
};

class BrambletownScribeVendorHooks : public PlayerScript
{
public:
    BrambletownScribeVendorHooks() : PlayerScript("BrambletownScribeVendorHooks",
        {PLAYERHOOK_ON_SEND_LIST_INVENTORY, PLAYERHOOK_ON_BEFORE_BUY_ITEM_FROM_VENDOR,
         PLAYERHOOK_CAN_SELL_ITEM, PLAYERHOOK_ON_BEFORE_LOGOUT}) { }

    void OnPlayerSendListInventory(Player* player, ObjectGuid vendorGuid, uint32& vendorEntry) override
    {
        if (!player)
            return;
        Creature* creature = player->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
        if (!creature || creature->GetEntry() != ScribeEntry)
            return; // Do not change regular merchants.

        uint32 selected = 0;
        vendorEntry = HasValidSelection(player, vendorGuid, &selected) ? selected : 0;
    }

    void OnPlayerBeforeBuyItemFromVendor(Player* player, ObjectGuid vendorGuid,
        uint32 /*vendorSlot*/, uint32& item, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/) override
    {
        if (!player || !player->GetSession())
            return;

        uint32 const current = player->GetSession()->GetCurrentVendor();
        Creature* creature = player->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
        if ((!creature || creature->GetEntry() != ScribeEntry) && !IsCatalog(current))
            return; // Ordinary vendors are never modified.

        uint32 selected = 0;
        if (!HasValidSelection(player, vendorGuid, &selected) || current != selected)
        {
            item = 0; // AzerothCore's vendor hook veto path.
            return;
        }
        // Core still validates stock slot and item ID, inventory room,
        // gold balance, item requirements, and regular purchase accounting.
    }

    bool OnPlayerCanSellItem(Player* player, Item* /*item*/, Creature* creature) override
    {
        if (!creature || creature->GetEntry() != ScribeEntry)
            return true;
        return HasValidSelection(player, creature->GetGUID());
    }

    void OnPlayerBeforeLogout(Player* player) override
    {
        if (!player)
            return;
        std::lock_guard<std::mutex> lock(SelectionsMutex);
        Selections.erase(player->GetGUID().GetCounter());
    }
};

void AddBrambletownScribeScripts()
{
    new BrambletownScribe();
    new BrambletownScribeVendorHooks();
}
