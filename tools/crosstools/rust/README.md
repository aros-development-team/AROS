# Rust cross compiler for AROS

`configure --enable-rust` selects an official, unmodified Rust compiler for
AArch64 with the LLVM C toolchain. The default pin is
`nightly-2026-08-04` (Rust 1.99). AROS support lives in an external JSON target,
a library-only std/libc patch and native C adapters. The compiler, bootstrap
and rustdoc sources are not patched.

AROS is a distinct operating system. The Unix-family cfg selects shared std
implementations where their contracts fit AROS; it does not identify AROS as
Linux. Networking uses AROS libraries and descriptor ownership, and file I/O
uses native DOS handles. There is no macOS or hosted-service fallback.

## Native AArch64 recipe

`tools-crosstools-rust` prepares official host `rustc`, host `rust-std`, Cargo
and stock `rust-src` components. [The channel manifest](unmodified/channel.json)
pins their archive hashes and compiler commit. The recipe requires Python
3.12 or newer for filtered archive extraction. It does not require rustup.
Downloads and extracted components belong to the build's private host work
directory, outside the compiler and source SDK inputs.

The [native builder](unmodified/README.md) copies stock library sources, applies
the AROS library patch, builds std offline with the official compiler, and
installs the resulting target rlibs in a private sysroot. It compiles native
platform adapters and links a diagnostic. Only the libraries are rebuilt.
Generation keys include compiler/Cargo identities, patch and adapter inputs,
and SDK headers and libraries. Verified target rlibs and the retained probe
archive are the products; unused per-generation Cargo intermediates are removed. Compilation and installation do not qualify
runtime execution.

The dedicated installation prefix is
`$(CROSSTOOLSDIR)/rust-nightly-2026-08-04-aarch64`. It contains `rustc`, `rustdoc`
and Cargo wrappers, the native C linker driver, a JSON target and Cargo config.
Wrappers refer to the prepared official host tools and private std generation;
they do not overwrite the existing C toolchain. Keep the host work directory
with the installation. `aros-official-rust.json` records the generation and
input/output hashes.

Official component pins are provided for macOS AArch64/x86_64, Linux GNU
AArch64/x86_64 and FreeBSD x86_64 hosts. The exercised host is macOS AArch64;
the other download profiles require their own build qualification. The target
qualification is generic AArch64 QEMU with one CPU, IPv4 loopback and RAM files.
It makes no Apple hardware, SMP or physical-filesystem claim.

`tools-crosstools-rust-bindgen` and `tools-crosstools-rust-cbindgen` install the
pinned crates.io tools in the same dedicated prefix. LLVM's libclang remains
the C toolchain's dependency. `tools-crosstools-rust-check` builds a std program
and rejects undefined symbols in the linked AROS command.

## Build consumers

Direct rustc consumers use `$(TARGET_RUSTC) $(TARGET_RUSTFLAGS)`; this supplies
the JSON target, native linker and aborting panics. Cargo consumers use
`$(RUST_CARGO_TARGET_FLAGS)` after the build subcommand, or the generated
`%rust_cargo_config`, which selects the JSON target and enables Cargo's nightly
JSON-target interface. `RUST_TARGET_TRIPLE` is an identifier for target tables
and output directories; it is not a built-in rustc target in this profile.
`$(RUST_CARGO)` loads the installed Cargo config as well as the crate-store
config, carrying the SDK generation into Cargo dependency fingerprints.
Program-specific generated configs add their path remapping to those target
flags. Loading the installed Cargo config selects AROS for all Cargo commands
that load it. Run host tests without that config, or override `--target` with the
host triple; host build scripts/proc macros are still compiled for the host.

The shared native crate store is `$(PORTSDIR)/rust-crates-native`. Its libc
0.2.190 patch uses the same AROS declarations and native adapter aliases as
std's locked libc 0.2.189, with the macro spelling required by each release.
Both libc versions contain FFI declarations for unavailable calls, rather than
exporting replacement C bodies. Their shared `aros_rs_*` provider belongs to
the native platform archive and reports ENOSYS with typed failure sentinels.
This avoids duplicate C definitions and reliance on archive extraction order.
The selected profile has one libc patch and a separate source-store directory.
This avoids reusing a legacy patched tree after reconfiguration. Network and
positioned-file operations resolve to the platform archive, rather than the
legacy network ENOSYS definitions or successful no-op memory advice.

AROS executables are relocatable ELF objects. The target uses static
relocations, a large code model, pthread-key TLS and ordinary native x18.
Automatic GDB script emission is disabled because its weak reference produces
GOT relocations rejected by LoadSeg; ordinary DWARF remains available. Do not
fully strip symbols needed by the AROS loader. Hosted x18 reservation belongs
to a separate target profile.

## Runtime requirements and limits

The native profile requires the matching owned-socket interface in
`bsdsocket.library` (vector 139) and its coherent POSIX descriptor dispatcher.
See [native socket ownership](../../../developer/debug/test/native-socket/README.md).

Atomic `FileExt::read_at`/`write_at` require posixc.library 0.41. The fcb semaphore
covers capture, seek, transfer and restoration across duplicated descriptors.
The native file handler controls offset and gap support. `pwrite` uses the
requested offset even on an O_APPEND descriptor. An older library returns
ENOSYS from the Rust adapter; there is no unlocked seek fallback.
Replace the POSIX **boot component** when composing a test or installed bundle:
a filesystem copy under `Libs/` cannot replace an already resident module.

Socket duplication, nonblocking mode and single-socket readiness use native
operations. Numeric resolver conversion, TCP/UDP loopback and RAM positioned
I/O have guest diagnostics. Socket-only batch poll has native operation 17;
its admission, event masks and shared wait engine have a coupled host fixture
in `developer/debug/test/native-poll`. Valid regular files, DOS pipes and legacy
owners report EOPNOTSUPP, so general POSIX poll and pipe wakers remain outside
that contract. Process execution, Unix socket pairs and message/ancillary
transport remain explicitly unavailable. FD_CLOEXEC
metadata does not establish qualified process inheritance. Missing interface
name lookup returns zero with ENOSYS; unsupported mkdtemp returns NULL with
ENOSYS and does not modify its template.

Routing, hostname DNS, IPv6, deadline expiry/cancellation, concurrent descriptor
closure, physical filesystems and native SMP require separate qualification.
These limits constrain application capabilities; they must not be hidden by
host services or fake success results.

## Other compiler profiles

Other CPU/C-toolchain configurations retain the source-bootstrap Rust 1.98.1
recipe and its existing patched built-in targets. That route fetches the source
release plus binary stage0 components, applies `rustc-1.98.1-src-aros.diff`, and
runs `x.py build/install --stage 1`. Its documented builds cover x86_64 and
riscv64; it does not inherit native AArch64 runtime qualification. Its libc
store and network/process limitations remain distinct from the native profile.
A new compiler pin needs its source patch or external target and an explicit
MetaMake backend entry. Reconfigure existing builds to select the native
nightly profile; an explicit `--with-rust-version=1.98.1` retains that recipe.
