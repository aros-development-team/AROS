#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Build a private poll probe with new adapters and an existing precompiled std (all inputs read-only)."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
RUST = ROOT / 'tools/crosstools/rust/unmodified'
p = argparse.ArgumentParser(description=__doc__)
for name in ('sdk','std-build','crosstools','output'):
    p.add_argument('--'+name,type=Path,required=True)
a = p.parse_args()
sdk, std, tools, out = [getattr(a,n).resolve() for n in ('sdk','std_build','crosstools','output')]
if out.exists() or any(out == x or x in out.parents for x in (ROOT,sdk,std,tools)):
    p.error('output must be new and outside input trees')
out.mkdir(parents=True)
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
inputs = [std / n for n in ('rustc','aarch64-aros-cc','libaros_rs_platform.a','aarch64-unknown-aros.json')]
inputs += [std / 'result.json', tools / 'bin/clang', tools / 'bin/llvm-ar', RUST / 'net/socket_native.h', RUST / 'poll-probe.rs', RUST / 'net/socket.c', RUST / 'sys/unavailable.c', ROOT / 'workbench/network/common/include/bsdsocket/native.h', Path(__file__).resolve()]
before = {str(path):digest(path) for path in inputs}
def run(cmd,name,env=None):
    (out/(name+'-command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
    r = subprocess.run(cmd,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60)
    (out/(name+'.log')).write_text(r.stdout)
    if r.returncode: raise RuntimeError(r.stdout)
    return r.stdout
include = out / 'include/bsdsocket';include.mkdir(parents=True)
shutil.copyfile(inputs[-2],include / 'native.h')
archive = out / 'libaros_rs_platform.a'
shutil.copyfile(std / archive.name,archive)
for name,directory in (('socket','net'),('unavailable','sys')):
    obj = out/(name+'.o')
    run([str(tools/'bin/clang'),'--target=aarch64-unknown-aros','-fno-pic','-mcmodel=large','-fno-builtin','-O2','-Wall','-Wextra','-Werror','-Wconversion','-Wsign-conversion',
        '-I'+str(out/'include'),'-I'+str(RUST/'net'),'-I'+str(sdk/'include/aros/posixc'),'-I'+str(sdk/'include/aros/stdc'),'-I'+str(sdk/'include'),
        '-c',str(RUST/directory/(name+'.c')),'-o',str(obj)],name)
    run([str(tools/'bin/llvm-ar'),'r',str(archive),str(obj)],'archive-'+name)
driver = out/'aarch64-aros-cc'
qualified = json.loads((std/'result.json').read_text())
collect = Path(qualified['collect_aros']).resolve()
arguments = [str(tools/'bin/clang'),'--target=aarch64-unknown-aros','--sysroot='+str(sdk),
             '-B'+str(collect.parent),'-B'+str(tools/'bin')]
driver.write_text('#!/bin/sh\nexec '+shlex.join(arguments)+' "$@" '+
                  shlex.join(['-L'+str(out),'-laros_rs_platform'])+'\n')
driver.chmod(0o755)
binary = out/'RustPollProbe'
run([str(std/'rustc'),'--edition=2021','-Zunstable-options','--target',str(std/'aarch64-unknown-aros.json'),'-Cpanic=abort',
     '-Copt-level=1','-Clinker='+str(driver),str(RUST/'poll-probe.rs'),'-o',str(binary)],'probe')
header = binary.read_bytes()[:64]
if header[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<HH',header,16)!=(1,183):
    raise RuntimeError('probe is not AArch64 ELF64 ET_REL')
if run([str(tools/'bin/llvm-nm'),'--undefined-only',str(binary)],'undefined').strip():
    raise RuntimeError('probe contains undefined symbols')
if before != {str(path):digest(path) for path in inputs}:raise RuntimeError('probe inputs changed')
(out/'result.json').write_text(json.dumps({'probe_link':'PASS','precompiled_std':'REUSED_READ_ONLY',
    'new_platform_archive':digest(archive),'binary_sha256':digest(binary),'guest':'UNTESTED',
    'source_sha256':before},indent=2)+'\n')
print('PASS private native RustPollProbe linked; matching batch-capable AROSTCP runtime required')
