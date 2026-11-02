#!/usr/bin/env python3
"""Run the production library loader's init/Open code with the checked-out Moira."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
import argparse
import importlib.util
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - the developer explicitly selects a compiler
import tempfile


def run(args, output):
    here = Path(__file__).resolve().parent
    spec = importlib.util.spec_from_file_location('openlibrary_test_functions',
                                                here / 'run_openlibrary_versions.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    bridge = args.source_root / 'rom/m68kemu'
    moira = args.moira_root or bridge / 'Moira'
    if not (moira / 'Moira/Moira.cpp').is_file():
        raise ValueError('Initialize rom/m68kemu/Moira before running this CPU test')
    output.mkdir(parents=True, exist_ok=True)
    source = (bridge / 'm68kemu_m68klib.c').read_text()
    (output / 'library_loading.inc').write_text('\n'.join(
        helper.extract_function(source, name, 'm68kemu_m68klib.c')
        for name in ('m68k_init_struct', 'M68KEmu_LoadM68KLibrary')))
    # The production offsets header also includes native DOS types. Retain
    # just the actual constant definitions this function-level fixture uses.
    fixture = (here / 'test_library_loading_versions.cpp').read_text()
    required = set(re.findall(r'\bM68K_[A-Z0-9_]+\b', source + fixture))
    offsets = (bridge / 'm68kemu_offsets.h').read_text()
    definitions = {m[1]: m[0] for m in re.finditer(
        r'^#define\s+(M68K_\w+)\s+[^\n]+', offsets, re.M)}
    pending = list(required)
    while pending:
        name = pending.pop()
        if name not in definitions:
            raise ValueError('Missing production constant: ' + name)
        for dependency in re.findall(r'\bM68K_[A-Z0-9_]+\b', definitions[name]):
            if dependency not in required:
                required.add(dependency)
                pending.append(dependency)
    (output / 'library_constants.inc').write_text(
        '\n'.join(definitions[name] for name in sorted(required)) + '\n')
    source = (bridge / 'm68kemu_intern.h').read_text()
    names = ['m68k_read16', 'm68k_read32']
    if re.search(r'^static inline BOOL m68k_write_allowed\s*\(', source, re.M):
        names.append('m68k_write_allowed')
    names += ['m68k_write16', 'm68k_write32']
    (output / 'accessors.inc').write_text('\n'.join(
        helper.extract_function(source, name, 'm68kemu_intern.h') for name in names))
    compiler = shlex.split(args.cxx)
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all'] if args.sanitize else []
    # Build the unchanged CPU separately at O0: no expensive whole-CPU
    # optimization is needed for these short instruction tests. Sanitizers
    # still instrument the core, production functions and fixture alike.
    objects = []
    for name in ('Moira', 'MoiraDebugger'):
        obj = output / (name + '.o')
        subprocess.run(compiler + ['-std=c++20', '-g', '-O0', '-c',  # nosec B603 - compiler argv, no shell
                       str(moira / 'Moira' / (name + '.cpp')), '-o', str(obj)] +
                       sanitizer, check=True)
        objects.append(str(obj))
    command = compiler + [
        '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-Wno-unused-parameter', '-g', '-O1', '-I' + str(output),
        '-I' + str(bridge), '-isystem', str(moira),
        str(here / 'test_library_loading_versions.cpp'), *objects,
        '-o', str(output / 'test_library_loading_versions')] + sanitizer
    subprocess.run(command, check=True)  # nosec B603 - developer-selected compiler argv
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_library_loading_versions')], check=True, env=env)  # nosec B603 - binary built above


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--moira-root', type=Path)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-loading-versions-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
