#!/usr/bin/env python3
"""Compile actual requested-root-stack functions with a recording C fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# Adapted from the focused memory-bounds and MemHeader regression runners.
# No native SDK, CPU backend or application binary is needed.
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - this developer test runs a selected compiler
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


def prepare(source, output):
    assert not output.exists(), 'Use a fresh build directory'
    output.mkdir(parents=True)
    def path(name):
        return source/'rom/m68kemu'/name
    header=path('m68kemu_intern.h');memory=path('m68kemu_memory.c')
    offsets=path('m68kemu_offsets.h');shadow=path('m68kemu_shadow.c')
    native=source/'compiler/include/exec/memory.h'
    constants=[]
    names=('M68KEMU_MEM_SIZE','M68KEMU_HEAP_BASE','M68KEMU_STACK_SIZE',
           'M68KEMU_ROM_BASE','M68KEMU_ROM_SIZE','M68KEMU_ROM_END',
           'RGN_CHIP','RGN_FAST','M68KEMU_CHIP_END','M68KEMU_CIA_BASE','M68KEMU_CIA_END',
           'M68KEMU_CUSTOM_BASE','M68KEMU_CUSTOM_END','FB_NEXT','FB_SIZE',
           'M68KEMU_HEAP_HDR','M68KEMU_HEAP_MIN_SPLIT','M68K_MH_FIRST','M68K_MH_FREE',
           'MEMB_CHIP','MEMF_CHIP','MEMB_FAST','MEMF_FAST','MEMB_CLEAR','MEMF_CLEAR',
           'MEMB_31BIT','MEMF_31BIT','MEMB_LARGEST','MEMF_LARGEST','MEMB_TOTAL','MEMF_TOTAL')
    # The official exec header retains its Latin-1 copyright byte. Keep its
    # raw source/pin unchanged; only ASCII macro lines enter the fixture.
    text='\n'.join((header.read_text(),offsets.read_text(),native.read_text(encoding='latin1')))
    for name in names:
        found=re.findall(r'^#define\s+'+re.escape(name)+r'\s+([^\n]+)',text,re.M)
        assert len(found)==1,(name,found)
        constants.append('#define '+name+' '+found[0])
    sized=bool(re.search(r'\bM68KEmu_CreateContextWithStack\s*\(',memory.read_text()))
    constants.append('#define HAVE_SIZED_CONTEXT '+str(int(sized)))
    (output/'constants.inc').write_text('\n'.join(constants)+'\n')
    accessors=['m68k_read16','m68k_read32','m68k_write_allowed','m68k_write16','m68k_write32']
    (output/'accessors.inc').write_text('\n'.join(extract_function(header,n) for n in accessors))
    creators=(['M68KEmu_CreateContextWithStack'] if sized else [])+['M68KEmu_CreateContext']
    heap=['heap_publish','M68KEmu_HeapRegion','heap_largest','M68KEmu_HeapAvail',
          'M68KEmu_HeapAllocAbs','heap_alloc_in','M68KEmu_HeapAlloc','M68KEmu_HeapFree']
    (output/'production.inc').write_text('\n'.join(extract_function(memory,n) for n in creators+heap)+
                                        '\n'+extract_function(shadow,'m68k_task_stack'))
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    record=dict(source_inputs={str(p):sha(p) for p in (header,memory,offsets,shadow,native)},
                actual_constructor_functions=creators,actual_heap_functions=heap,
                actual_shadow_consumer='m68k_task_stack',sized=sized,
                generated={p.name:sha(p) for p in output.glob('*.inc')},
                scope='Actual production function bodies; controlled native-operation/field fixture, no native SDK/ABI or launch-TU compilation')
    (output/'source-record.local.json').write_text(json.dumps(record,indent=2)+'\n')


def run(args, output):
    prepare(args.source_root.resolve(), output)
    if args.prepare_only:
        return 0
    command = shlex.split(args.cc) + [
        '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-O1', '-g', '-I' + str(output),
        str(Path(__file__).with_name('test_requested_stack.c')),
        '-o', str(output / 'test_requested_stack'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-fno-omit-frame-pointer']
    # Argument vectors avoid a shell; the developer selects the compiler.
    subprocess.run(command, check=True)  # nosec B603
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # Preserve the fixture's failure code; compiler failures raise above.
    return subprocess.run([str(output / 'test_requested_stack')],
                          env=env).returncode  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path,
                        default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    if args.prepare_only and args.build_dir is None:
        parser.error('--prepare-only requires --build-dir')
    if args.build_dir:
        return run(args, args.build_dir.resolve())
    with tempfile.TemporaryDirectory(prefix='m68kemu-requested-stack-') as name:
        return run(args, Path(name) / 'case')


if __name__ == '__main__':
    raise SystemExit(main())
