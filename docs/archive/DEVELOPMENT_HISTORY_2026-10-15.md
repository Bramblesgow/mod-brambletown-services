<!-- Historical planning reference. Not the current release documentation. -->

# Brambletown Services - development / peer-review branch

**Not built, installed, or tested. Do not deploy this draft to the live server.**

AzerothCore reference revision: `2a2211cd8` (WotLK 3.3.5a). The workspace is separate from production: `/root/brambletown-services-work/`.

## Intent

A real player uses `.services` to summon only Provider 1, the Steward. The Steward offers a **Call another service provider...** menu for separately requesting providers 2–5. No additional NPC spawns automatically. The Steward and its individually summoned providers share the original summon lifetime and cooldown, and no character can have two active groups. Providers are configured by numbered entries. A standard NPC vendor can later be added with configuration and its own existing creature template; providers with custom behavior also need their supporting scripts.

### Player feedback

- Successful summon (120s): `Steward summoned for 2 minutes. Ask the Steward to call other providers.`
- During cooldown: `90 seconds until services can be called.` (remaining time rounds up)
- Combat: `Services cannot be summoned while in combat.`
- Arena: `Services cannot be summoned in arenas.`
- Battleground: `Services cannot be summoned in battlegrounds.`
- Dungeon/raid: specifically names the restricted destination type.
- Dead: `You must be alive to summon services.`
- Entire module disabled: `Services are currently unavailable.`
- Steward disabled: `The Steward service is disabled.`
- Invalid provider configuration: `Services are unavailable due to a server configuration issue.`
- Failed summon: `Services could not be spawned. Contact a server administrator.`

The coordinator uses `BrambleServices.DurationSeconds` for **both** TempSummon despawn and cooldown. There is no separate cooldown setting. Values clamp to 30-600 seconds. An active group stores its expiration and spawned creature GUIDs; another group cannot be summoned before expiry. It uses the actual remaining time, rather than restarting on failed attempts.

On logout or teleport it attempts to despawn owned creatures while the original map is accessible; the cooldown still lasts to its original expiration. Native timed despawn handles cases where old-map creatures are no longer reachable. No world-database creature spawns are created.

### Configuration

See `conf/brambletown_services.conf.dist`.

- `BrambleServices.Enable` defaults OFF.
- `BrambleServices.DurationSeconds` defaults to 120; one timer for lifetime and summon cooldown.
- Spawn restrictions default to no combat and no instances. Arenas and battlegrounds are unconditionally prohibited.
- `BrambleServices.Interaction.AllowInCombat` defaults OFF for **general Services NPC** both on menu open and selection.
- `BrambleServices.ProviderCount` is currently limited to 12, with numbered `.Enable`, `.Entry`, `.OffsetX`, `.OffsetY`. Add provider 4 by defining provider 4 settings and setting count to 4. Future custom NPC services still require their own scripts.
- All twelve general services, including Fish Feast and Flask of the North, have independently enforced booleans; disabled menu actions are blocked from direct selection. `ResetInstances`, `ResetCooldowns` and `LeaveCombat` default OFF.
- Provider 3 (Scribe/glyph vendor) defaults OFF pending full build, staging SQL installation and runtime verification.

## Upstream reference code

- `../upstream/mod-npc-spawn`: command registration and timed summon pattern (AGPLv3).
- `../upstream/mod-npc-services`: general services behavior, adapted in `src/brambletown_npc_services.*` with preserved attribution.
- `../upstream/mod-npc-enchanter`: standalone enchant services reference; not yet integrated.
- C++ coordinator code has no verbatim upstream copied blocks.

## Outstanding development (do NOT call the module complete)

