#!/usr/bin/env python3
"""Assert bounded, opt-in, module-only Playerbots feast coordination."""
from pathlib import Path
import os
r=Path(__file__).resolve().parents[1]
s=(r/'src/brambletown_npc_services.cpp').read_text()
h=(r/'src/brambletown_npc_services.h').read_text()
c=(r/'conf/brambletown_services.conf.dist').read_text()
playerbots_src=os.environ.get('PLAYERBOTS_SRC')
if playerbots_src:
    playerbots=Path(playerbots_src)
    playerbot_impl=(playerbots/'Ai/Base/Actions/UseItemAction.cpp').read_text()
    playerbot_manager=(playerbots/'Bot/PlayerbotMgr.h').read_text()
    ai_header=(playerbots/'Bot/PlayerbotAI.h').read_text()
assert 'CreatureAI* GetAI(Creature* creature) const override;' in h
assert 'new BrambletownStewardAI(creature)' in s
assert 'struct BrambletownStewardAI : ScriptedAI' in s
assert 'void UpdateAI(uint32 diff) override' in s
assert 'OfferFeastToNearbyBots(feast, player)' in s
assert 'GO_SUMMON_TIMED_DESPAWN' in s
assert 'feast->GetEntry() != FishFeastEntry' in s
assert 'sPlayerbotsMgr.GetPlayerbotAI(bot)' in s
if playerbots_src:
    assert 'PlayerbotAI* GetPlayerbotAI(Player* player)' in playerbot_manager
    assert 'virtual bool DoSpecificAction(' in ai_header
    assert 'bool UseItemAction::UseGameObject(ObjectGuid guid)' in playerbot_impl
    assert 'HandleGameObjectUseOpcode(use)' in playerbot_impl
else:
    print('SKIP: external Playerbots API checks (set PLAYERBOTS_SRC)')
assert 'GameObject* feast = me->GetMap() ? me->GetMap()->GetGameObject(feastGuid)' in s
assert 'ObjectAccessor::GetPlayer(*me, pending.guid)' in s
assert 'for (auto const& ref : me->GetMap()->GetPlayers())' in s
assert 'bot->IsInCombat()' in s and 'bot->IsInFlight()' in s and 'bot->IsAlive()' in s
assert 'bot->InSamePhase(feast)' in s
assert 'feast->IsWithinDistInMap(bot, feast->GetInteractionDistance())' in s
assert 'ai->DoSpecificAction("use", Event("use", link), true)' in s
assert 'ai->DoSpecificAction("go", Event("go", link), true)' in s
assert 'ai->GetMaster()' in s # no random/unmanaged bot movement orders
assert 'pending.moveRequested = ai->DoSpecificAction("go", Event("go", link), true)' in s and 'pending.useRequested = true' in s
assert 'windowMs = 28000' in s and 'tickMs = 1000' in s
for setting in ('AutoBots','BotRadius','BotMax'):
    field=f'BrambleServices.Service.FishFeast.{setting}'
    assert field in s and field in c
assert 'BotRadius = 12.0' in c and 'BotMax = 25' in c
assert 'BotMax", 25u),\n            1u, 25u' in s
assert 'BotRadius", 12.0f),\n            3.0f, 25.0f' in s
assert 'Inviting {} nearby playerbots' in s
assert 'bots.clear();' in s and 'feastGuid.Clear();' in s
print('PASS: Services-only Playerbots API use/go; bounded time, range, count, map GUID lifecycle, combat safety')

# Spoken NPC announcements rotate after coordinator dispatch and are independent
# of whether bots are nearby; the one-feast-per-Steward guard prevents spam.
assert 'std::atomic<uint32> NextFeastAnnouncement{0};' in s
assert 'NextFeastAnnouncement.fetch_add(1, std::memory_order_relaxed) % FeastAnnouncements.size()' in s
assert 'creature->Say(FeastAnnouncements[announcementIndex], LANG_UNIVERSAL);' in s
lines=[
    "Fish Feast down! Get it while it's warm!",
    "Fresh Fish Feast! Come grab a plate!",
    "Feast is served! Everyone, dig in!",
    "Food's up! Don't let it go to waste!",
    "A Fish Feast for the crew! Help yourselves!",
]
assert all(line in s for line in lines)
assert s.index('OfferFeastToNearbyBots(feast, player)') < s.index('creature->Say(FeastAnnouncements[announcementIndex], LANG_UNIVERSAL);')
assert '1u, 40u' not in s
print('PASS: 25-bot raid cap, 5 rotating world-speech announcements, coordinator request precedes speech')

# The click isn't proof of food or buff. Require 57398 food aura and 57399
# actual Well Fed before saying a bot completed the meal.
assert 'static constexpr uint32 EatingAura = 57398;' in s
assert 'static constexpr uint32 WellFedAura = 57399;' in s
assert 'Aura const* wellFed = bot->GetAura(WellFedAura);' in s
assert 'pending.receivedWellFed = true;' in s
assert 'if (bot->HasAura(EatingAura))' in s
assert 'ai->YieldThread(bot, 14500);' in s
assert 'bot->SetStandState(UNIT_STAND_STATE_SIT);' in s
assert 'bot->CastSpell(bot, EatingAura, true);' in s
assert 'ai->ResetActionDuration();' in s
assert 'clickAgeMs >= 2000' in s
assert 'eatingAgeMs >= 13000' in s
assert 'Fish Feast: {} of {} invited bots refreshed Well Fed.' in s
assert 'pending.previousWellFedDurationMs = remainingWellFedMs;' in s
assert 'bot->IsInCombat() || bot->IsInFlight() || bot->IsMounted()' in s
assert 'windowMs = 28000' in s
print('PASS: 10-second real food aura, bot sitting/delay, fallback for unconfirmed click, verified Well Fed result')
