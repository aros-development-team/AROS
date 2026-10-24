#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Check typed failure sentinels and thread-local errno in the actual refusal provider."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
out = a.output.resolve()
if out.exists() or a.output.is_symlink() or HERE.parents[4] in out.parents:
    p.error('output must be a new directory outside the core source tree')
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
inputs = {str(path): digest(path) for path in (HERE / 'unavailable.c', HERE / 'test-host.c')}
out.mkdir(parents=True)
for name in ('unavailable.c', 'test-host.c'):
    shutil.copyfile(HERE / name, out / name)
command = ['clang', '-std=c11', '-pthread', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
           '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
           str(out / 'test-host.c'), '-o', str(out / 'fixture')]
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
for index, cmd in enumerate([command, [str(out / 'fixture')]]):
    r = subprocess.run(cmd, env=env, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=30)
    (out / f'{index}.log').write_text(r.stdout)
    if r.returncode:
        raise RuntimeError(r.stdout)
if any(digest(Path(name)) != expected for name, expected in inputs.items()):
    raise RuntimeError('provider source changed during the fixture')
(out / 'result.json').write_text(json.dumps({'typed_unavailable_provider': 'PASS',
    'sanitizers': ['address', 'undefined'], 'parallel_iterations': 2048,
    'errno_provider': 'HOST', 'native_errno': 'UNTESTED', 'source_sha256': inputs,
    'command': command}, indent=2) + '\n')
print('PASS typed sentinels, unchanged outputs and per-thread errno under sanitizers')
