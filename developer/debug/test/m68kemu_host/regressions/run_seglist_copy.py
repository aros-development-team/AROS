#!/usr/bin/env python3
"""Run the production copy of a LoadSeg() seglist into containment on seglists in order and not."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# M68KEmu_LoadHunks() and the release of a copy come verbatim from
# m68kemu_memory.c. The relocation list types, their kinds and the tag that
# asks for the list are cut out of dos/doshunks.h, dos/dostags.h and
# utility/tagitem.h of the tree. No AROS SDK, Moira checkout or emulated
# machine is needed.
import argparse
import importlib.util
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - the developer explicitly selects a compiler
import tempfile


def cut(text, pattern, what):
    match = re.search(pattern, text, re.M | re.S)
    if not match:
        raise ValueError('Missing in the headers of the tree: ' + what)
    return match[0]


def prepare(args, output):
    here = Path(__file__).resolve().parent
    spec = importlib.util.spec_from_file_location('openlibrary_test_functions',
                                                here / 'run_openlibrary_versions.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    bridge = args.source_root / 'rom/m68kemu'
    include = args.source_root / 'compiler/include'
    output.mkdir(parents=True, exist_ok=True)
    memory = (bridge / 'm68kemu_memory.c').read_text()
    # A tree from before a copy could be given back has neither the walk of
    # an image nor the release. The fixture is told which it is.
    release = [name for name in ('M68KEmu_ImageHunk', 'M68KEmu_FreeHunks')
               if re.search(r'\b' + name + r'\s*\(', memory)]
    parts = []
    if len(release) == 2:
        parts.append('#define SEGLIST_COPY_CAN_BE_GIVEN_BACK 1\n')
        parts.append(helper.extract_function(memory, 'M68KEmu_ImageHunk', 'm68kemu_memory.c'))
    parts.append(helper.extract_function(memory, 'M68KEmu_LoadHunks', 'm68kemu_memory.c'))
    if len(release) == 2:
        parts.append(helper.extract_function(memory, 'M68KEmu_FreeHunks', 'm68kemu_memory.c'))
    (output / 'seglist_copy.inc').write_text('\n'.join(parts))
    # The headers are not UTF-8 (a copyright sign is one Latin-1 byte).
    tagitem = (include / 'utility/tagitem.h').read_text(encoding='latin-1')
    dostags = (include / 'dos/dostags.h').read_text(encoding='latin-1')
    doshunks = (include / 'dos/doshunks.h').read_text(encoding='latin-1')
    bptr = (include / 'dos/bptr.h').read_text(encoding='latin-1')
    start = bptr.index('struct AROSSegment\n')
    layout = bptr[start:bptr.index('/* BCPL strings', start)]
    if not layout.isascii() or '#define AROS_SEG_DATALEN' not in layout:
        raise ValueError('Missing ASCII seglist layout in dos/bptr.h')
    (output / 'seglist_layout.inc').write_text(
        layout + '\n#ifdef __cplusplus\n#undef AROS_SEG_DATA\n'
        '#define AROS_SEG_DATA(seg) ((UBYTE *)AROS_SEG_FROMBPTR(seg)->seg_Data)\n#endif\n')
    record = '\n'.join([
        cut(tagitem, r'^typedef ULONG Tag;[^\n]*$', 'Tag'),
        cut(tagitem, r'^struct TagItem\s*\{.*?^\}[^\n;]*;', 'struct TagItem'),
        cut(tagitem, r'^#define TAG_DONE\b[^\n]*$', 'TAG_DONE'),
        cut(tagitem, r'^#define TAG_USER\b[^\n]*$', 'TAG_USER'),
        cut(dostags, r'^#define GSLI_Dummy\b[^\n]*$', 'GSLI_Dummy'),
        cut(dostags, r'^#define GSLI_68KRELOCS\b[^\n]*$', 'GSLI_68KRELOCS'),
        cut(doshunks, r'^struct HunkRelocGroup\s*\{.*?^struct HunkRelocTable\s*\{.*?^\};',
            'the relocation list of dos/doshunks.h'),
    ]) + '\n'
    if not record.isascii():
        raise ValueError('The parts cut out of the headers are not plain ASCII')
    (output / 'seglist_record.inc').write_text(record)
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
        str(here / 'test_seglist_copy.cpp'),
        '-o', str(output / 'test_seglist_copy')] + sanitizer
    subprocess.run(command, check=True)  # nosec B603 - developer-selected compiler argv
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_seglist_copy')], check=True, env=env)  # nosec B603 - binary built above


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
        with tempfile.TemporaryDirectory(prefix='m68kemu-seglist-copy-') as path:
            run(args, Path(path))


if __name__ == '__main__':
    main()
