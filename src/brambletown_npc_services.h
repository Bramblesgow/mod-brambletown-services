#pragma once

/*
**  Written by MtgCore
**  Rewritten by Poszer & Talamortis https://github.com/poszer/ & https://github.com/talamortis/
**  AzerothCore 2019 http://www.azerothcore.org/
**  Cleaned and made into a module by Micrah https://github.com/milestorme/
*/

#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

class BrambletownNpcServices : public CreatureScript
{
public:
    BrambletownNpcServices() : CreatureScript("brambletown_npc_services") { }

    CreatureAI* GetAI(Creature* creature) const override;
    bool OnGossipHello(Player* player, Creature* creature) override;
    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override;
};

void AddBrambletownNpcServices();
