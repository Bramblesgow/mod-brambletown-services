// Brambletown faction-specific temporary auctioneer access policy.
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ScriptMgr.h"
#include "Configuration/Config.h"
#include "WorldSession.h"
#include "Player.h"
#include "Creature.h"
#include "Map.h"

class BrambletownAuctionAccess : public MiscScript
{
public:
    BrambletownAuctionAccess() : MiscScript("BrambletownAuctionAccess", {MISCHOOK_CAN_SEND_AUCTIONHELLO}) { }

    bool CanSendAuctionHello(WorldSession const* session, ObjectGuid, Creature* creature) override
    {
        if (!creature || (creature->GetEntry() != 910104 && creature->GetEntry() != 910105))
            return true; // Never alter normal city auctioneers.
        if (!session || session->IsHeadless() || !session->GetPlayer() ||
            !sConfigMgr->GetOption<bool>("BrambleServices.Enable", false) ||
            !sConfigMgr->GetOption<bool>("BrambleServices.Provider.4.Enable", false) ||
            sConfigMgr->GetOption<uint32>("BrambleServices.ProviderCount", 5u) < 4u)
            return false;
        Player const* player = session->GetPlayer();
        if (sConfigMgr->GetOption<bool>("BrambleServices.Access.GMOnly", true) &&
            session->GetSecurity() < SEC_GAMEMASTER)
            return false;
        if (!player->IsAlive() || player->InArena() || player->InBattleground() ||
            (!sConfigMgr->GetOption<bool>("BrambleServices.Interaction.AllowInCombat", false) && player->IsInCombat()))
            return false;
        if (!sConfigMgr->GetOption<bool>("BrambleServices.AllowInInstances", false) &&
            player->GetMap() && player->GetMap()->Instanceable())
            return false;
        // Check the faction of the summoned NPC, not the auction market.
        // The core determines the market from this NPC's faction template
        // and its current AllowTwoSide.Interaction.Auction configuration.
        return (player->GetTeamId() == TEAM_ALLIANCE && creature->GetEntry() == 910104) ||
            (player->GetTeamId() == TEAM_HORDE && creature->GetEntry() == 910105);
    }
};

void AddBrambletownAuctionGuardScripts()
{
    new BrambletownAuctionAccess();
}
