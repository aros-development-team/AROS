#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Choose the sanitizer options for a host-side test.

The host tests build production code with ASan and UBSan. Some hosts ship
the two runtimes apart from the compiler (libasan and libubsan on Fedora).
The compiler is asked once to build and run an empty sanitized program.
When that fails the test is built without the sanitizers and says so,
unless the caller requires them.

Run this file to see what a compiler provides, or with --self-test to check
the probe against stand-in compilers.
"""
import argparse
import pathlib
import shlex
import subprocess
import sys
import tempfile

SANITIZERS = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']


def first_line(result):
    lines = (result.stderr or result.stdout).strip().splitlines()
    return lines[0] if lines else ''


def probe(compiler):
    """Return None if compiler builds a sanitized program that runs, else the reason."""
    with tempfile.TemporaryDirectory(prefix='aros-sanitizer-probe-') as directory:
        source = pathlib.Path(directory) / 'probe.c'
        binary = pathlib.Path(directory) / 'probe'
        source.write_text('int main(void) { return 0; }\n')
        try:
            build = subprocess.run(compiler + SANITIZERS + [str(source), '-o', str(binary)],
                                   stdin=subprocess.DEVNULL, capture_output=True,
                                   text=True, errors='replace')
            if build.returncode:
                return first_line(build) or f'compiler exit {build.returncode}'
            run = subprocess.run([str(binary)], stdin=subprocess.DEVNULL,
                                 capture_output=True, text=True, errors='replace')
            if run.returncode:
                return first_line(run) or f'sanitized program exit {run.returncode}'
        except OSError as error:
            return str(error)
    return None


def sanitizer_options(compiler, require=False):
    """Return SANITIZERS if compiler supports them, else an empty list."""
    reason = probe(compiler)
    if reason is None:
        return list(SANITIZERS)
    message = (f'{" ".join(compiler)} cannot build and run a program with '
               f'{SANITIZERS[0]}: {reason}')
    if require:
        raise SystemExit('Sanitizers required: ' + message)
    print('NOTICE: running without sanitizers: ' + message, flush=True)
    return []


def self_test():
    """Check each probe outcome with stand-in compilers."""
    program = 'out=; while [ $# -gt 0 ]; do [ "$1" = -o ] && out=$2; shift; done\n'
    cases = (
        # Stand-in compiler body, expected options, expected exit with require=True
        ('printf "#!/bin/sh\\nexit 0\\n" > "$out"; chmod +x "$out"', SANITIZERS, None),
        ('echo "ld: cannot find libasan.so" >&2; exit 1', [], 'Sanitizers required'),
        ('printf "#!/bin/sh\\nexit 1\\n" > "$out"; chmod +x "$out"', [], 'Sanitizers required'),
    )
    with tempfile.TemporaryDirectory(prefix='aros-sanitizer-selftest-') as directory:
        for number, (body, expected, required_error) in enumerate(cases):
            compiler = pathlib.Path(directory) / f'cc{number}'
            compiler.write_text('#!/bin/sh\n' + program + body + '\n')
            compiler.chmod(0o755)
            if sanitizer_options([str(compiler)]) != expected:
                raise SystemExit(f'Self-test case {number}: wrong options')
            try:
                sanitizer_options([str(compiler)], require=True)
                error = None
            except SystemExit as exit_:
                error = str(exit_.code)
            if (error is None) != (required_error is None) or \
                    (error is not None and not error.startswith(required_error)):
                raise SystemExit(f'Self-test case {number}: wrong result when required')
        missing = [str(pathlib.Path(directory) / 'no-such-compiler')]
        if sanitizer_options(missing) != []:
            raise SystemExit('Self-test: a missing compiler must give no options')
    print('host_sanitizers self-test passed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--require-sanitizers', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test()
        sys.exit(0)
    options = sanitizer_options(shlex.split(args.cc), args.require_sanitizers)
    print('Sanitizer options: ' + (' '.join(options) or 'none'))
