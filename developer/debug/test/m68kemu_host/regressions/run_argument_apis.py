#!/usr/bin/env python3
"""Compile the actual guest argument API functions with a recording C fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# Adapted from the focused memory-bounds and MemHeader regression runners.
# No native SDK, CPU backend or application binary is needed.
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - this developer test runs a selected compiler
import tempfile


def extract_function(path, name):
    source = path.read_text()
    masked = re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: ''.join('\n' if c == '\n' else ' ' for c in match[0]),
        source,
    )
    match = re.search(
        r'(?m)^[^\n;{}]*\b' + re.escape(name) +
        r'\s*\([^;{}]*\)[^;{}]*\{', masked)
    if not match:
        raise ValueError(f'{path}: definition of {name} not found')
    end = masked.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    line = source[:match.start()].count('\n') + 1
    return f'#line {line} "{path.name}"\n' + source[match.start():end] + '\n'


def prepare(source, output):
    assert not output.exists(), 'Use a fresh build directory'
    output.mkdir(parents=True)

    def path(name):
        return source / 'rom/m68kemu' / name
    header = path('m68kemu_intern.h')
    offsets = path('m68kemu_offsets.h')
    manual = path('m68kemu_thunks.c')
    generated = path('m68kemu_thunks_gen.c')
    bridge = path('m68kemu_moira.cpp')
    native = source / 'compiler/include/dos/dos.h'
    # The official header retains Latin-1 bytes. Keep its raw source
    # unchanged; only ASCII macro lines enter the fixture.
    text = offsets.read_text() + '\n' + native.read_text(encoding='latin1')
    constants = []
    for name in ('M68K_EXECBASE_THISTASK', 'M68K_PR_ARGUMENTS', 'ERROR_BAD_NUMBER'):
        found = re.findall(r'^\s*#define\s+' + re.escape(name) +
                           r'\s+([^\n]+)', text, re.M)
        assert len(found) == 1, (name, found)
        constants.append('#define ' + name + ' ' + found[0])
    table = manual.read_text().split(
        'const struct M68KThunkEntry m68kemu_thunks_dos[] = {', 1)[1].split('};', 1)[0]
    selected = {lvo: re.findall(r'\{\s*' + str(lvo) + r',\s*(\w+)\s*\}', table)
                for lvo in (534, 540)}
    assert all(len(v) <= 1 for v in selected.values()), selected
    has_manual = all(selected.values())
    # Both manual thunks are registered together, or neither is.
    assert has_manual or not any(selected.values()), selected
    body = ''
    if has_manual:
        assert selected == {534: ['thunk_dos_GetArgStr_m'], 540: ['thunk_dos_SetArgStr_m']}
        for name in ('m68k_argument_field', 'm68k_argument_string',
                     'thunk_dos_GetArgStr_m', 'thunk_dos_SetArgStr_m'):
            body += extract_function(manual, name) + '\n'
        names = ['thunk_dos_GetArgStr_m', 'thunk_dos_SetArgStr_m']
    else:
        names = ['thunk_dos_GetArgStr', 'thunk_dos_SetArgStr']
        for name in names:
            body += extract_function(generated, name) + '\n'
    constants += ['#define API_GET ' + names[0], '#define API_SET ' + names[1]]
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    (output / 'api.inc').write_text(body)
    (output / 'accessors.inc').write_text('\n'.join(
        extract_function(header, name) for name in (
            'm68k_to_host', 'm68k_in_mem', 'm68k_read32',
            'm68k_write_allowed', 'm68k_write32')))
    # A manual thunk is found and returns before the generated table is searched.
    dispatch = bridge.read_text()
    first = dispatch.index('for (ULONG j = 0; j < lib->num_thunks; j++)')
    second = dispatch.index('for (ULONG j = 0; j < lib->num_gen_thunks; j++)')
    assert first < second and 'return;' in dispatch[first:second]

    def sha(p):
        return hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(
        source_inputs={str(p): sha(p) for p in (header, offsets, manual, generated,
                                                 bridge, native)},
        manual_selected=has_manual, selected_functions=names,
        manual_registration={str(lvo): found for lvo, found in selected.items()},
        manual_dispatch_precedes_generated=True,
        generated={p.name: sha(p) for p in output.glob('*.inc')},
        scope='Actual paired API and accessor bodies; controlled native accessors, '
              'error and CPU-register collaborators, no native SDK/ABI or CPU')
    (output / 'source-record.local.json').write_text(json.dumps(record, indent=2) + '\n')


def run(args, output):
    prepare(args.source_root.resolve(), output)
    if args.prepare_only:
        return 0
    command = shlex.split(args.cc) + [
        '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-O1', '-g', '-I' + str(output),
        str(Path(__file__).with_name('test_argument_apis.c')),
        '-o', str(output / 'test_argument_apis'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-fno-omit-frame-pointer']
    # Argument vectors avoid a shell; the developer selects the compiler.
    subprocess.run(command, check=True)  # nosec B603
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # Preserve the fixture's failure code; compiler failures raise above.
    return subprocess.run([str(output / 'test_argument_apis')],
                          env=env).returncode  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path,
                        default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    if args.prepare_only and args.build_dir is None:
        parser.error('--prepare-only requires --build-dir')
    if args.build_dir:
        return run(args, args.build_dir.resolve())
    with tempfile.TemporaryDirectory(prefix='m68kemu-argument-apis-') as name:
        return run(args, Path(name) / 'case')


if __name__ == '__main__':
    raise SystemExit(main())
