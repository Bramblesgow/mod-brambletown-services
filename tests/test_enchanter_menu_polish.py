#!/usr/bin/env python3
"""UI labels, shoulder gating and forward feast position; no live changes."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
e=(r/'src/brambletown_enchanter.cpp').read_text()
s=(r/'src/brambletown_npc_services.cpp').read_text()
# The main Enchanter menu has one definition, reused by the Back action.
hello=e.split('bool OnGossipHello',1)[1].split('bool OnGossipSelect',1)[0]
select=e.split('bool OnGossipSelect',1)[1].split('void Enchant',1)[0]
assert 'Enchant Ranged (Bow/Gun/Xbow)' in hello
assert 'Scope Ranged Weapon' not in hello
assert ':24:24:-18' not in hello
assert hello.count(':18:18:-12')>=13
assert '|t[' not in hello
assert 'if (action == 300)' in select
assert 'return OnGossipHello(player, creature);' in select
assert 'Enchant Ranged (Bow/Gun/Xbow)' not in select
named={
 'Mongoose':'(+120 Agility, haste proc)',
 'Berserking':'(+400 AP proc, lowers armor)',
 'Executioner':'(+120 Armor Pen proc)',
 'Black Magic':'(+250 Haste proc)',
 'Blade Ward':'(+Parry proc)',
 'Blood Draining':'(heals at low HP)',
 'Battlemaster':'(healing proc)',
 'Icebreaker':'(Fire damage proc)',
 'Lifeward':'(healing proc)',
 'Darkglow Embroidery':'(mana restore proc)',
 'Lightweave Embroidery':'(+Spell Power proc)',
 'Swordguard Embroidery':'(+Attack Power proc)',
 'Parachute':'(Slow Fall use effect)',
 'Shadow Armor':'(+10 Agility, stealth)',
 'Hyperspeed Accelerators':'(+Haste use effect)',
 'Nitro Boots':'(speed boost use effect)',
 'Hand-Mounted Pyro Rocket':'(damage on use)',
 'Reticulated Armor Webbing':'(+885 Armor)',
}
for name,effect in named.items():
    assert f'AddEnchantChoice(player, "{name} {effect}"' in e,(name,effect)
shoulder=e.split('case 5: // Enchant Shoulders',1)[1].split('case 6: // Enchant Cloak',1)[0]
assert shoulder.index('Custom leveling: +5 Stamina') < shoulder.index('if (player->HasSkill(SKILL_INSCRIPTION)')
assert shoulder.index('Lesser Inscription: +26 Attack Power') < shoulder.index('if (player->HasSkill(SKILL_INSCRIPTION)')
assert shoulder.index('if (player->HasSkill(SKILL_INSCRIPTION)') < shoulder.index('120 Attack Power + 15 Crit')
assert 'creature->GetNearPosition(2.0f, 0.0f)' in s
assert 'position.GetPositionX()' in s and 'position.GetPositionY()' in s and 'position.GetPositionZ()' in s
assert 'Fish Feast prepared in front of the Steward.' in s
assert 'ServedFeasts.emplace(npcKey' in s
print('PASS: compact 18px Enchanter menus, 18 named-effect labels, leveling shoulders unrestricted, feast 2 yards in front')

# AzerothCore SendNotification formats with Acore::StringFormat, not printf.
assert e.count('|cffDA70D6{}') == 3
assert '|cffDA70D6%s' not in e
print('PASS: Enchanter yellow flavor notifications format equipped item name (fmt {})')
