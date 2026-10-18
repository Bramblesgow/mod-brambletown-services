# Brambletown Services — source cleanup and maintainability review

**Date:** October 15, 2026  
**Scope:** Complete module C++ sources, module headers, registration, configuration,
SQL fixtures, native AzerothCore integration patch, tests and operator documentation.  
**Status:** Reviewed/refactored in the *isolated development worktree*. **Not deployed.**
The running worldserver uses the earlier, field-tested binary.

## How the review was performed

Each C++ file was read in source order, function by function, checking:

1. **Purpose and naming:** Does each function/control flow do what its name
   says? Could another maintainer discover the behavior from the source?
2. **Duplication and dead code:** Redundant tables, unreachable branches,
   repeated menu generation, unused constants/includes, stale comments.
3. **Preconditions:** NPC identity, caller session, eligibility, combat,
   instance/PvP policy, sender authenticity and selected action IDs.
4. **Ownership/lifetime:** Which player controls each summoned NPC, its map,
   expiry, despawn and safe cleanup after logout/teleport.
5. **Data integrity:** Preserve WotLK DBC enchant identifiers and restrictions,
   vendor inventory, faction auction routing, item purchasing and price cap.
6. **Concurrency:** No unprotected global mutable maps when player/map hooks
   can run on different map-update threads.
7. **Player experience:** Menu labels and order, feedback, food-buff verification,
   enchant result messages, controlled background bot movement.
8. **Build and compatibility:** Recheck against installed AzerothCore and
   Playerbots headers; run full module tests and a linked worldserver build.

### High-impact cleanup completed

- **Removed approximately 675 lines of duplicated Enchanter execution code**:
  previously the same 151 action IDs, enchant DBC IDs and gear slots existed
  in both `GetEnchantChoice()` and a giant `OnGossipSelect` switch. Actions now
  flow through a **single mapping**; two equipped rings retain their special
  dual-enchant path. Verified against a 151-entry golden pre-cleanup fixture.
- **Removed the second Enchanter main-menu definition**. The Back action now
  invokes the same menu builder used by the initial greeting.
- Deleted 3 unused Enchanter enum values, obsolete commented-out emotes,
  unreachable `break` statements, conflicting error/close/menu paths for
  missing 2H weapons, shields and offhand weapons, and an unused `loader.h`.
- Standardized named types, indentation, include sets, comments and
  incorrectly spelled internal enchant constants **without changing IDs**.
- Fixed a **real access-control issue**: menu-only profession gating can be
  bypassed by fabricated gossip actions. The Enchanter now revalidates
  Enchanting, Inscription, Tailoring, Leatherworking or Engineering requirements
  in the application path, along with DBC-level restrictions.
- Checks `GOSSIP_SENDER_MAIN` and exact NPC identity on Steward and Enchanter
  selections. Provider upper bound is now the actual five providers.
- Replaced redundant provider-number→creature-GUID tracking with a set of
  already-summoned provider numbers (the independent vector still owns the GUIDs
  needed for despawning).
- Protected the **summon registry**, **per-Steward Fish Feast registry**, and
  **Scribe catalog selection registry** with targeted mutex guards; restored
  the explicit field-tested bot state completion flags after finding that
  removing them introduced a fallthrough regression in failure paths.
- Rewrote `README.md` to describe the **currently working five-provider build**
  rather than contradictory, unfinished GM-only plans. Historical planning
  documents remain available in `docs/archive` and upstream license text was
  preserved.

## Line-by-line function-level findings

The line references below correspond to the reviewed source after cleanup. They
are navigational sections, not indicators of newly added code only.

### `src/brambletown_services.cpp` — coordinator, 484 lines

| Lines | Review assessment |
|---|---|
| 1–69 | Clear module purpose, bounded provider count (5), typed configuration and per-player state; mutex protects shared registry |
| 70–116 | Lifetime clamped to 30–600 seconds, human-readable feedback, map-bound safe GUID cleanup, expiry purge requires lock |
| 117–180 | Pre-summon validation for alive/world/PvP/combat/instances; distinct reasons delivered to player |
| 181–223 | Per-provider config read, fixed entry guard for scripted providers, bounded display/offset validation |
| 224–267 | Player-facing spawn offsets, faction-specific NPC template chosen without hardcoding AH markets; group lookup must hold lock |
| 268–359 | Real-session access, expiry cooldown, Steward-only initial spawn, ownership and per-player state insertion guarded by mutex |
| 360–430 | On-demand provider owner check, duplicate prevention, remaining-time expiration; no unauthorized extra provider index |
| 433–484 | Player command registration and early logout/teleport cleanup; all shared map access protected |

