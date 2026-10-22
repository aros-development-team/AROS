#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Cross-build the whole POSIX library in private output, without invoking Make."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[4]
parser = argparse.ArgumentParser(description=__doc__)
for name in ('sdk', 'generated', 'crosstools', 'collect-aros', 'genmodule', 'output'):
    parser.add_argument('--' + name, type=Path, required=True)
args = parser.parse_args()
sdk, generated, tools, output = (getattr(args, name).resolve()
    for name in ('sdk', 'generated', 'crosstools', 'output'))
if 'struct __aros_libinit_sets' not in (sdk / 'include/aros/symbolsets.h').read_text():
    parser.error('SDK lacks the current libinit sets ABI; use an SDK matching genmodule and the core')
if output.exists() or args.output.is_symlink():
    parser.error('output must not exist')
if any(p == output or p in output.parents for p in
       (ROOT, sdk, generated, tools, args.collect_aros.resolve().parent, args.genmodule.resolve().parent)):
    parser.error('output must be outside all inputs')
ancestor = next(p for p in output.parents if p.is_dir())
if shutil.disk_usage(ancestor).free < 5 * 1024**3:
    parser.error('at least 5 GiB free disk space is required')

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

# Refuse a header/library change while another build updates shared inputs.
# These are read-only inputs: no generated output is installed into them.
frozen_paths = [args.genmodule.resolve(), args.collect_aros.resolve(), tools / 'bin/clang']
for directory in (sdk / 'include', generated / 'include', ROOT / 'compiler/crt'):
    frozen_paths.extend(p for p in directory.rglob('*.h') if p.is_file())
frozen_paths.extend(p for p in (sdk / 'lib').glob('*.a') if p.is_file())
frozen_paths.append(ROOT / 'compiler/crt/posixc/posixc.conf')
frozen = {str(p): digest(p) for p in frozen_paths}
output.mkdir(parents=True)
(output / 'include/libraries').mkdir(parents=True)
shutil.copyfile(ROOT / 'workbench/libs/fd/include/fd.h', output / 'include/libraries/fd.h')
crt = ROOT / 'compiler/crt'
module = output / 'module'
for directory in ('proto','clib','inline','defines'):
    (module / 'include' / directory).mkdir(parents=True)
generator_commands = []
for action, destination in [('writefiles', module), ('writelibdefs', module / 'include'),
                            ('writeincludes', module / 'include')]:
    command = [str(args.genmodule.resolve()), '-c', str(crt / 'posixc/posixc.conf'),
               '-d', str(destination), action, 'posixc', 'library']
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / (action + '.log')).write_text(result.stdout)
    if result.returncode:
        raise RuntimeError(result.stdout)
    generator_commands.append(command)
(output / 'generator-commands.json').write_text(json.dumps(generator_commands, indent=2) + '\n')
# Stale generated per-task bases can reuse a previous CLI command's closed
# standard handles. The generated code must include the core's handle guard.
if 'dupbase->cis !=' not in (module / 'posixc_start.c').read_text():
    raise RuntimeError('genmodule lacks the per-task standard-handle identity guard')
make = (crt / 'posixc/mmakefile.src').read_text()
names = []
for variable in ('POSIXC', 'POSIXC_LOCALE', 'POSIXC_WCHAR', 'POSIXC_WCHAR_LOCALE', 'POSIXC_COMMON'):
    match = re.search(r'^' + variable + r'\s*:?=\s*', make, re.M)
    for line in make[match.end():].splitlines():
        names.extend(line.rstrip('\\').split())
        if not line.endswith('\\'):
            break
if len(set(names)) != len(names):
    raise RuntimeError('duplicate POSIX source')
arch = ROOT / 'arch/aarch64-all/posixc'
sources = [(name, arch / (name + '.s') if (arch / (name + '.s')).is_file()
            else crt / 'posixc' / (name + '.c')) for name in names]
sources += [(name, ROOT / 'arch/aarch64-all/posixc' / (name + '.s'))
            for name in ('vfork', 'vfork_longjmp')]
