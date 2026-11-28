#!/usr/bin/env python3
"""Compile verbatim CreateMsgPort/DeleteMsgPort shadow lifecycle thunks with a recording C++ fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
#
# Extracts shadow and MsgPort ownership helpers from m68kemu_thunks.c and
# m68kemu_shadow.c, the generated MsgPort layout and Task shadow type from
# m68kemu_shadow_layouts.h, M68K_PR_MSGPORT from m68kemu_offsets.h, and the
# generated CreateMsgPort/DeleteMsgPort/FindPort thunks from
# m68kemu_thunks_gen.c, while checking byte-for-byte parity with
# m68kemu_thunkgen.py. No native SDK or CPU core is needed.
import argparse
import importlib.util
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - developer host test invokes selected compiler
import sys
import tempfile


def extract_function(source, name, filename):
    """Return the top-level definition of a function (column 0, brace depth 0) with #line."""
    masked = re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]),
        source,
    )
    pattern = re.compile(
        r'(?m)^(?=[A-Za-z_])[^\n;{}()=]*\b' + re.escape(name) +
        r'\s*\([^;{}]*\)[^;{}]*\{'
    )
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
                raise ValueError(f'unterminated body for {name} in {filename}')
            line = source[:match.start()].count('\n') + 1
            return f'#line {line} "{filename}"\n' + source[match.start():end] + '\n'
    raise ValueError(f'definition of {name} not found in {filename}')


def extract_regex(source, pattern, desc, filename):
    match = re.search(pattern, source)
    if not match:
        raise ValueError(f'{desc} not found in {filename}')
    line = source[:match.start()].count('\n') + 1
    return f'#line {line} "{filename}"\n' + match.group(0) + '\n'


