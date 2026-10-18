#!/usr/bin/env python3
"""Use an existing isolated-build compile database for syntax-only checks."""
from pathlib import Path
import json,shlex,subprocess,sys
root=Path('/root/brambletown-services-work/mod-brambletown-services')
db=json.loads(Path('/root/brambletown-upgrade-work/build-production/compile_commands.json').read_text())
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
