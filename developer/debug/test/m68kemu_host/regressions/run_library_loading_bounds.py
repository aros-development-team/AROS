#!/usr/bin/env python3
"""Run the production 68k disk library loader on well-formed and malformed images."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# The loader and its helpers come verbatim from m68kemu_m68klib.c and
# m68kemu_memory.c. No AROS SDK, Moira checkout or emulated machine is needed.
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
    library = (bridge / 'm68kemu_m68klib.c').read_text()
    memory = (bridge / 'm68kemu_memory.c').read_text()
    # A tree from before the loader walked and gave back the image it placed
    # has neither the two image helpers nor the rollback helper. The same
    # fixture then shows what that loader does with the same images.
    functions = [helper.extract_function(memory, name, 'm68kemu_memory.c')
                 for name in ('M68KEmu_ImageHunk', 'M68KEmu_FreeImage')
                 if re.search(r'\b' + name + r'\s*\(', memory)]
    functions += [helper.extract_function(library, name, 'm68kemu_m68klib.c')
                  for name in ('m68k_init_struct', 'm68k_library_abandon',
                               'M68KEmu_LoadM68KLibrary')
                  if re.search(r'\b' + name + r'\s*\(', library)]
    (output / 'library_loading.inc').write_text('\n'.join(functions))
    # The production offsets header also includes native DOS types. Retain
    # just the constant definitions the loader and this fixture use.
    fixture = (here / 'test_library_loading_bounds.cpp').read_text()
    required = set(re.findall(r'\bM68K_[A-Z0-9_]+\b', library + fixture))
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
    names = ['m68k_in_mem', 'm68k_read16', 'm68k_read32']
    if re.search(r'^static inline BOOL m68k_write_allowed\s*\(', source, re.M):
        names.append('m68k_write_allowed')
    names += ['m68k_write16', 'm68k_write32']
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
        str(here / 'test_library_loading_bounds.cpp'),
        '-o', str(output / 'test_library_loading_bounds')] + sanitizer
    subprocess.run(command, check=True)  # nosec B603 - developer-selected compiler argv
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_library_loading_bounds')], check=True, env=env)  # nosec B603 - binary built above


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
        with tempfile.TemporaryDirectory(prefix='m68kemu-loading-bounds-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
