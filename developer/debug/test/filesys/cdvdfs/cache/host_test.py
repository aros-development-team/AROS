#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Exercise the production CDVDFS cache with deterministic allocation and I/O."""
import argparse
import pathlib
import re
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--source', type=pathlib.Path)
parser.add_argument('--expect-failure', action='store_true')
args = parser.parse_args()
here = pathlib.Path(__file__).resolve().parent
source = args.source or here.parents[5] / 'rom/filesys/CDVDFS/src/cdrom.c'
text = source.read_text()
names = ['CDCacheLowMemHandler', 'Sector_Buffer', 'IO_Buffer',
         'Recover_File_Cache', 'Find_Cache_Slot', 'Read_Chunk_Internal',
         'Clear_Sector_Buffers']
functions = []
for name in names:
    pattern = (r'(?:static )?(?:AROS_INTH1\(' + name + r'[^\n]*\)|'
               r'(?:void|int|UBYTE \*)\s*' + name + r'\s*\([^;]*?\))\s*\{')
    match = re.search(pattern, text)
    if not match:
        if name == 'Recover_File_Cache':
            continue  # The original source has no recovery helper.
        raise SystemExit('Missing production function: ' + name)
    start = match.start()
    brace = text.index('{', match.start())
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    functions.append(text[start:end])
with tempfile.TemporaryDirectory(prefix='aros-cdcache-') as temporary:
    root = pathlib.Path(temporary)
    unit = root / 'cache_unit.c'
    unit.write_text('#include "host_support.h"\n' + '\n'.join(functions) +
                    '\n#include "cache_test.c"\n')
    binary = root / 'cache_test'
    subprocess.run(shlex.split(args.cc) + ['-std=c11', '-O1', '-g',
        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
        '-I', str(here), str(unit), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)])
    if args.expect_failure:
        if result.returncode != 20:
            raise SystemExit('Expected observable cache failures (exit 20), got ' + str(result.returncode))
    elif result.returncode:
        raise SystemExit(result.returncode)
