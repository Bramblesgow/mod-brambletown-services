#!/usr/bin/env python3
"""Use an existing isolated-build compile database for syntax-only checks."""
from pathlib import Path
import json, os, shlex, subprocess, sys

root = Path(__file__).resolve().parents[1]
compile_commands = os.environ.get("AZEROTHCORE_COMPILE_COMMANDS")
if not compile_commands:
    raise SystemExit("Set AZEROTHCORE_COMPILE_COMMANDS to an existing compilation database")
database = Path(compile_commands)
if not database.is_file():
    raise SystemExit(f"Compilation database does not exist: {database}")
db = json.loads(database.read_text())
sample=next(e for e in db if e['file'].endswith('/mod-custom-login/src/CL_loader.cpp'))
args=shlex.split(sample['command'])
args=args[:args.index('-o')] + ['-fsyntax-only', '-Wno-unused-parameter', '-I'+str(root/'src')]
for source in sorted((root/'src').glob('*.cpp')):
    cmd=args+[str(source)]
    print(f'CHECK {source.name}',flush=True)
    r=subprocess.run(cmd,cwd=sample['directory'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    print(r.stdout[-12000:],flush=True)
    if r.returncode:
        print(f'FAIL {source.name}',flush=True)
        sys.exit(r.returncode)
print('PASS syntax checks',flush=True)
