# Brambletown Services

**AzerothCore 3.3.5a / WotLK — five on-demand services providers.**

This is the development source for the currently deployed Brambletown Services
module. The running server remains unchanged by edits to this repository;
compile, install and restart require a separate deployment step. Historic
planning notes have been moved to [`docs/archive/`](docs/archive/) so reviewers
are not misled by old GM-only/draft status descriptions.

## Player flow

1. Type `.services` to summon the **Steward (Provider 1)** for the configured
   lifetime (default **120 seconds**).
2. Speak to the Steward for healing, repairs, bank, mail, talents, Fish Feast,
   Flask of the North, and other individually configurable services.
3. Choose **Call another service provider...** to summon the Enchanter (2),
   Scribe (3), faction Auctioneer (4), or General Goods merchant (5).
4. Each secondary provider is summoned only when requested and only once per
   Steward session. All expire with that Steward; the cooldown starts on
   the original summon and is not reset by secondary NPC requests.

Only the summoning player may ask the Steward to call other NPCs. Other
eligible players may use allowed services on an already-summoned NPC.

## Providers

| # | Template(s) | Features |
|---|---|---|
| 1 | `910101` | Steward: character services, Fish Feast, Flask of the North |
| 2 | `910102` | Enchanter: guarded enchant action catalog, including ranged scopes, low-level enchants, stat/effect labels |
| 3 | `910103` | Scribe: choose any of 10 WotLK classes, then buy Major/Minor glyphs in normal vendor UI |
| 4 | `910104` (Alliance), `910105` (Horde) | Native auctioneer; the core dynamically uses shared or factional auction pools |
| 5 | `910106` | Native vendor with 112 food/drink/ammunition/pet-food/reagent/supply items, repair, sell and buyback |

The Scribe's 348 glyphs use stock template IDs `910121`–`910129` and `910131`.
A **Scribe-only** native price cap restricts glyphs to **at most 1 gold** per
unit while preserving lower prices. Ordinary vendors and auctions are not
repriced. The Enchanter uses reviewed WotLK DBC enchant IDs and validates the
selected action, item slot, item owner, level, profession skill and type.

### Fish Feast and Playerbots

Fish Feast (`192675`) appears roughly two yards ahead of the Steward, persists
for 180 seconds and may be prepared once per Steward session. The Steward
rotates five spoken announcements. When `AutoBots` is enabled, the Steward
invites up to **25** eligible Playerbots initially within the configurable
**12-yard** radius. It uses existing Playerbots `use` and `go` actions, keeps
bots sitting while the genuine 10-second food aura (`57398`) runs, and checks
the actual Well Fed aura (`57399`). If the click failed, the module attempts
the normal eating aura, not a direct Well Fed award. The coordinator runs for
at most **28 seconds** and reports newly refreshed/gained buffs. Bots with
an existing Well Fed aura are **still invited** to eat again; it skips or
releases bots in combat, mounted, dead or ineligible. No Playerbots source patch is
needed for this integration; its public headers/API remain a dependency.

## Configuration and runtime

`conf/brambletown_services.conf.dist` is a **safe disabled-by-default template**;
it is not the running realm's configuration. The currently deployed configuration
permits regular players (`BrambleServices.Access.GMOnly = 0`), enables all five
providers and enables the 25-bot Fish Feast coordinator.

Key settings:

- `BrambleServices.Enable`: master enable switch
- `BrambleServices.Access.GMOnly`: restrict summon/interaction to Game Masters
- `BrambleServices.DurationSeconds`: 30–600 seconds, default 120
- `BrambleServices.AllowInCombat`, `.AllowInInstances`: summon restrictions
- `BrambleServices.Interaction.AllowInCombat`: subsequent service interactions
- `BrambleServices.ProviderCount`: number of available providers, **maximum 5**
- `BrambleServices.Provider.N.Enable`, `.Entry`, `.DisplayId`, `.OffsetX`, `.OffsetY`: configured NPCs and summon offsets
- `BrambleServices.Provider.4.AllianceEntry` / `.HordeEntry`: **fixed** native auctioneer IDs
- `BrambleServices.Service.*`: per-action Steward options
- `BrambleServices.Service.FishFeast.AutoBots`, `.BotRadius`, `.BotMax`: defaults enabled, 12 yards, 25 bots

