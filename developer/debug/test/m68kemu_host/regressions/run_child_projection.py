#!/usr/bin/env python3
"""Compile the actual 68k child creation, child entry and seglist copy with a recording C fixture."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# Adapted from the focused guest argument API regression runner.
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


def extract_struct(path, name):
    source = path.read_text()
    found = re.findall(r'(?m)^struct ' + re.escape(name) + r'\n\{\n[^{}]*\};\n', source)
    if len(found) != 1:
        raise ValueError(f'{path}: one definition of struct {name} expected')
    line = source[:source.index(found[0])].count('\n') + 1
    return f'#line {line} "{path.name}"\n' + found[0]


def prepare(source, output):
    assert not output.exists(), 'Use a fresh build directory'
    output.mkdir(parents=True)

    def path(name):
        return source / 'rom/m68kemu' / name
    header = path('m68kemu_intern.h')
    offsets = path('m68kemu_offsets.h')
    thunks = path('m68kemu_thunks.c')
    tasks = path('m68kemu_tasks.c')
    tags = path('m68kemu_tags.c')
    memory = path('m68kemu_memory.c')
    native = [source / 'compiler/include' / name for name in (
        'dos/dostags.h', 'utility/tagitem.h', 'exec/nodes.h', 'exec/memory.h')]
    doshunks = source / 'compiler/include/dos/doshunks.h'
    # The official headers retain Latin-1 bytes. Keep their raw source
    # unchanged; only ASCII macro lines enter the fixture.
    text = '\n'.join(f.read_text(encoding='latin1')
                     for f in (header, offsets, *native))
    names = ('M68KEMU_MAX_SHADOWS', 'M68KEMU_HANDLE_BASE',
             'M68K_SIZEOF_PROCESS', 'M68K_SIZEOF_CLI', 'M68K_PR_CLI',
             'M68K_CLI_DEFAULTSTACK', 'M68K_CLI_MODULE', 'M68K_CLI_COMMANDNAME',
             'M68K_PR_ARGUMENTS', 'M68K_PR_MSGPORT',
             'TAG_DONE', 'TAG_IGNORE', 'TAG_MORE', 'TAG_SKIP', 'TAG_USER',
             'NP_Dummy', 'NP_Seglist', 'NP_FreeSeglist', 'NP_Entry', 'NP_Input',
             'NP_Output', 'NP_CloseInput', 'NP_CloseOutput', 'NP_Error',
             'NP_CloseError', 'NP_CurrentDir', 'NP_StackSize', 'NP_Name',
             'NP_Priority', 'NP_HomeDir', 'NP_Cli', 'NP_ExitCode', 'NP_ExitData',
             'NP_UserData', 'GSLI_Dummy', 'GSLI_68KHUNK', 'GSLI_68KRELOCS', 'NT_PROCESS',
             'MEMF_ANY', 'MEMB_CLEAR', 'MEMF_CLEAR')
    constants = []
    for name in names:
        found = re.findall(r'^[ \t]*#define\s+' + re.escape(name) +
                           r'\s+([^\n]+)', text, re.M)
        assert len(found) == 1, (name, found)
        # A comment that starts on the line may end on a later one.
        constants.append('#define ' + name + ' ' + found[0].split('/*')[0].rstrip())
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    (output / 'accessors.inc').write_text('\n'.join(
        extract_function(header, name) for name in (
            'm68k_to_host', 'm68k_in_mem', 'm68k_read32',
            'm68k_write_allowed', 'm68k_write32')))
    # The Process lookup, the registry it searches and the forgetting of an
    # ended process at the same native address are the actual ones.
    registry = ['shadow_register', 'shadow_lookup', 'shadow_remove']
    (output / 'registry.inc').write_text('\n'.join(
        extract_function(thunks, name) for name in registry))
    (output / 'process.inc').write_text(extract_function(thunks, 'm68k_process_of'))
    lifetime = ['m68k_forget_entry', 'm68k_process_forget']
    (output / 'lifetime.inc').write_text('\n'.join(
        extract_function(tasks, name) for name in lifetime))
    # The program's tag list is read by the actual tag walker.
    walker = ['item_ok', 'next_item', 'm68k_next_tag']
    (output / 'tags.inc').write_text('\n'.join(
        extract_function(tags, name) for name in walker))
    # The record of a child, what its native process runs, its creation, and
    # the two dos entries that make one, each with its own default for
    # NP_FreeSeglist.
    children = ['child_entry', 'proc_name', 'create_proc', 'm68k_dos_CreateNewProc', 'm68k_dos_CreateProc']
    (output / 'children.inc').write_text(
        extract_struct(tasks, 'M68KChild') + '\n' + '\n'.join(
            extract_function(tasks, name) for name in children))
    # A child whose code is a seglist that dos loaded: the actual copy into
    # the program's memory, and, in a tree that gives a copy back, the walk
    # of the copy and its release. A tree from before that has neither; its
    # create_proc does not ask for them.
    seglist = {}
    for name in ('M68KEmu_ImageHunk', 'M68KEmu_LoadHunks', 'M68KEmu_FreeHunks'):
        try:
            seglist[name] = extract_function(memory, name)
        except ValueError:
            if name == 'M68KEmu_LoadHunks':
                raise
    (output / 'seglist.inc').write_text('\n'.join(seglist.values()))
    # The relocation list dos keeps for a 68k seglist, as the tree's header
    # has it: the HUNKRELOC_* kinds, struct HunkRelocGroup and struct
    # HunkRelocTable, each taken by itself. A kind the header defines more
    # than once must be defined alike. Only plain ASCII enters the fixture.
    hunks = doshunks.read_text(encoding='latin1')
    relocs = []
    for kind in ('HUNKRELOC_ABS', 'HUNKRELOC_REL', 'HUNKRELOC_BASE'):
        found = {d.split('/*')[0].rstrip() for d in re.findall(
            r'(?m)^[ \t]*#define\s+' + kind + r'\s+[^\n]+', hunks)}
        if len(found) != 1:
            raise ValueError(f'{doshunks}: one definition of {kind} expected')
        relocs.append(found.pop().strip())
    for name in ('HunkRelocGroup', 'HunkRelocTable'):
        found = re.findall(r'(?ms)^struct ' + name + r'\s*\{.*?^\};', hunks)
        if len(found) != 1:
            raise ValueError(f'{doshunks}: one struct {name} expected')
        relocs.append(found[0])
    if not all(part.isascii() for part in relocs):
        raise ValueError(f'{doshunks}: the relocation list is not plain ASCII')
    (output / 'relocs.inc').write_text('\n'.join(relocs) + '\n')

    def sha(p):
        return hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(
        source_inputs={str(p): sha(p) for p in (header, offsets, thunks, tasks, tags, memory,
                                                *native, doshunks)},
        actual_functions=children + ['m68k_process_of'] + registry + lifetime + walker + list(seglist),
        actual_structure='struct M68KChild',
        relocation_list='struct HunkRelocGroup, HUNKRELOC_*, struct HunkRelocTable',
        generated={p.name: sha(p) for p in output.glob('*.inc')},
        scope='Actual create_proc, child_entry, the CreateNewProc and CreateProc entries, Process lookup, registry, tag walker, '
              'seglist copy and release and accessor bodies; controlled native process '
              'creation, scheduling, lock, heaps, layouts, a native seglist with the '
              'relocation list dos keeps for it, and a recording stand-in for the run of '
              'guest code; no native SDK/ABI, CPU, lock, native process or LoadSeg()')
    (output / 'source-record.local.json').write_text(json.dumps(record, indent=2) + '\n')


def run(args, output):
    prepare(args.source_root.resolve(), output)
    if args.prepare_only:
        return 0
    command = shlex.split(args.cc) + [
        '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-O1', '-g', '-I' + str(output),
        str(Path(__file__).with_name('test_child_projection.c')),
        '-o', str(output / 'test_child_projection'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-fno-omit-frame-pointer']
    # Argument vectors avoid a shell; the developer selects the compiler.
    subprocess.run(command, check=True)  # nosec B603
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # Preserve the fixture's failure code; compiler failures raise above.
    return subprocess.run([str(output / 'test_child_projection')],
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
    with tempfile.TemporaryDirectory(prefix='m68kemu-child-projection-') as name:
        return run(args, Path(name) / 'case')


if __name__ == '__main__':
    raise SystemExit(main())
