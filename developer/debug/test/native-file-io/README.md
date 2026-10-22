<!-- Copyright (C) 2026 John KNIPPER <code@jkn.me>. -->
<!-- Licensed under the AROS Public License (APL), Version 1.1. -->

# Native positioned I/O qualification

`positioned-io.py --output /absolute/new/output` compiles the production
`pread64.c`, `pwrite64.c` and Rust native file adapter into host fixtures with
AddressSanitizer and UndefinedBehaviorSanitizer. DOS primitives are modeled;
this is not native runtime acceptance.

The fixtures cover validation, access modes, directory rejection, zero-length
operations, EOF without growth, 64-bit offsets, transfer and restoration
failures, first-error preservation and count limits. Parallel reads share one
fcb. A gated seek checks that ordinary I/O cannot acquire the fcb lock during
the positioned read or write; its cursor must be restored before release.
The descriptor fixture exercises the actual duplicate helper with modeled
registry/allocator operations, including rollback and 2,048 concurrent
duplicate/release cycles. The adapter fixture verifies that missing and pre-0.41 libraries return ENOSYS
without calling a native vector, and that supported versions preserve native
arguments and errors. It records consumed source hashes and exact commands.

`../native-socket/posix-check.py` builds the whole matching native POSIX library
and private import stubs. `tools/crosstools/rust/unmodified/consumer-check.py`
at the core root builds the actual Rust FileExt consumer for guest execution.
The guest requires posixc.library 0.41. The shared fcb lock covers POSIX/stdio
access through that fcb; independent raw DOS users and concurrent descriptor
closure are outside this qualification. Native file-handler seek limitations
are returned directly, without sparse-file or gap emulation.

The QEMU bundle must replace the boot component `posixc.library`, as well as
its filesystem copy if one exists. Replacing only `Libs/posixc.library` does
not replace a library already resident from boot. Retain hashes for every
bundle component and require the consumer's actual `[RUSTAPP_RUNTIME]`
version/revision marker before accepting the file diagnostic.