1. Validate loader, command registration, configuration and hooks against installed core in a **non-production** build.
2. Inspect Playerbot scheduling assumptions; the cooldown registry is accessed from world player hooks and commands and assumes world-thread serialization.
3. Add terrain/height/collision/LoS validation for NPC spawn positions; current offsets rely on player Z.
4. General Services NPC template entry 910101 is proposed with script name `brambletown_npc_services` in a safe, reviewed DB migration. Upstream SQL deletes/recreates template and must NOT be run unmodified.
5. Integrate external enchanter module, include proper licensing and validate enchant restrictions. Current interaction guard applies to general Services only.
6. Implement Supplies vendor: standard merchant NPC with safe sell/buyback, repair, ammunition, food, water and pet-food catalog for levels 1-80. Can be a fourth indexed provider if separating merchants makes more sense.
7. Scribe glyph vendor developed: 10 class-selectable native vendor lists totaling 348 glyphs; compile, review prices and run native purchase tests before release.
8. Confirm that providers are usable by other players without gaining GM privileges or bypassing level/instance restrictions.
9. Review progressive checks on sensitive general services: Reset Instances, Reset Cooldowns, Leave Combat (default OFF). Respect encounter and raid lockouts.
10. Design per-provider interaction guards, configurable service prices, and reload behavior before enabling on production.

## Acceptance scenarios before production

1. Eligible real player summons; proper duration text; cooldown seconds count down; no second group.
2. Dead, combat, arena, battleground, dungeon, raid and invalid configuration all produce the correct message.
3. Zero providers doesn't start a cooldown.
4. Bad Steward configuration prevents initial summon; invalid optional provider config only rejects that individual request.
5. Logout/teleport despawns group where reachable; reconnect does not reset cooldown.
6. Disabled general services absent from menu and impossible to trigger directly.
7. Playerbot/headless session cannot summon or invoke general services.
8. Two real players can have distinct groups, including during a concurrent dungeon queue.
9. NPCs don't persist in world database; no unsafe spawn locations.
10. Enchanting, glyph purchases, supplies, selling and buyback tested independently after implemented.

**STOP: no compilation, database changes, server restart, deployment or production modification without a separate approval.**

## GM-only access, live config reload, and visual identities (October 15)

- GM-only mode defaults ON: `BrambleServices.Access.GMOnly = 1` checks
  `SEC_GAMEMASTER` at the player command and General Services gossip entry/action.
- Module remains disabled by default (`BrambleServices.Enable = 0`).
- AzerothCore `.reload config` rereads the installed .conf and takes effect on
  subsequent commands and interactions without a restart. It reloads other
  worldserver configurations too, so review changes carefully.
- The complete `brambletown_services.conf.dist` includes a plain-language
  one-sentence explanation immediately before **every** setting.
- Individual providers accept optional `.DisplayId` overrides checked
  against `CreatureDisplayInfo.dbc` before summoning.
- Unique NPC names, subtitles and models are documented with a
  **review-only** database draft in `sql/REVIEW_ONLY_npc_identity_templates.sql`.
- Standalone Enchanter provider defaults OFF until its own GM-only
  interaction restrictions are implemented; General Services has guards.
- Future optional providers: Auctioneer and Alchemy. The proposed Food Buff NPC is replaced with the General Services Fish Feast option.
- See `docs/GM-TESTING-AND-NPC-IDENTITIES.md` for staging prerequisites,
  limitations and the exact `.reload config` behavior.

No code has been built or deployed. No live SQL has been executed.

## Fish Feast replaces a separate Food Buff NPC (October 15)
- Added an individual **Prepare Fish Feast** gossip option to the General
  Services NPC (action 11); option and direct selection both use
  `BrambleServices.Service.FishFeast` (default 1).
- Configurable minimum requester level via
  `BrambleServices.Service.FishFeast.MinLevel` (default 1, clamped 1..80).
- The **Steward directly summons** temporary game object 192675 at its feet
  for 180 seconds; neither player Cooking skill nor Fish Feast items are
  needed, and the feast's lifetime is independent of the temporary Steward.
- A GUID-keyed in-memory record prevents more than one Fish Feast per
  summoned Steward. The record expires after the greater of the Steward
  lifetime and the feast's normal three-minute duration.
- Normal General Services GM-only, instance/PvP and interaction restrictions
  continue to apply; Fish Feast additionally disallows combat regardless of
  Interaction.AllowInCombat.
