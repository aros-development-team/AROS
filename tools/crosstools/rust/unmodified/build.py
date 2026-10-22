#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>
# Licensed under the AROS Public License (APL) Version 1.1

"""Build the integrated AROS std port with an unmodified, pinned rustc."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import sys


HERE = Path(__file__).resolve().parent
PIN = json.loads((HERE / "channel.json").read_text())
TOOLCHAIN = PIN["toolchain"]
COMMIT = PIN["compiler_commit"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, *, cwd=None, env=None, log=None):
    result = subprocess.run(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if log is not None:
        log.write_text(result.stdout)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {shlex.join(command)}\n"
                           f"{result.stdout[-6000:]}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rustc", type=Path, help="official compiler binary; paired with --cargo")
    parser.add_argument("--cargo", type=Path, help="matching official Cargo binary")
    parser.add_argument("--rust-src", type=Path, required=True,
                        help="unmodified rust-src root containing library/")
    parser.add_argument("--output", type=Path, required=True,
                        help="new directory; existing paths are refused")
    parser.add_argument("--sdk", type=Path,
                        help="optional native Developer directory containing lib/startup.o")
    parser.add_argument("--crosstools", type=Path,
                        help="C tools directory containing bin/clang and bin/ld.lld")
    parser.add_argument("--collect-aros", type=Path)
    parser.add_argument("--posix-stubs", type=Path,
                        help="private import archive generated from the matching POSIX library")
    parser.add_argument("--probe", choices=("link", "network"), default="link",
                        help="compile/link coverage or executable IPv4 loopback diagnostic")
    args = parser.parse_args()
    if any((args.sdk, args.crosstools, args.collect_aros)) and not all(
            (args.sdk, args.crosstools, args.collect_aros)):
        parser.error("linking requires --sdk, --crosstools and --collect-aros together")

    if args.posix_stubs and not args.sdk:
        parser.error("--posix-stubs requires native linking inputs")
    if bool(args.rustc) != bool(args.cargo):
        parser.error("--rustc and --cargo must be supplied together")
    source = args.rust_src.resolve()
    output = args.output.resolve()
    if output.exists() or output.is_symlink():
        parser.error("--output must not exist")
    # Validate before copying: a patched/shared rust-src must never be our baseline.
    manifest = json.loads((HERE / "source-files.json").read_text())
    for relative, expected in manifest.items():
        path = source / relative
        if expected is None:
            if path.exists():
                parser.error(f"source already contains {relative}")
        elif not path.is_file() or digest(path) != expected:
            parser.error(f"source mismatch: {relative}; use stock {TOOLCHAIN} rust-src")

    compiler = args.rustc.resolve() if args.rustc else Path(
        run(["rustup", "which", "--toolchain", TOOLCHAIN, "rustc"]).strip())
    cargo = str(args.cargo.resolve()) if args.cargo else run(
        ["rustup", "which", "--toolchain", TOOLCHAIN, "cargo"]).strip()
    version = run([str(compiler), "-Vv"])
    if f"commit-hash: {COMMIT}\n" not in version:
        parser.error("compiler identity does not match the pinned nightly")
    compiler_sha256 = digest(compiler)
    official_sysroot = Path(run([str(compiler), "--print", "sysroot"]).strip())
    protected = [source, official_sysroot.resolve()]
    protected.extend(path.resolve() for path in (args.sdk, args.crosstools) if path)
    if any(output == path or path in output.parents for path in protected):
        parser.error("--output must be outside source, compiler sysroot and SDK/tool inputs")
    for ancestor in output.parents:
        for name in ("config", "config.toml"):
            config = ancestor / ".cargo" / name
            if config.exists():
                parser.error(f"ambient Cargo configuration: {config}; choose an isolated output")
    ancestor = next(p for p in output.parents if p.is_dir())
    if shutil.disk_usage(ancestor).free < 5 * 1024**3:
        parser.error("at least 5 GiB free disk space is required")
    output.mkdir(parents=True)
    (output / "rustc-version.txt").write_text(version)
    overlay = output / "sysroot"
    sources = overlay / "lib/rustlib/src/rust"
    sources.mkdir(parents=True)
    shutil.copytree(source / "library", sources / "library", symlinks=False)
    run(["patch", "--batch", "--forward", "--fuzz=0", "-p1", "-i",
         str(HERE / "rust-src-aros.diff")], cwd=sources, log=output / "patch.log")
    for entry in (official_sysroot / "lib/rustlib").iterdir():
        if entry.name not in ("src", "aarch64-unknown-aros"):
            (overlay / "lib/rustlib" / entry.name).symlink_to(entry)

    wrapper = output / "rustc"
    wrapper.write_text("#!/bin/sh\nexec " + shlex.quote(str(compiler)) +
                       " --sysroot " + shlex.quote(str(overlay)) + ' "$@"\n')
    wrapper.chmod(0o755)
    target = output / "aarch64-unknown-aros.json"
    shutil.copyfile(HERE / target.name, target)
    environment = os.environ.copy()
    for name in list(environment):
        if name.startswith("CARGO_"):
            environment.pop(name)
    for name in ("RUSTC_BOOTSTRAP", "RUSTC_WRAPPER", "RUSTC_WORKSPACE_WRAPPER",
                 "RUSTFLAGS", "CARGO_ENCODED_RUSTFLAGS", "CARGO_BUILD_TARGET"):
        environment.pop(name, None)
    environment.update(RUSTC=str(wrapper), CARGO_TARGET_DIR=str(output / "target"),
                       CARGO_INCREMENTAL="0", CARGO_HOME=str(output / "cargo-home"),
                       # Installed rlibs must support consumers using fat/thin
                       # LTO as well as ordinary native-object linking.
                       CARGO_ENCODED_RUSTFLAGS="-C\x1fembed-bitcode=yes")
    cfg = run([str(wrapper), "-Zunstable-options", "--print", "cfg", "--target",
               str(target)], env=environment)
    (output / "target-cfg.txt").write_text(cfg)
    if not {'target_os="aros"', 'target_family="unix"', 'target_arch="aarch64"'} <= set(cfg.splitlines()):
        raise RuntimeError("target cfg does not select the integrated Unix std port")

    probe = output / "probe"
    (probe / "src").mkdir(parents=True)
    (probe / "Cargo.toml").write_text(
        '[package]\nname="aros-integrated-std-probe"\nversion="0.1.0"\n'
        'edition="2021"\n[lib]\ncrate-type=["staticlib"]\n'
        '[profile.release]\npanic="abort"\n')
    probe_source = HERE / ("network-probe.rs" if args.probe == "network" else "probe.rs")
    shutil.copyfile(probe_source, probe / "src/lib.rs")
    command = [cargo, "--config", 'source.crates-io.replace-with="aros-std-vendor"',
               "--config", "source.aros-std-vendor.directory=" +
               json.dumps(str(sources / "library/vendor")),
               "build", "--offline", "--manifest-path", str(probe / "Cargo.toml"),
               "--target", str(target), "-Zjson-target-spec",
               "-Zbuild-std=std,panic_abort", "--release", "--message-format=json-render-diagnostics"]
    (output / "build-command.json").write_text(json.dumps(command, indent=2) + "\n")
    build_messages = run(command, cwd=output, env=environment, log=output / "build.log")
    # Cargo's artifact layout is not an SDK interface. Consume its declared
    # target artifacts rather than assuming a release/deps directory.
    std_artifacts = {}
    archives = []
    target_output = output / "target/aarch64-unknown-aros"
    for line in build_messages.splitlines():
        try:
            message = json.loads(line)
        except json.JSONDecodeError:
            continue
        if message.get("reason") != "compiler-artifact":
            continue
        for filename in message.get("filenames", []):
            artifact = Path(filename).resolve()
            if target_output not in artifact.parents:
                continue
            if artifact.suffix == ".rlib":
                if artifact.name in std_artifacts and std_artifacts[artifact.name] != artifact:
                    raise RuntimeError("ambiguous target std artifact: " + artifact.name)
                std_artifacts[artifact.name] = artifact
            if artifact.suffix == ".a" and message["target"]["name"] == "aros_integrated_std_probe":
                archives.append(artifact)
    for required in ("core", "alloc", "std", "panic_abort", "compiler_builtins", "libc"):
        if sum(p.name.startswith("lib" + required + "-") for p in std_artifacts.values()) != 1:
            raise RuntimeError("expected exactly one target std artifact for " + required)
    if len(set(archives)) != 1:
        raise RuntimeError("expected exactly one target probe static archive")
    # Applications consume the same compiled std without rebuilding its sources.
    # Never write through a target-directory alias into the official sysroot.
    target_library = overlay / "lib/rustlib/aarch64-unknown-aros/lib"
    target_library.mkdir(parents=True)
    std_rlibs = sorted(std_artifacts.values())
    for library in std_rlibs:
        shutil.copy2(library, target_library / library.name)
    archive = output / "std-probe.a"
    shutil.copyfile(archives[0], archive)
    result = {"probe": args.probe, "probe_sha256": digest(probe_source),
              "precompiled_std": {p.name: digest(p) for p in std_rlibs},
              "compiler_commit": COMMIT, "compiler_sha256": compiler_sha256,
              "target_sha256": digest(target), "std_patch_sha256": digest(HERE / "rust-src-aros.diff"),
              "archive_sha256": digest(archive), "std_compile": "PASS", "native_runtime": "UNTESTED"}
    if args.sdk:
        sdk, tools = args.sdk.resolve(), args.crosstools.resolve() / "bin"
        harness = output / "main.c"
        harness.write_text('#include <aros/debug.h>\n'
                           'extern unsigned long rust_std_probe(void);\n'
                           'void aros_rs_probe_event(const char *event) { bug("%s\\n", event); }\n'
                           'int main(void) {\n'
                           '  bug("[RUSTPROBE] main-entry\\n");\n'
                           '  unsigned long count = rust_std_probe();\n'
                           '  bug("[RUSTPROBE] main-return count=%lu\\n", count);\n'
                           '  return count > 0 ? 0 : 20;\n}\n')
        run([str(tools / "clang"), "--target=aarch64-unknown-aros", "-fno-pic",
             "-mcmodel=large", "-O2", "-I" + str(sdk / "include"),
             "-c", str(harness), "-o", str(output / "main.o")],
            log=output / "compile-harness.log")
        # Only private output receives the new header; the SDK is read-only.
        native_header = HERE.parents[3] / "workbench/network/common/include/bsdsocket/native.h"
        private_include = output / "include"
        (private_include / "bsdsocket").mkdir(parents=True)
        shutil.copyfile(native_header, private_include / "bsdsocket/native.h")
        positional_archives = []
        if args.posix_stubs:
            imported = output / "libaros_positional.a"
            shutil.copyfile(args.posix_stubs.resolve(), imported)
            positional_archives.append(str(imported))
        adapters = []
        for name in ("resolver", "resolver_aros", "socket", "socket_aros", "positioned", "unavailable"):
            obj = output / (name + ".o")
            command = [str(tools / "clang"), "--target=aarch64-unknown-aros", "-fno-pic",
                       "-mcmodel=large", "-fno-builtin", "-O2", "-Wall", "-Wextra", "-Werror",
                       "-Wconversion", "-Wsign-conversion",
                       "-I" + str(private_include), "-I" + str(HERE / "net"),
                       "-I" + str(sdk / "include/aros/posixc"),
                       "-I" + str(sdk / "include/aros/stdc"), "-I" + str(sdk / "include"),
                       "-c", str(HERE / ({"positioned": "fs", "unavailable": "sys"}.get(name, "net")) / (name + ".c")), "-o", str(obj)]
            (output / (name + "-command.json")).write_text(json.dumps(command, indent=2) + "\n")
            run(command, log=output / (name + "-compile.log"))
            adapters.append(str(obj))
        native_archive = output / "libaros_rs_platform.a"
        run([str(tools / "llvm-ar"), "rcs", str(native_archive), *adapters],
            log=output / "archive-platform.log")
        native_driver = output / "aarch64-aros-cc"
        native_driver.write_text("#!/bin/sh\nexec " + shlex.join([
            str(tools / "clang"), "--target=aarch64-unknown-aros", "--sysroot=" + str(sdk),
            "-B" + str(args.collect_aros.resolve().parent), "-B" + str(tools)]) +
            ' "$@" ' + shlex.join(["-L" + str(output), "-laros_rs_platform", *positional_archives]) + "\n")
        native_driver.chmod(0o755)
        (output / "cargo-config.toml").write_text(
            "[target.aarch64-unknown-aros]\nlinker = " + json.dumps(str(native_driver)) +
            "\nrustflags = [" + json.dumps("-Lnative=" + str(sdk / "lib")) + "]\n")
        shim = output / "linkshim"
        shim.mkdir()
        (shim / "ld").symlink_to(tools / "ld.lld")
        environment["COMPILER_PATH"] = str(shim)
        binary = output / ("RustNetworkProbe" if args.probe == "network" else "RustStdProbe")
        command = [str(args.collect_aros.resolve()), "--eh-frame-hdr", "-L" + str(sdk / "lib"),
                   "-o", str(binary), str(sdk / "lib/startup.o"), str(output / "main.o"),
                   *adapters, str(archive), "-(", *positional_archives, "-lposixc", "-lstdcio", "-lstdc", "-lpthread",
                   "-lamiga", "-larossupport", "-llibinit", "-lautoinit", "-ldos",
                   "-lutility", "-lexec", "-)"]
        (output / "link-command.json").write_text(json.dumps(command, indent=2) + "\n")
        run(command, env=environment, log=output / "link.log")
        with binary.open("rb") as stream:
            header = stream.read(64)
        if (len(header) != 64 or header[:4] != b"\x7fELF" or
                header[4:6] != b"\x02\x01" or header[7] != 15 or
                struct.unpack_from("<HH", header, 16) != (1, 183)):
            raise RuntimeError("linked command is not an AROS ELF64 little-endian AArch64 ET_REL")
        undefined = run([str(tools / "llvm-nm"), "--undefined-only", str(binary)],
                        log=output / "undefined.txt")
        if undefined.strip():
            raise RuntimeError("final command has undefined symbols")
        result.update(native_link="PASS", binary_sha256=digest(binary),
                      sdk=str(sdk), crosstools=str(args.crosstools.resolve()),
                      collect_aros=str(args.collect_aros.resolve()),
                      resolver_provider="AROS bsdsocket.library vector 136",
                      socket_provider="AROS bsdsocket.library native vector 139",
                      native_adapter_archive_sha256=digest(native_archive),
                      positioned_adapter_sha256=digest(HERE / "fs/positioned.c"),
                      unavailable_adapter_sha256=digest(HERE / "sys/unavailable.c"),
                      positional_import_archives={Path(p).name: digest(Path(p)) for p in positional_archives},
                      cargo_config="cargo-config.toml",
                      native_socket_header_sha256=digest(native_header),
                      network_adapter_sha256={name: digest(HERE / "net" / name) for name in
                          ("resolver.c", "resolver.h", "resolver_native.h", "resolver_aros.c",
                           "socket.c", "socket_native.h", "socket_aros.c")},
                      elf={"class": 64, "endian": "little", "osabi": "AROS",
                           "machine": "AArch64", "type": "ET_REL"})
    if digest(compiler) != result["compiler_sha256"]:
        raise RuntimeError("compiler changed during the build")
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
