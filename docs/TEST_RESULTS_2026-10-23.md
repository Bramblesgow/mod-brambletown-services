# NPC Services - October 23, 2026 test results and live release

Tested PR #5 head: `d8eb7611da3a194b27ea49cc3c49bc5e403f3179`. Tested stack #1, #2, #4 and #5 merged through `829236789863c6150c9d1597afaa6bcb7502f85d`; no untested sibling-branch gameplay changes were added.

## Verified scope and limitations

Owner recorded passes for command eligibility/restrictions, cooldown/expiry, provider menus, native disabled-provider denial, Steward services, Fish Feast/eating and bot refresh, Enchanter eligibility, displayed **and charged** Scribe glyph prices, faction auctioneers/shared AH and Provisioner stock/transactions. S03 repair and E06 stale Enchanter-menu action are confirmed Pass after correcting omitted PDF marks.

N14 was marked Pass with qualifications, but the placement screenshots show a real open defect. It is not reported as fixed.

- [Placement defect #7](https://github.com/Bramblesgow/mod-brambletown-services/issues/7): Steward and providers may clip into ground, graves or walls; validate safe placement for all summons.
- [Remaining coverage #8](https://github.com/Bramblesgow/mod-brambletown-services/issues/8): N04 **P0 blocked** cross-player GM-only native access; N12 second-GM independence; E07 other-player equipment; Y05 ordinary-vendor comparison.
- [Junk-to-Gold coverage #9](https://github.com/Bramblesgow/mod-brambletown-services/issues/9) and [Dungeon Clear handoff investigation #10](https://github.com/Bramblesgow/mod-brambletown-services/issues/10) are tracked on this board because the affected forks have Issues disabled.

## Deployment details

The production source includes the reviewed native interaction guards and native glyph UI/charged-price cap changes. Strict source checks passed against the production checkout. Only two existing Brambletown template changes were required: add the Steward mailbox NPC flag and migrate the Enchanter ScriptName to `brambletown_enchanter`. Existing class glyph and 112-entry Provisioner catalogs were retained.

Production settings were preserved, including GMOnly=0, AllowInInstances=1, Heal=0, existing service/provider toggles, 150-second timer and existing feast radius/bot limits. Test-only toggles were restored independently; the test settings were not copied to production.

Prior portable CI passed; exact-source compilation/linking and isolated gameplay were also completed. This release does not claim complete cross-player security coverage or repaired terrain/wall placement.

## Shared session and deployment record

Owner-completed sheet: `Brambletown_Combined_Test_Sheet_2026-10-23(1).pdf`. Corrected user marks: **65 Pass, 7 Deferred, 2 Blocked, 23 Not run** across 97 cases. This is the user-form tally, not a claim that all operator/restoration cases were unexecuted: operator evidence and subsequent deployment are reconciled separately. S03 and E06 were explicitly corrected to Pass after submission.

Combined isolated test binary SHA-256: `8d2fa8f0c0593d02b455b34a471450fff4fdb4b6b7c1af53f8393fcc6964bcdf`.
Production build from the same tested gameplay source, rebuilt with production configuration paths: `ac4d29bd37f436f495f8211ae4d3eb291ef181a8c43926a3e995497eb2f3f483`.

Released October 23, 2026 after explicit owner approval. Production controller recorded success at **11:48 AM America/Chicago (CDT)**. Auth/world ports responded, world console responded, and the inspected error log was empty. All **35 production configuration files remained byte-for-byte unchanged**; original authserver retained. Production characters/starter receipts and normal loot rates preserved; no cloned test data, guaranteed-drop fixtures or test configuration were imported.

Production rollback binaries, database dumps, configuration backup, migration output and deployment manifest are retained on the authorized server under `rollout-20261023/`. Test evidence is retained under `combined-runtime-20261023/evidence/`. Live startup checks are operational checks, not a substitute for later human gameplay confirmation.
