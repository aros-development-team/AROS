#!/usr/bin/env python3
"""The records of the BOOPSI classes a 68k program makes, across the first block of 32, and MakeClass refused."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - the developer explicitly selects a compiler
import tempfile


def function(text, name, origin, storage=r'static [^\n]*'):
    m = re.search(r'^' + storage + r'\b' + name + r'\(.*?\)\n\{', text, re.M | re.S)
    if not m:
        raise ValueError('%s: no function %s' % (origin, name))
    depth, i = 0, m.end() - 1
    while True:
        depth += {'{': 1, '}': -1}.get(text[i], 0)
        i += 1
        if depth == 0:
            return text[m.start():i]


def run(args, output):
    here = Path(__file__).resolve().parent
    bridge = args.source_root / 'rom/m68kemu'
    boopsi = (bridge / 'm68kemu_boopsi.c').read_text()
    intern = (bridge / 'm68kemu_intern.h').read_text()
    output.mkdir(parents=True, exist_ok=True)
    constants = [m.group(0) for m in re.finditer(r'^\s*#define (M68KEMU_CLASS_BLOCKS?)\s+\d+', intern, re.M)]
    if len(constants) != 2:
        raise ValueError('m68kemu_intern.h: the two class block constants')
    layout = [m.group(0) for m in re.finditer(r'^#define (CL_SIZE|CL_ID|SHADOW_BCLASS)\s+\d+', boopsi, re.M)]
    if len(layout) != 3:
        raise ValueError('m68kemu_boopsi.c: the size of a class, the offset of its ID and its shadow type')
    (output / 'class_constants.inc').write_text('\n'.join(c.strip() for c in constants + layout) + '\n')
    (output / 'class_storage.inc').write_text('\n'.join(
        function(boopsi, name, 'm68kemu_boopsi.c') for name in ('class_slot', 'class_room', 'class_drop', 'class_rec')))
    (output / 'class_makeclass.inc').write_text(function(boopsi, 'm68k_MakeClass', 'm68kemu_boopsi.c', 'IPTR ') + '\n')
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all'] if args.sanitize else []
    command = shlex.split(args.cxx) + [
        '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-g', '-O1', '-I' + str(output),
        str(here / 'test_class_storage.cpp'), '-o', str(output / 'test_class_storage')] + sanitizer
    subprocess.run(command, check=True)  # nosec B603 - developer-selected compiler argv
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')    # the test counts its own block frees
    subprocess.run([str(output / 'test_class_storage')], check=True, env=env)  # nosec B603 - binary built above


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-class-storage-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