def check_generator_parity(root, gen_src):
    """Verify m68kemu_thunkgen.py emits the exact CreateMsgPort and DeleteMsgPort in m68kemu_thunks_gen.c."""
    tg_path = root / 'rom/m68kemu/m68kemu_thunkgen.py'
    spec = importlib.util.spec_from_file_location('m68kemu_thunkgen_test', tg_path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

    protos = {
        'CreateMsgPort': ('struct MsgPort *', []),
        'DeleteMsgPort': ('VOID', [('struct MsgPort *', 'port')]),
    }

    checks = [
        ('CreateMsgPort', 666, [], [], r'(?ms)^/\* -666: CreateMsgPort\(\) \*/\nstatic IPTR thunk_exec_CreateMsgPort\b.*?\n\}\n'),
        ('DeleteMsgPort', 672, ['port'], ['a0'], r'(?ms)^/\* -672: DeleteMsgPort\(port=a0\) \*/\nstatic IPTR thunk_exec_DeleteMsgPort\b.*?\n\}\n'),
    ]
    for fn, bias, args, regs, pat in checks:
        emitted = mod.gen_thunk('exec', fn, bias, args, regs, protos)
        m = re.search(pat, gen_src)
        if not m:
            raise ValueError(f'thunk_exec_{fn} block not found in m68kemu_thunks_gen.c')
        checked_in = m.group(0)
        if emitted != checked_in:
            raise AssertionError(
                f'm68kemu_thunkgen.py output for {fn} does not match m68kemu_thunks_gen.c\n'
                f'--- emitted ---\n{emitted}\n--- checked_in ---\n{checked_in}'
            )

    has_create = hasattr(mod, 'SHADOW_CREATES') and mod.SHADOW_CREATES.get('CreateMsgPort') == ('MsgPort', 'm68k_msgport_own')
    has_destroy = hasattr(mod, 'SHADOW_DESTROYS') and mod.SHADOW_DESTROYS.get('DeleteMsgPort') == ('MsgPort', 'm68k_msgport_destroy')
    return has_create, has_destroy


def run(args, output):
    root = args.source_root
    shadow_src = (root / 'rom/m68kemu/m68kemu_shadow.c').read_text(encoding='latin-1')
    layouts_src = (root / 'rom/m68kemu/m68kemu_shadow_layouts.h').read_text(encoding='latin-1')
    thunks_src = (root / 'rom/m68kemu/m68kemu_thunks.c').read_text(encoding='latin-1')
    gen_src = (root / 'rom/m68kemu/m68kemu_thunks_gen.c').read_text(encoding='latin-1')
    intern_src = (root / 'rom/m68kemu/m68kemu_intern.h').read_text(encoding='latin-1')
    offsets_src = (root / 'rom/m68kemu/m68kemu_offsets.h').read_text(encoding='latin-1')

    gen_has_create, gen_has_destroy = check_generator_parity(root, gen_src)

    m_max_sh = re.search(r'^\s*#define\s+M68KEMU_MAX_SHADOWS\s+(\d+)', intern_src, re.M)
    if not m_max_sh:
        raise ValueError('M68KEMU_MAX_SHADOWS not found in m68kemu_intern.h')
    m_max_op = re.search(r'^\s*#define\s+M68KEMU_MAX_OWNED_PORTS\s+(\w+)', intern_src, re.M)
    if not m_max_op:
        raise ValueError('M68KEMU_MAX_OWNED_PORTS not found in m68kemu_intern.h')
    m_pr_mp = re.search(r'^\s*#define\s+M68K_PR_MSGPORT\s+(\d+)', offsets_src, re.M)
    if not m_pr_mp:
        raise ValueError('M68K_PR_MSGPORT not found in m68kemu_offsets.h')

    shadow_field_macros = extract_regex(
        layouts_src,
        r'(?ms)^#define SHADOW_FIELD_TYPE\(.*?^#define SHADOW_FIELD\(.*?0, 0, NULL \}',
        'SHADOW_FIELD macros',
        'm68kemu_shadow_layouts.h',
    )
    fields_msgport = extract_regex(
        layouts_src,
        r'(?ms)^static const struct M68KFieldMap shadow_fields_MsgPort\[\] = \{.*?\};',
        'shadow_fields_MsgPort',
        'm68kemu_shadow_layouts.h',
    )
    row_msgport = extract_regex(
        layouts_src,
        r'(?m)^\s*\{\s*"MsgPort",\s*\d+,\s*\d+,\s*shadow_fields_MsgPort\s*\},',
        'MsgPort layout row',
        'm68kemu_shadow_layouts.h',
    )
    m_task_row = re.search(
        r'(?m)^\s*\{\s*"Task",\s*(\d+),\s*(\d+),\s*shadow_fields_Task\s*\},',
        layouts_src,
    )
    if not m_task_row:
        raise ValueError('Task layout row not found in m68kemu_shadow_layouts.h')
    task_size, task_type = m_task_row.group(1), m_task_row.group(2)

    consts = [
        f'#define M68KEMU_MAX_SHADOWS {m_max_sh.group(1)}\n',
        f'#define M68KEMU_MAX_OWNED_PORTS {m_max_op.group(1)}\n',
        f'#define M68K_PR_MSGPORT {m_pr_mp.group(1)}\n',
        f'#define THUNKGEN_HAS_CREATEMSGPORT_OWN {1 if gen_has_create else 0}\n',
        f'#define THUNKGEN_HAS_DELETEMSGPORT_DESTROY {1 if gen_has_destroy else 0}\n',
    ]

    extracted = [
        shadow_field_macros,
        fields_msgport,
        'static const struct M68KStructLayout shadow_gen_layouts[] = {\n',
        row_msgport,
        f'    {{ "Task", {task_size}, {task_type}, NULL }},\n',
        '    { NULL, 0, 0, NULL }\n};\n',
        extract_function(thunks_src, 'shadow_register', 'm68kemu_thunks.c'),
        extract_function(thunks_src, 'shadow_lookup', 'm68kemu_thunks.c'),
        extract_function(thunks_src, 'shadow_remove', 'm68kemu_thunks.c'),
        extract_function(shadow_src, 'sync_fields', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'shadow_find_layout', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'shadow_create', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'shadow_destroy', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'msgport_is_embedded_proc_port', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'msgport_invalidate_aliases', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'm68k_msgport_own', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'm68k_msgport_destroy', 'm68kemu_shadow.c'),
        extract_function(shadow_src, 'shadow_create_by_name', 'm68kemu_shadow.c'),
        extract_function(gen_src, 'thunk_exec_FindPort', 'm68kemu_thunks_gen.c'),
        extract_function(gen_src, 'thunk_exec_CreateMsgPort', 'm68kemu_thunks_gen.c'),
        extract_function(gen_src, 'thunk_exec_DeleteMsgPort', 'm68kemu_thunks_gen.c'),
    ]

    output.mkdir(parents=True, exist_ok=True)
    (output / 'delport_consts.inc').write_text(''.join(consts), encoding='latin-1')
    (output / 'delport_extracted.inc').write_text('\n'.join(extracted), encoding='latin-1')

    if args.prepare_only:
        return

    command = shlex.split(args.cxx) + [
        '-std=c++17', '-Wall', '-Wextra', '-Werror',
        '-Wno-unused-function', '-Wno-unused-parameter',
        '-g', '-O1', '-I' + str(output),
        str(Path(__file__).with_name('test_deletemsgport_shadow.cpp')),
        '-o', str(output / 'test_deletemsgport_shadow'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    subprocess.run(command, check=True)  # nosec B603 - explicit compiler argument list

    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_deletemsgport_shadow')], check=True, env=env)  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--prepare-only', action='store_true', help='retain the includes; needs --build-dir')
    args = parser.parse_args()
    if args.prepare_only and not args.build_dir:
        parser.error('--prepare-only requires --build-dir')
    if args.build_dir:
        run(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-delport-shadow-') as path:
            run(args, Path(path))
    return 0


if __name__ == '__main__':
    sys.exit(main())
