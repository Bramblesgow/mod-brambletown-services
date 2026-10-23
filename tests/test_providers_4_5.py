#!/usr/bin/env python3
"""Static, read-only Provider 4/5 regression tests. Nothing touches live state."""
from pathlib import Path
import os,re,struct
root=Path(__file__).resolve().parents[1]
def optional_root(name):
    raw = os.environ.get(name)
    if raw is None:
        return None
    assert raw, f"{name} cannot be empty when explicitly configured"
    path = Path(raw)
    assert path.is_dir(), f"{name} must point to an existing AzerothCore root: {raw}"
    return path

core=optional_root('BRAMBLETOWN_CORE_ROOT')
live=optional_root('AZEROTHCORE_LIVE_ROOT')
strict_value = os.environ.get('BRAMBLETOWN_REQUIRE_NEW_GUARD', '0')
assert strict_value in ('0', '1'), 'BRAMBLETOWN_REQUIRE_NEW_GUARD must be 0 or 1'
strict_guard = strict_value == '1'
if strict_guard:
    assert live or core, (
        'Strict native guard verification requires AZEROTHCORE_LIVE_ROOT '
        'or BRAMBLETOWN_CORE_ROOT'
    )
auction=(root/'sql/GM_STAGING_auctioneers.sql').read_text()
goods=(root/'sql/GM_STAGING_general_goods_vendor.sql').read_text()
config=(root/'conf/brambletown_services.conf.dist').read_text()
coord=(root/'src/brambletown_services.cpp').read_text()
guard=(root/'src/brambletown_auction_guard.cpp').read_text()
player=None
if core:
    player_path=core/'src/server/game/Entities/Player/Player.cpp'
    assert player_path.is_file(), f"BRAMBLETOWN_CORE_ROOT missing Player.cpp: {player_path}"
    player=player_path.read_text()
else:
    print('SKIP: external Player.cpp verification (set BRAMBLETOWN_CORE_ROOT)')
native_patch=(root/'patches/brambletown-npc-access-and-scribe-price-cap.patch').read_text()
loader=(root/'src/MP_loader.cpp').read_text()

# Auctioneer: two verified native faction NPCs, no global auction policy change.
for entry,faction,model in ((910104,12,7991),(910105,29,8001)):
    assert f'SELECT {entry},\'Brambletown Auctioneer\'' in auction
    assert f"80,80,{faction},2097152" in auction
    assert f'SELECT {entry},0,{model},1,1,12340' in auction
    assert f'creature->GetEntry() != {entry}' in guard
    if player is not None:
        assert f'brambletownEntry == {entry}u' in player
assert 'MISCHOOK_CAN_SEND_AUCTIONHELLO' in guard
assert 'sConfigMgr->GetOption<bool>("BrambleServices.Enable", false)' in guard
assert 'BrambleServices.Access.GMOnly' in guard
assert 'TEAM_ALLIANCE' in guard and 'TEAM_HORDE' in guard
assert 'player->GetTeamId() == TEAM_ALLIANCE' in coord and 'player->GetTeamId() == TEAM_HORDE' in coord
assert 'sConfigMgr->GetOption<uint32>("BrambleServices.ProviderCount", 5u) < 4u' in guard

# Goods: exactly 112 tested source item IDs, 112 contiguous unique vendor slots.
assert '910106' in goods
assert "'Brambletown Provisioner','General Goods & Repair'" in goods
assert '35,4224' in goods
assert 'SELECT 910106,0,7355,1,1,12340' in goods
first=re.search(r'SELECT 910106 AS entry, (\d+) AS slot, (\d+) AS item',goods)
assert first
rows=[(int(first.group(1)),int(first.group(2)))]+[(int(slot),int(item)) for slot,item in re.findall(r'UNION ALL SELECT 910106,(\d+),(\d+)',goods)]
assert len(rows)==112,len(rows)
assert [slot for slot,item in rows]==list(range(112))
assert len({item for slot,item in rows})==112
assert 0 < len(rows) <= 150
for item in (159,1179,33445,2512,41586,2516,41584,21177,17031,4536,33454,33451,33452,35950,35952,35948,8925,6256):
    assert item in [i for _,i in rows],item
