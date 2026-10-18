#!/usr/bin/env python3
"""Compile only the draft module sources to isolated object files, with live server untouched."""
from pathlib import Path
import json,shlex,subprocess,sys
root=Path('/root/brambletown-services-work/mod-brambletown-services')
out=Path('/root/brambletown-services-work/staging-objects')
out.mkdir(parents=True,exist_ok=True)
db=json.loads(Path('/root/brambletown-upgrade-work/build-production/compile_commands.json').read_text())
sample=next(e for e in db if e['file'].endswith('/mod-custom-login/src/CL_loader.cpp'))
args=shlex.split(sample['command'])
args=args[:args.index('-o')] + ['-I'+str(root/'src')]
for source in sorted((root/'src').glob('*.cpp')):
    output=out/(source.stem+'.o')
    print(f'BUILD {source.name} -> {output.name}',flush=True)
    r=subprocess.run(args+['-o',str(output),'-c',str(source)],cwd=sample['directory'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    print(r.stdout[-15000:],flush=True)
    if r.returncode:
        print(f'FAILED {source.name}',flush=True)
        sys.exit(r.returncode)
print('PASS: three isolated C++ object files created. NOT linked into worldserver.',flush=True)
