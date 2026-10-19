"""Read-only structural regression checks (NOT a compile or runtime test)."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
config = (root / "conf/brambletown_services.conf.dist").read_text()
coordinator = (root / "src/brambletown_services.cpp").read_text()
services = (root / "src/brambletown_npc_services.cpp").read_text()
loader = (root / "src/MP_loader.cpp").read_text()

settings = set(re.findall(r"^(BrambleServices\.[A-Za-z0-9_.]+)\s*=", config, re.M))
assert len(settings) == len(re.findall(r"^BrambleServices\.[A-Za-z0-9_.]+\s*=", config, re.M)), "Duplicate config key"

assert "BrambleServices.DurationSeconds" in settings
assert "BrambleServices.CooldownSeconds" not in settings
assert "BrambleServices.Access.GMOnly" in settings
def active_value(key):
    values = re.findall(rf"^{re.escape(key)}\s*=\s*([^\s#;]+)", config, re.M)
    assert len(values) == 1, (key, values)
    return values[0]
assert active_value("BrambleServices.Access.GMOnly") == "1"
assert 'session->GetSecurity() < SEC_GAMEMASTER' in coordinator
assert 'player->GetSession()->GetSecurity() < SEC_GAMEMASTER' in services
assert 'Services are currently restricted to Game Masters.' in coordinator
assert active_value("BrambleServices.Provider.2.Enable") == "0"
assert '"DBCStores.h"' in coordinator
assert "sCreatureDisplayInfoStore.LookupEntry(provider.displayId)" in coordinator
assert "creature->SetDisplayId(provider.displayId)" in coordinator
assert coordinator.count('"BrambleServices.DurationSeconds"') == 1
assert "TEMPSUMMON_TIMED_DESPAWN" in coordinator
assert "ActiveGroups" in coordinator and "creatureGuids" in coordinator
assert "PLAYERHOOK_ON_BEFORE_LOGOUT" in coordinator
assert "PLAYERHOOK_ON_BEFORE_TELEPORT" in coordinator
assert 'seconds until services can be called.' in coordinator
assert 'Steward summoned for {}.' in coordinator
assert 'Services cannot be summoned in arenas.' in coordinator
assert 'Services cannot be summoned in battlegrounds.' in coordinator
assert 'Services cannot be summoned inside raids.' in coordinator
assert 'Services cannot be summoned inside dungeons.' in coordinator
assert '"BrambleServices.ProviderCount"' in coordinator
for n in ("Enable", "Entry", "OffsetX", "OffsetY"):
    assert f'".{n}"' in coordinator, n
for n in range(1, 6):
    for key in ("Enable", "Entry", "DisplayId", "OffsetX", "OffsetY"):
        assert f"BrambleServices.Provider.{n}.{key}" in settings

service_names = [
    "Heal", "ResetInstances", "ResetCooldowns", "LeaveCombat",
    "RemoveSickness", "Repair", "ResetTalents", "Bank", "Mailbox", "DualSpec", "FishFeast", "FlaskOfNorth",
]
for n in service_names:
    key = f"BrambleServices.Service.{n}"
    assert key in settings and f'"{key}"' in services, key
assert 'BrambleServices.Service.FishFeast.MinLevel' in settings
assert 'sConfigMgr->GetOption<uint32>("BrambleServices.Service.FishFeast.MinLevel", 1u)' in services
assert "case 11: // Prepare a communal Fish Feast" in services
assert "GOSSIP_SENDER_MAIN, 11" in services
assert 'creature->SummonGameObject(' in services
assert '192675, position.GetPositionX(), position.GetPositionY()' in services
assert '180u, true, GO_SUMMON_TIMED_DESPAWN' in services
assert '57426, true' not in services  # No player Cooking skill/item use.
assert '"GameObject.h"' in services
assert "std::unordered_map<ObjectGuid, FeastClock::time_point> ServedFeasts;" in services
assert "This Steward has already prepared a Fish Feast." in services
assert "Fish Feast cannot be prepared during combat." in services
assert '"BrambleServices.Service.FlaskOfNorth"' in services
assert "GOSSIP_SENDER_MAIN, 12" in services
for action, stat, spell in ((121, "Strength", 67018), (122, "Attack Power", 67017), (123, "Spell Power", 67016)):
    assert f'GOSSIP_SENDER_MAIN, {action}' in services, action
    assert f'case {action}:' in services, action
    assert f'{spell}u' in services, spell
    assert stat in services, stat
assert "player->CastSpell(player, spellId, true)" in services
assert "player->RemoveAurasDueToSpell(otherSpell)" in services
assert 'BrambleServices.Interaction.AllowInCombat' in settings
assert '"BrambleServices.Interaction.AllowInCombat"' in services
assert services.count('!CanInteract(player)') == 2
assert 'sender != GOSSIP_SENDER_MAIN || !CanInteract(player)' in services
assert services.count('creature->GetEntry() != 910101') >= 2
assert "if (!ServiceEnabled(action))" in services
assert "void Addmod_brambletown_servicesScripts()" in loader

# Every configuration key has a human-readable explanation immediately above.
lines = config.splitlines()
for index, line in enumerate(lines):
    if line.startswith("BrambleServices.") and "=" in line:
        assert index > 0 and lines[index - 1].startswith("# "), f"Missing setting explanation: {line}"
        assert len(lines[index - 1].strip()) >= 20, f"Too brief explanation: {line}"

maintainer = (root / "README.md").read_text()
for provider in ("Steward", "Enchanter", "Scribe", "Auctioneer", "General Goods"):
    assert provider in maintainer
assert ".reload config" in maintainer
assert "maximum 5" in maintainer
assert "UPSTREAM_ENCHANTER_LICENSE.md" in maintainer
assert "disposable object files" in maintainer

print("PASS: static configuration/command/lifecycle invariants")
print("NOTE: no C++ compilation, linked API checks, database tests, or gameplay tests performed")

# Steward-only spawning and owner-gated, on-demand provider summons.
assert 'for (Provider const& provider : providers)' not in coordinator
assert 'SpawnAt(player, player, stewardProvider, duration * 1000u)' in coordinator
assert 'SpawnAt(player, steward, provider, remainingMs)' in coordinator
assert 'it->second.stewardGuid != steward->GetGUID()' in coordinator
assert 'group->requestedProviders.contains(index)' in coordinator
assert 'group->expiresAt - Clock::now()' in coordinator
assert 'bool SummonRequestedProvider(Player* player, Creature* steward, uint32 index)' in coordinator
assert 'Call another service provider...' in services
assert 'if (!BrambletownServices::IsStewardOwner(player, creature))' in services
for provider in range(2,6):
    assert f'GOSSIP_SENDER_MAIN, {2000+provider}' in services
assert 'action - 2000' in services
assert 'BrambleServices.Provider.4.AllianceEntry' in settings
assert 'BrambleServices.Provider.4.HordeEntry' in settings
