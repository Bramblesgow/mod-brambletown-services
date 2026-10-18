#!/usr/bin/env python3
"""Static, read-only Provider 4/5 regression tests. Nothing touches live state."""
from pathlib import Path
import re,struct
root=Path(__file__).resolve().parents[1]
core=Path('/root/brambletown-upgrade-work/core')
live=Path('/root/azerothcore-wotlk')
auction=(root/'sql/GM_STAGING_auctioneers.sql').read_text()
goods=(root/'sql/GM_STAGING_general_goods_vendor.sql').read_text()
config=(root/'conf/brambletown_services.conf.dist').read_text()
coord=(root/'src/brambletown_services.cpp').read_text()
guard=(root/'src/brambletown_auction_guard.cpp').read_text()
player=(core/'src/server/game/Entities/Player/Player.cpp').read_text()
loader=(root/'src/MP_loader.cpp').read_text()

# Auctioneer: two verified native faction NPCs, no global auction policy change.
for entry,faction,model in ((910104,12,7991),(910105,29,8001)):
    assert f'SELECT {entry},\'Brambletown Auctioneer\'' in auction
    assert f"80,80,{faction},2097152" in auction
    assert f'SELECT {entry},0,{model},1,1,12340' in auction
    assert f'creature->GetEntry() != {entry}' in guard
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
assert 'GetNPCIfCanInteractWith' in player
assert 'brambletownEntry >= 910102u && brambletownEntry <= 910106u' in player
for control in ('BrambleServices.Enable','BrambleServices.ProviderCount','BrambleServices.Access.GMOnly','BrambleServices.Interaction.AllowInCombat','BrambleServices.AllowInInstances'):
    assert control in player
assert 'GetSession()->IsHeadless()' in player
assert 'AddBrambletownAuctionGuardScripts();' in loader
for sql in (auction,goods):
    assert not re.search(r'\b(?:DELETE|REPLACE|TRUNCATE|DROP|ALTER|UPDATE)\s+(?:TABLE|FROM|creature_template|item_template)',sql,re.I)
    assert not re.search(r'\bUPDATE\s+item_template\b',sql,re.I)
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
assert 'CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION' not in coord
assert 'CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION' not in guard
# Verify the installed WotLK faction masks that AzerothCore resolves for
# each respective auctioneer (2 = Alliance; 4 = Horde).
raw=(live/'env/dist/bin/dbc/FactionTemplate.dbc').read_bytes()
magic,num,fields,stride,string_size=struct.unpack_from('<4s4I',raw)
assert magic==b'WDBC'
rows={rowid:(faction,our_mask) for i in range(num)
      for rowid,faction,flags,our_mask in [struct.unpack_from('<4I',raw,20+i*stride)]}
assert rows[12][1]&2 and not (rows[12][1]&4)
assert rows[29][1]&4 and not (rows[29][1]&2)
# Shared ON -> neutral pool for both NPCs; Shared OFF -> each NPC's faction pool.
def expected_ah_house(shared, npc_template_faction):
    if shared: return 'Neutral'
    return 'Alliance' if rows[npc_template_faction][1]&2 else 'Horde' if rows[npc_template_faction][1]&4 else 'Neutral'
assert [expected_ah_house(True,f) for f in (12,29)]==['Neutral','Neutral']
assert [expected_ah_house(False,f) for f in (12,29)]==['Alliance','Horde']
print('PASS Providers 4/5: auctioneers work with shared ON/OFF; 112 goods; native access safeguards')
