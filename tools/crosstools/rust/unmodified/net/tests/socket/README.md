# Socket bridge host tests

Run from any working directory with a fresh artifact directory:

```sh
python3 tools/crosstools/rust/unmodified/net/tests/socket/run.py --output /tmp/aros-socket-host
```

The runner locates the native socket request contract in the current core tree
or the sibling `aros-core-rust-native-handles` worktree. `--native-header PATH`
selects another real `bsdsocket/native.h`. The exact header is hashed and copied
into the artifact before compilation. Both actual production sources,
`net/socket.c` and `net/socket_aros.c`, compile in separate executables.

The test headers provide AROS byte-sized sockaddr length/family, IPv4/IPv6
records, `AF_INET6=28`, 128-byte aligned sockaddr storage, and Exec's 32-bit
LONG/ULONG with pointer-sized IPTR/SIPTR. Static assertions check the actual
native request's LP64 record size and offsets. Host socket headers are excluded.
The mock library structure and library-call macros exercise adapter control
flow; they do not establish the real library or AArch64 vector calling ABI.

The bridge fixture inspects every request scalar, pointer and length during
the synchronous dispatcher call. It checks all 15 initial operation codes with
success and returned errno, IPv4/IPv6 destination copies with normalized
`sa_len` and unchanged caller storage, invalid address family/length/null
arguments, partial I/O, output address propagation, getsockopt length overflow
and failure preservation, clone/close and boolean nonblocking conversion.
The dispatcher does not retain request or stack address pointers.

The writable-wait fixture checks the native POLL operation with only the WRITE
mask, four timeout values, all eight result mask combinations, and returned
EINVAL/EINTR/EBADF. This is a single-descriptor readiness operation, not generic
POSIX poll. Unsupported socketpair and message-I/O operations return the exact
capability errors without dispatch or mutation of descriptors, message records,
address storage or control buffers, including null-pointer arguments.

The adapter fixture checks library name/version, absence, the vector table
size boundary, exact vector number and request size, success and error return
values, and one close per successful open. Close deliberately changes errno
to verify that the native error returned by value survives cleanup.

Clang ASan and UBSan report fatal diagnostics. Darwin's unsupported
LeakSanitizer feature is disabled explicitly; other hosts enable it. Logs,
compiler version, exact source snapshots, SHA-256 hashes, and a strict
`result.json` are retained. Sources changing during execution fail the result.

This is host qualification of conversion and adapter control flow. It does
not qualify the native socket dispatcher, descriptor ownership across tasks,
the real vector calling convention, network packets, target runtime, or
hardware. Generic POSIX poll and successful socketpair/message-I/O are not provided by
these production contracts.
