#!/usr/bin/env python3
"""Compile the actual ExecBase list, nesting-count and FindName code with a recording C fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# Adapted from the focused MemHeader and guest argument regression runners.
# No native SDK, CPU backend or application binary is needed.
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - this developer test runs a selected compiler
import tempfile

LISTS = ('INTRLIST', 'PORTLIST', 'TASKREADY', 'TASKWAIT', 'SEMAPHORELIST')


def extract_function(path, name):
    source = path.read_text()
    masked = re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: ''.join('\n' if c == '\n' else ' ' for c in match[0]),
        source,
    )
    # A definition starts in the first column. A call inside a statement
    # that a brace follows, such as a condition, does not.
    match = re.search(
        r'(?m)^(?=\S)[^\n;{}]*\b' + re.escape(name) +
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
        return source / 'rom/m68kemu' / name
    header, offsets = path('m68kemu_intern.h'), path('m68kemu_offsets.h')
    shadow, thunks, tasks = path('m68kemu_shadow.c'), path('m68kemu_thunks.c'), path('m68kemu_tasks.c')
    nodes = source / 'compiler/include/exec/nodes.h'
    # The official headers retain Latin-1 bytes. Keep their raw source
    # unchanged; only ASCII macro lines enter the fixture.
    text = '\n'.join((offsets.read_text(), thunks.read_text(),
                      nodes.read_text(encoding='latin1')))
    constants = []
    wanted = ['M68K_EXECBASE_THISTASK', 'M68K_EXECBASE_ATTNFLAGS', 'M68K_EXECBASE_MEMLIST',
              'M68K_EXECBASE_RESOURCELIST', 'M68K_EXECBASE_DEVICELIST', 'M68K_EXECBASE_LIBLIST']
    wanted += ['M68K_EXECBASE_' + name for name in LISTS]
    wanted += ['NT_TASK', 'NT_INTERRUPT', 'NT_DEVICE', 'NT_MSGPORT', 'NT_LIBRARY', 'NT_SEMAPHORE']
    for name in wanted:
        found = re.findall(r'^[ \t]*#define\s+' + re.escape(name) + r'\s+(\S+)', text, re.M)
        assert len(found) == 1, (name, found)
        constants.append('#define ' + name + ' ' + found[0])
    # The fields exec keeps per CPU or for its scheduler alone. A tree that
    # still copies them from the native ExecBase names no offsets for them;
    # the fixture then uses the classic ones, and its checks fail.
    classic = {'IDLECOUNT': 280, 'DISPCOUNT': 284, 'QUANTUM': 288, 'ELAPSED': 290, 'SYSFLAGS': 292,
               'IDNESTCNT': 294, 'TDNESTCNT': 295, 'ATTNRESCHED': 298}
    for name, value in classic.items():
        found = re.findall(r'^[ \t]*#define\s+M68K_EXECBASE_' + name + r'\s+(\S+)', text, re.M)
        assert len(found) <= 1 and (not found or int(found[0]) == value), (name, found)
        constants.append('#define M68K_EXECBASE_%s %d' % (name, value))
    # The test that tells a list in the program's memory from a native one.
    native = re.findall(r'^#define ML_NATIVE\(a\)[^\n]*', text, re.M)
    assert len(native) == 1, native
    constants.append(native[0])
    shadow_text = shadow.read_text()
    private = bool(re.search(r'^static void m68k_execbase_private\(', shadow_text, re.M))
    # A tree that maps the lists registers each one as a shadow of the
    # native list when it sets up the program's ExecBase.
    init = extract_function(shadow, 'shadow_init_execbase')
    maps = bool(re.search(r'shadow_register\(ctx,\s*m68k_base\s*\+\s*execbase_lists\[', init))
    if private:
        assert not maps and 'm68k_execbase_private(ctx, m68k_base)' in init, \
            'shadow_init_execbase must leave the lists to m68k_execbase_private'
        assert not re.search(r'shadow_register\([^;]*M68K_EXECBASE_(?:%s)\b' % '|'.join(LISTS), shadow_text), \
            'a private ExecBase list is registered as a shadow'
        (output / 'private.inc').write_text('\n'.join(
            extract_function(shadow, name) for name in (
                'm68k_nest_counts_at', 'm68k_nest_counts', 'm68k_execbase_private')))
    else:
        assert maps, 'neither m68k_execbase_private nor the mapping of the lists was found'
        (output / 'private.inc').write_text('')
    constants.append('#define HAVE_EXECBASE_PRIVATE ' + ('1' if private else '0'))
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    (output / 'accessors.inc').write_text('\n'.join(
        extract_function(header, name) for name in (
            'm68k_to_host', 'm68k_read32', 'm68k_write_allowed', 'm68k_write16', 'm68k_write32')))
    (output / 'thunks.inc').write_text('\n'.join(
        extract_function(thunks, name) for name in (
            'thunk_exec_Forbid', 'thunk_exec_Disable_m', 'thunk_exec_Enable_m', 'thunk_exec_Permit',
            'thunk_exec_FindName')))
    (output / 'tasks.inc').write_text(extract_function(tasks, 'm68k_gil_obtain'))


def run(args, output):
    prepare(args.source_root, output)
    if args.prepare_only:
        return
    command = shlex.split(args.cc) + [
        '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-O1', '-g',
        '-I' + str(output), str(Path(__file__).with_name('test_execbase_private.c')),
        '-o', str(output / 'test_execbase_private')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    subprocess.run(command, check=True)  # nosec B603 - explicit compiler argument list
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_execbase_private')], check=True, env=env)  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--prepare-only', action='store_true',
                        help='retain includes for a native C compiler; needs --build-dir')
    args = parser.parse_args()
    if args.prepare_only and not args.build_dir:
        parser.error('--prepare-only requires --build-dir')
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-execbase-private-') as path:
            run(args, Path(path) / 'build')


if __name__ == '__main__':
    main()
