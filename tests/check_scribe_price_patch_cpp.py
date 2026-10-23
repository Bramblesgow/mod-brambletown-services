#!/usr/bin/env python3
"""Apply the staged native patch to scratch source files and syntax-compile both call sites.

Uses AzerothCore's existing CMake compile database. Neither installed nor
staged source is modified, and no worldserver binary is linked/deployed.
"""
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
configured_core = os.environ.get("BRAMBLETOWN_NATIVE_BASE_ROOT")
if not configured_core:
    sys.exit("Set BRAMBLETOWN_NATIVE_BASE_ROOT to the unpatched AzerothCore source root")
configured_build = os.environ.get("BRAMBLETOWN_BUILD_DIR")
if not configured_build:
    sys.exit("Set BRAMBLETOWN_BUILD_DIR to a build directory with compile_commands.json")
core = Path(configured_core)
build = Path(configured_build)
patch = root / "patches/brambletown-npc-access-and-scribe-price-cap.patch"
database = build / "compile_commands.json"
for required in (patch, database):
    if not required.is_file():
        sys.exit(f"Missing native validation input: {required}")
commands = json.loads(database.read_text())
targets = ("src/server/game/Entities/Player/Player.cpp",
           "src/server/game/Handlers/ItemHandler.cpp")

with tempfile.TemporaryDirectory(prefix="brambletown-native-patch-") as temp:
    scratch = Path(temp)
    for name in targets:
        original = core / name
        if not original.is_file():
            sys.exit(f"Native base source missing: {original}")
        destination = scratch / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(original.read_bytes())

    # Isolate git discovery from a TMPDIR located inside another worktree.
    subprocess.run(["git", "init", "-q"], cwd=scratch, check=True)
    before = {name: (scratch / name).read_bytes() for name in targets}
    for args in (["git", "apply", "--check", str(patch)], ["git", "apply", str(patch)]):
        process = subprocess.run(args, cwd=scratch, capture_output=True, text=True)
        if process.returncode:
            sys.exit(f"Native patch not applicable: {process.stderr}")

    # A zero status alone is insufficient: git apply can print "Skipped patch".
    for name in targets:
        if (scratch / name).read_bytes() == before[name]:
            sys.exit(f"Native patch did not modify expected source: {name}")

    for name in targets:
        matches = [entry for entry in commands if entry["file"].endswith("/" + name)]
        if len(matches) != 1:
            sys.exit(f"Expected one compile command for {name}, found {len(matches)}")
        entry = matches[0]
        cmd = shlex.split(entry.get("command", ""))
        if "-o" not in cmd:
            sys.exit(f"Unsupported compile command for {name} (missing -o)")
        cmd = cmd[:cmd.index("-o")] + [
            "-fsyntax-only", "-Wno-unused-parameter",
            "-I" + str((core / name).parent), str(scratch / name)
        ]
        print(f"CHECK PATCHED {name}", flush=True)
        result = subprocess.run(cmd, cwd=entry["directory"], capture_output=True, text=True)
        if result.returncode:
            print((result.stdout + result.stderr)[-18000:], flush=True)
            sys.exit(f"Native C++ syntax failure: {name}")

print("PASS: actual native patch applied and both isolated C++ translation units compiled", flush=True)
