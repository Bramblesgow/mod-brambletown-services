// Brambletown Services: Steward-controlled provider summon operations.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <cstdint>
class Player;
class Creature;
namespace BrambletownServices
{
    bool IsProviderEnabled(uint32_t index);
    bool IsStewardOwner(Player* player, Creature* steward);
    bool SummonRequestedProvider(Player* player, Creature* steward, uint32_t index);
}
