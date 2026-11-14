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
    # A definition starts in the first column. A call inside a statement
    # that a brace follows, such as a condition, does not.
    match = re.search(
        r'(?m)^(?=\S)[^\n;{}]*\b' + re.escape(name) +
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
    bridge = path('m68kemu_moira.cpp')
    native = source / 'compiler/include/exec/memory.h'
    dos = source / 'compiler/include/dos/dos.h'
    # The official headers retain Latin-1 bytes. Keep their raw source
    # unchanged; only ASCII macro lines enter the fixture.
    text = '\n'.join((header.read_text(), offsets.read_text(),
                      native.read_text(encoding='latin1'),
                      dos.read_text(encoding='latin1')))
    constants = []
    for name in ('M68KEMU_MAX_SHADOWS', 'M68KEMU_MAX_HANDLES',
                 'M68KEMU_HANDLE_BASE', 'M68K_SCR_BITMAP',
                 'MEMB_CLEAR', 'MEMF_CLEAR', 'DOS_EXALLCONTROL', 'DOS_FIB'):
        found = re.findall(r'^[ \t]*#define\s+' + re.escape(name) +
                           r'\s+([^\n]+)', text, re.M)
        assert len(found) == 1, (name, found)
        constants.append('#define ' + name + ' ' + found[0])
    bitmap = re.findall(r'\{\s*"BitMap",\s*(\d+),\s*(\d+),\s*'
                        r'shadow_fields_BitMap\s*\}', layouts.read_text())
    assert len(bitmap) == 1, bitmap
    constants += ['#define BITMAP_GUEST_SIZE ' + bitmap[0][0],
                  '#define BITMAP_SHADOW_TYPE ' + bitmap[0][1]]
    # The tag of an ExAll control's registry entry, as the tree defines it.
    # It is passed on as it is: whether it is the BitMap layout's number is
    # for the fixture's checks to show, not for this runner to refuse.
    exall = re.findall(r'^#define\s+SHADOW_EXALL\s+(\d+)\s*$', thunks.read_text(), re.M)
    assert len(exall) == 1, exall
    constants.append('#define SHADOW_EXALL ' + exall[0])
    # The manual thunk must be the one registered for AllocBitMap, LVO -918.
    table = thunks.read_text().split(
        'const struct M68KThunkEntry m68kemu_thunks_graphics_manual[] = {', 1)[1].split('};', 1)[0]
    assert re.findall(r'\{\s*918,\s*(\w+)\s*\}', table) == ['thunk_gfx_AllocBitMap_m']
    # FreeBitMap, LVO -924: the manual thunk when the tree registers one,
    # else the generated thunk that the generated table registers.
    free_manual = re.findall(r'\{\s*924,\s*(\w+)\s*\}', table)
    assert free_manual in ([], ['thunk_gfx_FreeBitMap_m']), free_manual
    assert re.search(r'\{\s*924,\s*thunk_graphics_FreeBitMap\s*\}', generated.read_text())
    free_source, free_name = ((thunks, free_manual[0]) if free_manual
                              else (generated, 'thunk_graphics_FreeBitMap'))
    # A manual thunk is found and returns before the generated table is searched.
    dispatch = bridge.read_text()
    first = dispatch.index('for (ULONG j = 0; j < lib->num_thunks; j++)')
    second = dispatch.index('for (ULONG j = 0; j < lib->num_gen_thunks; j++)')
    assert first < second and 'return;' in dispatch[first:second]
    constants.append('#define API_FREE ' + free_name)
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    groups = {
        'accessors.inc': [(header, n) for n in (
            'm68k_to_host', 'm68k_to_host_or_shadow', 'm68k_in_mem',
            'm68k_read16', 'm68k_read32')],
        'registry.inc': [(runtime, n) for n in ('m68k_bptr_out', 'm68k_bptr_in')] +
                        [(thunks, n) for n in ('shadow_register', 'shadow_lookup',
                                               'shadow_remove')],
        'factory.inc': [(shadow, n) for n in ('shadow_create', 'shadow_create_by_name',
                                              'shadow_destroy')],
        'bitmap-consumer.inc': [(thunks, n) for n in (
            'bitmap_view', 'bitmap_native', 'm68k_bitmap_in')],
        'allocation.inc': [(thunks, 'thunk_gfx_AllocBitMap_m')],
        'free-consumer.inc': [(free_source, free_name)],
        'dos-objects.inc': [(thunks, n) for n in (
            'exall_control_new', 'exall_control', 'thunk_dos_FreeDosObject')],
    }
    for name, items in groups.items():
        (output / name).write_text(
            '\n'.join(extract_function(source_file, function)
                      for source_file, function in items))

    def sha(p):
        return hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(
        source_inputs={str(p): sha(p) for p in (header, offsets, thunks, shadow,
                                                 runtime, generated, layouts, bridge,
                                                 native, dos)},
        functions={name: [function for _, function in items]
                   for name, items in groups.items()},
        manual_918_registered=True,
        manual_924_registered=bool(free_manual),
        free_thunk=free_name,
        bitmap_shadow_type=int(bitmap[0][1]),
        exall_shadow_type=int(exall[0]),
        manual_dispatch_precedes_generated=True,
        generated={p.name: sha(p) for p in output.glob('*.inc')},
        scope='Actual production function bodies; controlled native allocation '
              'and recording guest heap fixture, no native SDK/ABI, '
              'graphics.library or CPU')
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
