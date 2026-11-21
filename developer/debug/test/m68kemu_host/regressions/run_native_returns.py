#!/usr/bin/env python3
"""Check how m68kemu.library hands the pointers native functions return to a 68k program."""
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
#
# A native pointer does not fit a 32-bit register on every target, and on a
# target whose memory lies below 4 GB it fits but can look like an address
# of the program. Two checks:
#
# - The generated thunks: every one whose native function returns a pointer
#   (by the return type in the library's .conf) must not return the value of
#   the native call as it is: it goes through m68k_native_out(), a shadow
#   copy or a BPTR handle, or the thunk returns something else, unless the
#   function is one of those returns_arg_pointer() in m68kemu_moira.cpp
#   lists. Needs no compiler.
# - m68k_native_out() itself, with m68k_bptr_out(), m68k_bptr_in() and
#   m68k_to_host_or_shadow() taken verbatim from the sources and compiled
#   with test_native_returns.c: native pointers below and above 4 GB become
#   handles that lead back to them, pointers into the program's memory
#   become its addresses.
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - the developer selects a compiler
import sys
import tempfile

CONF = {
    'asl': 'workbench/libs/asl/asl.conf',
    'commodities': 'workbench/libs/commodities/commodities.conf',
    'cybergraphics': 'workbench/libs/cgfx/cybergraphics.conf',
    'datatypes': 'workbench/libs/datatypes/datatypes.conf',
    'diskfont': 'workbench/libs/diskfont/diskfont.conf',
    'dos': 'rom/dos/dos.conf',
    'exec': 'rom/exec/exec.conf',
    'expansion': 'rom/expansion/expansion.conf',
    'gadtools': 'workbench/libs/gadtools/gadtools.conf',
    'graphics': 'rom/graphics/graphics.conf',
    'icon': 'workbench/libs/icon/icon.conf',
    'iffparse': 'workbench/libs/iffparse/iffparse.conf',
    'intuition': 'rom/intuition/intuition.conf',
    'keymap': 'rom/keymap/keymap.conf',
    'layers': 'rom/hyperlayers/layers.conf',
    'locale': 'workbench/libs/locale/locale.conf',
    'utility': 'rom/utility/utility.conf',
}
POINTER_NAMES = ('APTR', 'CONST_APTR', 'STRPTR', 'CONST_STRPTR', 'PLANEPTR', 'DisplayInfoHandle')


def masked(source):
    """Return the source with comments and string contents blanked, lines kept."""
    return re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]),
        source)


def function(path, name):
    """Return the definition of a function, as it is in the source, with a #line."""
    source = path.read_text(encoding='latin1')
    text = masked(source)
    match = re.search(r'(?m)^[^\n;{}]*\b' + re.escape(name) + r'\s*\([^;{}]*\)[^;{}]*\{', text)
    if not match:
        raise ValueError('%s: definition of %s not found' % (path, name))
    end = text.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    line = source[:match.start()].count('\n') + 1
    return '#line %d "%s"\n%s\n' % (line, path.name, source[match.start():end])


def return_types(conf_text):
    """Return {function: return type} of a .conf function list."""
    body = re.search(r'##begin functionlist(.*?)##end functionlist', conf_text, re.S)[1]
    types = {}
    for line in body.split('\n'):
        line = re.sub(r'\)\s*#.*$', ')', line.strip())
        if not line or line.startswith(('.', '#')):
            continue
        m = re.match(r'(.*?)\b(\w+)\s*\((.*)\)\s*\(([^)]*)\)$', line)
        if m:
            types[m[2]] = m[1].strip()
    return types


def is_pointer(rtype):
    clean = re.sub(r'\b(CONST|const)\b', '', rtype).strip()
    return '*' in clean or clean in POINTER_NAMES


def arg_pointer_functions(bridge_source):
    """Return the functions returns_arg_pointer() lists, from their comments."""
    m = re.search(r'returns_arg_pointer\s*\([^)]*\)\s*\{(.*?)\n\}', bridge_source, re.S)
    if not m:
        raise ValueError('returns_arg_pointer() not found')
    return set(re.findall(r'/\*\s*(\w+)\s*\*/', m[1]))


