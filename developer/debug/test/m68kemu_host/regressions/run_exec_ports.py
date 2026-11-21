#!/usr/bin/env python3
"""Build the exec thunks for ports, I/O requests, tasks, CopyMem and NewMinList with a C fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
#
# The thunks and the accessors they use are taken verbatim from
# rom/m68kemu; test_exec_ports.c supplies the context fields, the CPU
# registers, the heap and the native calls they reach, and records them.
# No SDK, CPU core or 68k program is needed.
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - the developer selects a compiler
import sys
import tempfile

THUNKS = ('copy_span_ok', 'thunk_exec_CopyMem', 'thunk_exec_AddPort', 'thunk_exec_RemPort',
          'thunk_exec_FindPort', 'thunk_exec_CreateIORequest_m', 'thunk_exec_DeleteIORequest_m',
          'thunk_exec_AddTask_m', 'thunk_exec_RemTask_m', 'thunk_exec_NewMinList_m')
ACCESSORS = ('m68k_to_host', 'm68k_to_host_or_shadow', 'm68k_in_mem', 'm68k_read16', 'm68k_read32',
             'm68k_write_allowed', 'm68k_write16', 'm68k_write32')
ENTRIES = (('390', 'thunk_exec_FindPort'), ('630', 'thunk_exec_CopyMem'),
           ('654', 'thunk_exec_CreateIORequest_m'), ('660', 'thunk_exec_DeleteIORequest_m'),
           ('282', 'thunk_exec_AddTask_m'), ('288', 'thunk_exec_RemTask_m'),
           ('828', 'thunk_exec_NewMinList_m'), ('354', 'thunk_exec_AddPort'), ('360', 'thunk_exec_RemPort'))


def function(path, name):
    """Return the definition of a function, as it is in the source, with a #line."""
    source = path.read_text(encoding='latin1')
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source)
    match = re.search(r'(?m)^[^\n;{}]*\b' + re.escape(name) + r'\s*\([^;{}]*\)[^;{}]*\{', masked)
    if not match:
        raise ValueError('%s: definition of %s not found' % (path, name))
    end = masked.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    line = source[:match.start()].count('\n') + 1
    return '#line %d "%s"\n%s\n' % (line, path.name, source[match.start():end])


def run(args, output):
    bridge = args.source_root / 'rom/m68kemu'
    header, thunks = bridge / 'm68kemu_intern.h', bridge / 'm68kemu_thunks.c'
    text = thunks.read_text(encoding='latin1')
    table = re.search(r'const struct M68KThunkEntry m68kemu_thunks_exec\[\]\s*=\s*\{(.*?)\n\};', text, re.S)[1]
    missing = [lvo for lvo, name in ENTRIES if not re.search(r'\{\s*%s\s*,\s*%s\s*\}' % (lvo, name), table)]
    if missing:
        raise ValueError('the exec table has no entry for -%s' % ', -'.join(missing))
    native = re.search(r'^#define ML_NATIVE\(a\).*$', text, re.M)
    if not native:
        raise ValueError('ML_NATIVE not found')
    output.mkdir(parents=True, exist_ok=True)
    head = header.read_text(encoding='latin1')
    constants = []
    for name in ('M68KEMU_HANDLE_BASE',):
        m = re.search(r'^\s*#define\s+' + name + r'\s+(\S+)', head, re.M)
        constants.append('#define %s %s' % (name, m[1]))
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    (output / 'accessors.inc').write_text(''.join(function(header, n) for n in ACCESSORS))
    (output / 'thunks.inc').write_text(native[0] + '\n' + ''.join(function(thunks, n) for n in THUNKS))
    if args.prepare_only:
        return
    command = shlex.split(args.cc) + ['-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                                      '-Wno-unused-parameter', '-O1', '-g', '-I' + str(output),
                                      str(Path(__file__).with_name('test_exec_ports.c')),
                                      '-o', str(output / 'test_exec_ports')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    subprocess.run(command, check=True)  # nosec B603 - explicit compiler argument list
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_exec_ports')], check=True, env=env)  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--prepare-only', action='store_true', help='retain the includes; needs --build-dir')
    args = parser.parse_args()
    if args.prepare_only and not args.build_dir:
        parser.error('--prepare-only requires --build-dir')
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-exec-ports-') as path:
            run(args, Path(path))
    return 0


if __name__ == '__main__':
    sys.exit(main())
