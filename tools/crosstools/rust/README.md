# Rust cross compiler for AROS

`tools-crosstools-rust` builds a host `rustc` (plus `cargo` and, on request,
`bindgen`) with a Rust standard library for the AROS target of the current
build, and installs it into `$(CROSSTOOLSDIR)/bin`. It is enabled with
`configure --enable-rust` (`--with-rust-version=` picks the release, see
`../rust.cfg`) and works with both the gnu and the llvm AROS toolchains: rustc
only ever reaches the target C driver through the
`$(CROSSTOOLSDIR)/bin/<cpu>-aros-cc` wrapper written here.

## How it is built

* The upstream source release `rustc-<ver>-src.tar.xz` is fetched from
  static.rust-lang.org with the AROS fetch macro and patched with
  `rustc-<ver>-src-aros.diff`.
* A binary release of the same version (`rustc`, `rust-std`, `cargo` for the
  build machine) is fetched the same way and used as the stage0 compiler:
  rust's bootstrap accepts the same or the previous minor release, so no
  network access beyond the fetches is needed (the source tarball vendors
  every crate).
* `x.py build --stage 1 compiler library` then `x.py install --stage 1`, with
  a generated `bootstrap.toml`. LLVM is built from the bundled sources with
  only the X86 and the target CPU backends.
* `tools-crosstools-rust-bindgen` and `tools-crosstools-rust-cbindgen` install
  bindgen-cli and cbindgen from crates.io with the freshly installed cargo
  (these need network access, and bindgen needs a libclang at run time: the
  llvm toolchain provides one, with the gnu toolchain the host's is used - see
  `RUST_LIBCLANG_PATH` in `../rust.cfg`, or point `LIBCLANG_PATH` at one).
* `workbench/libs/mesa/libnak` uses all of this to build mesa's NAK and NIL
  (the Rust parts of the NVK Vulkan driver) as AROS link libraries.
* `tools-crosstools-rust-check` links a std program for the target and fails
  if the AROS executable has undefined symbols.

`crosstools-rust-create-patch` regenerates the diff from a patched source
tree, as for the other crosstools.

## What the patch adds

* Built-in targets `x86_64-unknown-aros`, `i686-unknown-aros`,
  `arm-unknown-aros-eabihf`, `aarch64-unknown-aros`, `riscv64gc-unknown-aros`,
  `m68k-unknown-aros` and `powerpc-unknown-aros`
  (`compiler/rustc_target/src/spec/base/aros.rs`). AROS executables are
  relocatable ELF objects, so: static relocation model, no PIC/PIE, no ELF
  TLS (`thread_local!` uses pthread keys), `panic = abort` with no unwinder,
  `-Wl,--no-gc-sections` after rustc's own flags, and `crt-static` is not
  claimed (the AROS driver drops posixc.library's link library when it sees
  `-static`). x86_64 uses the large code model and riscv64 the medium one,
  like the C toolchain. Only x86_64 and riscv64 have been built here.
* A `libc` crate module for AROS (`vendor/libc-0.2.185/src/unix/aros`):
  types and structures from the SDK headers, constants generated from them
  with the target compiler, and, with their C symbol names, ENOSYS stubs for
  the POSIX calls the AROS C library does not have (sockets, fork, poll,
  pread/pwrite, mmap, ...) so that std's shared unix code links.
* std as a unix-family target: errno through `__stdc_geterrnoptr()`, the
  environment through `__stdcio_get_environptr()`, `arc4random_buf` for
  randomness, `pthread_setname_np` for thread names, `T:` as the temporary
  directory, argv[0] as the current executable, positional reads emulated
  with lseek, and the platform lists of `sys/fs` and `sys/fd` extended for
  what the AROS C library lacks (`readdir_r`, `futimens`, vectored I/O, ...).

## Known limitations

* `std::process::Command` and `std::net` compile but fail at run time
  (there is no `fork`, and sockets live in bsdsocket.library behind its
  library base).
* Binaries carry std and must not be fully stripped (the AROS loader needs
  the relocations' symbols).
* Verified on hosted AROS x86_64: threads, channels, mutexes, time, HashMap,
  files and directories.
