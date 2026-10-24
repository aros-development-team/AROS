#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>
# Licensed under the AROS Public License (APL), Version 1.1
"""Build a normal Cargo binary against the precompiled AROS std, without build-std."""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import shutil
import struct
import sys

sys.dont_write_bytecode = True

from build import TOOLCHAIN, digest, run

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--std-build', type=Path)
    parser.add_argument('--install-prefix', type=Path)
    parser.add_argument('--application-libc', type=Path)
    parser.add_argument('--lto', choices=('thin', 'fat'), help='qualify LTO with the installed target libraries')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    installation = None
    if args.install_prefix:
        prefix = args.install_prefix.resolve()
        installation = json.loads((prefix / 'aros-official-rust.json').read_text())
        for name, expected in installation['installed'].items():
            if digest(prefix / name) != expected:
                parser.error('installed toolchain changed: ' + name)
        installed_build = Path(installation['native_build']).resolve()
        if args.std_build and args.std_build.resolve() != installed_build:
            parser.error('std-build differs from the installed toolchain')
        build = installed_build
    elif args.std_build:
        build = args.std_build.resolve()
    else:
        parser.error('provide --std-build or --install-prefix')
    output = args.output.resolve()
    if output.exists() or args.output.is_symlink():
        parser.error('output must not exist')
    result = json.loads((build / 'result.json').read_text())
    if (result.get('native_link') != 'PASS' or not result.get('precompiled_std')
            or not result.get('sdk') or not result.get('crosstools')):
        parser.error('std-build must contain a linked build with installed std and SDK/tool identities')
    if installation:
        candidates = [Path(name) for name, expected in installation['inputs'].items()
                      if name.endswith('/bin/rustc') and expected == result['compiler_sha256']]
        if len(candidates) != 1:
            parser.error('installed toolchain lacks a unique official compiler identity')
        compiler = candidates[0]
    else:
        compiler = Path(run(['rustup', 'which', '--toolchain', TOOLCHAIN, 'rustc']).strip())
    official_root = Path(run([str(compiler), '--print', 'sysroot']).strip()).resolve()
    protected = [build, official_root, HERE.parents[3]]
    if installation:
        protected.append(prefix)
    if args.application_libc:
        protected.append(args.application_libc.resolve())
    protected += [Path(result[name]).resolve() for name in ('sdk', 'crosstools') if name in result]
    if any(output == p or p in output.parents for p in protected):
        parser.error('output must be outside source, std-build, compiler and SDK/tool inputs')
    for ancestor in output.parents:
        if any((ancestor / '.cargo' / name).exists() for name in ('config', 'config.toml')):
            parser.error('ambient Cargo configuration; select an isolated output')
    if digest(compiler) != result['compiler_sha256']:
        parser.error('the official compiler does not match the produced std')
    target_lib = build / 'sysroot/lib/rustlib/aarch64-unknown-aros/lib'
    for name, expected in result['precompiled_std'].items():
        if digest(target_lib / name) != expected:
            parser.error('installed std rlib changed: ' + name)
    archive = build / 'libaros_rs_platform.a'
    if digest(archive) != result['native_adapter_archive_sha256']:
        parser.error('native platform archive changed')
    imports = []
    for name, expected in result.get('positional_import_archives', {}).items():
        path = build / name
        if digest(path) != expected:
            parser.error('native import archive changed: ' + name)
        imports.append(path)
    external_expected = {}
    if installation:
        external_roots = [official_root, Path(result['sdk']).resolve(),
                          Path(result['crosstools']).resolve(),
                          Path(result['collect_aros']).resolve().parent]
        external_expected = {name: expected for name, expected in installation['inputs'].items()
                             if any(Path(name) == root or root in Path(name).parents
                                    for root in external_roots)}
        for name, expected in external_expected.items():
            if not Path(name).is_file() or digest(Path(name)) != expected:
                parser.error('qualified SDK/tool input changed: ' + name)
    vendor = build / 'sysroot/lib/rustlib/src/rust/library/vendor'
    libc = ([args.application_libc.resolve() / 'Cargo.toml'] if args.application_libc else
            list(vendor.glob('libc-*/Cargo.toml')))
    if len(libc) != 1:
        parser.error('expected one libc source version in the qualified overlay')
    cargo = str(prefix / 'bin/cargo') if installation else run(
        ['rustup', 'which', '--toolchain', TOOLCHAIN, 'cargo']).strip()
    config = prefix / 'cargo-config.toml' if installation else build / 'cargo-config.toml'
    rustc = prefix / 'bin/rustc' if installation else build / 'rustc'
    inputs = [compiler, archive, build / 'cargo-config.toml', build / 'result.json',
              build / 'aarch64-unknown-aros.json', build / 'rustc',
              build / 'aarch64-aros-cc', HERE / 'consumer-probe.rs', HERE / 'consumer-trace.c']
    if installation:
        inputs += [prefix / 'aros-official-rust.json', *(prefix / name for name in installation['installed'])]
    inputs += imports
    inputs += [p for p in libc[0].parent.rglob('*') if p.is_file()]
    before = {str(p): digest(p) for p in inputs}
    ancestor = next(p for p in output.parents if p.is_dir())
    if shutil.disk_usage(ancestor).free < 5 * 1024**3:
        parser.error('at least 5 GiB free disk space is required')
    output.mkdir(parents=True)
    (output / 'src').mkdir()
    (output / 'Cargo.toml').write_text(
        '[package]\nname="aros-precompiled-std-consumer"\nversion="0.1.0"\n'
        'edition="2021"\n[workspace]\n[dependencies]\nlibc = { path = ' +
        json.dumps(str(libc[0].parent)) + ' }\n[profile.dev]\npanic="abort"\n' +
        ('lto=' + json.dumps(args.lto) + '\n' if args.lto else ''))
    shutil.copyfile(HERE / 'consumer-probe.rs', output / 'src/main.rs')
    tools = Path(result['crosstools']) / 'bin'
    trace = output / 'consumer-trace.o'
    trace_command = [str(tools / 'clang'), '--target=aarch64-unknown-aros',
                     '-fno-pic', '-mcmodel=large', '-O2',
                     '-fno-builtin', '-I' + str(Path(result['sdk']) / 'include/aros/posixc'),
                     '-I' + str(Path(result['sdk']) / 'include/aros/stdc'),
                     '-I' + str(Path(result['sdk']) / 'include'),
                     '-c', str(HERE / 'consumer-trace.c'), '-o', str(trace)]
    (output / 'trace-command.json').write_text(json.dumps(trace_command, indent=2) + '\n')
    run(trace_command, log=output / 'trace-compile.log')
    env = {k: v for k, v in os.environ.items() if not k.startswith('CARGO_') and k not in
           ('RUSTFLAGS', 'CARGO_ENCODED_RUSTFLAGS', 'RUSTC_WRAPPER',
            'RUSTC_WORKSPACE_WRAPPER', 'RUSTC_BOOTSTRAP')}
    env.update(RUSTC=str(rustc), CARGO_HOME=str(output / 'cargo-home'),
               CARGO_TARGET_DIR=str(output / 'target'), CARGO_INCREMENTAL='0')
    command = [cargo, '--config', str(config), '--config',
               'source.crates-io.replace-with="aros-std-vendor"', '--config',
               'target.aarch64-unknown-aros.rustflags=' + json.dumps([
                   '-Lnative=' + str(Path(result['sdk']) / 'lib'), '-Clink-arg=' + str(trace)]), '--config',
               'source.aros-std-vendor.directory=' + json.dumps(str(vendor)),
               'build', '--offline', '--manifest-path', str(output / 'Cargo.toml'),
               *([] if installation else ['--target', str(build / 'aarch64-unknown-aros.json'), '-Zjson-target-spec'])]
    (output / 'build-command.json').write_text(json.dumps(command, indent=2) + '\n')
    run(command, cwd=output, env=env, log=output / 'build.log')
    binary = output / 'target/aarch64-unknown-aros/debug/aros-precompiled-std-consumer'
    data = binary.read_bytes()
    if (data[:6] != b'\x7fELF\x02\x01' or data[7] != 15 or
            struct.unpack_from('<HH', data, 16) != (1, 183)):
        raise RuntimeError('consumer is not native AROS AArch64 ELF ET_REL')
    header = struct.unpack_from('<HHIQQQIHHHHHH', data, 16)
    sections = [struct.unpack_from('<IIQQQQIIQQ', data, header[5] + i * header[10])
                for i in range(header[11])]
    relocations = Counter()
    for section in sections:
        if section[1] == 4 and sections[section[7]][2] & 2:
            for off in range(section[4], section[4] + section[5], section[9]):
                relocations[struct.unpack_from('<Q', data, off + 8)[0] & 0xffffffff] += 1
    if relocations[311] or relocations[312]:
        raise RuntimeError('consumer contains GOT relocations rejected by LoadSeg')
    tools = Path(result['crosstools']) / 'bin'
    if not (tools / 'llvm-nm').is_file():
        parser.error('std-build metadata must identify its C tools')
    undefined = run([str(tools / 'llvm-nm'), '--undefined-only', str(binary)],
                    log=output / 'undefined.txt')
    if undefined.strip():
        raise RuntimeError('consumer has undefined symbols')
    if before != {str(p): digest(p) for p in inputs}:
        raise RuntimeError('qualified inputs changed during consumer compilation')
    if any(not Path(name).is_file() or digest(Path(name)) != expected
           for name, expected in external_expected.items()):
        raise RuntimeError('qualified SDK/tool inputs changed during consumer compilation')
    report = {'compile_link': 'PASS', 'build_std': False, 'native_runtime': 'UNTESTED',
              'lto': args.lto,
              'installed_toolchain': str(prefix) if installation else None,
              'external_sdk_inputs_verified': len(external_expected),
              'application_libc': str(libc[0].parent), 'allocated_relocations': dict(relocations), 'binary_sha256': digest(binary),
              'input_sha256': before}
    (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS normal Cargo binary: precompiled std, AROS ELF, no GOT or undefined symbols')


if __name__ == '__main__':
    main()
