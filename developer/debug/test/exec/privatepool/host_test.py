#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Compile the production allocator functions with 64-bit host types and guards."""
import argparse
import pathlib
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--expect-failure', action='store_true')
parser.add_argument('--source', type=pathlib.Path,
                    help='alternate production memory.c for before/after checks')
args = parser.parse_args()
here = pathlib.Path(__file__).resolve().parent
source = args.source or here.parents[4] / 'rom/exec/memory.c'
text = source.read_text()
start = text.index('#ifdef NO_CONSISTENCY_CHECKS')
end = text.index('/*\n * TODO:\n * During transition', start)
core = text[start:end]
with tempfile.TemporaryDirectory(prefix='aros-privatepool-') as temporary:
    root = pathlib.Path(temporary)
    unit = root / 'allocator_test.c'
    unit.write_text('#include "host_support.h"\n' + core +
                    '\n#include "privatepool.c"\n')
    for smp in (False, True):
        for alignment in (16, 32):
            binary = root / f'privatepool-{alignment}-{smp}'
            options = ['-D__AROSEXEC_SMP__'] if smp else []
            subprocess.run(shlex.split(args.cc) + [
                '-std=gnu11', '-O1', '-g', '-fsanitize=address,undefined',
                '-fno-sanitize-recover=all', '-pthread', '-DPRIVATEPOOL_HOST_TEST',
                f'-DHOST_ALIGNMENT={alignment}', '-I', str(here), str(unit),
                '-o', str(binary)] + options, check=True)
            result = subprocess.run([str(binary)])
            print(f'64-bit host, alignment {alignment}, SMP {smp}: '
                  f'exit {result.returncode}', flush=True)
            if args.expect_failure:
                if result.returncode == 0:
                    raise SystemExit('Expected the original allocator to fail')
            elif result.returncode:
                raise SystemExit(result.returncode)
