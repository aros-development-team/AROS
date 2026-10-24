#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Run the actual pread64/pwrite64 implementations against modeled DOS under sanitizers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
output = args.output.resolve()
if output.exists() or args.output.is_symlink() or output == ROOT or ROOT in output.parents:
    parser.error('output must be a new directory outside the source tree')
sources = [ROOT / 'compiler/crt/posixc' / name for name in ('pread64.c','pwrite64.c')]
bridge = ROOT / 'tools/crosstools/rust/unmodified/fs/positioned.c'
dupfd = ROOT / 'compiler/crt/posixc/__fdesc.c'
inputs = [dupfd, HERE / 'dupfd-host.c', bridge, HERE / 'positioned-bridge-host.c'] + sources + [HERE / 'positioned-io-host.c', Path(__file__).resolve()]
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
before = {str(p): digest(p) for p in inputs}
output.mkdir(parents=True)
for source, name in zip(sources, ('pread64','pwrite64')):
    text = re.sub(r'^#include[^\n]*\n', '', source.read_text(), flags=re.M)
    text = re.sub(r'\b' + name + r'\s*\(', 'aros_test_' + name + '(', text)
    text = text.replace('__off64_t', 'int64_t')
    (output / ('production-' + name.replace('64','') + '.c')).write_text(text)
text = re.sub(r'^#(?:include|define)[^\n]*\n', '', bridge.read_text(), flags=re.M)
text = text.replace('__off64_t', 'int64_t')
(output / 'production-positioned.c').write_text(text)
shutil.copyfile(HERE / 'positioned-bridge-host.c', output / 'bridge-fixture.c')
text = dupfd.read_text()
start = text.index('int __dupfd(')
end = text.index('\nLONG __oflags2amode', start)
(output / 'production-dupfd.c').write_text(text[start:end])
shutil.copyfile(HERE / 'dupfd-host.c', output / 'dupfd-fixture.c')
shutil.copyfile(HERE / 'positioned-io-host.c', output / 'fixture.c')
command = ['clang', '-std=c11', '-g', '-O1', '-pthread', '-Wall', '-Wextra', '-Werror',
           '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
           str(output / 'fixture.c'), '-o', str(output / 'fixture')]
env = dict(os.environ, ASAN_OPTIONS=('detect_leaks=0:abort_on_error=1' if sys.platform == 'darwin'
                                    else 'detect_leaks=1:abort_on_error=1'),
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
bridge_command = command[:-3] + [str(output / 'bridge-fixture.c'), '-o', str(output / 'bridge-fixture')]
dup_command = command[:-3] + [str(output / 'dupfd-fixture.c'), '-o', str(output / 'dupfd-fixture')]
for index, cmd in enumerate([command, [str(output / 'fixture')], bridge_command, [str(output / 'bridge-fixture')], dup_command, [str(output / 'dupfd-fixture')]]):
    result = subprocess.run(cmd, env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=30)
    (output / f'{index}.log').write_text(result.stdout)
    if result.returncode:
        raise RuntimeError(result.stdout)
if before != {str(p): digest(p) for p in inputs}:
    raise RuntimeError('source changed during positioned-read fixture')
(output / 'result.json').write_text(json.dumps({'host_positioned_io': 'PASS',
    'sanitizers': ['address', 'undefined'], 'DOS': 'MODELED', 'native_runtime': 'UNTESTED',
    'source_sha256': before, 'command': command}, indent=2) + '\n')
print('PASS actual pread64/pwrite64 host fixture; native DOS behavior remains a separate qualification')