**Intentional design:** A player may not summon additional NPCs after the
Steward's original timer expires, even if a timed-despawned creature somehow
remains visible. Faction auction routing is handled by the core's normal AH
configuration, so both shared and separate realm modes work.

### `src/brambletown_npc_services.cpp` — Steward and feast, ~710 lines

| Lines | Review assessment |
|---|---|
| 1–136 | Upstream attribution, complete includes, rotating NPC speech, single-feast registry and independently checked service toggles |
| 137–184 | AI state records only GUIDs and action state; reports **confirmed Well Fed** rather than treating a click as success |
| 185–225 | Scans nearby eligible bots once; radius/count clamped; avoids combat, mounts, flights and already-buffed bots |
| 226–367 | Bounded 28s world-tick state machine; interact/move through existing Playerbots APIs, sit and wait 10s, fallback normal food aura, cancel on combat; explicit state completion to avoid accidental fallthrough |
| 368–431 | Steward gossip registration, exact NPC identity, clear menu order, access checks on open |
| 432–527 | Protected summon submenu, Flask variants and forged-action rejection; sender and owner checks |
| 528–597 | Fish Feast level check, atomic check/spawn/record for one feast, forward position and announcements; completion is asynchronous |
| 598–710 | Ordinary player services preserved; redundant nested CloseGossip calls and unreachable breaks removed; old optional high-impact services remain disabled by default |

**Important:** The module never directly awards Well Fed (57399); if needed it
applies the normal eating aura (57398). Keeping this behavior retains the real
10-second food interaction and respects interruption by combat.

### `src/brambletown_enchanter.cpp` — 862 lines, down from 1,541

| Lines | Review assessment |
|---|---|
| 1–142 | Attribution shortened but retained in full in the upstream license file; unused headers/constants removed; enchant enum values still match old DBC IDs |
| 143–310 | The **single canonical action→DBC enchant→equipment slot map**, one record per available action, 100 through 250; IDs match the golden fixture |
| 311–343 | Reusable user-friendly label builder, including character-level and profession requirement warnings |
| 344–389 | Server-side special profession checks and standard combat/PvP/instance/GM/real-player safeguards |
| 390–458 | One Enchanter main category menu, shared with Back; exact NPC/sender check and generic safe dispatch of all 151 actions |
| 459–759 | Each category offers only its reviewed actions. Leveling head/shoulder/leg and hunter ranged scopes preserved. 2H/shield/offhand invalid-item branches have one consistent outcome |
| 760–862 | Equipment owner/worn-slot/type validation, specialization and DBC restrictions, permanent bonus reapplication, item-name flavor text and visual confirmation |

**Compatibility:** The original database script name `npc_enchantment` is
unchanged although the C++ class is now `BrambletownEnchanter`. Ring enchanting
still applies to each equipped ring separately. The original cosmetic messages
are preserved, including the corrected `{}` equipment-name substitution.

### `src/brambletown_scribe.cpp` — 224 lines

| Lines | Review assessment |
|---|---|
| 1–102 | Ten legal WotLK class choices, class stock IDs, scoped selection state, native interaction eligibility and locking |
| 103–163 | Class-selection gossip, per-character catalog binding to the specific Scribe GUID, normal vendor inventory display |
| 164–224 | Core hooks select only the chosen stock, block invalid purchases, leave all normal merchants untouched, and discard selections on logout |

**Economy:** The class-specific 1g maximum per glyph lives in the separate
AzerothCore core patch, and is applied at both the displayed-price and
charged-price sites. Cheaper glyphs remain cheaper.

### `src/brambletown_auction_guard.cpp`, `src/MP_loader.cpp`, headers

- 45-line AH hook touches only the two special temporary NPCs; native core
  chooses shared/neutral or factional market on its own.
- Loader lists and registers each available service exactly once.
- Small headers declare only interfaces that other module files need.
- Removed one unused header (`src/loader.h`) with a redundant loader prototype.

### Related core patch, SQL and configuration

- Reviewed the 41-line native core patch in
  `patches/brambletown-npc-access-and-scribe-price-cap.patch`. It affects only
  the five fixed temporary NPC IDs for interaction access and only Scribe glyph
  prices for the purchase/display cap. **Do not omit it on future core merges.**