for diet in ('meat','fish','cheese','bread','fungus','fruit'):
    assert f'Pet-food {diet}' in goods
assert 'PLAYERHOOK_ON_SEND_LIST_INVENTORY' in (root/'src/brambletown_scribe.cpp').read_text()
assert 'BrambleServices.Provider.5.Enable = 0' in config
assert 'BrambleServices.Provider.4.Enable = 0' in config
assert 'BrambleServices.ProviderCount = 5' in config
assert 'index == 4 && (provider.allianceEntry != 910104u || provider.hordeEntry != 910105u)' in coord
assert 'index == 5 && provider.entry != 910106u' in coord
if player is not None:
    assert 'GetNPCIfCanInteractWith' in player
# The staged patch must guard all native interactions, including Steward mailbox.
assert 'brambletownEntry >= 910101u && brambletownEntry <= 910106u' in native_patch
# The separately deployed core may still carry the older guard until approved deployment.
if player is not None:
    # The staged core may retain the previous native guard until deployment.
    assert any(f'brambletownEntry >= {start}u && brambletownEntry <= 910106u' in player
               for start in (910101, 910102))
def braced_body(source, marker):
    """Extract one C++ braced block after a unique source anchor."""
    assert source.count(marker) == 1, f"Expected one C++ anchor: {marker}"
    remainder = source.split(marker, 1)[1]
    opening = remainder.find('{')
    assert opening >= 0, f"Missing C++ block after {marker}"
    depth = 0
    for position in range(opening, len(remainder)):
        if remainder[position] == '{':
            depth += 1
        elif remainder[position] == '}':
            depth -= 1
            if depth == 0:
                return remainder[opening + 1:position]
    raise AssertionError(f"Unbalanced C++ block after {marker}")


if strict_guard:
    checked_root = live or core
    checked_path = checked_root/'src/server/game/Entities/Player/Player.cpp'
    assert checked_path.is_file(), f'Strict guard source missing: {checked_path}'
    native_source = checked_path.read_text()
    interaction = braced_body(
        native_source, 'Creature* Player::GetNPCIfCanInteractWith(ObjectGuid const& guid, uint32 npcflagmask)'
    )
    guard = braced_body(
        interaction, 'if (brambletownEntry >= 910101u && brambletownEntry <= 910106u)'
    )
    assert 'brambletownEntry = creature->GetEntry();' in interaction
    assert 'brambletownEntry <= 910103u ? brambletownEntry - 910100u' in guard
    assert 'brambletownEntry <= 910105u ? 4u : 5u' in guard
    required_denials = (
        'GetSession()->IsHeadless()',
        '!sConfigMgr->GetOption<bool>("BrambleServices.Enable", false)',
        'BrambleServices.ProviderCount',
        'BrambleServices.Provider.',
        'BrambleServices.Access.GMOnly',
        'GetSession()->GetSecurity() < SEC_GAMEMASTER',
        'BrambleServices.Interaction.AllowInCombat',
        'BrambleServices.AllowInInstances',
        'InArena()', 'InBattleground()',
    )
    first_deny = guard.split('return nullptr;', 1)[0]
    for check in required_denials:
        assert check in first_deny, f'Native Steward guard missing denial: {check}'
    # A raw mailbox opcode must not bypass the separate Mailbox service toggle.
    mailbox_deny = (
        r'if\s*\(\s*brambletownEntry\s*==\s*910101u\s*&&\s*'
        r'\(npcflagmask\s*&\s*UNIT_NPC_FLAG_MAILBOX\)\s*&&\s*'
        r'!sConfigMgr->GetOption<bool>\("BrambleServices.Service.Mailbox",\s*true\)\s*\)'
        r'\s*return nullptr\s*;'
    )
    assert re.search(mailbox_deny, guard), 'Disabled Steward mailbox can be used via native packets'
    assert 'brambletownEntry == 910104u && GetTeamId() != TEAM_ALLIANCE' in guard
    assert 'brambletownEntry == 910105u && GetTeamId() != TEAM_HORDE' in guard
    print(f'PASS: scoped strict native Steward, mailbox, provider and faction guards in {checked_root}')
