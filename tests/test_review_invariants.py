#!/usr/bin/env python3
"""Source review regression: access, action routing, concurrency, and known behaviors."""
from pathlib import Path
import json,re
r=Path(__file__).resolve().parents[1]
files={name:(r/'src'/name).read_text() for name in (
 'brambletown_services.cpp','brambletown_npc_services.cpp','brambletown_enchanter.cpp',
 'brambletown_scribe.cpp','brambletown_auction_guard.cpp','MP_loader.cpp')}
co=files['brambletown_services.cpp']
st=files['brambletown_npc_services.cpp']
en=files['brambletown_enchanter.cpp']
sc=files['brambletown_scribe.cpp']
au=files['brambletown_auction_guard.cpp']
# The only available provider IDs are 1-5; no forged extra provider action.
assert 'constexpr uint32 MaxProviders = 5;' in co
assert 'index < 2 || index > MaxProviders' in co
assert 'std::unordered_set<uint32> requestedProviders;' in co
assert 'group->requestedProviders.insert(index);' in co
assert 'group->requestedProviders.contains(index)' in co
assert 'it->second.stewardGuid != steward->GetGUID()' in co
assert 'GetTeamId() == TEAM_ALLIANCE' in co and 'GetTeamId() == TEAM_HORDE' in co
assert 'AllowTwoSide.Interaction.Auction' in co
# Lock all mutable registries and never return an unprotected map pointer to callers.
assert 'std::mutex ActiveGroupsMutex' in co
assert co.count('std::lock_guard<std::mutex> lock(ActiveGroupsMutex)') >= 5
assert 'Caller holds ActiveGroupsMutex' in co
assert 'std::mutex ServedFeastsMutex' in st
assert 'std::lock_guard<std::mutex> lock(ServedFeastsMutex)' in st
assert 'std::mutex SelectionsMutex' in sc
assert sc.count('std::lock_guard<std::mutex> lock(SelectionsMutex)') >= 4
# UI entries cannot be invoked using another sender or unrelated creature.
assert 'sender != GOSSIP_SENDER_MAIN || !CanInteract(player)' in st
assert st.count('creature->GetEntry() != 910101') >= 2
assert 'sender != GOSSIP_SENDER_MAIN || !CanEnchant(player)' in en
assert 'sender != GOSSIP_SENDER_MAIN' in sc
# All 151 menu action IDs/slot/enchant triples are unchanged from the verified build.
fixture=json.loads((r/'tests/enchant_action_baseline.json').read_text())['actions']
enums=dict((k,int(v)) for k,v in re.findall(r'^\s*(ENCHANT_\w+)\s*=\s*(\d+)',en,re.M))
lookup={a:{'slot':slot,'enchant':int(eid) if eid.isdigit() else enums[eid]}
    for a,eid,slot in re.findall(r'case (\d+): return \{(\w+), (EQUIPMENT_SLOT_\w+)\};',en)}
assert lookup==fixture
assert 'if (action >= 100 && action <= 250)' in en
assert en.count('Enchant(player, creature,')==2
assert 'GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_FINGER2)' in en
assert 'return OnGossipHello(player, creature);' in en
# Enchanting, Inscription, Tailoring, Leatherworking and Engineering gates.
for profession in ('SKILL_ENCHANTING','SKILL_INSCRIPTION','SKILL_TAILORING',
                   'SKILL_LEATHERWORKING','SKILL_ENGINEERING'):
    assert profession in en.split('static bool MeetsSpecializationRequirement',1)[1].split('// Recheck access',1)[0]
assert 'if (!MeetsSpecializationRequirement(player, action))' in en
assert 'sConfigMgr->GetOption<bool>("BrambleServices.Interaction.AllowInCombat", false)' in en
# Playerbots: real 10s aura -> 1-hour Well Fed; no explicit final-buff grant.
assert 'static constexpr uint32 EatingAura = 57398;' in st
assert 'static constexpr uint32 WellFedAura = 57399;' in st
assert 'bot->CastSpell(bot, EatingAura, true)' in st
assert 'bot->CastSpell(bot, WellFedAura' not in st
assert 'windowMs = 28000;' in st
assert 'sConfigMgr->GetOption<uint32>("BrambleServices.Service.FishFeast.BotMax", 25u)' in st
assert 'bool allDone = true;' in st
assert 'if (allDone)\n            ReportResult();' in st
assert 'ServedFeasts.emplace(npcKey,' in st
# Vendor and auction hooks must remain isolated from ordinary NPCs.
assert 'creature->GetEntry() != ScribeEntry' in sc
assert 'HasValidSelection(player, vendorGuid, &selected)' in sc
assert 'creature->GetEntry() != 910104 && creature->GetEntry() != 910105' in au
print('PASS: entire provider action/catalog contract, guarded state, sender/ownership, profession, feast, and native vendor isolation')
