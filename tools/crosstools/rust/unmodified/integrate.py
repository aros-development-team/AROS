#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Licensed under the AROS Public License (APL), Version 1.1.
"""Prepare official host tools and install the native AROS Rust SDK in a dedicated prefix."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tarfile
import urllib.request

sys.dont_write_bytecode = True

from build import HERE, PIN, COMMIT, TOOLCHAIN, MIN_BUILD_FREE, digest, run


def write(path, text, executable=False):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.new')
    temporary.write_text(text)
    if executable:
        temporary.chmod(0o755)
    os.replace(temporary, path)


def component(item, cache, offline):
    archive = cache / item['url'].rsplit('/', 1)[-1]
    if not archive.is_file():
        if offline:
            raise RuntimeError('missing offline component: ' + str(archive))
        temporary = archive.with_suffix(archive.suffix + '.download')
        with urllib.request.urlopen(item['url'], timeout=60) as source, temporary.open('wb') as target:
            shutil.copyfileobj(source, target)
        if digest(temporary) != item['sha256']:
            temporary.unlink()
            raise RuntimeError('official component checksum mismatch: ' + item['url'])
        os.replace(temporary, archive)
    if digest(archive) != item['sha256']:
        raise RuntimeError('cached official component checksum mismatch: ' + str(archive))
    tree = cache / (archive.name.removesuffix('.tar.xz') + '.unpacked')
    if not tree.exists():
        temporary = tree.with_name(tree.name + '.new')
        temporary.mkdir()
        with tarfile.open(archive) as source:
            source.extractall(temporary, filter='data')
        os.replace(temporary, tree)
    roots = list(tree.iterdir())
    if len(roots) != 1 or not roots[0].is_dir():
        raise RuntimeError('unexpected official component layout: ' + str(tree))
    return roots[0]


def native_build_command(compiler, cargo, stock, native, args):
    """Pass the explicit worker bound through to the actual std builder."""
    if args.jobs < 1:
        raise ValueError("jobs must be positive")
    command = [str(HERE / 'build.py'), '--rustc', str(compiler), '--cargo', str(cargo),
               '--rust-src', str(stock), '--output', str(native), '--sdk', str(args.sdk.resolve()),
               '--crosstools', str(args.crosstools.resolve()),
               '--collect-aros', str(args.collect_aros.resolve()), '--jobs', str(args.jobs)]
    if args.posix_stubs:
        command += ['--posix-stubs', str(args.posix_stubs.resolve())]
    return command


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('prefix', 'work', 'sdk', 'crosstools', 'collect-aros'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--host-triple', required=True, choices=sorted(PIN['hosts']))
    p.add_argument('--host-root', type=Path, help='reuse matching official host tools read-only')
    p.add_argument('--rust-src', type=Path, help='reuse stock source read-only')
    p.add_argument('--std-build', type=Path, help='install an existing matching native std qualification output')
    p.add_argument('--posix-stubs', type=Path, help='private matching import archive for an older read-only SDK')
    p.add_argument('--offline', action='store_true')
    p.add_argument('--jobs', type=int, default=1, help='positive Cargo worker bound (default: 1)')
    a = p.parse_args()
    if a.jobs < 1:
        p.error('--jobs must be positive')
    if sys.version_info < (3, 12):
        p.error("Python 3.12 or newer is required for filtered archive extraction")
    prefix, work = a.prefix.resolve(), a.work.resolve()
    protected = [HERE.parents[3], a.sdk.resolve(), a.crosstools.resolve()]
    protected += [x.resolve() for x in (a.host_root, a.rust_src) if x]
    if any(work == x or x in work.parents for x in protected):
        p.error('--work must be outside source and SDK/C tool inputs')
    if any(prefix == x or x in prefix.parents for x in protected if x != a.crosstools.resolve()):
        p.error('--prefix must be outside source, SDK and official host/source inputs')
    if a.std_build and not (a.std_build / 'result.json').is_file():
        p.error('--std-build must contain a completed native build')
    if prefix == work or prefix in work.parents or work in prefix.parents:
        p.error('--prefix and --work must be separate trees')
    if prefix.exists() and not (prefix / 'aros-official-rust.json').is_file():
        p.error('an existing prefix must be a managed AROS official Rust installation')
    ancestor = next(x for x in work.parents if x.is_dir())
    required = 32 * 1024**2 if a.std_build and a.host_root else MIN_BUILD_FREE
    if shutil.disk_usage(ancestor).free < required:
        p.error('insufficient disk space: building requires 6 GiB; installing existing artifacts requires 32 MiB')
    work.mkdir(parents=True, exist_ok=True)
    cache = work / 'components'
    cache.mkdir(exist_ok=True)
    if a.host_root:
        host = a.host_root.resolve()
    else:
        host = work / ('host-' + a.host_triple)
        marker = host / 'official-components.json'
        if not marker.is_file():
            if host.exists():
                raise RuntimeError('incomplete host installation: ' + str(host))
            host.mkdir()
            for name, item in PIN['hosts'][a.host_triple].items():
                package = component(item, cache, a.offline)
                run([str(package / 'install.sh'), '--prefix=' + str(host),
                     '--disable-ldconfig', '--without=rust-docs'], log=work / (name + '-install.log'))
            marker.write_text(json.dumps(PIN['hosts'][a.host_triple], indent=2) + '\n')
        if json.loads(marker.read_text()) != PIN['hosts'][a.host_triple]:
            raise RuntimeError('host component pin changed')
    compiler, cargo = host / 'bin/rustc', host / 'bin/cargo'
    version = run([str(compiler), '-Vv'])
    if 'commit-hash: ' + COMMIT + '\n' not in version or 'host: ' + a.host_triple + '\n' not in version:
        raise RuntimeError('official compiler commit or host does not match the pin')
    if a.rust_src:
        stock = a.rust_src.resolve()
    elif a.std_build:
        stock = None
    else:
        package = component(PIN['source'], cache, a.offline)
        stock = package / 'rust-src/lib/rustlib/src/rust'
    inputs = {str(x): digest(x) for x in (compiler, cargo, host / 'bin/rustdoc', HERE / 'channel.json',
              HERE / 'source-files.json', HERE / 'build.py', HERE / 'integrate.py',
              a.crosstools.resolve() / 'bin/clang', a.collect_aros.resolve(), HERE / 'rust-src-aros.diff', HERE / 'aarch64-unknown-aros.json')}
    native_header = HERE.parents[3] / 'workbench/network/common/include/bsdsocket/native.h'
    inputs[str(native_header)] = digest(native_header)
    inputs.update({str(x): digest(x) for directory in ('net', 'fs', 'sys') for x in (HERE / directory).rglob('*')
                   if x.is_file()})
    if a.posix_stubs:
        inputs[str(a.posix_stubs.resolve())] = digest(a.posix_stubs.resolve())
    # Include SDK libraries and headers in the generation key: a rebuilt SDK
    # must never silently reuse a std/adapter generation from another ABI.
    for directory in ('include', 'lib'):
        inputs.update({str(x): digest(x) for x in (a.sdk / directory).rglob('*') if x.is_file()})
    key = hashlib.sha256(json.dumps(inputs, sort_keys=True).encode()).hexdigest()
    native = a.std_build.resolve() if a.std_build else work / ('native-' + key[:20])
    if not (native / 'result.json').is_file():
        command = native_build_command(compiler, cargo, stock, native, a)
        run([sys.executable, *command], log=work / 'std-build.log')
    result = json.loads((native / 'result.json').read_text())
    if (result.get('native_link') != 'PASS' or result['compiler_sha256'] != digest(compiler)
            or result['compiler_commit'] != COMMIT or result['std_patch_sha256'] != digest(HERE / 'rust-src-aros.diff')
            or result['target_sha256'] != digest(HERE / 'aarch64-unknown-aros.json')):
        raise RuntimeError('native std output does not match the pinned inputs')
    if digest(native_header) != result['native_socket_header_sha256']:
        raise RuntimeError('native std output uses a different socket ABI header')
    if digest(HERE / 'sys/unavailable.c') != result['unavailable_adapter_sha256']:
        raise RuntimeError('native std output uses a different unavailable-interface provider')
    if digest(HERE / 'fs/positioned.c') != result['positioned_adapter_sha256']:
        raise RuntimeError('native std output uses a different file adapter')
    for name, expected in result['network_adapter_sha256'].items():
        if digest(HERE / 'net' / name) != expected:
            raise RuntimeError('native std output uses different network adapters')
    for name in ('sdk', 'crosstools', 'collect_aros'):
        if Path(result[name]).resolve() != getattr(a, name).resolve():
            raise RuntimeError('native std output uses a different ' + name)
    target_lib = native / 'sysroot/lib/rustlib/aarch64-unknown-aros/lib'
    for name, expected in result['precompiled_std'].items():
        if digest(target_lib / name) != expected:
            raise RuntimeError('precompiled std changed: ' + name)
    for name in ('core', 'alloc', 'std', 'panic_abort', 'compiler_builtins', 'libc'):
        if sum(x.startswith('lib' + name + '-') for x in result['precompiled_std']) != 1:
            raise RuntimeError('incomplete precompiled std: ' + name)
    if digest(native / 'std-probe.a') != result['archive_sha256']:
        raise RuntimeError('retained std probe archive changed')
    if digest(native / 'libaros_rs_platform.a') != result['native_adapter_archive_sha256']:
        raise RuntimeError('native platform archive changed')
    for name, expected in result.get('positional_import_archives', {}).items():
        if digest(native / name) != expected:
            raise RuntimeError('native import archive changed: ' + name)
    if inputs != {name: digest(Path(name)) for name in inputs}:
        raise RuntimeError('native toolchain inputs changed during preparation')
    if not a.std_build and (native / 'target').is_dir():
        # The installed rlibs and retained probe are the products. Generation
        # reuse reads result.json; no later build consumes this Cargo cache.
        shutil.rmtree(native / 'target')
    # The prefix is dedicated to this toolchain. C tools and the source SDK
    # remain read-only; only managed wrappers/configuration are replaced.
    prefix.mkdir(parents=True, exist_ok=True)
    target = prefix / 'share/aarch64-unknown-aros.json'
    write(target, (native / 'aarch64-unknown-aros.json').read_text())
    for name, executable in (('rustc', native / 'rustc'), ('cargo', cargo),
                             ('aarch64-aros-cc', native / 'aarch64-aros-cc'),
                             ('rustdoc', host / 'bin/rustdoc')):
        arguments = [str(executable)]
        if name == 'rustdoc':
            arguments += ['--sysroot', str(native / 'sysroot')]
        write(prefix / 'bin' / name, '#!/bin/sh\nexec ' + shlex.join(arguments) + ' "$@"\n', True)
    write(prefix / 'cargo-config.toml', '[target.aarch64-unknown-aros]\nlinker = ' +
          json.dumps(str(prefix / 'bin/aarch64-aros-cc')) + '\nrustflags = [' +
          json.dumps('-Lnative=' + str(a.sdk.resolve() / 'lib')) + ', ' +
          # Cargo cannot infer a rebuilt custom sysroot from rustc -Vv.
          # Carry the SDK identity in target flags so it invalidates cached
          # application crates whenever the installed generation changes.
          json.dumps('--cfg=aros_sdk_generation="' + key + '"') + ']\n[build]\ntarget = ' +
          json.dumps(str(target)) + '\n[unstable]\njson-target-spec = true\n')
    installed = {str(x.relative_to(prefix)): digest(x) for x in
                 [target, prefix / 'cargo-config.toml', *(prefix / 'bin' / n for n in
                  ('rustc', 'cargo', 'aarch64-aros-cc', 'rustdoc'))]}
    write(prefix / 'aros-official-rust.json', json.dumps({'toolchain': TOOLCHAIN,
          'compiler_commit': COMMIT, 'host_triple': a.host_triple, 'generation': key,
          'native_build': str(native), 'inputs': inputs, 'installed': installed,
          'compile_link': 'PASS', 'native_runtime': 'UNTESTED'}, indent=2) + '\n')
    print('PASS official compiler and precompiled native AROS std installed: ' + str(prefix))


if __name__ == '__main__':
    main()
