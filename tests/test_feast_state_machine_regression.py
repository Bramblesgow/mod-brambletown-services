#!/usr/bin/env python3
"""Prevent the October 2026 bot eating state-machine fallthrough regression.

The prior cleanup converted an explicit in-progress flag into a final
`all_of(completed)` test, but also deleted unconditional `continue` statements
at the end of completed failure branches. That accidentally let the bot issue
another `use` command in the same update after an eating timeout/click failure.
"""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/brambletown_npc_services.cpp').read_text()
update=s.split('void UpdateAI(uint32 diff) override',1)[1].split('\n};\n\nCreatureAI*',1)[0]
assert 'bool allDone = true;' in update
assert 'if (allDone)\n            ReportResult();' in update
assert 'std::all_of(bots.begin(), bots.end()' not in update
# The food-started state and dispatched-click state MUST each exit the bot
# iteration even when their timeout/error paths mark the bot as completed.
food=update.split('if (pending.foodStarted)\n            {',1)[1].split('if (pending.useRequested)',1)[0]
assert 'pending.completed = true;' in food
assert 'allDone = false;' in food
assert 'continue;' in food.split('allDone = false;',1)[1]
click=update.split('if (pending.useRequested)\n            {',1)[1].split('if (feast->IsWithinDistInMap(bot, feast->GetInteractionDistance()))\n            {',1)[0]
assert 'pending.completed = true;' in click
assert 'allDone = false;' in click
assert 'continue;' in click.split('allDone = false;',1)[1]
assert 'ai->ResetActionDuration();' in food and 'ai->ResetActionDuration();' in click
assert 'bot->GetAura(WellFedAura)' in update
assert 'pending.receivedWellFed = true;' in update
assert 'bot->CastSpell(bot, EatingAura, true)' in update
assert 'No eligible playerbots nearby.' in s
assert 'BrambleServices.Service.FishFeast.AutoBots' in s
print('PASS: completed click/eating states cannot fall through to duplicate bot interactions; previously-buffed bots still eligible to refresh')
