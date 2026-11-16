#!/usr/bin/env python3
"""Compile the actual Process stream functions with a recording C fixture."""
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
    header = path('m68kemu_intern.h')
    offsets = path('m68kemu_offsets.h')
    shadow = path('m68kemu_shadow.c')
    launch = path('m68kemu_run.c')
    thunks = path('m68kemu_thunks.c')
    runtime = path('m68kemu_runtime.c')
    layouts = path('m68kemu_shadow_layouts.h')
    tasks = path('m68kemu_tasks.c')
    generated = path('m68kemu_thunks_gen.c')
    bridge = path('m68kemu_moira.cpp')
    native = [source / 'compiler/include' / name for name in (
        'dos/dostags.h', 'utility/tagitem.h', 'exec/nodes.h', 'exec/memory.h')]
    # The official headers retain Latin-1 bytes. Keep their raw source
    # unchanged; only ASCII macro lines enter the fixture.
    text = '\n'.join(f.read_text(encoding='latin1')
                     for f in (header, offsets, *native))
    names = ('M68KEMU_MAX_SHADOWS',
             'M68KEMU_MAX_HANDLES', 'M68KEMU_HANDLE_BASE', 'M68KEMU_MAX_LVO',
             'M68K_SIZEOF_PROCESS', 'M68K_SIZEOF_CLI', 'M68K_PR_CLI',
             'M68K_CLI_DEFAULTSTACK', 'M68K_CLI_MODULE', 'M68K_CLI_COMMANDNAME',
             'M68K_PR_WINDOWPTR', 'M68K_PR_ARGUMENTS', 'M68K_PR_CIS', 'M68K_PR_COS',
             'M68K_PR_MSGPORT', 'M68K_EXECBASE_THISTASK',
             'NP_Dummy', 'NP_Entry', 'NP_Name', 'NP_Synchronous', 'NP_UserData',
             'NP_FreeSeglist', 'NP_Input', 'NP_Arguments', 'NP_CloseInput', 'NP_Output',
             'NP_CloseOutput', 'NP_Error', 'NP_CloseError', 'NP_CurrentDir', 'NP_HomeDir',
             'NP_Cli', 'TAG_USER', 'TAG_DONE', 'TAG_IGNORE', 'NT_PROCESS',
             'MEMB_CLEAR', 'MEMF_CLEAR')
    constants = []
    for name in names:
        found = re.findall(r'^[ \t]*#define\s+' + re.escape(name) +
                           r'\s+([^\n]+)', text, re.M)
        assert len(found) == 1, (name, found)
        constants.append('#define ' + name + ' ' + found[0])
    # The independent generated layout places pr_CES; it is not derived
    # from the helper under test.
    assert re.search(r'\{\s*224,\s*712,.*?/\* pr_CES \*/', layouts.read_text())
    ces = re.findall(r'^#define\s+M68K_PR_CES\s+(\d+)', offsets.read_text(), re.M)
    assert not ces or ces == ['224'], ces
    constants.append('#define M68K_PR_CES 224')
    # Input (LVO -54) and Output (-60) are manual thunks. SelectInput (-294)
    # and SelectOutput (-300): the manual thunks when the tree registers
    # them, else the generated ones that the generated table registers.
    table = thunks.read_text().split(
        'const struct M68KThunkEntry m68kemu_thunks_dos[] = {', 1)[1].split('};', 1)[0]
    registered = {lvo: re.findall(r'\{\s*' + str(lvo) + r',\s*(\w+)\s*\}', table)
                  for lvo in (54, 60, 294, 300)}
    assert registered[54] == ['thunk_dos_Input'], registered
    assert registered[60] == ['thunk_dos_Output'], registered
    has_selectors = bool(registered[294] and registered[300])
    # Both manual selectors are registered together, or neither is.
    assert has_selectors or not (registered[294] or registered[300]), registered
    if has_selectors:
        assert registered[294] == ['thunk_dos_SelectInput_m'], registered
        assert registered[300] == ['thunk_dos_SelectOutput_m'], registered
        selector_source = thunks
        selectors = ['thunk_dos_SelectInput_m', 'thunk_dos_SelectOutput_m']
    else:
        selector_source = generated
        selectors = ['thunk_dos_SelectInput', 'thunk_dos_SelectOutput']
        generated_text = generated.read_text()
        assert re.search(r'\{\s*294,\s*thunk_dos_SelectInput\s*\}', generated_text)
        assert re.search(r'\{\s*300,\s*thunk_dos_SelectOutput\s*\}', generated_text)
    # A manual thunk is found and returns before the generated table is searched.
    dispatch = bridge.read_text()
    first = dispatch.index('for (ULONG j = 0; j < lib->num_thunks; j++)')
    second = dispatch.index('for (ULONG j = 0; j < lib->num_gen_thunks; j++)')
    assert first < second and 'return;' in dispatch[first:second]
    (output / 'selectors.inc').write_text('\n'.join(
        extract_function(selector_source, name) for name in selectors))
    consumers = ['thunk_dos_Input', 'thunk_dos_Output']
    (output / 'native-consumers.inc').write_text('\n'.join(
        extract_function(thunks, name) for name in consumers))
    # A new process at an ended one's native address: the actual handling of
    # create_proc, when the tree has it.
    lifetime = ['m68k_forget_entry', 'm68k_process_forget']
    has_forget = bool(re.search(r'^static void m68k_process_forget\(',
                                tasks.read_text(), re.M))
    (output / 'lifetime.inc').write_text(
        '\n'.join(extract_function(tasks, name) for name in lifetime)
        if has_forget else '')
    constants += ['#define API_SELECT_IN ' + selectors[0],
                  '#define API_SELECT_OUT ' + selectors[1],
                  '#define HAVE_PROCESS_FORGET ' + ('1' if has_forget else '0')]
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    registry = ['shadow_register', 'shadow_lookup', 'shadow_remove']
    (output / 'registry.inc').write_text('\n'.join(
        extract_function(thunks, name) for name in registry))
    shadow_source = shadow.read_text()
    has_helper = bool(re.search(r'^void m68k_process_streams\(', shadow_source, re.M))
    has_selected = bool(re.search(r'^void m68k_process_stream_selected\(', shadow_source, re.M))
    handles = ['m68k_bptr_out', 'm68k_bptr_in', 'm68k_bptr_drop']
    body = '\n'.join(extract_function(runtime, name) for name in handles)
    if has_helper:
        body += '\n' + extract_function(shadow, 'm68k_process_streams')
    if has_selected:
        body += '\n' + extract_function(shadow, 'm68k_process_stream_selected')
    (output / 'handles.inc').write_text(body)
    (output / 'launch.inc').write_text(extract_function(launch, 'setup_and_execute'))
    (output / 'process.inc').write_text(extract_function(thunks, 'm68k_process_of'))
    shadow_text = shadow.read_text()
    start = shadow_text.index('    /* Override ThisTask')
    end = shadow_text.index('    /* Library node fields */', start)
    # The complete original compound block is kept verbatim in a test wrapper.
    (output / 'initial.inc').write_text(
        'static void initial_process_projection(struct M68KEmuContext *ctx, ULONG m68k_base)\n{\n' +
        shadow_text[start:end] + '}\n')

    def sha(p):
        return hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(
        source_inputs={str(p): sha(p) for p in (header, offsets, shadow, launch, thunks,
                                                 runtime, layouts, tasks, generated,
                                                 bridge, *native)},
        has_stream_helper=has_helper,
        has_selector_helper=has_selected,
        manual_selectors_registered=has_selectors,
        manual_registration={str(lvo): found for lvo, found in registered.items()},
        manual_dispatch_precedes_generated=True,
        process_forget_selected=has_forget,
        actual_functions=handles + ['setup_and_execute', 'm68k_process_of'] + registry +
                         (['m68k_process_streams'] if has_helper else []) +
                         (['m68k_process_stream_selected'] if has_selected else []) +
                         selectors + consumers + (lifetime if has_forget else []),
        actual_initializer='Complete Override ThisTask compound block of shadow_init_execbase',
        generated={p.name: sha(p) for p in output.glob('*.inc')},
        scope='Actual source bodies and compound block with controlled native task, '
              'stream selector, layout, allocation and tag collaborators; no full '
              'SDK ABI, CPU, locks or native child execution')
    (output / 'source-record.local.json').write_text(json.dumps(record, indent=2) + '\n')


def run(args, output):
    prepare(args.source_root.resolve(), output)
    if args.prepare_only:
        return 0
    command = shlex.split(args.cc) + [
        '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-O1', '-g', '-I' + str(output),
        str(Path(__file__).with_name('test_process_streams.c')),
        '-o', str(output / 'test_process_streams'),
    ]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-fno-omit-frame-pointer']
    # Argument vectors avoid a shell; the developer selects the compiler.
    subprocess.run(command, check=True)  # nosec B603
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # Preserve the fixture's failure code; compiler failures raise above.
    return subprocess.run([str(output / 'test_process_streams')],
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
    with tempfile.TemporaryDirectory(prefix='m68kemu-process-streams-') as name:
        return run(args, Path(name) / 'case')


if __name__ == '__main__':
    raise SystemExit(main())
