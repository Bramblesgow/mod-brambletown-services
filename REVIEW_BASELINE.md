# Brambletown Services review baseline (2026-10-18)

This branch is a snapshot of the local module **working tree**, not a release.
The original developer workspace was intentionally left unchanged. GitHub
repository publication and CodeRabbit installation are separate actions.

The module is maintained as a standalone source repository, and its separate
AzerothCore native patch lives under `patches/`; neither alone represents
a reproducible production executable. The live worldserver binary, runtime
configuration, and realm database must be verified independently.

On initial review of this snapshot:
- Module provider code, SQL setup, documentation, test scripts, and the
  core patch are included.
- The `conf/*.dist` template is not the live server's configuration.
- `sql/GM_STAGING_*.sql` are staging migrations and must not run automatically.
- Run automated tests and inspect CodeRabbit findings before merging.
- Do not disclose runtime credentials, authentication keys, or live DB dumps.
- Deploy an explicitly approved and verified commit only, with rollback.
