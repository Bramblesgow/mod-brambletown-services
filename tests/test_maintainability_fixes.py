#!/usr/bin/env python3
"""Guard reviewed fixes without assuming source contracts are runtime tests."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
feast = (root / "src/brambletown_npc_services.cpp").read_text()
coordinator = (root / "src/brambletown_services.cpp").read_text()

# Playerbots' DoSpecificAction can reject a go order. Never mark an attempted
# move as successful before the API accepts it; bounded retries avoid spam.
assert 'pending.moveRequested = ai->DoSpecificAction("go", Event("go", link), true);' in feast
assert 'pending.moveAttempts < 3' in feast
assert 'pending.moveRetryDelayMs = 3000;' in feast
assert 'pending.completed = true;' in feast
assert 'pending.moveRequested = true' not in feast
# Steward-owned bot state cannot outlive its source NPC: stop late coordinator
# requests instead of starting a meal with less than a full 28s window.
assert 'summon->GetTimer() <= 30000u' in feast
assert 'This Steward is expiring too soon to coordinate a Fish Feast.' in feast

# The global registry mutex protects reservations and GUIDs only. Do not call
# AzerothCore summon/despawn functions while a registry lock is active.
assert 'group->requestedProviders.insert(index); // Reserve once, before unlocking.' in coordinator
assert 'group->requestedProviders.erase(index); // Failed summon can be retried.' in coordinator
assert 'if (steward)\n                steward->DespawnOrUnsummon();' in coordinator
assert 'if (summoned)\n                summoned->DespawnOrUnsummon();' in coordinator
assert 'DespawnOwned(player, *expiredGroup);' in coordinator
assert 'DespawnOwned(player, *group);' in coordinator
assert 'std::optional<ActiveGroup>' in coordinator

# No changes to production or persistent data are made by these checks.
print("PASS: bounded movement retries, late-feast guard and lock-free world operations contracts")
