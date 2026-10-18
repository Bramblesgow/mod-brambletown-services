#!/usr/bin/env python3
"""Guard against ChatHandler USAGE spam for intended .services rejections."""
from pathlib import Path
src=(Path(__file__).resolve().parents[1]/'src/brambletown_services.cpp').read_text()
eligibility=src.split('    bool CanSpawn(Player const* player, ChatHandler* handler)',1)[1].split('    bool ProviderEnabled(uint32 index)',1)[0]
command=src.split('    bool SummonServices(ChatHandler* handler)',1)[1].split('\n}\n\nnamespace BrambletownServices',1)[0]
# Eligibility helpers remain predicates: no actual summon when dead, fighting
# or when inside dungeons/raids (unless a config explicitly allows it).
assert 'if (!player->IsAlive())' in eligibility
assert 'You must be alive to summon services.' in eligibility
assert 'if (map->IsDungeon())' in eligibility
assert 'Services cannot be summoned inside dungeons.' in eligibility
assert 'if (map->IsRaid())' in eligibility
assert 'Services cannot be summoned inside raids.' in eligibility
assert 'return false;' in eligibility
assert eligibility.index('if (!player->IsAlive())') < eligibility.index('if (map->IsDungeon())')
# Every rejection in the CommandScript handler sends a reason and returns
# true to ChatHandler, which means "handled; don't print generic USAGE".
assert command.count('return false;')==0
assert command.count('return true;')==9
assert 'if (!CanSpawn(player, handler))\n            return true;' in command
for msg in ('Services are currently unavailable.', 'Only real player characters',
            'Services are currently restricted', 'seconds until services can be called',
            'The Steward service is disabled.', 'server configuration issue.',
            'The Steward could not be summoned.'):
    assert msg in command
assert '{"services", SummonServices, SEC_PLAYER, Console::No}' in src
print('PASS: dead/dungeon/raid restrictions preserved and all handled .services rejections suppress generic command usage')
