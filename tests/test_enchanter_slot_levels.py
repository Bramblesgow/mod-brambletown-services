#!/usr/bin/env python3
"""Validate enchant action contracts; optionally check installed 3.3.5a DBC. No writes."""
from pathlib import Path
import json,re,struct,os
root=Path(__file__).resolve().parents[1]
src=(root/'src/brambletown_enchanter.cpp').read_text()
steward=(root/'src/brambletown_npc_services.cpp').read_text()
dbc_file=os.environ.get('SPELL_ENCHANT_DBC')
if dbc_file is not None:
    assert Path(dbc_file).is_file(), f"SpellItemEnchantment DBC not found: {dbc_file}"
    dbc=Path(dbc_file).read_bytes()
else:
    dbc=None
    print('SKIP: external SpellItemEnchantment DBC check (set SPELL_ENCHANT_DBC)')
store={}
if dbc:
    magic,n,cols,step,nstrings=struct.unpack_from('<4s4I',dbc)
    assert magic==b'WDBC' and cols==38 and step==152
    for i in range(n):
        u=struct.unpack_from('<38I',dbc,20+i*step)
        store[u[0]]={'level':u[37],'skill':u[35],'skill_value':u[36]}
enums={key:int(value) for key,value in re.findall(r'^\s*(ENCHANT_\w+)\s*=\s*(\d+)',src,re.M)}
choices={int(action):(int(value) if value.isdigit() else enums[value],slot)
 for action,value,slot in re.findall(r'case (\d+): return \{(\w+), (EQUIPMENT_SLOT_\w+)\};',src)}
assert set(choices)==set(range(100,251)),len(choices)
assert len(choices)==151
if dbc:
    assert all(eid in store for eid,slot in choices.values())
for act,slot in [(240,'EQUIPMENT_SLOT_RANGED'),(249,'EQUIPMENT_SLOT_RANGED'),(220,'EQUIPMENT_SLOT_HEAD'),(226,'EQUIPMENT_SLOT_SHOULDERS'),(233,'EQUIPMENT_SLOT_LEGS')]:
 assert choices[act][1]==slot
for a,eid,level in [(240,30,5),(241,32,10),(242,33,20),(243,663,30),(244,664,40),(245,2722,55),(246,2723,55),(247,2724,60),(248,3607,70),(249,3608,70),(250,3843,70),
    (220,103,0),(221,91,0),(222,209,0),(223,1507,50),(224,1508,50),(225,1509,50),
    (226,103,0),(227,91,0),(228,209,0),(229,2983,64),(230,2981,64),(231,2990,64),(232,2992,64),
    (233,103,0),(234,91,0),(235,209,0),(236,3010,50),(237,3011,50),(238,3012,60),(239,3013,60)]:
 assert choices[a][0]==eid
 if dbc:
  assert store[eid]['level']==level,(a,eid,store[eid])
assert src.count('Enchant Ranged (Bow/Gun/Xbow)')==1
assert 'item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);' in src
for subclass in ('BOW','GUN','CROSSBOW'):
 assert f'ITEM_SUBCLASS_WEAPON_{subclass}' in src
assert 'item->GetSlot()' in src and 'uint32 const enchantId = choice.enchantId;' in src
assert 'enchant->requiredLevel && player->GetLevel() < enchant->requiredLevel' in src
assert 'enchant->requiredSkill && player->GetSkillValue(enchant->requiredSkill) < enchant->requiredSkillValue' in src
assert '" |cffff5555[Requires Lv "' in src
assert '" |cffff5555[Requires "' in src
assert 'action >= 202 && action <= 204 &&' in src
assert 'item->GetSlot() == EQUIPMENT_SLOT_FINGER2' in src
for act in range(220,251):
 assert len(re.findall(rf'AddEnchantChoice\(player, "[^"]+", {act}\);',src))==1
assert 'if (action >= 100 && action <= 250)' in src
assert 'EnchantChoice const choice = GetEnchantChoice(action);' in src
assert 'if (action >= 202 && action <= 204)' in src
assert 'GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_FINGER2)' in src
assert 'return OnGossipHello(player, creature);' in src
assert 'case 100:\n' not in src.split('bool OnGossipSelect',1)[1]
ref=json.loads((root/'tests/enchant_action_baseline.json').read_text())['actions']
assert {str(k):{'enchant':eid,'slot':slot} for k,(eid,slot) in choices.items()} == ref
# User-requested priority: summon submenu is first item at initial Steward greeting.
hello=steward.split('bool BrambletownNpcServices::OnGossipHello',1)[1].split('bool BrambletownNpcServices::OnGossipSelect',1)[0]
assert hello.index('Call another service provider...') < hello.index('// Restore Health and Mana')
print('PASS: 151 enchant action/slot combinations, action fixture, 11 scope options, leveling bonuses, Steward summon first '
      + ('(external DBC verified)' if dbc is not None else '(external DBC skipped)'))
