#!/usr/bin/env python3
"""Compile the actual AllocBitMap result functions with a recording C fixture."""
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
    thunks = path('m68kemu_thunks.c')
    shadow = path('m68kemu_shadow.c')
    runtime = path('m68kemu_runtime.c')
    generated = path('m68kemu_thunks_gen.c')
    layouts = path('m68kemu_shadow_layouts.h')
    native = source / 'compiler/include/exec/memory.h'
    # The official exec header retains its Latin-1 copyright byte. Keep its
    # raw source unchanged; only ASCII macro lines enter the fixture.
    text = '\n'.join((header.read_text(), offsets.read_text(),
                      native.read_text(encoding='latin1')))
    constants = []
    for name in ('M68KEMU_MAX_SHADOWS', 'M68KEMU_MAX_HANDLES',
                 'M68KEMU_HANDLE_BASE', 'M68K_SCR_BITMAP',
                 'MEMB_CLEAR', 'MEMF_CLEAR'):
        found = re.findall(r'^[ \t]*#define\s+' + re.escape(name) +
                           r'\s+([^\n]+)', text, re.M)
        assert len(found) == 1, (name, found)
        constants.append('#define ' + name + ' ' + found[0])
    bitmap = re.findall(r'\{\s*"BitMap",\s*(\d+),\s*(\d+),\s*'
                        r'shadow_fields_BitMap\s*\}', layouts.read_text())
    assert len(bitmap) == 1, bitmap
    constants += ['#define BITMAP_GUEST_SIZE ' + bitmap[0][0],
                  '#define BITMAP_SHADOW_TYPE ' + bitmap[0][1]]
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    groups = {
        'accessors.inc': [(header, n) for n in (
            'm68k_to_host', 'm68k_to_host_or_shadow', 'm68k_in_mem',
            'm68k_read16', 'm68k_read32')],
        'registry.inc': [(runtime, n) for n in ('m68k_bptr_out', 'm68k_bptr_in')] +
                        [(thunks, n) for n in ('shadow_register', 'shadow_lookup')],
        'factory.inc': [(shadow, n) for n in ('shadow_create', 'shadow_create_by_name')],
        'bitmap-consumer.inc': [(thunks, n) for n in (
            'bitmap_view', 'bitmap_native', 'm68k_bitmap_in')],
        'allocation.inc': [(thunks, 'thunk_gfx_AllocBitMap_m')],
        'free-consumer.inc': [(generated, 'thunk_graphics_FreeBitMap')],
    }
    for name, items in groups.items():
        (output / name).write_text(
            '\n'.join(extract_function(source_file, function)
                      for source_file, function in items))
    # The manual thunk must be the one registered for AllocBitMap, LVO -918.
    table = thunks.read_text().split(
        'const struct M68KThunkEntry m68kemu_thunks_graphics_manual[] = {', 1)[1].split('};', 1)[0]
    assert re.findall(r'\{\s*918,\s*(\w+)\s*\}', table) == ['thunk_gfx_AllocBitMap_m']

    def sha(p):
        return hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(
        source_inputs={str(p): sha(p) for p in (header, offsets, thunks, shadow,
                                                 runtime, generated, layouts, native)},
        functions={name: [function for _, function in items]
                   for name, items in groups.items()},
        manual_918_registered=True,
        generated={p.name: sha(p) for p in output.glob('*.inc')},
        scope='Actual production function bodies; controlled native allocation '
              'and guest heap fixture, no native SDK/ABI, graphics.library or CPU')
    (output / 'source-record.local.json').write_text(json.dumps(record, indent=2) + '\n')


def run(args, output):
    prepare(args.source_root.resolve(), output)
    if args.prepare_only:
        return 0
    command = shlex.split(args.cc) + [
        '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-O1', '-g', '-I' + str(output),
        str(Path(__file__).with_name('test_bitmap_result.c')),
        '-o', str(output / 'test_bitmap_result'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-fno-omit-frame-pointer']
    # Argument vectors avoid a shell; the developer selects the compiler.
    subprocess.run(command, check=True)  # nosec B603
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # Preserve the fixture's failure code; compiler failures raise above.
    return subprocess.run([str(output / 'test_bitmap_result')],
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
    with tempfile.TemporaryDirectory(prefix='m68kemu-bitmap-result-') as name:
        return run(args, Path(name) / 'case')


if __name__ == '__main__':
    raise SystemExit(main())
