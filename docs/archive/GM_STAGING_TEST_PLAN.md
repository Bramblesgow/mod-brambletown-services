<!-- Historical planning reference. Not the current release documentation. -->

# Brambletown Services: first GM-only integration test

## Scope / readiness

This is a **GM-only Steward smoke test**, not a public release or the finished
multi-NPC suite. The standalone Enchanter, Glyph Merchant and Supplies Merchant
are not integrated; leave all three off. Existing character services, Feast
and Flask selection form the first testable component.

The three .cpp files were independently compiled to object files using
the exact AzerothCore revision `2a2211cd8` flags from the isolated
`/root/brambletown-upgrade-work/build-production/compile_commands.json`.
Object files: `/root/brambletown-services-work/staging-objects/`.

**This is not a linked module or worldserver build.** CMake module registration,
full linking, creature DB migration, worldserver startup, and real-player
interaction tests are required. No production deployment has occurred.

## Before the first live GM test (requires explicit permission)

1. Back up the world database, record the current worldserver build, and
   confirm whether real players are online before any scheduled restart.
2. Verify template ID `910101` and model mapping are unused using the
   read-only SELECTs at the top of `sql/GM_STAGING_steward_only.sql`.
   If occupied by another NPC, STOP and choose an unused ID instead.
3. Build the complete module within an AzerothCore source/build environment,
   with a separate output prefix; confirm loader registration and link success.
   Never edit the running executable during compilation.
4. Keep the following settings in the **installed** module configuration:
   `BrambleServices.Enable=1`;
   `BrambleServices.Access.GMOnly=1`;
   `BrambleServices.ProviderCount=3`;
   `BrambleServices.Provider.1.Enable=1`;
   `BrambleServices.Provider.1.Entry=910101`;
   `BrambleServices.Provider.2.Enable=0`;
   `BrambleServices.Provider.3.Enable=0`.
5. Only after approval and a backup, apply the non-destructive SQL staging
   script against the **world** database. It creates only ID 910101 and its
   model, if absent; never modifies the old mod-npc-services entry 55002.
6. Install the reviewed worldserver build in a controlled maintenance window,
   with a 30-second warning if real players are online, then restart.
7. Confirm worldserver starts without errors, module config loads correctly,
   creature template loads and command `.services` is registered.

## In-game GM test checklist

- [ ] A normal player cannot summon with `.services` and is told GM access is required.
- [ ] GM summons a single `Brambletown Steward <Character Services>` and sees
      `Services spawned for 2 minutes.`
- [ ] Repeating `.services` returns remaining seconds, with no second NPC.
- [ ] GM logout, teleport, cooldown and expired NPC cleanup work without leaving ghosts.
- [ ] Denied attempts while dead, fighting, in BG, arena, raid and dungeon explain why.
- [ ] No summon is created by bots/headless sessions.
- [ ] Existing services operate only when toggled on; danger features stay disabled.
- [ ] Normal player cannot select Steward actions even while standing next to GM's NPC.
- [ ] `Prepare Fish Feast` summons clickable GO 192675 at the Steward's feet without
      Cooking skill, profession, or consuming an item.
- [ ] Fish Feast is visible and can be consumed by nearby players and expires at 3 minutes,
      including after the Steward despawns.
- [ ] A second Fish Feast action on the same Steward is rejected.
- [ ] `Apply Flask of the North` offers +40 Strength, +80 Attack Power and +47 Spell Power.
- [ ] Each choice applies the correct spell without Alchemy and retains the expected 1-hour
      duration; changing choices removes only the previous Flask of the North variant.
- [ ] Verify normal flask/elixir stacking, death persistence and low-level eligibility.
- [ ] `.reload config` applies switches and access changes to subsequent interactions
      without a restart (it reloads other module configs too).
- [ ] Two different GM accounts can summon without interfering with one another.
- [ ] GM-only mode does not prevent other players from *eating* a communal feast
      spawned by a GM. This is expected; do not test feast secrecy in public areas.

## Issues to resolve before public deployment

- Terrain height, obstacle and LoS validation at spawn coordinates.
- Review/test cooldown map threading and periodic cleanup.
- Test full CMake regeneration, static linking, and interaction with Playerbots/DC.
- Complete Supplies Merchant catalog and normal transactions/buyback.
- Integrate the Enchanter with its own GM-level and combat checks.
- Implement class-filtered glyph stock without exceeding WotLK vendor UI limit.
- Validate the character services that interact with player/instance lockouts.

## Test stop / rollback

If GM testing shows dangerous behavior, set
`BrambleServices.Enable = 0` in the installed config and run
`.reload config` to block subsequent commands and general-service actions.
This DOES NOT despawn already active NPCs immediately, and cannot unload the
compiled module. For severe issues, stop the test, remove players from the
affected area, and perform a controlled rollback of the worldserver executable.

Do not remove NPC database records until verifying they are still our
`brambletown_npc_services` template and are not in active use.
