<!-- Historical planning reference. Not the current release documentation. -->

# GM-only testing, live reload, NPC identities and expansion backlog

## GM-only staging behavior

Set `BrambleServices.Enable = 1` and retain `BrambleServices.Access.GMOnly = 1`.
The `.services` command is registered at SEC_PLAYER so permissions can switch
without a restart, but its handler checks the current session's account
security against `SEC_GAMEMASTER` (level 2). The adapted General Services
NPC also checks GM level on BOTH gossip open and action selection. Playerbot
headless sessions are blocked.

**Important limitation:** this GM-only guarantee currently covers the
command and the adapted **General Services NPC only**. The standalone
upstream Enchanter has not been adapted with the same interaction guard.
Provider 2 defaults **disabled** and must NOT be enabled in GM testing
until its permission checks are implemented and reviewed. The Glyph vendor
is also disabled until its implementation is complete.

Testing on the actual live server still requires a reviewed module build,
module install, NPC template migration, and a worldserver restart.
GM-only mode prevents non-GM use but does not make first deployment
hot-loadable or risk-free. A separate test instance is safer.

## Live config changes

On this installed AzerothCore revision, the GM command:

`.reload config`

calls `World::LoadConfigSettings(true)` which calls
`sConfigMgr->Reload()`, which reloads module configuration files.
This module fetches switches and durations from ConfigMgr when a player
uses the summon command or interacts with the General Services NPC.
After editing the INSTALLED `brambletown_services.conf`, a GM can run
`.reload config`; settings will affect subsequent actions without a
restart. `BrambleServices.Access.GMOnly` can be switched live. Note:
`.reload config` also reloads **other worldserver/module settings**, not
just Brambletown. Changes to C++ sources, new scripts, and plugin installation
still require rebuilding/restarting. Names/titles stored in
`creature_template` require validated DB reload/restart, and already-
spawned NPC groups retain their original lifetime and appearance. Provider
appearance overrides are applied at the time of **new** summons.

## Character names, titles, and distinct models

The World of Warcraft client obtains creature names and subtitles from
`creature_template` rather than a simple .conf setting. Use unique
creature template entries rather than mutating the upstream global NPC IDs.
The SQL draft `sql/REVIEW_ONLY_npc_identity_templates.sql` is a review-only
proposal, NOT ready to apply:

| Proposed template | Display model | NPC name | Title | Status |
|-------------------|--------------|----------|-------|--------|
| 910101 | 31833 | Brambletown Steward | Character Services | SQL draft; General Services script exists |
| 910102 | 9353 | Brambletown Enchanter | Master Enchanter | SQL draft; upstream enchanter not integrated |
| 910103 | TBD | Brambletown Scribe | Glyph Merchant | Reserved, no script or template yet |

The Steward and Enchanter models are different and derive from the upstream
module's original example models. The config includes an optional
`Provider.N.DisplayId` override, validated against the server's
`CreatureDisplayInfo` DBC; value 0 uses the NPC template model.
If choosing different models later, validate their IDs before enabling.
Avoid overwriting existing IDs; inspect existing world DB entries before
applying the SQL migration. Names/titles are DB template properties.

## Future optional NPC providers (not yet implemented)

- **Auctioneer NPC**: provides a standard auction-house interface with
  faction/neutral restrictions and normal auction transaction rules.
- **Alchemy NPC**: potential alchemy-related convenience or profession vendor;
  exact service scope not yet selected.
- **Food buff option within General Services (replaces Food Buff NPC)**:
  implemented as the configurable **Prepare Fish Feast** gossip option.
  The Steward creates game object **192675** (normal WotLK Fish Feast,
  interactive game-object spell 57397) at its feet for **180 seconds** via
  `WorldObject::SummonGameObject(..., GO_SUMMON_TIMED_DESPAWN)`. It does
  not require the player's Cooking skill or Fish Feast inventory item,
  and the feast is not owned by the temporary Steward or the player.
  One feast is allowed per temporary Steward to discourage spam.
  `BrambleServices.Service.FishFeast.MinLevel` controls minimum requester
  level (defaults to 1). Visibility, terrain placement, buff eligibility
  for low-level players, and timeout must be confirmed in staging.
- **Flask of the North within General Services (not a separate NPC)**:
  `BrambleServices.Service.FlaskOfNorth` enables a submenu where the
  player chooses +40 Strength (spell 67018), +80 Attack Power (67017), or
  +47 Spell Power (67016). The selected aura is cast on the player
  directly, without using item 47499, spell 67019, or Alchemy. Other
  Flask of the North variants are removed on success to keep only one
  active at a time. All existing GM, combat, location, and bot restrictions
  still apply. Validate buff duration, stacking with other flasks/elixirs,
  switching between variants, and persistence through death in staging.

New standard vendors can be enabled by adding numbered provider settings
(1 through 12) plus database templates; specialized services require a
script component and encounter-security review. These are BACKLOG only.

## Current test sequence (once a separately approved staging build exists)

1. Default conf master enable 0; confirm command unavailable.
2. Set master enable 1, GMOnly 1; reload config and confirm player blocked.
3. Confirm GM can summon the lone General Services NPC; check chat duration.
4. Verify ordinary players cannot use the GM's General Services NPC.
5. Toggle each individually supported service and reload config; confirm its
   menu option disappears and forged selections are rejected.
6. Confirm updates to provider display IDs affect next spawn only.
7. Set GMOnly 0 and reload config; confirm ordinary player can use after
   approval and no restart.
8. Confirm enabling provider 2 remains prohibited until enchanter guards exist.

No compilation, server restart, or database writes performed by this document.

## Superseding Provider 3 implementation checkpoint (October 15, 2026)

Provider 3's previously reserved Scribe is now coded in the development branch
as `brambletown_scribe`. Class-select gossip drives 10 native vendor catalogs,
using NPC 910103 and proxy inventory keys 910121-910129/910131. The single
native vendor UI for each class contains 33-38 glyphs. No production SQL or
restart has occurred. The earlier reserved/no-script table describes the former
state, not the present draft. See README and the review-only SQL.

## Updated Providers 4/5 development checkpoint (October 15, 2026)

The Auctioneer is now implemented using two native faction template entries,
`910104/910105`, and is summoned only through Steward option 4. Provider 5
has a 112-item native goods/repair vendor catalog at `910106`, also on demand.
Provider 4 uses the existing combined-AH configuration (`AllowTwoSide.Interaction.Auction=1`)
without modifying the economy. A staged isolated-core interaction guard secures
native purchases and repairs. No live changes have been performed.
