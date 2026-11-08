#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Exercise the production CD32 interrupt-mask helpers with an injected IRQ."""
import argparse
import pathlib
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--source', type=pathlib.Path)
parser.add_argument('--expect-failure', action='store_true')
args = parser.parse_args()
here = pathlib.Path(__file__).resolve().parent
source = args.source or here.parents[5] / 'arch/m68k-amiga/devs/cd/cd32.c'
text = source.read_text()
start = text.index('static VOID CD32_IntEnable(')
end = text.index('static AROS_INTH1(', start)
with tempfile.TemporaryDirectory(prefix='aros-cd32-intmask-') as directory:
    unit = pathlib.Path(directory) / 'test.c'
    binary = pathlib.Path(directory) / 'test'
    unit.write_text('#include "host_support.h"\n' + text[start:end] +
                    '\n#include "intmask.c"\n')
    subprocess.run(shlex.split(args.cc) + ['-std=c11', '-O1', '-g',
                   '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                   '-I', str(here), '-I', str(source.parent), str(unit),
                   '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)])
    expected = 20 if args.expect_failure else 0
    if result.returncode != expected:
        raise SystemExit(f'Expected exit {expected}, got {result.returncode}')
