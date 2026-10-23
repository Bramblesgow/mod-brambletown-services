#!/usr/bin/env python3
"""Ensure the primary Steward gossip remains compact and stylistically consistent."""
from pathlib import Path
import struct,re,os
root=Path(__file__).resolve().parents[1]
source=(root/'src/brambletown_npc_services.cpp').read_text()
hello=source.split('bool BrambletownNpcServices::OnGossipHello',1)[1].split('bool BrambletownNpcServices::OnGossipSelect',1)[0]
lines=[line.strip() for line in hello.splitlines() if 'AddGossipItemFor(player' in line]
assert len(lines)==13, len(lines)
assert 'Call another service provider...' in lines[0]
assert 'GOSSIP_SENDER_MAIN, 2000' in lines[0]
assert all('AddGossipItemFor(player, 10,' in line for line in lines)
assert all(':32:32:-14|t ' in line for line in lines)
assert all(':40:40:-18' not in line for line in lines)
assert lines[0].count('Spell_Holy_SummonChampion')==1
assert len({int(re.search(r'GOSSIP_SENDER_MAIN, (\d+)\);',line).group(1)) for line in lines})==13
dbc_path=os.environ.get('SPELL_ICON_DBC')
if dbc_path is not None:
    assert dbc_path, 'SPELL_ICON_DBC must not be empty when configured'
    assert Path(dbc_path).is_file(), f"SpellIcon DBC not found: {dbc_path}"
    b=Path(dbc_path).read_bytes()
    magic,count,cols,sz,_=struct.unpack_from('<4s4I',b)
    assert magic==b'WDBC' and cols==2
    base=20+count*sz
    assert any(b[base+struct.unpack_from('<2I',b,20+i*sz)[1]:].split(b'\0',1)[0].decode('utf8').lower() == 'interface\\icons\\spell_holy_summonchampion' for i in range(count))
    print('PASS: configured WoW 3.3.5 art confirmed')
else:
    print('SKIP: external SpellIcon DBC check (set SPELL_ICON_DBC)')
print('PASS: summon provider menu first; all 13 icon lines consistently 32x32')
