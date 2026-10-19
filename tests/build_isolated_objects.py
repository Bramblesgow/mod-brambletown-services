#!/usr/bin/env python3
"""Compile the reviewed module's C++ objects into an ephemeral directory only."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get("BRAMBLETOWN_BUILD_DIR", "/root/brambletown-upgrade-work/build-production"))
database = build / "compile_commands.json"
if not database.is_file():
    sys.exit(f"No AzerothCore compile database: {database}")
commands = json.loads(database.read_text())
entries = [entry for entry in commands if "/modules/" in entry["file"] and entry["file"].endswith(".cpp")]
if not entries:
    sys.exit("No AzerothCore module compiler command found")
sample = next((item for item in entries if item["file"].endswith("/mod-brambletown-services/src/MP_loader.cpp")), entries[0])
args = shlex.split(sample["command"])
args = args[:args.index("-o")] + ["-I" + str(root / "src")]
sources = sorted((root / "src").glob("*.cpp"))
with tempfile.TemporaryDirectory(prefix="brambletown-module-objects-") as dirname:
    for source in sources:
        output = Path(dirname) / (source.stem + ".o")
        print(f"BUILD {source.relative_to(root)}", flush=True)
        result = subprocess.run(args + ["-o", str(output), "-c", str(source)],
                                cwd=sample["directory"], capture_output=True, text=True)
        if result.returncode:
            print((result.stdout + result.stderr)[-18000:], flush=True)
            sys.exit(f"Compilation failed: {source}")
print(f"PASS: compiled {len(sources)} isolated module C++ objects (no linking/deployment)", flush=True)