def static_check(generated, types, arg_functions):
    """Return the generated thunks that hand a native pointer out as it is."""
    problems, checked = [], 0
    for m in re.finditer(r'static IPTR thunk_([a-z0-9]+)_(\w+)\(struct M68KEmuContext \*ctx, void \*cpu\)\n\{(.*?)\n\}',
                         generated, re.S):
        lib, name, body = m[1], m[2], m[3]
        rtype = types.get(lib, {}).get(name)
        if rtype is None or not is_pointer(rtype) or name in arg_functions:
            continue
        checked += 1
        raw = re.search(r'\breturn \(IPTR\)' + name + r'\(', body) or (
            re.search(r'\bIPTR _ret = \(IPTR\)' + name + r'\(', body) and re.search(r'\breturn _ret;', body))
        if raw:
            problems.append('thunk_%s_%s: %s returns %s and the thunk hands it out as it is' % (lib, name, name, rtype))
    return checked, problems


SELF_TEST = """
static IPTR thunk_layers_NewLayerInfo(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)NewLayerInfo();
}

static IPTR thunk_layers_DisposeLayerInfo(struct M68KEmuContext *ctx, void *cpu)
{
    DisposeLayerInfo(arg_li);
    return 0;
}

static IPTR thunk_dos_FilePart(struct M68KEmuContext *ctx, void *cpu)
{
    return (IPTR)FilePart(arg_path);
}
"""


def self_test():
    """Check the static check on two thunks that are right and one that is not."""
    types = {'layers': {'NewLayerInfo': 'struct Layer_Info *', 'DisposeLayerInfo': 'void'},
             'dos': {'FilePart': 'STRPTR'}}
    checked, problems = static_check(SELF_TEST, types, {'FilePart'})
    if checked != 1 or len(problems) != 1 or 'thunk_layers_NewLayerInfo' not in problems[0]:
        print('self-test failed: %d checked, %r' % (checked, problems))
        return False
    print('self-test: the static check finds a pointer handed out as it is')
    return True


def compiled_check(args, output):
    bridge = args.source_root / 'rom/m68kemu'
    runtime, header = bridge / 'm68kemu_runtime.c', bridge / 'm68kemu_intern.h'
    output.mkdir(parents=True, exist_ok=True)
    text = header.read_text(encoding='latin1')
    constants = []
    for name in ('M68KEMU_MAX_HANDLES', 'M68KEMU_HANDLE_BASE'):
        m = re.search(r'^\s*#define\s+' + name + r'\s+(\S+)', text, re.M)
        if not m:
            raise ValueError('%s: %s not found' % (header, name))
        constants.append('#define %s %s' % (name, m[1]))
    (output / 'constants.inc').write_text('\n'.join(constants) + '\n')
    (output / 'handles.inc').write_text(
        function(header, 'm68k_to_host') +
        function(header, 'm68k_to_host_or_shadow') +
        ''.join(function(runtime, n) for n in ('m68k_bptr_out', 'm68k_bptr_in', 'm68k_native_out')))
    if args.prepare_only:
        return
    command = shlex.split(args.cc) + ['-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                                      '-O1', '-g', '-I' + str(output),
                                      str(Path(__file__).with_name('test_native_returns.c')),
                                      '-o', str(output / 'test_native_returns')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    subprocess.run(command, check=True)  # nosec B603 - explicit compiler argument list
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(output / 'test_native_returns')], check=True, env=env)  # nosec B603


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[5])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--static-only', action='store_true', help='only the check of the generated thunks')
    parser.add_argument('--prepare-only', action='store_true', help='retain the includes; needs --build-dir')
    args = parser.parse_args()
    if args.prepare_only and not args.build_dir:
        parser.error('--prepare-only requires --build-dir')
    root = args.source_root
    if not self_test():
        return 1
    types = {lib: return_types((root / path).read_text(encoding='latin1')) for lib, path in CONF.items()}
    arg_functions = arg_pointer_functions((root / 'rom/m68kemu/m68kemu_moira.cpp').read_text(encoding='latin1'))
    generated = (root / 'rom/m68kemu/m68kemu_thunks_gen.c').read_text(encoding='latin1')
    checked, problems = static_check(generated, types, arg_functions)
    for p in problems:
        print(p)
    print('generated thunks that return a pointer: %d checked, %d hand it out as it is (%s left to the dispatcher)'
          % (checked, len(problems), ', '.join(sorted(arg_functions))))
    if problems:
        return 1
    if args.static_only:
        return 0
    if args.build_dir:
        compiled_check(args, args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='m68kemu-native-returns-') as path:
            compiled_check(args, Path(path))
    return 0


if __name__ == '__main__':
    sys.exit(main())
