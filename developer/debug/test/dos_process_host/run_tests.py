#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""
Run the production DOS process entry and cleanup with controlled collaborators.
"""
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


def extract(path, name):
    text = path.read_text(encoding='latin1')
    code = masked(text)
    pattern = re.compile(r'(?m)^[ \t]*static\s+[^;{}]*?\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{')
    matches = [m for m in pattern.finditer(code)
               if code[:m.start()].count('{') == code[:m.start()].count('}')]
    if len(matches) != 1:
        raise RuntimeError(f'{path}: expected one {name} body, got {len(matches)}')
    start = matches[0].start()
    opening = code.find('{', start)
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
    source = root / 'rom/dos/createnewproc.c'
    (build / 'production_dosentry.inc').write_text(
        extract(source, 'streamSane') + extract(source, 'DosEntry'))
    names = {'RETURN_FAIL', 'ERROR_INVALID_RESIDENT_LIBRARY', 'DOS_CLI', 'GSLI_Dummy', 'GSLI_68KHUNK'}
    constants = []
    for path in sorted((root / 'compiler/include/dos').glob('*.h')):
        for line in path.read_text(encoding='latin1').splitlines():
            match = re.match(r'#define\s+(\w+)\b', line)
            if match and (match[1] in names or match[1].startswith(('PRB_', 'PRF_'))):
                constants.append(line)
    (build / 'production_constants.h').write_text('\n'.join(constants) + '\n')
    command = shlex.split(args.cc) + ['-std=c99', '-Wall', '-Wextra', '-Werror',
        '-Wno-unused-parameter', '-g', '-O1', '-I' + str(build)]
    if args.sanitize:
        command += ['-fno-omit-frame-pointer', '-fsanitize=address,undefined']
    executable = build / 'test_dosentry'
    command += [str(Path(__file__).with_name('test_dosentry.c')), '-o', str(executable)]
    print('+', shlex.join(command), flush=True)
    subprocess.run(command, check=True)  # nosec B603 - explicit argument list
    env = os.environ.copy()
    if args.sanitize:
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0' if sys.platform == 'darwin' else 'detect_leaks=1')
        env.setdefault('UBSAN_OPTIONS', 'halt_on_error=1')
    print('+', str(executable), flush=True)
    subprocess.run([str(executable)], check=True, env=env)  # nosec B603 - explicit argument list


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