if player is not None:
    for control in ('BrambleServices.Enable','BrambleServices.ProviderCount',
                    'BrambleServices.Access.GMOnly','BrambleServices.Interaction.AllowInCombat',
                    'BrambleServices.AllowInInstances'):
        assert control in player
    assert 'GetSession()->IsHeadless()' in player
assert 'AddBrambletownAuctionGuardScripts();' in loader
for sql in (auction,goods):
    assert not re.search(r'\b(?:DELETE|REPLACE|TRUNCATE|DROP|ALTER|UPDATE)\s+(?:TABLE|FROM|creature_template|item_template)',sql,re.I)
    assert not re.search(r'\bUPDATE\s+item_template\b',sql,re.I)
assert 'CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION' not in coord
assert 'CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION' not in guard
if live:
    for file,ids in [('CreatureDisplayInfo.dbc',[7991,8001,7355]),('FactionTemplate.dbc',[12,29])]:
        data=(live/'env/dist/bin/dbc'/file).read_bytes()
        magic,n,cols,step,strings=struct.unpack_from('<4s4I',data)
        assert magic==b'WDBC'
        found={struct.unpack_from('<I',data,20+i*step)[0] for i in range(n)}
        assert all(identifier in found for identifier in ids)
    # Native auction routing must remain dynamic for BOTH supported AH layouts,
    # regardless of the current production config.
    manager=(live/'src/server/game/AuctionHouse/AuctionHouseMgr.cpp').read_text()
    handler=(live/'src/server/game/Handlers/AuctionHouseHandler.cpp').read_text()
    assert 'GetAuctionHouseEntryFromFactionTemplate(unit->GetFaction())' in handler
    assert 'sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION)' in manager
    assert 'houseid = AuctionHouseId::Neutral;' in manager
    assert 'uEntry->ourMask & FACTION_MASK_ALLIANCE' in manager
    assert 'uEntry->ourMask & FACTION_MASK_HORDE' in manager
    assert 'houseid = AuctionHouseId::Alliance;' in manager
    assert 'houseid = AuctionHouseId::Horde;' in manager
    # Verify the installed WotLK faction masks that AzerothCore resolves for
    # each respective auctioneer (2 = Alliance; 4 = Horde).
    raw=(live/'env/dist/bin/dbc/FactionTemplate.dbc').read_bytes()
    magic,num,fields,stride,string_size=struct.unpack_from('<4s4I',raw)
    assert magic==b'WDBC'
    faction_rows={rowid:(faction,our_mask) for i in range(num)
                  for rowid,faction,flags,our_mask in [struct.unpack_from('<4I',raw,20+i*stride)]}
    assert faction_rows[12][1] & 2
    assert not (faction_rows[12][1] & 4)
    assert faction_rows[29][1] & 4
    assert not (faction_rows[29][1] & 2)
    # Shared ON -> neutral pool for both NPCs; Shared OFF -> each NPC's faction pool.
    def expected_ah_house(shared, npc_template_faction):
        if shared:
            return 'Neutral'
        if faction_rows[npc_template_faction][1] & 2:
            return 'Alliance'
        if faction_rows[npc_template_faction][1] & 4:
            return 'Horde'
        return 'Neutral'
    assert [expected_ah_house(True,f) for f in (12,29)]==['Neutral','Neutral']
    assert [expected_ah_house(False,f) for f in (12,29)]==['Alliance','Horde']
else:
    print('SKIP: external DBC/auction source checks (set AZEROTHCORE_LIVE_ROOT)')
print('PASS Providers 4/5: staged auction/vendor contracts and optional native routing checks')
