#!/usr/bin/env python3
"""Scribe-only 1g cap and native provider-access regression; read-only."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
staged=Path('/root/brambletown-upgrade-work/core')
live=Path('/root/azerothcore-wotlk')
p1='src/server/game/Entities/Player/Player.cpp'
p2='src/server/game/Handlers/ItemHandler.cpp'
buy=(staged/p1).read_text()
ui=(staged/p2).read_text()
assert buy.count('creature->GetEntry() == 910103 && pProto->Class == ITEM_CLASS_GLYPH')==1
assert 'price > 10000u * count' in buy and 'price = 10000u * count;' in buy
assert ui.count('vendor->GetEntry() == 910103 && itemTemplate->Class == ITEM_CLASS_GLYPH && price > 10000')==1
assert 'price = 10000;' in ui
assert 'creature->GetEntry() == 910103 && pProto->Class == ITEM_CLASS_GLYPH' not in (live/p1).read_text()
assert 'vendor->GetEntry() == 910103 && itemTemplate->Class == ITEM_CLASS_GLYPH' not in (live/p2).read_text()
patch=(r/'patches/brambletown-npc-access-and-scribe-price-cap.patch').read_text()
assert 'diff --git a/'+p1 in patch and 'diff --git a/'+p2 in patch
assert 'Brambletown Scribe' in patch and 'Brambletown temporary NPC providers' in patch
sql=(r/'sql/GM_STAGING_scribe_class_glyph_vendors.sql').read_text()
assert 'UPDATE item_template' not in sql and 'DELETE FROM item_template' not in sql

def glyph_price(base_copper, quantity=1, discount=1):
    return min(int(base_copper*quantity*discount), 10000*quantity)
for orig, qty, discount, expected in ((0,1,1,0),(400,1,1,400),(400,3,1,1200),(9000,1,1,9000),(10000,1,1,10000),(1000000,1,1,10000),(1000000,3,1,30000),(10000,1,0.9,9000)):
    assert glyph_price(orig,qty,discount)==expected
print('PASS: glyph prices <=1g; lower prices preserved, custom guard staged, live source untouched')
