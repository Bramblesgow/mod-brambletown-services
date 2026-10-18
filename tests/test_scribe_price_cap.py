#!/usr/bin/env python3
"""Portable native Scribe patch/price regression; no running-server writes."""
from pathlib import Path
import os

root = Path(__file__).resolve().parents[1]
patch = (root/'patches/brambletown-npc-access-and-scribe-price-cap.patch').read_text()
player = 'src/server/game/Entities/Player/Player.cpp'
handler = 'src/server/game/Handlers/ItemHandler.cpp'
assert f'diff --git a/{player}' in patch and f'diff --git a/{handler}' in patch
assert 'Brambletown Scribe' in patch and 'Brambletown NPC providers (1-5)' in patch
assert 'creature->GetEntry() == 910103 && pProto->Class == ITEM_CLASS_GLYPH' in patch
assert 'price = std::min<uint64>(price, uint64(10000u) * count);' in patch
assert 'vendor->GetEntry() == 910103 && itemTemplate->Class == ITEM_CLASS_GLYPH && price > 10000' in patch
assert 'price = 10000;' in patch
sql = (root/'sql/GM_STAGING_scribe_class_glyph_vendors.sql').read_text()
assert 'UPDATE item_template' not in sql and 'DELETE FROM item_template' not in sql

# Quantity and ordinary-merchant behavior is exercised in
# test_native_patch_contract.py alongside the actual conditional patch hunks.

# Optional installed source audit, not needed for the portable patch test.
for env,filename in (('BRAMBLETOWN_CORE_ROOT',player),('AZEROTHCORE_LIVE_ROOT',handler)):
    configured = os.environ.get(env)
    if configured:
        path = Path(configured)/filename
        assert path.is_file(), f"{env} source not found: {path}"
        expected = 'GetNPCIfCanInteractWith' if env == 'BRAMBLETOWN_CORE_ROOT' else 'SendListInventory'
        assert expected in path.read_text(), f'{env} does not contain {expected}'
    else:
        print(f'SKIP: {env} external source audit (not configured)')

print('PASS: staged Scribe price-cap syntax, isolated core hooks and SQL scope')