- No separate Food Buff NPC provider is planned. A future separate Food/Water
  *supplies* vendor is still under consideration, independent of this feast.
- Feast visibility, exact duration, ground placement and low-level consumption
  must be tested in a non-production build before release.

## Flask of the North choice submenu (October 15)
- Added **Apply Flask of the North** (action 12) and three player-selected
  options: +40 Strength (67018), +80 Attack Power (67017), and +47 Spell
  Power (67016), each enforcing `BrambleServices.Service.FlaskOfNorth`.
- Directly applies the selected aura to the interacting player without
  item 47499, its Alchemy requirement, or the item's random-selection spell.
- Successfully selecting one variant removes the other two Flask of the
  North variants, but relies on the core to handle other elixir/flask
  stacking rules and the normal buff duration.
- Retains General Services GM/headless/PvP/instance/combat restrictions.
  Verify buff switching, level independence, stacking and death behavior
  in the approved staging build before production.

## GM-only test readiness checkpoint (October 15)
- All three module .cpp sources passed standalone Clang C++20 **syntax checks**
  and produced optimized object files in `/root/brambletown-services-work/staging-objects/`
  using the flags from the isolated prior production-build compile database.
- Isolated CMake configuration completed in
  `/root/brambletown-services-work/build-gmtest` using core revision
  `2a2211cd8`, prefix `/root/brambletown-services-work/staging-dist`.
  It discovered **mod-brambletown-services**, registered
  `Addmod_brambletown_servicesScripts()`, and discovered
  `brambletown_services.conf`.
- A staging-only symlink in `/root/brambletown-upgrade-work/core/modules`
  points to the development module; the live core and production executable
  were not changed.
- GM-only Steward test plan: `docs/GM_STAGING_TEST_PLAN.md`.
- Non-overwriting Steward SQL preflight/install script (NOT executed):
  `sql/GM_STAGING_steward_only.sql` (NPC ID 910101).
- Full worldserver/static-module target was NOT linked; this is still a
  pre-deployment staging build. No DB writes, live config changes, restart,
  or public player access were performed. Explicit permission is required.

# Enchanter integration draft: GM-only, Provider.2 disabled until tested.

## On-demand provider flow (October 15, 2026; development-only)

1. `.services` summons **only the Steward (Provider 1)** for 120 seconds by default.
2. The player speaks to the Steward and selects **Call another service provider...**.
3. The next menu lists enabled Enchanter (2), Scribe (3), faction-specific Auctioneer (4), and General Goods (5).
4. Each selection summons **only that NPC**, beside the Steward, with remaining time on the Steward's original expiration. The timer does not reset.
5. A player can request different providers during one Steward session, but each type may be summoned only once per session. Other players cannot order NPC summons from someone else's Steward. Existing accessible Steward services remain unaffected.
6. Logout/teleport tries to clean up the Steward and all NPCs summoned through it. Native timed despawn is the fallback if they are no longer reachable.

Provider 3 code, NPC models, glyph catalog, and general-goods inventory remain development work. All optional providers are default disabled. No live deployment or worldserver restart has been performed for this change.

## Provider 3 — Scribe implementation (October 15, 2026)

The Scribe is summoned **only on demand** by requesting Provider 3 from the Steward.
Right-clicking the Scribe displays all ten playable WotLK classes. Selecting a class
opens the **native client vendor window** for all available non-deprecated class
Major and Minor glyph items, alphabetically sorted. Class selection is user-directed;
the buyer may select any class (useful for alts), not only their current class.

The 10 catalog IDs are `910120 + classId` for class IDs 1–9 and 11, with 348
distinct item entries total. The Scribe entry `910103` is the only actual spawned NPC.
A PlayerScript hook maps its standard vendor packets to the selected stock list;
buying/selling uses the normal AzerothCore native transaction code. Checks bind
selected catalog to character GUID and the spawned Scribe GUID, protecting against
stale/fabricated vendor requests. Menu display, inventory refresh and purchase
must pass normal provider enabled, GM-only, headless, combat, PvP and instance guards.

