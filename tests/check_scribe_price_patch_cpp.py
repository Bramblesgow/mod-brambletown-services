#!/usr/bin/env python3
"""C++ syntax-only validation of the two isolated native Scribe pricing call sites."""
import json,shlex,subprocess,sys
from pathlib import Path
core=Path('/root/brambletown-upgrade-work/core')
build=Path('/root/brambletown-upgrade-work/build-production')
commands=json.loads((build/'compile_commands.json').read_text())
for suffix in ('src/server/game/Handlers/ItemHandler.cpp','src/server/game/Entities/Player/Player.cpp'):
    found=[e for e in commands if e['file'].endswith(suffix)]
    assert len(found)==1,(suffix,len(found))
    entry=found[0]
    cmd=shlex.split(entry['command'])
    pos=cmd.index('-o')
    del cmd[pos:pos+2]
    pos=cmd.index('-c')
    cmd[pos]='-fsyntax-only'
    print('CHECK',suffix,flush=True)
    p=subprocess.run(cmd,cwd=entry['directory'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.stdout: print(p.stdout[-7000:],flush=True)
    if p.returncode:
        print('FAIL',suffix,flush=True)
        sys.exit(p.returncode)
print('PASS: native vendor display/purchase call site C++ syntax',flush=True)
