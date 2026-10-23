#!/usr/bin/env python3
"""Regression tests for mandatory refresh on EVERY Fish Feast, even if buffed."""
from pathlib import Path
import re
s=(Path(__file__).resolve().parents[1]/'src/brambletown_npc_services.cpp').read_text()
scan=s.split('uint32 OfferFeastToNearbyBots(GameObject* feast, Player* requester)',1)[1].split('void UpdateAI(uint32 diff) override',1)[0]
update=s.split('void UpdateAI(uint32 diff) override',1)[1].split('};\n\nCreatureAI*',1)[0]
assert 'bot->HasAura(WellFedAura)' not in scan
assert 'Aura const* buff = bot->GetAura(WellFedAura)' in scan
assert 'pending.previousWellFedDurationMs = buff->GetDuration()' in scan
assert 'bots.push_back(pending);' in scan
assert 'bool const refreshed = pending.foodStarted && wellFed &&' in update
assert 'pending.previousWellFedDurationMs == 0 ||' in update
assert 'remainingWellFedMs > pending.previousWellFedDurationMs + 2000' in update
assert update.index('bool const refreshed =') < update.index('pending.previousWellFedDurationMs = remainingWellFedMs;')
assert update.index('pending.previousWellFedDurationMs = remainingWellFedMs;') < update.index('if (refreshed)')
assert 'pending.receivedWellFed = true;' in update
assert 'bot->CastSpell(bot, WellFedAura' not in s
assert 'bot->CastSpell(bot, EatingAura, true);' in update
assert 'Fish Feast: {} of {} invited bots refreshed Well Fed.' in s
assert 'No eligible playerbots nearby.' in s
assert 'bots.clear();' in s and 'feastGuid.Clear();' in s

# Pure model of the state-machine condition (not a game simulation). Exercise
# legitimate refresh, stale aura, new aura and prematurely applied buffs.
def refreshed(started:bool, current_ms:int, previous_ms:int)->bool:
    return started and current_ms>0 and (previous_ms==0 or current_ms>previous_ms+2000)

cases = [
    (False,3600000,3600000,False,'existing full-duration aura without eating'),
    (False,3600000,0,False,'no food started'),
    (True,1190000,1200000,False,'existing aura ticking down'),
    (True,3590000,3591000,False,'recent buff still counting down'),
    (True,3599500,3598600,False,'minor remaining-time variation, no refresh'),
    (True,3599000,3588000,True,'fresh duration reset after eating'),
    (True,3600000,1200000,True,'buff reset from twenty minutes to one hour'),
    (True,3599500,0,True,'previously unbuffed bot gains Well Fed'),
    (True,3599000,0,True,'expired Well Fed reacquired while eating'),
]
for started,current,prev,expected,label in cases:
    assert refreshed(started,current,prev)==expected,label
print('PASS: every-feast invites already-buffed bots; an old aura is not completion; fresh duration resets and first-time buffs are counted')
