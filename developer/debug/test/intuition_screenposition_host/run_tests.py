#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Run the production Intuition ScreenPosition and MoveScreen bodies with controlled collaborators."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess  # nosec B404 - this developer test runs a selected compiler
import sys


def masked(text):
    result = list(text)
    token = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
    for match in token.finditer(text):
        for i in range(match.start(), match.end()):
            if result[i] != '\n':
                result[i] = ' '
    return ''.join(result)


def extract_lh(path, name):
    text = path.read_text()
    code = masked(text)
    pattern = re.compile(r'\bAROS_LH\d+\s*\(\s*void\s*,\s*' + re.escape(name) + r'\b')
    matches = list(pattern.finditer(code))
    if len(matches) != 1:
        raise RuntimeError(f'{path}: expected one {name} definition, got {len(matches)}')
    start = matches[0].start()
    opening = code.find('{', matches[0].end())
    if opening < 0:
        raise RuntimeError(f'{path}: missing opening brace for {name}')
    depth = 1
    end = opening + 1
    while end < len(code) and depth:
        depth += (code[end] == '{') - (code[end] == '}')
        end += 1
    if depth:
        raise RuntimeError(f'{path}: unterminated {name} body')
    line = text.count('\n', 0, start) + 1
    return f'#line {line} "{path}"\n' + text[start:end] + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    args = parser.parse_args()
    root, build = args.source_root.resolve(), args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    screenposition_src = root / 'rom/intuition/screenposition.c'
    movescreen_src = root / 'rom/intuition/movescreen.c'
    (build / 'production_screenposition.inc').write_text(
        extract_lh(screenposition_src, 'ScreenPosition') +
        extract_lh(movescreen_src, 'MoveScreen'))

    constants = []
    screens_h = (root / 'compiler/include/intuition/screens.h').read_text(encoding='latin1')
    for line in screens_h.splitlines():
        if re.match(r'^\s*#\s*define\s+SPOS_\w+\b', line):
            constants.append(line)
    intern_h = (root / 'rom/intuition/intuition_intern.h').read_text(encoding='latin1')
    for line in intern_h.splitlines():
        if re.match(r'^\s*#\s*define\s+SF_Draggable\b', line):
            constants.append(line)
    if len(constants) < 5:
        raise RuntimeError(f'expected SPOS_* and SF_Draggable macros, found {constants}')
    (build / 'production_constants.h').write_text('\n'.join(constants) + '\n')

    command = shlex.split(args.cc) + [
        '-std=gnu11', '-Wall', '-Wextra', '-Werror',
        '-Wno-unused-parameter', '-g', '-O1', '-I' + str(build)
    ]
    if args.sanitize:
        command += ['-fno-omit-frame-pointer', '-fsanitize=address,undefined']
    executable = build / 'test_screenposition'
    command += [str(Path(__file__).with_name('test_screenposition.c')), '-o', str(executable)]
    print('+', shlex.join(command), flush=True)
    subprocess.run(command, check=True)  # nosec B603 - explicit argument list
    env = os.environ.copy()
    if args.sanitize:
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0' if sys.platform == 'darwin' else 'detect_leaks=1')
        env.setdefault('UBSAN_OPTIONS', 'halt_on_error=1')
    print('+', str(executable), flush=True)
    subprocess.run([str(executable)], check=True, env=env)  # nosec B603 - binary built above


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
