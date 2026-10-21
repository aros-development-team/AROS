<!-- Copyright (C) 2026 John KNIPPER <code@jkn.me> -->
<!-- Licensed under the AROS Public License (APL) Version 1.1 -->

# Unmodified Rust compiler qualification

This build separates the AROS standard-library changes from compiler target
declarations. The core Rust recipe selects it for the official AArch64 profile. It uses the official `nightly-2026-08-04`
compiler, an external native AArch64 target JSON, and a private source/sysroot
overlay. `integrate.py` prepares and installs this profile for
`tools-crosstools-rust` in a dedicated prefix.

The compiler pin is official nightly 1.99. Other CPU profiles retain the
source-bootstrap 1.98.1 recipe. See the [core recipe](../README.md) for selection,
installation paths and runtime requirements.

## Inputs and provenance

- `aarch64-unknown-aros.json` expresses the integrated Rust 1.98.1 AROS target
  parameters, including Unix-family std selection, large code model, static
  relocation, inline stack probes, aborting panics and native x18 use.
  Automatic GDB script emission is disabled: its weak reference produces GOT
  relocations that LoadSeg rejects. Ordinary DWARF debug information is retained.
- `rust-src-aros.diff` contains only library changes. It derives from the
  adjacent `rustc-1.98.1-src-aros.diff`, rebased onto the pinned nightly's
  rust-src. Its libc paths refer to `library/vendor` in the rust-src component.
  It contains no `compiler/`, `src/bootstrap/` or `src/librustdoc/` changes.
- `source-files.json` checks the preimages of the 31 affected files, including
  absence of added files. It detects use of the already-patched shared
  `rust-aros` source. It does not authenticate the complete source distribution.
- `probe.rs` retains std filesystem, thread, socket and process paths in the
  static archive. It is a compile/link test, not a runtime acceptance test.

The standard-library sources retain their Rust upstream licenses. The AROS
library changes retain the provenance of the existing AROS toolchain patch.

## Build

Install the pinned official compiler and obtain its matching stock rust-src
component. Pass the root that contains `library/` explicitly; the shared nightly
sysroot may already point at a modified `rust-aros` tree.

```sh
python3 tools/crosstools/rust/unmodified/build.py \
  --rust-src /path/to/stock-rust-src \
  --output /path/to/new-output
```

The output path must not exist. The build copies the source library, applies
the library-only patch without fuzz, uses the copied vendor directory offline,
and builds std plus the probe archive. The official compiler and host libraries
are read-only inputs. The source and target caches belong to the output path.
`RUSTC_BOOTSTRAP` and external Rust compiler wrappers/flags are removed from the
build environment; nightly provides the unstable build-std and JSON interfaces.

To link an AROS command, add all three native SDK/tool inputs:

```sh
python3 tools/crosstools/rust/unmodified/build.py \
  --rust-src /path/to/stock-rust-src \
  --output /path/to/new-output \
  --sdk /path/to/Developer \
  --crosstools /path/to/aros-crosstools \
  --collect-aros /path/to/collect-aros
```

The link does not use `--allow-multiple-definition`. `result.json` records the
compiler, target, patch and product hashes, compilation/link results, and the
explicit `UNTESTED` native runtime state. Logs and exact Cargo/link commands
accompany it. No deployment or execution is performed.

## Scope

AROS is a distinct operating system. The Unix-family cfg in this experiment
selects the integrated port's shared std implementation; it does not identify
AROS as Linux or establish support for every POSIX interface. Retain shared
code only where AROS provides its actual contract, and use AROS-specific
implementations for its libraries, handles, paths and process model.

The network adapter uses AROS's native resolver vectors with an explicit
LP64-to-Rust addrinfo conversion, and native owned sockets through vector 139.
`build.py` compiles these adapters and copies the new native header into its
private output; it never installs a header or library into the SDK. Runtime
requires a matching AROSTCP provider and POSIX descriptor dispatcher, described
in `developer/debug/test/native-socket/README.md` at the core root. Linking
against an SDK alone does not install or qualify those runtime components.

Socket duplication and nonblocking mode use AROS operations. Connection timeout
uses single-socket native readiness followed by `SO_ERROR`, without depending
on Linux poll event semantics. IPv4 multicast TTL/loop options use the native
one-byte payload. Unsupported socketpair and message/ancillary transport report
explicit errors. Socket-only batch poll uses native operation 17 with a single
wait context and deadline; POSIX files, DOS pipes and legacy owners return
EOPNOTSUPP. General POSIX readiness and pipe wakers remain unsupported. See
`developer/debug/test/native-poll/README.md` at the core root for the exact
ERR/HUP/invalid-fd and concurrent-close contract. Process execution retains its
unsupported implementation. `FD_CLOEXEC` is stored metadata; an execution backend
must consume it before inheritance can be qualified. Native `dup2` replacement
and enabled `SO_LINGER` are outside the owned socket contract.

The ABI gate in `ABI/` compares the actual SDK and produced Rust libc objects.
`net/tests/` exercises the resolver conversion and `net/tests/socket/` exercises
the actual socket bridge and vector adapter under host sanitizers. The core's
native-socket fixture separately exercises descriptor retirement and per-call
context admission. These checks do not execute the native TCP/IP stack.

## Application consumption

The private sysroot contains the compiled target std rlibs. An SDK build also
provides `libaros_rs_platform.a`, the `aarch64-aros-cc` linker driver and a generated
`cargo-config.toml` that selects this driver and the SDK's native library path.
The platform archive contains one provider for unsupported POSIX calls as well
as the network/file adapters. Both std and application libc declare the same
explicit adapter symbols; neither exports duplicate C fallback bodies.
The driver links the AROS adapters through the real target C toolchain; it adds
no host-library or Linux fallback.

