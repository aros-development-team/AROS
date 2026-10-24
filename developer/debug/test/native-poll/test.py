#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Exercise actual native batch scan/admission and shared wait engine with modeled Exec I/O."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
SOCKET = HERE.parent / 'native-socket'
API = ROOT / 'workbench/network/stacks/AROSTCP/bsdsocket/api'
FD = ROOT / 'workbench/libs/fd'
def strip(path):
    return '\n'.join(line for line in path.read_text().splitlines() if not line.startswith('#include')) + '\n'
def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    level = 1
    end = brace + 1
    while level:
        level += {'{': 1, '}': -1}.get(source[end], 0)
        end += 1
    return source[start:end] + '\n'
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
out = a.output.resolve()
if out == ROOT or ROOT in out.parents:
    p.error('output must be a new directory outside source')
out.mkdir(parents=True, exist_ok=False)
inputs = [FD / name for name in ('include/fd.h','fd_private.h','fd_alloc.c','fd_hooks.c','fd_lease.c')]
inputs += [SOCKET / name for name in ('host.h','adapter.h')]
inputs += [API / name for name in ('native_socket.c','native_socket.h','amiga_generic.c')]
inputs += [ROOT / 'workbench/network/common/include/bsdsocket/native.h', HERE / 'wait-host.h', HERE / 'cases.c', Path(__file__).resolve()]
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
before = {str(path): digest(path) for path in inputs}
header = strip(FD / 'include/fd.h')
header = re.sub(r'^(?:LONG|void|APTR|fd_owner_t|const struct fd_hooks \*)\s*FD_\w+\([^;]*;', '', header, flags=re.M)
unit = (SOCKET / 'host.h').read_text() + header + strip(FD / 'fd_private.h')
unit += '\n'.join(strip(FD / name) for name in ('fd_alloc.c','fd_hooks.c','fd_lease.c'))
unit += '#define NATIVE_POLL_ENGINE_TEST 1\n' + (SOCKET / 'adapter.h').read_text()
unit += strip(ROOT / 'workbench/network/common/include/bsdsocket/native.h')
common = (API / 'amiga_generic.c').read_text()
unit += strip(API / 'native_socket.h')
unit += function(common, 'struct newselbuf {') + ';\n'
unit += (HERE / 'wait-host.h').read_text()
# Extract the actual production wait function, preserving its loop/timer logic;
# no handwritten model of the production algorithm is accepted here.
common = (API / 'amiga_generic.c').read_text()
unit += function(common, 'static LONG wait_select(')
unit += function(common, 'LONG __WaitSelect(')
unit += function(common, 'LONG __WaitNativePoll(')
unit += function(common, 'static int\nselscan(')
unit += function(common, 'static int\nsoo_select(')
unit += strip(API / 'native_socket.c') + (HERE / 'cases.c').read_text()
source = out / 'fixture.c'; binary = out / 'fixture'
source.write_text(unit)
command = ['clang','-std=c11','-g','-O1','-pthread','-Wall','-Wextra',
           '-Werror=implicit-function-declaration','-fsanitize=address,undefined',
           '-fno-omit-frame-pointer',str(source),'-o',str(binary)]
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
for i, cmd in enumerate((command, [str(binary)])):
    r = subprocess.run(cmd, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    (out / f'{i}.log').write_text(r.stdout)
    if r.returncode:
        raise RuntimeError(r.stdout)
if before != {str(path): digest(path) for path in inputs}:
    raise RuntimeError('poll source changed during fixture')
(out / 'result.json').write_text(json.dumps({'native_batch_poll_host':'PASS', 'source_sha256':before,
    'sanitizers':['address','undefined'], 'wait_engine':'ACTUAL_SOURCE', 'Exec_transport':'MODELED',
    'host_deadlines':'REAL_MONOTONIC_SLEEP', 'native_guest':'UNTESTED', 'command':command}, indent=2) + '\n')
print((out / '1.log').read_text(), end='')
