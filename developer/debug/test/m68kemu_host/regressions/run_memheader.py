#!/usr/bin/env python3
"""Build the production classic MemHeader handlers with a standalone C fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - the developer selects a compiler
import tempfile


def function(path, name):
    source = path.read_text()
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source)
    match = re.search(r'(?m)^[^\n;{}]*\b' + re.escape(name) + r'\s*\([^;{}]*\)[^;{}]*\{', masked)
    if not match:
        raise ValueError(f'{path}: definition of {name} not found')
    end = masked.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    line = source[:match.start()].count('\n') + 1
    return f'#line {line} "{path.name}"\n' + source[match.start():end] + '\n'


def run(args, output):
    bridge = args.source_root / 'rom/m68kemu'
    header, offsets = bridge / 'm68kemu_intern.h', bridge / 'm68kemu_offsets.h'
    thunks = args.thunks or bridge / 'm68kemu_thunks.c'
    source = thunks.read_text()
    start = source.index('/* Allocate() and Deallocate() on a ')
    end = source.index('/* -960: GetBitMapAttr', start)
    output.mkdir(parents=True, exist_ok=True)
    line = source[:start].count('\n') + 1
    (output / 'memheader.inc').write_text(f'#line {line} "m68kemu_thunks.c"\n' + source[start:end])
    accessors = ['m68k_read32']
    if re.search(r'^static inline BOOL m68k_write_allowed\s*\(', header.read_text(), re.M):
        accessors.append('m68k_write_allowed')
    accessors.append('m68k_write32')
    (output / 'accessors.inc').write_text('\n'.join(function(header, n) for n in accessors))
    constants = []
    for name in ('M68K_SIZEOF_MEMHEADER', 'M68K_MH_FIRST', 'M68K_MH_FREE'):
        match = re.search(r'^#define\s+' + name + r'\s+(\S+)', offsets.read_text(), re.M)
        if not match:
            raise ValueError(f'{offsets}: {name} not found')
        constants.append(f'#define {name} {match[1]}')
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    if args.prepare_only:
        return
    command = shlex.split(args.cc) + ['-std=c99', '-Wall', '-Wextra', '-Werror',
        '-Wno-unused-function', '-O1', '-g', '-I' + str(output),
        str(Path(__file__).with_name('test_memheader.c')), '-o', str(output / 'test_memheader')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    subprocess.run(command, check=True)  # nosec B603 - explicit compiler argument list
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_memheader')], check=True, env=env)  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--thunks', type=Path, help='test a frozen production source without editing the checkout')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--prepare-only', action='store_true', help='retain includes for a native C compiler; needs --build-dir')
    args = parser.parse_args()
    if args.prepare_only and not args.build_dir:
        parser.error('--prepare-only requires --build-dir')
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-memheader-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
