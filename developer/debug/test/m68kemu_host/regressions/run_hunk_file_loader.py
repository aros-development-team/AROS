#!/usr/bin/env python3
"""Run the production loader of hunk files on well-formed and malformed files."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# M68KEmu_LoadHunksFromMemory() and rd32() come verbatim from m68kemu_memory.c,
# the hunk numbers from dos/doshunks.h. No AROS SDK, Moira checkout or
# emulated machine is needed.
import argparse
import importlib.util
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - the developer explicitly selects a compiler
import tempfile


def prepare(args, output):
    here = Path(__file__).resolve().parent
    spec = importlib.util.spec_from_file_location('openlibrary_test_functions',
                                                here / 'run_openlibrary_versions.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    bridge = args.source_root / 'rom/m68kemu'
    output.mkdir(parents=True, exist_ok=True)
    memory = (bridge / 'm68kemu_memory.c').read_text()
    (output / 'hunk_file_loader.inc').write_text('\n'.join(
        helper.extract_function(memory, name, 'm68kemu_memory.c')
        for name in ('rd32', 'M68KEmu_LoadHunksFromMemory')))
    # The hunk numbers are those of the DOS header; the mask is the bridge's.
    fixture = (here / 'test_hunk_file_loader.cpp').read_text()
    loader = helper.extract_function(memory, 'M68KEmu_LoadHunksFromMemory', 'm68kemu_memory.c')
    required = sorted(set(re.findall(r'\bHUNK_[A-Z0-9_]+\b', loader + fixture)))
    # dos/doshunks.h is not UTF-8 (its copyright sign is one Latin-1 byte)
    headers = ((args.source_root / 'compiler/include/dos/doshunks.h').read_text(encoding='latin-1') +
               (bridge / 'm68kemu_offsets.h').read_text())
    definitions = {m[1]: m[0] for m in re.finditer(
        r'^#define\s+(HUNK_\w+)\s+[^\n/]+', headers, re.M)}
    missing = [name for name in required if name not in definitions]
    if missing:
        raise ValueError('Missing production constants: ' + ', '.join(missing))
    (output / 'hunk_constants.inc').write_text(
        '\n'.join(definitions[name].rstrip() for name in required) + '\n')
    source = (bridge / 'm68kemu_intern.h').read_text()
    names = ['m68k_to_host', 'm68k_in_mem', 'm68k_read32']
    if re.search(r'^static inline BOOL m68k_write_allowed\s*\(', source, re.M):
        names.append('m68k_write_allowed')
    names.append('m68k_write32')
    (output / 'accessors.inc').write_text('\n'.join(
        helper.extract_function(source, name, 'm68kemu_intern.h') for name in names))
    return here


def run(args, output):
    here = prepare(args, output)
    if args.prepare_only:
        return
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all'] if args.sanitize else []
    command = shlex.split(args.cxx) + [
        '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-Wno-unused-parameter', '-g', '-O1', '-I' + str(output),
        str(here / 'test_hunk_file_loader.cpp'),
        '-o', str(output / 'test_hunk_file_loader')] + sanitizer
    subprocess.run(command, check=True)  # nosec B603 - developer-selected compiler argv
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_hunk_file_loader')], check=True, env=env)  # nosec B603 - binary built above


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--prepare-only', action='store_true',
                        help='write the extracted sources and stop: no compiler, no run')
    args = parser.parse_args()
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-hunk-file-loader-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