Applications can use the precompiled std without `-Zbuild-std`:

```sh
RUSTC=/path/to/output/rustc cargo +nightly-2026-08-04 \
  --config /path/to/output/cargo-config.toml \
  build --target /path/to/output/aarch64-unknown-aros.json -Zjson-target-spec
```

The positioned-I/O adapter calls the native POSIX library through its normal
stack-call import stubs. Runtime revision 0.41 provides atomic `pread64` and
`pwrite64` under the shared descriptor's fcb lock. Older runtimes report ENOSYS;
there is no unlocked Rust seek/read/write fallback. The file handler controls
seek and offset limits; unsupported gaps are not emulated as sparse files.

When qualifying against a read-only SDK that predates the new exports, pass
`--posix-stubs /path/to/native-socket-posix-check/libaros_positional.a` to the
build. The build copies that privately generated import archive into its own
output, fingerprints it, and links it with the platform adapter. A normally
rebuilt SDK supplies the same imports in its native POSIX import library.

An application that uses libc directly also needs AROS-patched bindings. The
default qualification uses libc 0.2.189 from the private source overlay, whose
version matches the std lockfile. The native application store's 0.2.190 uses
the same native declarations and aliases, with its required f! macro spelling. A JSON target and its std rlibs have one identity: changing a
target option requires rebuilding std. The external official compiler and host
libraries remain read-only dependencies of this local qualification output.

`consumer-check.py --std-build /path/to/output --output /path/to/new-consumer`
builds a normal debug-profile Cargo binary with this precompiled std and a
direct libc dependency. It checks the native ELF format, unresolved symbols,
absence of allocated GOT relocations and unchanged input identities. Its
runtime probe expects the arguments `alpha beta`; success requires its marker
and a clean supervised return. It validates argc/argv, native UDP/options,
application libc calls, a real empty native poll and explicit EOPNOTSUPP for
regular-file poll. `poll-probe.rs` adds actual socket batch readiness diagnostics.
It requires and prints the actual POSIX runtime revision. Its trace and panic
messages use the native serial debug path independently of redirected logs.
It also verifies `F_DUPFD`/`F_DUPFD_CLOEXEC` flags and minimum descriptor numbers,
and RAM: positioned reads/writes, EOF without growth, the shared
cursor after a cloned-descriptor write, and 256 positioned reads across two
threads. Its file marker is `[RUSTFILE] result=PASS scope=ram-positioned-io
concurrent-reads=256` on one line. This does not qualify physical filesystems,
large native files, or concurrent descriptor closure.
Compilation alone leaves its runtime state UNTESTED. Pass
`--install-prefix /path/to/installed-prefix` to exercise installed wrappers and
Cargo's configured JSON target without supplying target flags manually. It
checks the installed generation's external SDK/tool hashes before and after
compilation, so a shared SDK rebuild cannot silently change the qualified inputs.
`--application-libc /path/to/patched-libc-0.2.190` qualifies the application
store alongside std's internal 0.2.189. The probe also checks zero/NULL error
sentinels and ENOSYS for unsupported interface-name lookup and mkdtemp.

## Runtime diagnostic

Add `--probe network` to the SDK build command to produce `RustNetworkProbe`.
It requires a running matching AROSTCP stack with IPv4 loopback configured.
Its resolver check calls the conversion directly with a numeric address;
TCP/UDP checks cover creator-thread exit, duplication, real payload round trips,
connect timeout/refused connection, nonblocking receive and multicast option
widths. Socket I/O uses three-second deadlines; the guest harness must also
bound execution because thread creation or stack entry can fail before an I/O
deadline applies.

A successful command returns zero and emits `[RUSTNET] result=PASS
scope=ipv4-loopback` on one line. Acceptance also requires the supervising
harness to observe clean termination and no kernel faults. This diagnostic
covers IPv4 loopback and the connect-with-deadline API, without forcing a
deadline to expire. External routing, hostname DNS, IPv6, deadline expiry,
cancellation and
concurrent close/reuse require separate native tests. Compiling the same source
for a host checks test logic while excluding the AROS resolver bridge; it is
not AROS evidence.

Installed std libraries retain LLVM bitcode for application thin/fat LTO.
`consumer-check.py --lto fat` qualifies that link against the precompiled
libraries. Installed Cargo flags carry the SDK generation identity, so Cargo
rebuilds target dependencies after a custom sysroot changes even when the
official compiler version stays the same.

The native recipe uses `integrate.py`; its official component URLs and hashes
are pinned in `channel.json`. It consumes the configured C driver and native
SDK; top-level `crosstools-toolchain` prepares the C tools when required.
Selecting official Rust does not force an LLVM source rebuild over installed
C tools. The source-bootstrap Rust profile retains its toolchain prerequisites.
It can also consume read-only `--host-root` and
`--rust-src` inputs, or install an existing matching `--std-build` for local
qualification. Default preparation needs 5 GiB free before building; installing
already-built artifacts with existing host tools needs 32 MiB. Output and work
trees are separate from the source SDK and official compiler. Other CPU targets
and the hosted x18 profile require separate qualification.

`sys/test.py --output /path/to/new-output` exercises the actual refusal provider
under host ASan/UBSan, checking typed failure sentinels, unchanged output buffers
and thread-local errno. Native errno behavior remains a guest-test requirement.
`ABI/libc-exports.py` extracts and links all ELF members of std/application libc
explicitly, rejecting duplicate C providers independently of archive order. Its
optional `--binary` checks that the application contains one native provider.