The SQL is **review-only**: `sql/GM_STAGING_scribe_class_glyph_vendors.sql`.
All selected item spell IDs were verified present in the installed WotLK Spell.dbc.
Two existing world-item price anomalies should be reviewed before production:
`45908 Glyph of Envenom` has `BuyPrice=1000000` copper (100 gold) and
`44432 Glyph of Raise Dead` has `BuyPrice=0` copper (free). No changes were
made to item_template or economy. Normal native item prices are used.

**Not deployed**. C++ syntax and regression tests are development checks only;
full link/build, reviewed staging SQL, and in-game purchase/sell/buyback testing
remain required. Provider 3 remains disabled on the live server.

## Providers 4 and 5 (October 15, 2026, staged only)

### Provider 4 — faction-specific Auctioneer

Provider 4's on-demand Steward action spawns only **Alliance Auctioneer 910104**
for an Alliance character, or **Horde Auctioneer 910105** for a Horde
character. Both are standard native NPC auctioneers, not a custom or simulated
market. Faction templates 12/29 and display models 7991/8001 are verified
from the installed AzerothCore WotLK databases and client DBC. Normal AH level,
listing, bidding, duration, deposit and mailing rules remain in effect.

**Important existing server behavior:** `AllowTwoSide.Interaction.Auction = 1`.
AzerothCore routes *all* auctioneers into the combined neutral auction pool
under this setting. The provider NPC is faction-specific, but auction stock
is currently shared across the factions. This development does NOT change
that setting or move auction records. Separate Alliance/Horde auction pools
would require a deliberate realm-wide configuration change and migration/risk
assessment of existing AH auctions and AH-bot behavior.

Staging template SQL: `sql/GM_STAGING_auctioneers.sql`. No live writes.

### Provider 5 — General Goods and Repair

Provider 5's Steward option summons **Brambletown Provisioner 910106**, with a
verified NPC display model and a native repairable vendor (`npcflag=4224`).
Review-only SQL `sql/GM_STAGING_general_goods_vendor.sql` contains **112**
verified items across 12 categories. Included are 9 levels of drinking water,
food for players and all six hunter pet food diets, arrows, bullets, class and
crafting reagents, basic bags and tools, and fishing tackle. Normal WotLK item
prices, level requirements, and stack/buy-count rules remain in effect.
Items are accessible through a single vendor window (112 < 150 WotLK client
maximum). Standard AzerothCore handles purchases, selling, buyback and repairs.
Full stock audit: `docs/PROVIDER5_GOODS_CATALOG.md`.

The combined dev-core patch at `patches/brambletown-npc-access-and-scribe-price-cap.patch`
secures **all** native interactions with temporary Provider 2–5 entries:
GM-only during testing, headless/instance/PvP/combat restrictions, per-provider
enable switches and faction matching for Provider 4. It also contains the
Scribe-only 1g glyph price cap. The patch is prepared in the isolated core
worktree and has NOT touched the live core or worldserver.

All optional providers remain disabled by default. Full linked build,
reviewed staging SQL, live deployment and transaction tests are pending.

### Provider 4: Combined OR separate auction houses, without code/config changes

Provider 4 only selects the temporary NPC by the requesting player's faction:
Alliance -> `910104` (faction template `12`), Horde -> `910105` (faction
`29`). AzerothCore's unmodified `SendAuctionHello` then calls
`AuctionHouseMgr::GetAuctionHouseEntryFromFactionTemplate(unit->GetFaction())`.
The core dynamically resolves the correct auction-house pool:

| Realm option `AllowTwoSide.Interaction.Auction` | Alliance NPC | Horde NPC |
|---|---|---|
| `1` (combined) | Shared/neutral pool | Shared/neutral pool |
| `0` (separate) | Alliance pool | Horde pool |

No new module setting and no additional change are needed when the realm's
setting is altered. Only the appearance/faction of the summoned NPC is
provider-specific. The module intentionally never hardcodes the market ID.

### October 15 Enchanter menu and Fish Feast polish (not yet deployed)

