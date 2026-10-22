#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>
# Licensed under the AROS Public License (APL) Version 1.1

"""Compare native SDK networking ABI with the libc in a qualified std build.

No target execution is involved. Compile constant records in both languages,
extract their ELF sections, and compare every record. The resolver's addrinfo
layout is reported separately because its explicit conversion is a bridge,
not binary ABI compatibility. A matching report does not qualify that bridge.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys


LAYOUTS = {
    "sockaddr": "sa_len sa_family sa_data",
    "sockaddr_in": "sin_len sin_family sin_port sin_addr sin_zero",
    "sockaddr_in6": "sin6_len sin6_family sin6_port sin6_flowinfo sin6_addr sin6_scope_id",
    "sockaddr_storage": "ss_len ss_family",
    "addrinfo": "ai_flags ai_family ai_socktype ai_protocol ai_addrlen ai_canonname ai_addr ai_next",
    "iovec": "iov_base iov_len",
    "msghdr": "msg_name msg_namelen msg_iov msg_iovlen msg_control msg_controllen msg_flags",
    "cmsghdr": "cmsg_len cmsg_level cmsg_type",
    "linger": "l_onoff l_linger",
    "timeval": "tv_sec tv_usec",
}
CONSTANTS = """
AF_UNSPEC AF_UNIX AF_INET AF_INET6 SOCK_STREAM SOCK_DGRAM SOCK_RAW
SOL_SOCKET SO_DEBUG SO_REUSEADDR SO_KEEPALIVE SO_BROADCAST SO_LINGER
SO_REUSEPORT SO_SNDBUF SO_RCVBUF SO_SNDTIMEO SO_RCVTIMEO SO_ERROR SO_TYPE
IPPROTO_IP IPPROTO_ICMP IPPROTO_ICMPV6 IPPROTO_TCP IPPROTO_UDP IPPROTO_IPV6
TCP_NODELAY IP_TTL IP_MULTICAST_TTL IP_MULTICAST_LOOP IP_ADD_MEMBERSHIP
IP_DROP_MEMBERSHIP IPV6_MULTICAST_LOOP IPV6_V6ONLY IPV6_JOIN_GROUP
IPV6_LEAVE_GROUP MSG_PEEK MSG_NOSIGNAL SHUT_RD SHUT_WR SHUT_RDWR
FIONBIO FIONREAD FIOCLEX FIONCLEX EAI_SYSTEM
ENOSYS EINVAL EBADF EAFNOSUPPORT EOPNOTSUPP EWOULDBLOCK ECONNREFUSED
""".split()
TYPES = "c_int c_long size_t socklen_t sa_family_t pid_t".split()
HEADER = "// Copyright (C) 2026 John KNIPPER <code@jkn.me>\n// Licensed under the AROS Public License (APL) Version 1.1\n"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(argv, output, name, env=None):
    (output / (name + "-command.json")).write_text(json.dumps(argv, indent=2) + "\n")
    result = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, env=env)
    (output / (name + ".log")).write_text(result.stdout)
    if result.returncode:
        raise RuntimeError(f"{name} failed ({result.returncode}): {result.stdout[-6000:]}")
    return result.stdout


def one_rlib(deps, name):
    matches = sorted(deps.glob(f"lib{name}-*.rlib"))
    if len(matches) != 1:
        raise RuntimeError(f"expected exactly one lib{name} rlib in {deps}: {matches}")
    return matches[0]


def records():
    values = []
    for name, fields in LAYOUTS.items():
        group = "resolver_bridge" if name == "addrinfo" else "direct"
        values.extend([(group, name + ".size", f"sizeof(struct {name})",
                        f"size_of::<libc::{name}>()"),
                       (group, name + ".align", f"_Alignof(struct {name})",
                        f"align_of::<libc::{name}>()")])
        for field in fields.split():
            values.append((group, name + "." + field + ".offset",
                           f"__builtin_offsetof(struct {name}, {field})",
                           f"offset_of!(libc::{name}, {field})"))
            values.append((group, name + "." + field + ".size",
                           f"sizeof(((struct {name} *)0)->{field})",
                           f"field_size!(libc::{name}, {field})"))
    for name in CONSTANTS:
        values.append(("direct", name, name, f"libc::{name}"))
    for name in TYPES:
        cname = {"c_int": "int", "c_long": "long"}.get(name, name)
        values.append(("direct", name + ".size", f"sizeof({cname})",
                       f"size_of::<libc::{name}>()"))
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("sdk", "crosstools", "std-build", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--application-libc-rlib", type=Path)
    args = parser.parse_args()
    sdk, tools, build, output = (getattr(args, n).resolve()
                                  for n in ("sdk", "crosstools", "std_build", "output"))
    if output.exists() or args.output.is_symlink():
        parser.error("output must be a new directory")
    for protected in (sdk, tools, build):
        if output == protected or protected in output.parents:
            parser.error("output must be outside all input trees")
    qualified = json.loads((build / "result.json").read_text())
    if qualified.get("std_compile") != "PASS":
        parser.error("std-build result.json must record std_compile PASS")
    deps = build / "sysroot/lib/rustlib/aarch64-unknown-aros/lib"
    core = one_rlib(deps, "core")
    libc = args.application_libc_rlib.resolve() if args.application_libc_rlib else one_rlib(deps, "libc")
    wrapper, target = build / "rustc", build / "aarch64-unknown-aros.json"
    output.mkdir(parents=True)
    inputs = {str(p): digest(p) for p in (core, libc, wrapper, target, build / "result.json")}
    entries = records()
    c = HEADER + "\n".join("#include <" + name + ">" for name in (
        "errno.h", "sys/types.h", "sys/socket.h", "sys/uio.h", "sys/time.h", "sys/ioctl.h",
        "netinet/in.h", "netinet/tcp.h", "netdb.h"))
    c += '\n__attribute__((used,section(".aros_abi")))\nconst unsigned long ABI[] = {\n'
    for entry in entries:
        if entry[1] in CONSTANTS:
            c += (f"#ifdef {entry[1]}\n(unsigned long)({entry[2]}),\n"
                  "#else\n~0UL, /* absent SDK constant */\n#endif\n")
        else:
            c += f"(unsigned long)({entry[2]}),\n"
    c += "};\n"
    rust = HEADER + '''#![no_std]
#![feature(rustc_private)]
extern crate libc;
use core::mem::{size_of, align_of, offset_of, MaybeUninit};
macro_rules! field_size {
    ($ty:ty, $field:ident) => {{
        const fn pointer_size<T>(_: *const T) -> usize { size_of::<T>() }
        let object = MaybeUninit::<$ty>::uninit();
        // addr_of never reads the uninitialized object.
        unsafe { pointer_size(core::ptr::addr_of!((*object.as_ptr()).$field)) }
    }};
}
#[used]
#[no_mangle]
#[link_section = ".aros_abi"]
'''
    rust += f"pub static ABI: [u64; {len(entries)}] = [\n"
    rust += ",\n".join(f"({entry[3]}) as u64" for entry in entries) + "\n];\n"
    (output / "native.c").write_text(c)
    (output / "rust.rs").write_text(rust)
    env = os.environ.copy()
    for name in ("RUSTC_BOOTSTRAP", "RUSTFLAGS", "CARGO_ENCODED_RUSTFLAGS"):
        env.pop(name, None)
    rustflags = [str(wrapper), "--edition=2021", "--crate-type=lib", "--emit=obj",
                 "-Zunstable-options", "--target", str(target),
                 "--extern", "core=" + str(core), "--extern", "libc=" + str(libc),
                 "-Ldependency=" + str(deps), "-Ldependency=" + str(libc.parent)]
    # Probe optional capability by compilation against the actual rlib. A missing
    # Rust constant and a missing native constant means unsupported on both sides.
    optional = output / "optional.rs"
    optional.write_text(HEADER + '#![no_std]\n#![feature(rustc_private)]\n'
                        'extern crate libc;\npub const FLAG: i32 = libc::MSG_NOSIGNAL;\n')
    try:
        command(rustflags + [str(optional), "-o", str(output / "optional.o")],
                output, "optional", env)
    except RuntimeError:
        diagnostic = (output / "optional.log").read_text()
        if ("cannot find value `MSG_NOSIGNAL` in crate `libc`" not in diagnostic or
                "aborting due to 1 previous error" not in diagnostic):
            raise
        rust = rust.replace("(libc::MSG_NOSIGNAL) as u64", "u64::MAX")
        (output / "rust.rs").write_text(rust)
    cflags = [str(tools / "bin/clang"), "--target=aarch64-unknown-aros", "-std=c11",
              "-D_BSD_SOURCE=1", "-I" + str(sdk / "include/aros/posixc"),
              "-I" + str(sdk / "include/aros/stdc"), "-I" + str(sdk / "include")]
    deptext = command(cflags + ["-M", str(output / "native.c")], output, "dependencies")
    headers = shlex.split(deptext.replace("\\\n", " ").split(":", 1)[1])
    inputs.update({str(Path(p).resolve()): digest(Path(p)) for p in headers})
    for tool in (tools / "bin/clang", tools / "bin/llvm-objcopy"):
        inputs[str(tool)] = digest(tool)
    command(cflags + ["-c", str(output / "native.c"), "-o", str(output / "native.o")],
            output, "native")
    command(rustflags + [str(output / "rust.rs"), "-o", str(output / "rust.o")],
            output, "rust", env)
    values = {}
    for language in ("native", "rust"):
        obj = output / (language + ".o")
        header = obj.read_bytes()[:64]
        if (header[:6] != b"\x7fELF\x02\x01" or
                struct.unpack_from("<HH", header, 16) != (1, 183)):
            raise RuntimeError(f"{language} object is not ELF64 LE AArch64 ET_REL")
        raw = output / (language + ".bin")
        command([str(tools / "bin/llvm-objcopy"), "--dump-section",
                 ".aros_abi=" + str(raw), str(obj)], output, language + "-extract")
        blob = raw.read_bytes()
        if len(blob) != len(entries) * 8:
            raise RuntimeError(f"{language} ABI section length {len(blob)} is invalid")
        values[language] = struct.unpack("<" + "Q" * len(entries), blob)
    rows = []
    for entry, n, r in zip(entries, values["native"], values["rust"]):
        absent = entry[1] in CONSTANTS and n == 2**64 - 1
        rust_absent = entry[1] == "MSG_NOSIGNAL" and r == 2**64 - 1
        rows.append({"group": entry[0], "name": entry[1],
                     "native": None if absent else n, "rust": None if rust_absent else r,
                     "equal": (absent and rust_absent) or (not absent and not rust_absent and n == r),
                     **({"capability": "UNSUPPORTED"} if absent and rust_absent else {}),
                     **({"reason": "constant absent from native SDK"} if absent else {})})
    direct = [r for r in rows if r["group"] == "direct" and not r["equal"]]
    bridge = [r for r in rows if r["group"] == "resolver_bridge" and not r["equal"]]
    if any(digest(Path(p)) != sha for p, sha in inputs.items()):
        raise RuntimeError("qualified std inputs changed during qualification")
    report = {"verdict": "FAIL" if direct else "DIRECT_ABI_MATCH_BRIDGE_UNQUALIFIED",
              "direct_abi": "FAIL" if direct else "PASS", "direct_mismatches": direct,
              "resolver_bridge": {"verdict": "CONVERSION_REQUIRED" if bridge else "LAYOUT_MATCH",
                                  "mismatches": bridge, "conversion_execution": "UNTESTED"},
              "native_runtime": "UNTESTED", "inputs": inputs,
              "source_sha256": digest(Path(__file__)), "records": rows}
    (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k not in ("records", "inputs")}, indent=2))
    return 1 if direct else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, ValueError) as error:
        print(error, file=sys.stderr)
        sys.exit(2)
