# NPC Services deployment and verification gates

This document describes future authorized deployment, not permission to modify
the running realm.

## Before merge or deployment

1. Record the exact reviewed commit, AzerothCore/Playerbots revisions, installed
   worldserver checksum, and active Services config.
2. Run every tests/test_*.py regression test, tests/check_cpp_syntax.py,
   tests/build_isolated_objects.py and tests/check_scribe_price_patch_cpp.py.
3. The native validator applies the proposed patch to temporary copies from an
   unpatched core. Do not apply a patch twice to staged or production sources.
4. GitHub's portable Python checks do not establish C++ compilation. The optional
   native GitHub job runs only on a configured self-hosted runner. A skipped
   native job is not a verified build.
5. Link and test an isolated development worldserver; obtain approval before
   installing any files.

## Authorized deployment order

1. Back up binary, configuration, source revision, and relevant world SQL tables:
   creature_template, creature_template_model and npc_vendor.
2. Apply the reviewed native patch to the matching development core and compile
   the complete worldserver with the reviewed module.
3. Confirm the candidate registers the unique Enchanter script
   brambletown_enchanter before applying the Enchanter SQL migration.
   GM_STAGING_enchanter_only.sql changes 910102 from npc_enchantment to
   brambletown_enchanter; applying SQL before the binary can break gossip.
4. Stage the reviewed binary/module and perform SQL changes under the agreed
   maintenance procedure. Check logged-in players and give the agreed warning.
5. On restart check worldserver and authserver health and log errors.
6. Validate all providers, mail/repair, glyph quantity and <1g/1g cap prices,
   auction routing, enchant class/slot/profession rules, and Fish Feast
   including bots with existing Well Fed buffs.
7. With AZEROTHCORE_LIVE_ROOT pointing to the deployed source, run both
   strict native guard tests, e.g.:
   BRAMBLETOWN_REQUIRE_NEW_GUARD=1 AZEROTHCORE_LIVE_ROOT=/path/to/core python3 tests/test_scribe_price_cap.py
   BRAMBLETOWN_REQUIRE_NEW_GUARD=1 AZEROTHCORE_LIVE_ROOT=/path/to/core python3 tests/test_providers_4_5.py
8. Exercise a rejected Playerbots movement request (bounded retry), a late
   Steward feast request, repeated summons, a failed summon and retry,
   logout/teleport cleanup, and cooldown expiry. Confirm ordinary vendors are
   unchanged.

Source-only checks and syntax compilation do not replace linked-build or
in-game verification; do not report these as tested until performed.
