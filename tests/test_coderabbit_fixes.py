#!/usr/bin/env python3
"""Portable checks for the CodeRabbit fix set; no database or runtime writes."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
read=lambda path:(r/path).read_text()
service=read('src/brambletown_npc_services.cpp')
coordinator=read('src/brambletown_services.cpp')
enchanter=read('src/brambletown_enchanter.cpp')
steward_sql=read('sql/GM_STAGING_steward_only.sql')
enchanter_sql=read('sql/GM_STAGING_enchanter_only.sql')
goods_sql=read('sql/GM_STAGING_general_goods_vendor.sql')
header=read('src/brambletown_npc_services.h')
workflow=read('.github/workflows/static-checks.yml')
assert 'SendShowMailBox(creature->GetGUID())' in service
assert 'SendShowMailBox(player->GetGUID())' not in service
assert '67108865' in steward_sql
assert 'npcflag = npcflag | 67108864' in steward_sql
assert "ScriptName = 'brambletown_npc_services'" in steward_sql
assert '(index == 1 && provider.entry != 910101u)' in coordinator
assert 'GetSkillValue(SKILL_ENGINEERING) >= 400)' in enchanter
assert 'GetSkillValue(SKILL_ENGINEERING) == 400)' not in enchanter
assert 'CreatureScript("brambletown_enchanter")' in enchanter
assert enchanter_sql.count("'brambletown_enchanter'") >= 2
assert "ScriptName='npc_enchantment'" in enchanter_sql  # migration of prior record
assert 'CHECK (projected_count <= 150)' in goods_sql
assert 'COUNT(*) FROM npc_vendor WHERE entry=910106' in goods_sql
assert 'NOT EXISTS (' in goods_sql and 'existing.ExtendedCost=0' in goods_sql
assert 'AND (SELECT MAX(projected_count) FROM brambletown_provider5_capacity_check) <= 150' in goods_sql
assert 'DROP TEMPORARY TABLE brambletown_provider5_catalog' in goods_sql
assert header.startswith('#pragma once')
assert header.count('void AddBrambletownNpcServices();')==1
assert 'void AddBrambletownNpcServices()\n{' not in header
assert 'void AddBrambletownNpcServices()\n{' in service
assert 'actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683' in workflow
print('PASS: mailbox, stewardship, enchant, vendor-cap, tests, CI pin invariants')
