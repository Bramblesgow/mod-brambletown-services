#!/usr/bin/env python3
"""Syntax-compile this checkout's NPC Services sources against an isolated AzerothCore build."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get("BRAMBLETOWN_BUILD_DIR", "/root/brambletown-upgrade-work/build-production"))
database = build / "compile_commands.json"
if not database.is_file():
    sys.exit(f"Missing AzerothCore compile database: {database} (set BRAMBLETOWN_BUILD_DIR)")
commands = json.loads(database.read_text())
# Use a real module compilation entry to inherit the installed Playerbots/API headers.
module_entries = [item for item in commands if "/modules/" in item["file"] and item["file"].endswith(".cpp")]
if not module_entries:
    sys.exit("No module C++ compile command found in the AzerothCore compile database")
sample = next((item for item in module_entries if item["file"].endswith("/mod-brambletown-services/src/MP_loader.cpp")), module_entries[0])
args = shlex.split(sample["command"])
args = args[:args.index("-o")] + ["-fsyntax-only", "-Wno-unused-parameter", "-I" + str(root / "src")]
files = sorted((root / "src").glob("*.cpp"))
if not files:
    sys.exit(f"No C++ source files in {root / 'src'}")
for source in files:
    print(f"CHECK {source.relative_to(root)}", flush=True)
    result = subprocess.run(args + [str(source)], cwd=sample["directory"],
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        print(result.stdout[-18000:], flush=True)
        sys.exit(f"FAIL: {source.name}")
print(f"PASS: C++ syntax checked all {len(files)} source files from {root}", flush=True)