Other players cannot use `.services` to bypass arena, battleground, character
alive, or configured instance restrictions. Invalid provider action requests
are checked independently of visible menus. `.reload config` can apply a
configuration change but reloads other worldserver settings as well; use care.

## Code map for reviewers

| File | Responsibility |
|---|---|
| `src/brambletown_services.cpp` | `.services` command, spawn/cooldown registry, faction selection, owner checks, logout/teleport cleanup |
| `src/brambletown_npc_services.cpp` | Steward menus and services, Fish Feast bot state machine and NPC announcements |
| `src/brambletown_enchanter.cpp` | Enchant action table, menu labels, requirements and equipment mutation |
| `src/brambletown_scribe.cpp` | Class picker and session-scoped native vendor inventory hooks |
| `src/brambletown_auction_guard.cpp` | Protect the two temporary faction-specific native auctioneers |
| `src/MP_loader.cpp` | Registers scripts, by name, in the AzerothCore module loader |
| `src/brambletown_provider_summons.h` | Steward and coordinator interface |
| `conf/brambletown_services.conf.dist` | Documented default settings |
| `sql/GM_STAGING_*.sql` | Historical/fresh-install idempotent template/inventory migrations; back up DB before applying |
| `patches/brambletown-npc-access-and-scribe-price-cap.patch` | Reviewed AzerothCore native interaction guard and Scribe display/charge price cap |

**Integration detail:** The native core patch is separate from the module and
must accompany the module binary when reproducing this deployment. Module code
may use the Playerbots module's public API but does not alter Playerbots itself.
The full upstream Enchanter attribution and license are retained in
`UPSTREAM_ENCHANTER_LICENSE.md`; original General Services attribution is
preserved in its header.

## Development validation

In the isolated development worktree, use:

```bash
cd /root/brambletown-services-work/mod-brambletown-services
set -e
for test in tests/test_*.py; do python3 "$test"; done
python3 tests/check_cpp_syntax.py
cmake --build /root/brambletown-upgrade-work/build-production --target worldserver -j 2
```

When optional external validation data is available, set PLAYERBOTS_SRC to
the installed Playerbots src directory and SPELL_ICON_DBC to a readable
SpellIcon.dbc file. Missing optional data skips only its external checks.
The new portable checks also assert mailbox registration, script-name
migration, vendor inventory capacity protection and Engineering visibility.

The **151-entry golden fixture** at `tests/enchant_action_baseline.json` is a
regression baseline for enchant gossip actions, DBC IDs and equipment slots.
Other tests validate spell data, Scribe inventory size and prices, native
auction faction behavior, all five provider configurations and Fish Feast
coordination. See `docs/CODE_REVIEW_2026-10-15.md` for review findings and
remaining runtime-test limits.

This command builds the isolated development binary only; it does **not**
replace `/root/azerothcore-wotlk/env/dist/bin/worldserver`, restart the server,
or apply SQL. Deploy only after approval, with backup and online-player warning.

### Rejected-command feedback (October 15, 2026, staged)

A `.services` request that fails eligibility (dead, arena, battleground,
combat, raid, dungeon, cooldown, disabled service, etc.) now **returns
success to the chat command dispatcher after giving its specific reason**.
This is not a successful summon: `CanSpawn()` still blocks those conditions.
It prevents stock AzerothCore from printing misleading `### USAGE: services`
help text after an already-explained rejection. When dead inside a dungeon,
the first reported reason is the death restriction; when alive inside a
forbidden dungeon, the dungeon restriction is reported. This fix is part of
the isolated reviewed development build, **not yet deployed**.

### Fish Feast — refresh Well Fed on every new feast

Every time a Steward prepares a new Fish Feast, eligible nearby Playerbots are
invited even if they already have Well Fed (spell 57399). The coordinator does
not remove the old buff. It tracks the remaining duration of Well Fed and
counts a successful new meal **only when that duration increases** after the
normal 10-second eating aura (57398) begins. Both previously buffed and
unbuffed bots follow the same normal 57398 -> 57399 spell chain. An old active
buff alone never satisfies the new-meal completion check. A no-eligible-bots
message lists actual reasons: out of range, dead, mounted, in combat or in a
different phase. See `tests/test_feast_refresh_every_drop.py`.
