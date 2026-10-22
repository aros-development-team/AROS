#!/usr/bin/env python3
# Copyright (C) 2026, John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL) Version 1.1.
"""Compile the actual resolver conversion with LP64 AROS mocks and sanitizers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True, help='new artifact directory')
    parser.add_argument('--cc', default=os.environ.get('CC', 'clang'))
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    sources = [here.parent / 'resolver.c', here.parent / 'resolver.h',
               here.parent / 'resolver_native.h', here / 'README.md',
               here / 'resolver_host.c', here / 'alloc_hooks.h',
               here / 'include/resolver_native.h', Path(__file__).resolve()]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
    snapshots = output / 'sources'
    snapshots.mkdir()
    for p in sources:
        shutil.copy2(p, snapshots / ('mock_' + p.name if p.parent.name == 'include' else p.name))
    common = [args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-g', '-O1',
              '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-pthread',
              '-I' + str(here / 'include'), '-I' + str(here.parent)]
    commands = [
        [args.cc, '--version'],
        common + ['-include', str(here / 'alloc_hooks.h'), '-Dmalloc=test_malloc',
                  '-Dcalloc=test_calloc', '-Dfree=test_free', '-c', str(here.parent / 'resolver.c'),
                  '-o', str(output / 'resolver.o')],
        common + [str(here / 'resolver_host.c'), str(output / 'resolver.o'), '-o', str(output / 'resolver-host')],
        [str(output / 'resolver-host')],
    ]
    records = []
    verdict = 'FAIL'
    try:
        for index, command in enumerate(commands):
            environment = dict(os.environ, ASAN_OPTIONS=('detect_leaks=0:abort_on_error=1' if sys.platform == 'darwin'
                                                          else 'detect_leaks=1:abort_on_error=1'),
                               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, env=environment, timeout=90)
            log = output / f'{index:02d}.log'
            log.write_text(shlex.join(command) + '\n' + result.stdout)
            records.append({'command': command, 'exit_code': result.returncode, 'log': log.name})
            if result.returncode:
                print(result.stdout, file=sys.stderr)
                break
        else:
            verdict = 'PASS'
            print(result.stdout.strip())
    except (OSError, subprocess.TimeoutExpired) as error:
        records.append({'error': str(error)})
    finally:
        after = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
        if after != hashes:
            verdict = 'FAIL'
            records.append({'error': 'sources changed during test'})
        (output / 'result.json').write_text(json.dumps({
            'verdict': verdict, 'scope': 'host conversion fixture only; no native execution',
            'source_sha256': hashes, 'sources_unchanged': after == hashes, 'commands': records,
        }, indent=2) + '\n')
    print(output / 'result.json')
    return 0 if verdict == 'PASS' else 1


if __name__ == '__main__':
    sys.exit(main())
