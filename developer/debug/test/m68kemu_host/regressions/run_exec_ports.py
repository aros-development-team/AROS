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

THUNKS = ('thunk_exec_CopyMem', 'thunk_exec_AddPort', 'thunk_exec_RemPort',
          'thunk_exec_FindPort', 'thunk_exec_CreateIORequest_m', 'thunk_exec_DeleteIORequest_m',
          'thunk_exec_AddTask_m', 'thunk_exec_RemTask_m', 'thunk_exec_NewMinList_m')
ACCESSORS = ('m68k_to_host', 'm68k_to_host_or_shadow', 'm68k_in_mem', 'm68k_read16', 'm68k_read32',
             'm68k_write_allowed', 'm68k_guest_write_guarded', 'm68k_write16', 'm68k_write32')
ENTRIES = (('390', 'thunk_exec_FindPort'), ('630', 'thunk_exec_CopyMem'),
           ('654', 'thunk_exec_CreateIORequest_m'), ('660', 'thunk_exec_DeleteIORequest_m'),
           ('282', 'thunk_exec_AddTask_m'), ('288', 'thunk_exec_RemTask_m'),
           ('828', 'thunk_exec_NewMinList_m'), ('354', 'thunk_exec_AddPort'), ('360', 'thunk_exec_RemPort'))


def function(path, name):
    """Return the top-level definition of a function, as it is in the source, with a #line."""
    source = path.read_text(encoding='latin1')
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source)
    pattern = re.compile(r'(?m)^(?=[A-Za-z_])[^\n;{}()=]*\b' + re.escape(name) +
                         r'\s*\([^;{}]*\)[^;{}]*\{')
    depth, pos = 0, 0
    for match in pattern.finditer(masked):
        chunk = masked[pos:match.start()]
        depth += chunk.count('{') - chunk.count('}')
        pos = match.start()
        if depth == 0:
            end = masked.index('{', match.start()) + 1
            body = 1
            while body and end < len(masked):
                body += (masked[end] == '{') - (masked[end] == '}')
                end += 1
            if body:
                raise ValueError('%s: unterminated body for %s' % (path, name))
            line = source[:match.start()].count('\n') + 1
            return '#line %d "%s"\n%s\n' % (line, path.name, source[match.start():end])
    raise ValueError('%s: definition of %s not found' % (path, name))


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
    # CopyMem's low-memory rule repeats the two ranges that the CPU's write8 and
    # write16 drop. When those change, m68k_guest_write_guarded() has to follow.
    cpu_rule = 'ctx->low_guard && (addr < 8 || (addr >= 0x100 && addr < 0x1C0))'
    if (bridge / 'm68kemu_moira.cpp').read_text(encoding='latin1').count(cpu_rule) != 2:
        raise ValueError('write8/write16 no longer drop 0-7 and 0x100-0x1BF: review m68k_guest_write_guarded()')
    output.mkdir(parents=True, exist_ok=True)
    head = header.read_text(encoding='latin1')
    layouts = (bridge / 'm68kemu_shadow_layouts.h').read_text(encoding='latin1')
    constants = []
    for name in ('M68KEMU_HANDLE_BASE', 'M68KEMU_MAX_IOREQS'):
        m = re.search(r'^\s*#define\s+' + name + r'\s+(\S+)', head, re.M)
        if not m:
            raise ValueError('%s not found in %s' % (name, header.name))
        constants.append('#define %s %s' % (name, m[1]))
    for struct_name, macro in (('IORequest', 'SHADOW_TYPE_IOREQUEST'), ('MsgPort', 'SHADOW_TYPE_MSGPORT')):
        m = re.search(r'\{\s*"' + struct_name + r'",\s*\d+,\s*(\d+),', layouts)
        if not m:
            raise ValueError('shadow layout type for %s not found' % struct_name)
        constants.append('#define %s %s' % (macro, m[1]))
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


