#!/usr/bin/env python3
"""Syntax-check the reviewed module checkout using an existing core compile database."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
configured = os.environ.get("AZEROTHCORE_COMPILE_COMMANDS")
if not configured:
    sys.exit("Set AZEROTHCORE_COMPILE_COMMANDS to a compatible compile_commands.json")
database = Path(configured)
if not database.is_file():
    sys.exit(f"Compilation database missing: {database}")
commands = json.loads(database.read_text())
entries = [item for item in commands
           if "/modules/" in item["file"] and item["file"].endswith(".cpp")]
if not entries:
    sys.exit("No AzerothCore module compiler command found")
sample = next((item for item in entries
               if item["file"].endswith("/mod-brambletown-services/src/MP_loader.cpp")),
              entries[0])
args = shlex.split(sample.get("command", ""))
if "-o" not in args:
    sys.exit("Unsupported compile command (missing -o)")
args = args[:args.index("-o")] + ["-fsyntax-only", "-Wno-unused-parameter",
                                "-I" + str(root / "src")]
sources = sorted((root / "src").glob("*.cpp"))
if not sources:
    sys.exit("No NPC Services C++ translation units found")
for source in sources:
    print(f"CHECK {source.relative_to(root)}", flush=True)
    result = subprocess.run(args + [str(source)], cwd=sample["directory"],
                            capture_output=True, text=True)
    if result.returncode:
        print((result.stdout + result.stderr)[-18000:], flush=True)
        sys.exit(f"Syntax failure: {source.name}")
print(f"PASS: syntax checked {len(sources)} module C++ files at {root}", flush=True)
