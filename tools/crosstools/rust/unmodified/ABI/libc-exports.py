#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Force all ELF members of std/application libc into one object; reject duplicate C providers."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
p = argparse.ArgumentParser(description=__doc__)
for name in ('std-build', 'application-libc-rlib', 'crosstools', 'output'):
    p.add_argument('--' + name, type=Path, required=True)
p.add_argument('--binary', type=Path)
a = p.parse_args()
build, app, tools, out = (getattr(a, n).resolve() for n in
                         ('std_build', 'application_libc_rlib', 'crosstools', 'output'))
if out.exists() or a.output.is_symlink() or any(out == root or root in out.parents for root in (build, app.parent, tools, HERE.parents[4])):
    p.error('output must be new and outside the input trees')
stdlib = list((build / 'sysroot/lib/rustlib/aarch64-unknown-aros/lib').glob('liblibc-*.rlib'))
if len(stdlib) != 1:
    p.error('std-build must contain exactly one target libc')
inputs = [stdlib[0], app, HERE.parent / 'sys/unavailable.c']
if a.binary:
    inputs.append(a.binary.resolve())
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
before = {str(path): digest(path) for path in inputs}
out.mkdir(parents=True)
objects, commands, names = [], [], set()
for index, library in enumerate((stdlib[0], app)):
    directory = out / str(index)
    directory.mkdir()
    command = [str(tools / 'bin/llvm-ar'), 'x', str(library)]
    subprocess.run(command, cwd=directory, check=True)
    commands.append(command)
    for obj in sorted(directory.glob('*.o')):
        objects.append(str(obj))
        r = subprocess.run([str(tools / 'bin/llvm-nm'), '--defined-only', str(obj)],
                           text=True, stdout=subprocess.PIPE, check=True)
        names.update(line.split()[-1] for line in r.stdout.splitlines() if line.split())
forbidden = set(re.findall(r'^REFUSE\([^,]+,\s*(\w+),', (HERE.parent / 'sys/unavailable.c').read_text(), re.M))
forbidden.add('hstrerror')
problems = sorted(forbidden & names)
code = 0
if objects:
    command = [str(tools / 'bin/ld.lld'), '-r', '-o', str(out / 'both-libc.o'), *objects]
    r = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out / 'link.log').write_text(r.stdout)
    commands.append(command)
    code = r.returncode
if a.binary:
    r = subprocess.run([str(tools / 'bin/llvm-nm'), '--defined-only', str(a.binary.resolve())],
                       text=True, stdout=subprocess.PIPE, check=True)
    symbols = [line.split()[-1] for line in r.stdout.splitlines() if line.split()]
    for name in ('poll', 'if_nametoindex', 'mkdtemp'):
        if symbols.count('aros_rs_' + name) != 1:
            problems.append('native provider count: ' + name)
if any(digest(Path(name)) != expected for name, expected in before.items()):
    raise RuntimeError('libc inputs changed during export qualification')
report = {'verdict': 'FAIL' if code or problems else 'PASS',
          'all_libc_elf_members_merge': 'FAIL' if code else 'PASS',
          'rust_crate_c_provider_exports': problems, 'commands': commands,
          'source_sha256': before, 'elf_objects': len(objects),
          'runtime': 'UNTESTED'}
(out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({k: v for k, v in report.items() if k not in ('commands', 'source_sha256')}))
raise SystemExit(1 if code or problems else 0)