- Enchanter category icons: 24×24 → **18×18**, labels simplified without decorative brackets
- Ranged category: **Enchant Ranged (Bow/Gun/Xbow)**
- Named effects (Mongoose, Berserking, Executioner, Black Magic, Blade Ward, etc.) now describe their proc/stat or utility effect in the selection menu
- Corrected a prior menu-gating error so low-level shoulder enchants are visible to non-inscribers; Master's inscriptions remain profession-restricted
- Fish Feast now spawns **2 yards ahead of the Steward**, using the core's terrain-aware GetNearPosition helper instead of directly beneath the NPC; one-feast-per-Steward/180s lifetime unchanged
- Existing NPC templates, database, and live worldserver are unaffected until a controlled rebuild/deployment

### Automatic Playerbot Fish Feast interactions (development only)

The Steward's Fish Feast is still prepared only when requested by a real player.
When enabled (`BrambleServices.Service.FishFeast.AutoBots = 1`, default), the
Steward selects up to **25 nearby online Playerbots within 12 yards** of the
new feast (configurable max: 25 bots / 25 yards), regardless of party membership.
It coordinates existing mod-playerbots `DoSpecificAction("use", ...)` with an
actual Fish Feast game-object link, and `DoSpecificAction("go", ...)` for bots
whose current Playerbots AI has a master and can approach. It does not directly
cast feast buffs or invent a custom playerbot action. Bots in combat, dead,
flying, in other phases or otherwise not available are skipped. Unmanaged/random
bots outside interaction range are not forced to move; they may still interact
if they independently come within range during the **28-second coordination window**.
The Steward's map-thread AI holds only GUIDs, with no global scanner, no stale
player references and no persistent bot state. Interactions go through normal
Playerbots and native gameobject checks; being invited does not guarantee a buff.
The existing one-feast-per-Steward and feast duration policies are unchanged.

**Playerbots module changes:** None. This feature is entirely in
`src/brambletown_npc_services.cpp` + Steward `GetAI()` + Services config.
**Live deployment:** Not yet; requires a new worldserver binary and GM testing.

### Fish Feast announcement rotation (October 15, 2026, staged)

Every successful Fish Feast placement (even with zero eligible bots) triggers
one short, spoken Steward announcement in `/say`, with an in-world speech bubble.
The lines cycle across successive feast placements using an atomic counter:

1. "Fish Feast down! Get it while it's warm!"
2. "Fresh Fish Feast! Come grab a plate!"
3. "Feast is served! Everyone, dig in!"
4. "Food's up! Don't let it go to waste!"
5. "A Fish Feast for the crew! Help yourselves!"

The announcement is emitted after the Steward has asked the Services coordinator
to invite eligible bots. The exact hard maximum is now **25 bots per feast**
(default 25; any higher setting is clamped down to 25). No Playerbots code
or database changes are required. This is staged development only.

### October 15 — Fish Feast eating verification and enchant popup correction (development)

Playerbots' `use` action replies "Game object interaction requested" merely when
it dispatches the GO click; that **does not verify any food spell or buff**.
The installed 3.3.5a world database runs Fish Feast GO spell `57397`, whose
`spell_scripts` casts the **57398 eating aura**; after 10 seconds its periodic
aura triggers the actual **57399 Well Fed** 1-hour buff. The Services-only
coordinator now keeps clicking bots seated, temporarily delays ordinary bot
AI/follow behavior for up to 14.5 seconds, and reports the actual count of
bots that received 57399. If a GO click fails to start the aura, Services
falls back to the same genuine 10-second 57398 food aura after two seconds.
It never directly grants Well Fed. Combat always cancels the forced eating
state immediately, and the total coordination period is bounded to 28 seconds
(to allow bot approach and the 10-second food timer). The cap remains 25 bots.

The Enchanter's three yellow flavor notifications now use AzerothCore's
`StringFormat` placeholder `{}` instead of `%s`, so the chosen equipment name
should display correctly in all three variants.

**Both changes still require full build, deployment, and in-game confirmation.**
