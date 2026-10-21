#!/usr/bin/env python3
# Copyright (C) 2026, John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL) Version 1.1.
"""Run actual socket bridge and AROS adapter against synchronous host mocks."""
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
    here = Path(__file__).resolve().parent
    net = here.parents[1]
    core = here.parents[6]
    default_native = core / 'workbench/network/common/include/bsdsocket/native.h'
    if not default_native.is_file():
        default_native = core.parent / 'aros-core-rust-native-handles/workbench/network/common/include/bsdsocket/native.h'
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True, help='fresh artifact directory')
    parser.add_argument('--cc', default=os.environ.get('CC', 'clang'))
    parser.add_argument('--native-header', type=Path, default=default_native,
                        help='real bsdsocket/native.h contract to compile')
    args = parser.parse_args()
    native = args.native_header.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    sources = [net / name for name in ('socket.c', 'socket_aros.c', 'socket_native.h')]
    sources += [p for p in sorted(here.rglob('*')) if p.is_file() and '__pycache__' not in p.parts]
    sources += [native]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
    snapshots = output / 'sources'
    snapshots.mkdir()
    for p in sources:
        if p.is_relative_to(here):
            destination = snapshots / 'tests' / p.relative_to(here)
        elif p == native:
            destination = snapshots / 'native-contract/bsdsocket/native.h'
        else:
            destination = snapshots / p.name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(p, destination)
    native_include = snapshots / 'native-contract'
    common = [args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-g', '-O1',
              '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
              '-I' + str(here / 'include'), '-I' + str(native_include), '-I' + str(net)]
    commands = [[args.cc, '--version']]
    for production, fixture, binary in [('socket.c', 'bridge_host.c', 'socket-host'),
                                         ('socket_aros.c', 'bridge_aros_host.c', 'socket-aros-host')]:
        commands += [common + [str(net / production), str(here / fixture), '-o', str(output / binary)],
                     [str(output / binary)]]
    environment = dict(os.environ, ASAN_OPTIONS=('detect_leaks=0:abort_on_error=1' if sys.platform == 'darwin'
                                               else 'detect_leaks=1:abort_on_error=1'),
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    records = []
    verdict = 'FAIL'
    try:
        for index, command in enumerate(commands):
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, env=environment, timeout=90)
            log = output / f'{index:02d}.log'
            log.write_text(shlex.join(command) + '\n' + result.stdout)
            records.append({'command': command, 'exit_code': result.returncode, 'log': log.name})
            if result.returncode:
                print(result.stdout, file=sys.stderr)
                break
            if len(command) == 1:
                print(result.stdout.strip())
        else:
            verdict = 'PASS'
    except (OSError, subprocess.TimeoutExpired) as error:
        records.append({'error': str(error)})
    finally:
        after = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
        if after != hashes:
            verdict = 'FAIL'
            records.append({'error': 'source changed during run'})
        (output / 'result.json').write_text(json.dumps({
            'verdict': verdict, 'scope': 'host bridge conversion and adapter control flow only; no native execution',
            'native_header': str(native), 'source_sha256': hashes, 'sources_unchanged': after == hashes,
            'commands': records,
        }, indent=2) + '\n')
    print(output / 'result.json')
    return 0 if verdict == 'PASS' else 1


if __name__ == '__main__':
    sys.exit(main())
