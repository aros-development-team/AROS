#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Compile native socket and POSIX dispatch objects without installing an SDK."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
ROOT=Path(__file__).resolve().parents[4]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--sdk',type=Path,required=True,help='native AROS/Developer directory')
p.add_argument('--generated',type=Path,required=True,help='native target gen directory')
p.add_argument('--crosstools',type=Path,required=True,help='directory containing bin/clang')
p.add_argument('--output',type=Path,required=True,help='new artifact directory')
a=p.parse_args()
sdk=a.sdk.resolve();gen=a.generated.resolve();out=a.output.resolve()
if out.exists():p.error('output exists')
if any(parent==out or parent in out.parents for parent in (ROOT,sdk,gen,a.crosstools.resolve())):
 p.error('output must be outside source and SDK/tool inputs')
out.mkdir(parents=True)
(out/'include/libraries').mkdir(parents=True)
shutil.copyfile(ROOT/'workbench/libs/fd/include/fd.h',out/'include/libraries/fd.h')
clang=str(a.crosstools.resolve()/'bin/clang')
common=[clang,'--target=aarch64-unknown-aros','-mcmodel=large','-fno-pic','-O2','-std=gnu99',
 '-Werror=implicit-function-declaration','-Wno-pointer-sign','-Wno-format','-Wno-parentheses',
 '-I'+str(out/'include'),'-isystem',str(sdk/'include/aros/posixc'),
 '-isystem',str(sdk/'include/aros/stdc'),'-isystem',str(sdk/'include'),'-I'+str(gen/'include')]
stack=ROOT/'workbench/network/stacks/AROSTCP/bsdsocket'
stackflags=['-DDIRECTED_BROADCAST','-DENABLE_MULTICAST','-DENABLE_FDLIBRARY',
 '-DAMITCP','-DKERNEL','-DENABLE_SYSCTL','-D_XOPEN_SOURCE=600','-D__BSD_VISIBLE=1',
 '-I'+str(ROOT/'workbench/network/common/include'),'-idirafter',str(stack),'-idirafter',str(stack/'protos')]
crt=ROOT/'compiler/crt'
posixflags=['-D_GNU_SOURCE','-D_LARGEFILE64_SOURCE',
 '-DLC_LIBDEFS_FILE="posixc_libdefs.h"',
 '-I'+str(gen/'compiler/crt/posixc/posixc/include'),
 '-I'+str(crt/'posixc'),'-I'+str(crt/'stdc'),'-I'+str(crt/'stdcio')]
config=(sdk/'include/aros/config.h').read_text()
if '#define __AROSEXEC_SMP__' not in config:p.error('SDK must select native SMP layout')
commands=[];identities={}
for prefix,folder,files,flags in [
 ('stack',stack/'api',['native_socket','amiga_api','amiga_syscalls','amiga_sendrecv','amiga_fdhooks','amiga_libtables'],stackflags),
 ('posix',crt/'posixc',['__fdesc','read','write','close','ioctl','fcntl','dup2','pread64','pwrite','pwrite64'],posixflags)]:
 for name in files:
  source=folder/(name+'.c');obj=out/(prefix+'-'+name+'.o')
  command=common+flags+['-c',str(source),'-o',str(obj)]
  result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
  (out/(prefix+'-'+name+'.log')).write_text(result.stdout)
  commands.append(command)
  if result.returncode:raise RuntimeError(str(source)+'\n'+result.stdout)
  identities[str(source.relative_to(ROOT))]=hashlib.sha256(source.read_bytes()).hexdigest()
  identities[obj.name]=hashlib.sha256(obj.read_bytes()).hexdigest()
(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
(out/'result.json').write_text(json.dumps({'objects':'PASS','count':len(commands),
 'runtime':'UNTESTED','sha256':identities},indent=2)+'\n')
print(f'PASS: {len(commands)} actual AArch64 native SMP objects; no deployment/runtime claim')
