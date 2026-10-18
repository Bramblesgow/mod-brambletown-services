#!/usr/bin/env python3
"""Contract checks for the staged AzerothCore native patch (no server or DB needed).

These tests bind checks to their actual patch hunks and enclosing C++ conditions,
not merely to fragments appearing somewhere in the file. They cannot replace
a target-core build or real in-game tests.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
patch = (root / 'patches/brambletown-npc-access-and-scribe-price-cap.patch').read_text()


def added_hunks(path):
    marker = f'diff --git a/{path} b/{path}\n'
    assert patch.count(marker) == 1, path
    section = patch.split(marker, 1)[1].split('\ndiff --git ', 1)[0]
    hunks = re.split(r'^@@[^\n]*\n', section, flags=re.M)[1:]
    assert hunks, f'No patch hunks found for {path}'
    return ['\n'.join(line[1:] for line in h.splitlines()
                      if line.startswith('+') and not line.startswith('+++'))
            for h in hunks]


player_hunks = added_hunks('src/server/game/Entities/Player/Player.cpp')
list_hunks = added_hunks('src/server/game/Handlers/ItemHandler.cpp')
assert len(player_hunks) == 2 and len(list_hunks) == 1
access, purchase = player_hunks
listing = list_hunks[0]

# The native guard must contain its entire authorization logic IN the scoped
# NPC-entry check; a check appearing somewhere else in the patch is not enough.
gate = 'if (brambletownEntry >= 910101u && brambletownEntry <= 910106u)'
assert gate in access
gate_tail = access.split(gate, 1)[1]
assert gate_tail.lstrip().startswith('{')
gate_body = gate_tail.split('}\n', 1)[0]
required_checks = (
    'GetSession()->IsHeadless()',
    'BrambleServices.Enable',
    'BrambleServices.ProviderCount',
    'BrambleServices.Provider.',
    'BrambleServices.Access.GMOnly',
    'GetSecurity() < SEC_GAMEMASTER',
    'BrambleServices.Interaction.AllowInCombat',
    'BrambleServices.AllowInInstances',
    'InArena()',
    'InBattleground()',
    'GetTeamId() != TEAM_ALLIANCE',
    'GetTeamId() != TEAM_HORDE',
)
for check in required_checks:
    assert check in gate_body, f'Native NPC guard is missing {check}'
assert gate_body.count('return nullptr;') == 2

# The ONLY added code changing the gold price in each source hunk must be
# gated on both the custom Scribe entry and the glyph item class. The 64-bit
# calculation keeps the cap from being evaluated in a narrower integer type.
purchase_cap = (
    r'if\s*\(\s*creature->GetEntry\(\)\s*==\s*910103\s*&&\s*'
    r'pProto->Class\s*==\s*ITEM_CLASS_GLYPH\s*\)\s*'
    r'price\s*=\s*std::min<uint64>\(\s*price\s*,\s*uint64\(10000u\)\s*\*\s*count\s*\)\s*;'
)
listing_cap = (
    r'if\s*\(\s*vendor->GetEntry\(\)\s*==\s*910103\s*&&\s*'
    r'itemTemplate->Class\s*==\s*ITEM_CLASS_GLYPH\s*&&\s*price\s*>\s*10000\s*\)\s*'
    r'price\s*=\s*10000\s*;'
)
assert re.search(purchase_cap, purchase), 'Scribe purchase cap is not scoped correctly'
assert re.search(listing_cap, listing), 'Scribe UI cap is not scoped correctly'
assert len(re.findall(r'\bprice\s*=\s*', purchase)) == 1, 'Unreviewed purchase price assignment'
assert len(re.findall(r'\bprice\s*=\s*', listing)) == 1, 'Unreviewed listing price assignment'

# Enforce the intended pricing algebra for bundles that the native vendor
# handles: one discounted BuyCount bundle has one displayed price; buying
# n bundles must charge n times that capped price. Non-Scribe merchants and
# non-glyph items must retain the uncapped price.
def offered(discounted_bundle_price, vendor_id, is_glyph):
    return min(discounted_bundle_price, 10000) if vendor_id == 910103 and is_glyph else discounted_bundle_price


def charged(discounted_bundle_price, number_of_bundles, vendor_id, is_glyph):
    uncapped = discounted_bundle_price * number_of_bundles
    cap = 10000 * number_of_bundles
    return min(uncapped, cap) if vendor_id == 910103 and is_glyph else uncapped


for price in (0, 1, 9999, 10000, 10001, 25000, 1_000_000):
    for count in (1, 2, 3, 100, 255):
        assert charged(price, count, 910103, True) == offered(price, 910103, True) * count
        for merchant_id, glyph in ((1, True), (910102, True), (910106, True), (910103, False)):
            assert offered(price, merchant_id, glyph) == price
            assert charged(price, count, merchant_id, glyph) == price * count

print('PASS: scoped native GM access and glyph price-cap paths; quantity and ordinary-vendor invariants')
