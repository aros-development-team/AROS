#!/usr/bin/env python3
"""Compile the production commodities CreateCxObj thunk with a host fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# The function bodies come verbatim from m68kemu_math.c and the CX_ object
# types from libraries/commodities.h. No AROS SDK or emulated machine is needed.

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - this developer test must run a compiler
import tempfile


def extract_function(source, name, filename="m68kemu_math.c"):
    masked = re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: ''.join('\n' if c == '\n' else ' ' for c in match[0]),
        source,
    )
    match = re.search(
        r'(?m)^[^\n;{}]*\b' + re.escape(name) +
        r'\s*\([^;{}]*\)[^;{}]*\{', masked)
    if not match:
        raise ValueError(f'definition of {name} not found')
    end = masked.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    line = source[:match.start()].count('\n') + 1
    return f'#line {line} "{filename}"\n' + source[match.start():end] + '\n'


def run(args, output):
    source = (args.source_root / 'rom/m68kemu/m68kemu_math.c').read_text()
    functions = [extract_function(source, name) for name in ('cx_str', 'cx_Create')]
    header = (args.source_root / 'compiler/include/libraries/commodities.h').read_text(encoding='latin-1')
    constants = []
    for name in ('CX_INVALID', 'CX_FILTER', 'CX_TYPEFILTER', 'CX_SEND', 'CX_SIGNAL',
                 'CX_TRANSLATE', 'CX_BROKER', 'CX_DEBUG', 'CX_CUSTOM', 'CX_ZERO'):
        found = re.search(r'^#\s*define\s+' + name + r'\s+(\d+)', header, re.M)
        if not found:
            raise ValueError(f'{name} not found')
        constants.append(f'#define {name} {found[1]}\n')
    output.mkdir(parents=True, exist_ok=True)
    (output / 'functions.inc').write_text('\n'.join(functions))
    (output / 'constants.inc').write_text(''.join(constants))
    command = shlex.split(args.cxx) + [
        '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-Wno-unused-parameter', '-g', '-O1', '-I' + str(output),
        str(Path(__file__).with_name('test_cx_objects.cpp')),
        '-o', str(output / 'test_cx_objects'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    # Argument lists avoid a shell; the developer selects the compiler.
    subprocess.run(command, check=True)  # nosec B603
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # Execute only the test binary built above in the selected build directory.
    subprocess.run([str(output / 'test_cx_objects')], check=True, env=env)  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path,
                        default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-cx-objects-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