sources += [(name, module / (name + '.c')) for name in ('posixc_start', 'posixc_end')]
common = [str(tools / 'bin/clang'), '--target=aarch64-unknown-aros', '-mcmodel=large',
          '-fno-pic', '-fno-builtin', '-O2', '-std=gnu99', '-Wno-error=implicit-function-declaration',
          '-Wno-pointer-sign', '-Wno-format', '-Wno-parentheses',
          '-DAROS_POSIXC_BUILD', '-D__TIMER_NOLIBBASE__', '-D_GNU_SOURCE', '-D_LARGEFILE64_SOURCE',
          '-DLC_LIBDEFS_FILE="posixc_libdefs.h"',
          '-I' + str(output / 'include'), '-I' + str(module / 'include'),
          '-I' + str(generated / 'include'), '-I' + str(module),
          '-I' + str(crt / 'posixc'), '-I' + str(crt / 'stdc'),
          '-I' + str(crt / 'stdcio'), '-I' + str(crt / 'common'),
          '-I' + str(crt / 'posixc/include/aros/posixc'),
          '-isystem', str(sdk / 'include/aros/posixc'),
          '-isystem', str(sdk / 'include/aros/stdc'), '-isystem', str(sdk / 'include')]

def compile_one(entry):
    name, source = entry
    name = name.replace('/', '-')
    obj = output / (name + '.o')
    command = common + (['-x', 'assembler-with-cpp'] if source.suffix == '.s' else [])
    command += ['-c', str(source), '-o', str(obj)]
    before = digest(source)
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / (name + '.log')).write_text(result.stdout)
    if result.returncode or digest(source) != before:
        raise RuntimeError(str(source) + '\n' + result.stdout)
    return {'source': str(source), 'source_sha256': before,
            'object': str(obj), 'object_sha256': digest(obj), 'command': command}

with ThreadPoolExecutor(max_workers=4) as executor:
    records = list(executor.map(compile_one, sources))
(output / 'objects.json').write_text(json.dumps(records, indent=2) + '\n')
shim = output / 'linkshim'
shim.mkdir()
(shim / 'ld').symlink_to(tools / 'bin/ld.lld')
command = [str(args.collect_aros.resolve()), '--eh-frame-hdr', '-o', str(output / 'posixc.library'),
           '-L' + str(sdk / 'lib')] + [r['object'] for r in records] + [
           '-(', '-lstdc_rel', '-lstdcio_rel', '-lamiga', '-larossupport',
           '-llibinit', '-lautoinit', '-ldos', '-lexec', '-lutility', '-laros', '-)']
environment = dict(os.environ, COMPILER_PATH=str(shim))
result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, env=environment)
(output / 'link-command.json').write_text(json.dumps(command, indent=2) + '\n')
(output / 'link.log').write_text(result.stdout)
if result.returncode:
    raise RuntimeError(result.stdout)
undefined = subprocess.check_output([str(tools / 'bin/llvm-nm'), '--undefined-only',
                                    str(output / 'posixc.library')], text=True)
(output / 'undefined.txt').write_text(undefined)
if undefined.strip():
    raise RuntimeError('POSIX library has undefined symbols')
header = (output / 'posixc.library').read_bytes()[:64]
if (header[:6] != b'\x7fELF\x02\x01' or header[7] != 15 or
        struct.unpack_from('<HH', header, 16) != (1, 183)):
    raise RuntimeError('POSIX library is not native AROS AArch64 ELF ET_REL')
if frozen != {str(p): digest(p) for p in frozen_paths}:
    raise RuntimeError('SDK, source headers, configuration or tools changed during the build')
(output / 'input-sha256.json').write_text(json.dumps(frozen, indent=2) + '\n')
stub_records = []
for name in ('pwrite', 'pwrite64'):
    source = module / ('posixc_' + name + '_stub.c')
    if not source.is_file():
        raise RuntimeError('missing generated positioned-I/O import stub: ' + str(source))
    stub_records.append(compile_one(('stub-' + name, source)))
if stub_records:
    stub_archive = output / 'libaros_positional.a'
    archive_command = [str(tools / 'bin/llvm-ar'), 'rcs', str(stub_archive)] + [r['object'] for r in stub_records]
    subprocess.run(archive_command, check=True)
    (output / 'stub-command.json').write_text(json.dumps(archive_command, indent=2) + '\n')
    (output / 'stub-objects.json').write_text(json.dumps(stub_records, indent=2) + '\n')
if frozen != {str(p): digest(p) for p in frozen_paths}:
    raise RuntimeError('native build inputs changed while generating import stubs')
(output / 'result.json').write_text(json.dumps({'posix_library_link': 'PASS',
    'objects': len(records), 'runtime': 'UNTESTED',
    'positional_stub_archive': 'libaros_positional.a',
    'positional_stub_sha256': digest(stub_archive),
    'binary_sha256': digest(output / 'posixc.library')}, indent=2) + '\n')
print(f'PASS: {len(records)} POSIX objects and library link; runtime UNTESTED')
