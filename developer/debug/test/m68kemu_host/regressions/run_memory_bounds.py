#!/usr/bin/env python3
"""
Compile the actual memory-access functions with a small host fixture.

No AROS SDK or Moira checkout is needed. Only the selected production
functions are extracted; their bodies are not replaced by test copies.
"""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - this developer test must run a compiler
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


def run(args, output):
    bridge = args.source_root / 'rom/m68kemu'
    header = bridge / 'm68kemu_intern.h'
    cpu = bridge / 'm68kemu_moira.cpp'
    output.mkdir(parents=True, exist_ok=True)
    accessors = '\n'.join(extract_function(header, name) for name in
                          ('m68k_read16', 'm68k_read32',
                           'm68k_write16', 'm68k_write32'))
    callbacks = '\n'.join(extract_function(cpu, name) for name in
                          ('read16', 'write16', 'read16OnReset', 'read16Dasm'))
    (output / 'accessors.inc').write_text(accessors)
    (output / 'callbacks.inc').write_text(callbacks)
    text = cpu.read_text() + '\n' + (bridge / 'm68kemu_offsets.h').read_text()
    constants = []
    for name in ('SENTINEL_ADDR', 'NEST_SENTINEL', 'CUSTOM_BASE',
                 'CUSTOM_MASK', 'M68K_OP_STOP'):
        match = re.search(r'^#define\s+' + name + r'\s+(\S+)', text, re.M)
        if match:
            constants.append(f'#define {name} {match[1]}')
            if name == 'NEST_SENTINEL':
                constants.append('#define HAVE_NEST_SENTINEL 1')
    if 'low_guard' in accessors + callbacks:
        constants.append('#define HAVE_LOW_GUARD 1')
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    command = shlex.split(args.cxx) + [
        '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-g', '-O1', '-I' + str(output),
        str(Path(__file__).with_name('test_memory_bounds.cpp')),
        '-o', str(output / 'test_memory_bounds'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    # Argument lists avoid a shell; the developer selects the compiler.
    subprocess.run(command, check=True)  # nosec B603
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # Execute only the test binary built above in the selected build directory.
    subprocess.run([str(output / 'test_memory_bounds')], check=True, env=env)  # nosec B603


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
        with tempfile.TemporaryDirectory(prefix='m68kemu-memory-bounds-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
