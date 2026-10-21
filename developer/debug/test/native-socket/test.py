#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Host lifetime/admission fixture; real registry and native socket adapter code.

Transport primitives and Exec task/timer APIs are modeled. This is not native
network validation. ASan/UBSan and actual pthread overlap exercise retirement,
creator exit, duplicate identity, provider exclusion and POSIX dispatch.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[4]
FD = ROOT / 'workbench/libs/fd'
API = ROOT / 'workbench/network/stacks/AROSTCP/bsdsocket/api'
HERE = Path(__file__).resolve().parent

def stripped(path):
    return '\n'.join(x for x in path.read_text().splitlines()
                     if not x.startswith('#include')) + '\n'

def function(source, signature):
    start=source.index(signature)
    brace=source.index('{',start)
    level=1
    end=brace+1
    while level:
        level += {'{':1,'}':-1}.get(source[end],0)
        end+=1
    return source[start:end]+'\n'

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',type=Path,required=True,help='new artifact directory')
a=p.parse_args()
a.output=a.output.resolve()
if a.output==ROOT or ROOT in a.output.parents:
    p.error('output must be outside the source tree')
a.output.mkdir(parents=True,exist_ok=False)
inputs=[FD/'include/fd.h',FD/'fd_private.h',FD/'fd_alloc.c',FD/'fd_hooks.c',FD/'fd_lease.c',
       ROOT/'workbench/network/common/include/bsdsocket/native.h',API/'native_socket.h',
       API/'native_socket.c',ROOT/'compiler/crt/posixc/__fdesc.c',
       HERE/'host.h',HERE/'adapter.h',HERE/'cases.c', HERE/'context-cases.c',
       API/'amiga_api.c', ROOT/'compiler/crt/posixc/ioctl.c']
identities={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}
header=stripped(FD/'include/fd.h')
header=re.sub(r'^(?:LONG|void|APTR|fd_owner_t|const struct fd_hooks \*)\s*FD_\w+\([^;]*;', '',header,flags=re.M)
unit=(HERE/'host.h').read_text()+header+stripped(FD/'fd_private.h')
unit+='\n'.join(stripped(FD/name) for name in ['fd_alloc.c','fd_hooks.c','fd_lease.c'])
unit+=(HERE/'adapter.h').read_text()
unit+=stripped(ROOT/'workbench/network/common/include/bsdsocket/native.h')
unit+=stripped(API/'native_socket.h')+stripped(API/'native_socket.c')
# The actual POSIX dispatcher is included too; its call-site wrappers merely
# map arguments and errno, whereas this code owns operation admission/release.
unit+=function((ROOT/'compiler/crt/posixc/__fdesc.c').read_text(),'SIPTR __fdhook(')
unit+=function(re.sub(r'/\*.*?\*/', '', (ROOT/'compiler/crt/posixc/ioctl.c').read_text(), flags=re.S),'int ioctl(').replace('int ioctl(', 'int fixture_ioctl(',1)
unit+=(HERE/'cases.c').read_text()
source=a.output/'fixture.c';binary=a.output/'fixture'
source.write_text(unit)
command=['clang','-std=c11','-g','-O1','-pthread','-Wall','-Wextra',
         '-Werror=implicit-function-declaration','-fsanitize=address,undefined',
         '-fno-omit-frame-pointer',str(source),'-o',str(binary)]
subprocess.run(command,check=True)
subprocess.run([str(binary)],check=True,timeout=30)

context=a.output/'api_hookbase.c'
context.write_text(function((API/'amiga_api.c').read_text(),'struct SocketBase *api_hookbase_open(VOID)'))
context_source=a.output/'context-cases.c'
context_source.write_text((HERE/'context-cases.c').read_text())
context_command=['clang','-std=c11','-g','-O1','-Wall','-Wextra','-Werror',
 '-fsanitize=address,undefined','-fno-omit-frame-pointer',str(context_source),'-o',str(a.output/'context-fixture')]
subprocess.run(context_command,check=True)
subprocess.run([str(a.output/'context-fixture')],check=True,timeout=30)

if identities!={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}:
    raise RuntimeError('source changed during host fixture')
(a.output/'result.json').write_text(json.dumps({'host_lifetime':'PASS','sanitizers':['address','undefined'],
 'transport':'MODELED','native_runtime':'UNTESTED','sources_sha256':identities,'command':command,'context_command':context_command},indent=2)+'\n')
