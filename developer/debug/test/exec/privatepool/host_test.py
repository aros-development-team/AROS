#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Test production allocators with 64-bit sanitizers and optional 32-bit WASI."""
import argparse
import pathlib
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--wasi-sdk', type=pathlib.Path,
                    help='also run the real 32-bit/eight-byte MemChunk ABI with WASI SDK')
parser.add_argument('--node', default='node', help='Node.js runner for the WASI check')
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
nommu = source.with_name("memory_nommu.c").read_text()
nommu = nommu[nommu.index("APTR nommu_AllocMem("):]
# Exercise the public wrapper bodies as well as their allocator core.
wrappers = ''
for name, signature in (
        ('allocate', 'static APTR Allocate(struct MemHeader *freeList, IPTR byteSize)'),
        ('deallocate', 'static void Deallocate(struct MemHeader *freeList, APTR memoryBlock, IPTR byteSize)')):
    wrapper = source.with_name(name + '.c').read_text()
    body = wrapper.split('    AROS_LIBFUNC_INIT', 1)[1].split('    AROS_LIBFUNC_EXIT', 1)[0]
    wrappers += signature + '{\nstruct ExecBase *SysBase = &host_sys;\n' + body + '}\n'
with tempfile.TemporaryDirectory(prefix='aros-privatepool-') as temporary:
    root = pathlib.Path(temporary)
    unit = root / 'allocator_test.c'
    unit.write_text('#include "host_support.h"\n' + core + '\n' + nommu + '\n' + wrappers +
                    '\n#include "privatepool.c"\n')
    configurations = [(16, False), (32, False)]
    if args.wasi_sdk:
        configurations.append((8, True))
    for smp in (False, True):
        for alignment, wasi in configurations:
            binary = root / f'privatepool-{alignment}-{smp}'
            options = ['-D__AROSEXEC_SMP__'] if smp else []
            compiler = [str(args.wasi_sdk / 'bin/clang')] if wasi else shlex.split(args.cc)
            sanitizers = [] if wasi else ['-fsanitize=address,undefined',
                                         '-fno-sanitize-recover=all', '-pthread']
            subprocess.run(compiler + [
                '-std=gnu11', '-O1', '-g', '-DPRIVATEPOOL_HOST_TEST',
                f'-DHOST_ALIGNMENT={alignment}', '-I', str(here), str(unit),
                '-o', str(binary)] + options + sanitizers, check=True)
            runner = [args.node, '--disable-warning=ExperimentalWarning',
                      str(here / 'run_wasi.mjs')] if wasi else []
            result = subprocess.run(runner + [str(binary)])
            print(f'{32 if wasi else 64}-bit host, alignment {alignment}, SMP {smp}: '
                  f'exit {result.returncode}', flush=True)
            if args.expect_failure:
                if result.returncode != 20:
                    raise SystemExit('Expected assertion failures (exit 20) from the original allocator')
            elif result.returncode:
                raise SystemExit(result.returncode)
