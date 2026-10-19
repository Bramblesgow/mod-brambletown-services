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

# Native scoping/quantity expressions are checked against the proposed patch
# by test_native_patch_contract.py; behavior still needs in-game verification.

# Staged core checks: verify actual custom hooks, not ordinary AzerothCore functions.
staged_root = os.environ.get('BRAMBLETOWN_CORE_ROOT')
if staged_root is not None:
    base = Path(staged_root)
    player_file = base/player
    handler_file = base/handler
    assert player_file.is_file(), f'Staged Player.cpp missing: {player_file}'
    assert handler_file.is_file(), f'Staged ItemHandler.cpp missing: {handler_file}'
    staged_player = player_file.read_text()
    staged_handler = handler_file.read_text()
    assert 'brambletownEntry' in staged_player, 'Staged Brambletown entry guard missing'
    assert 'creature->GetEntry() == 910103 && pProto->Class == ITEM_CLASS_GLYPH' in staged_player, (
        'Staged Scribe purchase cap missing'
    )
    assert 'vendor->GetEntry() == 910103 && itemTemplate->Class == ITEM_CLASS_GLYPH' in staged_handler, (
        'Staged Scribe listing cap missing'
    )
    print('PASS: staged core contains Scribe-specific hooks')
else:
    print('SKIP: staged core source checks (set BRAMBLETOWN_CORE_ROOT)')

strict_value = os.environ.get('BRAMBLETOWN_REQUIRE_NEW_GUARD', '0')
assert strict_value in ('0', '1'), 'BRAMBLETOWN_REQUIRE_NEW_GUARD must be 0 or 1'
live_root = os.environ.get('AZEROTHCORE_LIVE_ROOT')
if strict_value == '1':
    assert live_root is not None or staged_root is not None, (
        'Strict native patch check needs AZEROTHCORE_LIVE_ROOT or BRAMBLETOWN_CORE_ROOT'
    )
    checked_root = Path(live_root if live_root is not None else staged_root)
    player_file = checked_root/player
    handler_file = checked_root/handler
    assert player_file.is_file(), f'Strict Player.cpp missing: {player_file}'
    assert handler_file.is_file(), f'Strict ItemHandler.cpp missing: {handler_file}'
    installed_player = player_file.read_text()
    installed_handler = handler_file.read_text()
    assert 'brambletownEntry >= 910101u && brambletownEntry <= 910106u' in installed_player, (
        'Native Steward mailbox guard not installed'
    )
    assert 'price = std::min<uint64>(price, uint64(10000u) * count);' in installed_player, (
        'Overflow-safe Scribe purchase cap not installed'
    )
    assert 'vendor->GetEntry() == 910103 && itemTemplate->Class == ITEM_CLASS_GLYPH' in installed_handler, (
        'Native Scribe listing cap not installed'
    )
    print(f'PASS: strict native Scribe and Steward changes present in {checked_root}')
elif live_root is not None:
    # Production is not required to have the proposed patch before deployment.
    installed_player = Path(live_root)/player
    installed_handler = Path(live_root)/handler
    assert installed_player.is_file(), f'Installed Player.cpp missing: {installed_player}'
    assert installed_handler.is_file(), f'Installed ItemHandler.cpp missing: {installed_handler}'
    print('SKIP: installed native patch not required before deployment (set BRAMBLETOWN_REQUIRE_NEW_GUARD=1)')
else:
    print('SKIP: installed native patch verification (predeployment mode)')

print('PASS: proposed Scribe price-cap contracts and SQL scope; in-game purchase untested')
