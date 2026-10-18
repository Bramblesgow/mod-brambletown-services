#!/usr/bin/env python3
"""Read-only static test of the generated 3.3.5a class glyph vendor catalog."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
sql=(root/'sql/GM_STAGING_scribe_class_glyph_vendors.sql').read_text()
scribe=(root/'src/brambletown_scribe.cpp').read_text()
loader=(root/'src/MP_loader.cpp').read_text()
conf=(root/'conf/brambletown_services.conf.dist').read_text()
expected={1:34,2:34,3:33,4:35,5:33,6:34,7:35,8:37,9:35,11:38}
assert sum(expected.values())==348
for class_id,count in expected.items():
    proxy=910120+class_id
    anchor=f'SELECT {proxy} AS entry, 0 AS slot,'
    start=sql.index(anchor)
    end=sql.index(') AS catalog',start)
    group=sql[start:end]
    found=[(0,int(re.search(r'SELECT \d+ AS entry, 0 AS slot, (\d+) AS item',group).group(1)))]
    found += [(int(a),int(b)) for a,b in re.findall(rf'UNION ALL SELECT {proxy},(\d+),(\d+)',group)]
    assert len(found)==count,(class_id,len(found))
    assert [slot for slot,_ in found]==list(range(count)),class_id
    assert len({item for _,item in found})==count
    assert f'{{{class_id}, ' in scribe
assert 'GetCurrentVendor()' in scribe
assert 'OnPlayerSendListInventory' in scribe
assert 'OnPlayerBeforeBuyItemFromVendor' in scribe
assert 'OnPlayerCanSellItem' in scribe
assert 'void OnPlayerBeforeLogout' in scribe
assert 'HasValidSelection' in scribe
assert 'item = 0;' in scribe
assert 'SendListInventory(creature->GetGUID(), catalog)' in scribe
assert 'BrambleServices.Provider.3.Enable' in scribe
assert 'BrambleServices.Access.GMOnly' in scribe
assert 'AddBrambletownScribeScripts();' in loader
assert 'BrambleServices.Provider.3.Enable = 0' in conf
assert 'ScriptName' in sql and "'brambletown_scribe'" in sql
assert 'npcflag' in sql and "35,129" in sql
assert len(re.findall(r'INSERT INTO npc_vendor\(',sql))==10
assert 'DELETE FROM ' not in sql and 'UPDATE creature_template' not in sql
print('PASS: 10 selectable classes, 348 unique glyph items, native vendor hooks and safe staging SQL')
