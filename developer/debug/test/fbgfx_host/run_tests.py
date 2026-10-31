#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Compile actual framebuffer methods without an AROS SDK."""
# Only declarations and external collaborators are replaced. Extracted method
# bodies retain their source locations in compiler and sanitizer diagnostics.
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - this developer test invokes a selected compiler
import tempfile


def code_mask(text):
    """Mask comments and literals while retaining offsets and line endings."""
    result = list(text)
    token = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
    for match in token.finditer(text):
        for i in range(match.start(), match.end()):
            if result[i] != '\n':
                result[i] = ' '
    return ''.join(result)


def extract(path, name):
    text = path.read_text()
    masked = code_mask(text)
    pattern = re.compile(r'(?m)^[ \t]*[^\n;{}]*\b' + re.escape(name) + r'\s*\(')
    candidates = [m for m in pattern.finditer(masked)
                  if masked[:m.start()].count('{') == masked[:m.start()].count('}')]
    if len(candidates) != 1:
        raise RuntimeError(f'{path}: expected one definition of {name}, got {len(candidates)}')
    start = candidates[0].start()
    opening = masked.find('{', candidates[0].end())
    depth = 1
    end = opening + 1
    while depth and end < len(masked):
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    if depth:
        raise RuntimeError(f'{path}: unterminated body for {name}')
    line = text.count('\n', 0, start) + 1
    return f'#line {line} "{path}"\n' + text[start:end] + '\n'


def run(source, build, cxx, sanitize):
    here = Path(__file__).resolve().parent
    driver = source / 'arch/aarch64-raspi/hidd/fbgfx'
    support = driver / 'fbgfx_support.c'
    bitmap = driver / 'fbgfx_bitmapclass.c'
    display = driver / 'fbgfx_displayclass.c'
    build.mkdir(parents=True, exist_ok=True)
    pieces = []
    for name, alias in [('ClearBuffer', 'test_clear_pixels'),
                        ('fbDoRefreshArea', 'test_refresh_pixels')]:
        pieces += [f'#define {name} {alias}\n', extract(support, name), f'#undef {name}\n']
    pieces += [extract(support, name) for name in ('DACLoad', 'initFBGfxHW')]
    (build / 'production_support.inc').write_text(''.join(pieces))
    (build / 'production_bitmap.inc').write_text(''.join(
        extract(bitmap, name) for name in ('MNAME_ROOT(Dispose)', 'MNAME_ROOT(New)',
                                           'MNAME_ROOT(Set)', 'MNAME_BM(SetColors)',
                                           'MNAME_BM(UpdateRect)')))
    (build / 'production_display.inc').write_text(
        extract(display, 'FBGfxDisplay__Hidd_Display__Show'))
    command = shlex.split(cxx) + ['-std=c++17', '-Wall', '-Wextra', '-Werror',
                                '-Wno-unused-parameter', '-Wno-missing-field-initializers',
                                '-g', '-O1', '-I' + str(driver), '-I' + str(build)]
    if sanitize:
        command += ['-fno-omit-frame-pointer', '-fsanitize=address,undefined']
    env = os.environ.copy()
    if sanitize:
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        env.setdefault('UBSAN_OPTIONS', 'halt_on_error=1')
    for test in ('test_fbgfx_methods', 'test_refresh'):
        executable = build / test
        compile_command = command + [str(here / (test + '.cpp')), '-o', str(executable)]
        print('+', shlex.join(compile_command), flush=True)
        subprocess.run(compile_command, check=True)  # nosec B603 - explicit compiler argument list
        print('+', str(executable), flush=True)
        subprocess.run([str(executable)], env=env, check=True)  # nosec B603 - the test built above


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path,
                        default=Path(__file__).resolve().parents[4])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    if args.build_dir:
        run(args.source_root.resolve(), args.build_dir.resolve(), args.cxx, args.sanitize)
    else:
        with tempfile.TemporaryDirectory(prefix='aros-fbgfx-host-') as directory:
            run(args.source_root.resolve(), Path(directory), args.cxx, args.sanitize)


if __name__ == '__main__':
    main()
