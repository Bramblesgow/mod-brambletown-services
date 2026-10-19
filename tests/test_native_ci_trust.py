#!/usr/bin/env python3
"""Check repository CI policy; not a substitute for runner access configuration."""
from pathlib import Path
import re

workflow = (Path(__file__).resolve().parents[1] / ".github/workflows/static-checks.yml").read_text()
assert "branches: [main]" in workflow
assert "pull_request:" in workflow
assert "contents: read" in workflow
assert workflow.count("persist-credentials: false") == 2
assert workflow.count("runs-on: self-hosted") == 1
native = workflow.split("  native-core-validation:", 1)[1]
assert re.search(r"if:.*github.event_name == 'push'.*github.ref == 'refs/heads/main'.*github.ref_protected", native)
assert "tests/check_scribe_price_patch_cpp.py" in native
for variable in ("AZEROTHCORE_COMPILE_COMMANDS", "BRAMBLETOWN_BUILD_DIR",
                 "BRAMBLETOWN_NATIVE_BASE_ROOT"):
    assert f"{variable}: ${{{{ vars.{variable} }}}}" in native
print("PASS: native CI requires protected-main push and explicit runner paths; no checkout credentials")
