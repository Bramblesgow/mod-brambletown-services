#!/usr/bin/env python3
"""Check explicitly empty DBC environment configuration is an error."""
import os
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
for variable, test in (
    ("SPELL_ICON_DBC", "test_steward_menu_layout.py"),
    ("SPELL_ENCHANT_DBC", "test_enchanter_slot_levels.py"),
):
    environment = dict(os.environ)
    environment[variable] = ""
    result = subprocess.run([sys.executable, str(root / "tests" / test)],
                            cwd=root, env=environment, capture_output=True, text=True)
    assert result.returncode != 0, f"{variable}= should fail rather than skip"
    assert f"{variable} must not be empty when configured" in result.stderr + result.stdout, (
        f"No empty-path failure for {variable}"
    )
print("PASS: both explicitly empty optional DBC paths reject validation")