- Kept the idempotent fresh-install SQL for Steward, Enchanter, class Scribe
  catalogs, faction auctioneers and 112-item General Goods vendor. Removed an
  obsolete *proposal-only* SQL draft from the active SQL folder, preserving it
  in the historical archive.
- Clarified disabled-by-default distribution settings. The live configuration
  is intentionally not overwritten with development defaults.

## Test coverage / evidence

- `tests/enchant_action_baseline.json`: all 151 existing action, enchant ID,
  slot triples preserved after the ~675-line deletion.
- `tests/test_review_invariants.py`: protected global mutable state,
  sender/NPC/owner checks, five-provider bound, profession gates, feast actual
  Well Fed checks, class-vendor isolation and auction restrictions.
- `tests/test_enchanter_slot_levels.py`: installed WotLK enchant DBC levels,
  11 ranged scopes, leveling choices and all original action mappings.
- `tests/test_enchanter_menu_polish.py` and `tests/test_steward_menu_layout.py`:
  menu order, effect descriptions, icon dimensions, 2-yard feast position and
  correct `{}` enchant result formatting.
- `tests/test_feast_bot_coordinator.py`: Playerbots interface use, 25-bot
  limit, rotating announcements, session/time boundaries and genuine food buff.
- `tests/test_scribe_catalog.py`, `tests/test_scribe_price_cap.py`,
  `tests/test_providers_4_5.py`: 348 class glyphs, two dynamic AH modes,
  native vendor access, 112 items and Scribe-only pricing.
- `tests/test_draft_structure.py`: configuration and module-loader invariants.
- Full isolated `worldserver` C++ compilation and linking are additional gates;
  these do **not** replace in-game testing after redeployment.

## Intentional behavior / deployment cautions

1. **Not yet field-tested after refactor.** The current live server retains
   the previously tested build. A new build/restart should be treated as a
   controlled maintenance change and verified with all five providers.
2. **Core dependency:** A stock AzerothCore build without the local core patch
   will not preserve the identical Scribe glyph-price cap or access checks.
3. **Unrestricted enchant effects:** The Enchanter is deliberately a convenience
   NPC. DBC level and profession restrictions are enforced, but the service
   does not pretend to be a retail profession/material economy.
4. **Playerbots dependency:** The Steward's feast automation calls existing
   Playerbots actions and yield APIs. If those public APIs change on a future
   Playerbots update, recompile and retest eating/Well Fed results.
5. **No unsafe deletion:** Fresh-install SQL and original upstream license text
   are retained for reproducibility. The old planning docs are archived.

**Reviewer conclusion:** Source structure and documentation are substantially
simpler; action catalogs and player-visible behavior are preserved, with
additional guardrails for forged actions and concurrent player state. Proceed
to a controlled GM test only after the final build and validation succeed.

## October 16 follow-up: Fish Feast behavior after cleanup

A regression review compared the exact previously in-game-confirmed bot
coordinator with the current build. Removing the loop's `allDone` flag had
also removed an unconditional `continue` from both the `foodStarted` and
`useRequested` branches, allowing extra GO actions after a failed/finished
eating attempt. Restored the verified flow verbatim (except for explanatory
comments), without changing the coordinator's existing GUID, distance,
combat or timing rules. A targeted regression test now guards both branches.

The live character database also recorded Well Fed (`57399`) on five currently
online characters during investigation. Because the coordinator deliberately
skips already-Well-Fed characters, an otherwise successful feast summon
can result in no bot eating until the buff expires. Added a useful player
message when zero nearby eligible unbuffed bots can be invited. The database
values are saved aura records, not guaranteed snapshots of live aura state.

**Worktree fix only:** requires a new build and explicit deployment approval.

## October 16 — explicit repeat-feast requirement

Requirement clarification: the owner wants bots to eat every newly placed Fish
Feast, regardless of whether Well Fed is already active. The older scan had
skipped such bots, so existing buffed characters never received the action.
The coordinator now remembers each bot's previous Well Fed duration, waits
for the new eating aura, and counts completion only after the duration
increases (normal one-hour buff refresh). Existing buffs are not removed,
and the service never directly grants 57399. Updated labels now say
"refreshed Well Fed" and the zero-eligible diagnostic no longer refers to
"unbuffed bots". Development-only until explicit deployment and in-game
verification.