def self_test():
    """Verify function() ignores calls before definitions and rejects call-only files."""
    with tempfile.TemporaryDirectory(prefix='m68kemu-exec-ports-selftest-') as d:
        root = Path(d)

        # Negative 1: indented conditional call before the top-level definition
        p_indented = root / 'indented_call.c'
        p_indented.write_text(
            'static inline void w16(struct M68KEmuContext *ctx, ULONG a)\n'
            '{\n'
            '    if (!m68k_write_allowed(ctx, a)) { trap(ctx, a); return; }\n'
            '}\n'
            '\n'
            'static inline BOOL m68k_write_allowed(struct M68KEmuContext *ctx, ULONG addr)\n'
            '{\n'
            '    return !ctx->low_guard || addr >= 0x400;\n'
            '}\n',
            encoding='latin1',
        )
        got = function(p_indented, 'm68k_write_allowed')
        if not got.startswith('#line 6 "indented_call.c"\nstatic inline BOOL m68k_write_allowed') or 'trap(' in got:
            raise AssertionError('self_test indented_call failed: %r' % (got,))

        # Negative 2: column-0 conditional call inside a function body (brace depth > 0)
        p_depth = root / 'col0_in_body.c'
        p_depth.write_text(
            'static void caller(struct M68KEmuContext *ctx, ULONG a)\n'
            '{\n'
            'if (m68k_in_mem(ctx, a, 4)) {\n'
            '    return;\n'
            '}\n'
            '}\n'
            '\n'
            'static inline BOOL m68k_in_mem(struct M68KEmuContext *ctx, ULONG addr, ULONG len)\n'
            '{\n'
            '    return addr && addr < ctx->mem_size && len <= ctx->mem_size - addr;\n'
            '}\n',
            encoding='latin1',
        )
        got = function(p_depth, 'm68k_in_mem')
        if not got.startswith('#line 8 "col0_in_body.c"\nstatic inline BOOL m68k_in_mem') or 'if (m68k_in_mem' in got:
            raise AssertionError('self_test col0_in_body failed: %r' % (got,))

        # Negative 3: call-only file (prototype, macro, indented and column-0 in-body calls) must fail
        p_call_only = root / 'call_only.c'
        p_call_only.write_text(
            'static inline BOOL m68k_write_allowed(struct M68KEmuContext *ctx, ULONG addr);\n'
            '#define GUARD(c, a) if (!m68k_write_allowed(c, a)) { return; }\n'
            'static void caller(struct M68KEmuContext *ctx, ULONG a)\n'
            '{\n'
            '    if (!m68k_write_allowed(ctx, a)) { return; }\n'
            'if (!m68k_write_allowed(ctx, a)) { return; }\n'
            '}\n',
            encoding='latin1',
        )
        try:
            function(p_call_only, 'm68k_write_allowed')
        except ValueError as exc:
            if 'definition of m68k_write_allowed not found' not in str(exc):
                raise AssertionError('self_test call_only wrong error: %r' % (exc,))
        else:
            raise AssertionError('self_test call_only unexpectedly matched')

        # Positives: current pointer-return, prototype+def, single-line and multi-line declarations
        p_pos = root / 'declarations.c'
        p_pos.write_text(
            'static inline void *m68k_to_host_or_shadow(struct M68KEmuContext *ctx, ULONG addr);\n'
            'static inline void *m68k_to_host_or_shadow(struct M68KEmuContext *ctx, ULONG addr)\n'
            '{\n'
            '    /* comment with } brace */\n'
            '    return addr ? ctx->mem + addr : "}";\n'
            '}\n'
            'static IPTR thunk_exec_RemPort(struct M68KEmuContext *ctx, void *cpu) { return 0; }\n'
            'static IPTR thunk_exec_CopyMem(\n'
            '    struct M68KEmuContext *ctx,\n'
            '    void *cpu)\n'
            '{\n'
            '    return 0;\n'
            '}\n',
            encoding='latin1',
        )
        got = function(p_pos, 'm68k_to_host_or_shadow')
        if not got.startswith('#line 2 "declarations.c"\nstatic inline void *m68k_to_host_or_shadow') or not got.rstrip().endswith('}'):
            raise AssertionError('self_test pointer declaration failed: %r' % (got,))
        got = function(p_pos, 'thunk_exec_RemPort')
        if got != '#line 7 "declarations.c"\nstatic IPTR thunk_exec_RemPort(struct M68KEmuContext *ctx, void *cpu) { return 0; }\n':
            raise AssertionError('self_test single-line declaration failed: %r' % (got,))
        got = function(p_pos, 'thunk_exec_CopyMem')
        if not got.startswith('#line 8 "declarations.c"\nstatic IPTR thunk_exec_CopyMem(\n') or not got.rstrip().endswith('}'):
            raise AssertionError('self_test multi-line declaration failed: %r' % (got,))


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
    self_test()
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-exec-ports-') as path:
            run(args, Path(path))
    return 0


if __name__ == '__main__':
    sys.exit(main())
