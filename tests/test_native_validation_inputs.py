#!/usr/bin/env python3
"""Reject missing native-validator inputs instead of silently using host defaults."""
import os
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
checks = (
    ("check_cpp_syntax.py", "AZEROTHCORE_COMPILE_COMMANDS"),
    ("build_isolated_objects.py", "BRAMBLETOWN_BUILD_DIR"),
    ("check_scribe_price_patch_cpp.py", "BRAMBLETOWN_NATIVE_BASE_ROOT"),
    ("check_scribe_price_patch_cpp.py", "BRAMBLETOWN_BUILD_DIR"),
)
for script, name in checks:
    for setting in (None, ""):
        environment = dict(os.environ)
        # Other required paths are deliberately invalid: the tested variable
        # must be rejected first, before any filesystem-dependent work.
        environment["AZEROTHCORE_COMPILE_COMMANDS"] = "/nonexistent-db"
        environment["BRAMBLETOWN_BUILD_DIR"] = "/nonexistent-build"
        environment["BRAMBLETOWN_NATIVE_BASE_ROOT"] = "/nonexistent-source"
        if setting is None:
            environment.pop(name)
        else:
            environment[name] = setting
        try:
            result = subprocess.run(
                [sys.executable, str(root / "tests" / script)],
                cwd=root, env=environment, capture_output=True, text=True,
                check=False, timeout=15,
            )
        except subprocess.TimeoutExpired as exc:
            raise AssertionError(
                f"{script} timed out validating {name}={setting!r} (15-second limit)"
            ) from exc
        assert result.returncode != 0, f"{script} unexpectedly accepts {name}={setting!r}"
        assert f"Set {name} " in result.stderr + result.stdout, (
            f"{script} did not reject {name}={setting!r}: {result.stderr}{result.stdout}"
        )
print("PASS: native validators reject missing and empty mandatory inputs")
