<!-- Copyright (C) 2026 John KNIPPER <code@jkn.me>. -->
<!-- Licensed under the AROS Public License (APL), Version 1.1. -->

# Native owned socket contract

`bsdsocket/native.h` defines an AROS interface for sockets whose descriptors
survive their creating task and the short-lived library opener used for a call.
The caller opens its own bsdsocket base, calls vector 139 with the versioned
request and exact structure size, and closes that base after return. The native
Roadshow vector table supplies this extension. Its result is a native errno
value; zero means that `request.result` is valid.

The stack keeps the socket in `fd.library` as `FD_OWNER_NATIVE_SOCKET`, with
one reservation identity per descriptor and one shared object for duplicates.
It does not put that socket in a task's persistent descriptor table, use socket
number tags, retain a creator Task, or borrow another task's sleep state.
Each blocking operation opens a private caller context, attaches one private
slot for the existing internal stack operations, and removes that slot before
closing the context. A native integer of any registry-supported magnitude maps
to private slot zero for readiness; it is never inserted into a caller's fd_set.

Operation admission retains the exact fd reservation and provider hook table
through `FD_Acquire`/`FD_Release`. Close retires that identity, so its number can
be reused immediately while admitted operations finish on the original object.
Only its last retirement callback drops the final socket reference. Descriptor
clones have independent flag storage and share the socket's nonblocking state.
POSIX read/write/close/ioctl/fcntl/dup hooks retain a version-51 fd.library
reference and an operation lease across dispatch. Legacy task-owned BSD sockets
remain a separate owner; their provider teardown must also adopt retirement
before the same concurrent-close guarantee can apply to them.

The shared socket holds the master library reference until destruction. API
withdrawal refuses live owned objects and admitted operations, and a refused
legacy-provider withdrawal restores native admission. A caller must explicitly
close every descriptor or arrange RAII cleanup; abnormal program termination
cleanup is not implemented by this interface. Close admission can fail under
memory pressure when a new fd lease cannot be allocated. This returns an error
and keeps the descriptor live, permitting an explicit caller retry.

TCP/UDP creation, bind/listen/accept/connect, stream/datagram I/O, shutdown,
options, local/peer addresses, clone/close, nonblocking mode, and single-socket
native WaitSelect readiness use the existing AROS stack. The `POLL_EXCEPTION`
bit denotes exceptional/urgent readiness, not an error code. Connection
completion requires `GETOPT(SO_ERROR)` after writable readiness. Counts above
`INT_MAX` are rejected before narrowing. Enabled `SO_LINGER` is refused because
the deferred finalizer cannot retain a caller's sleep context. Batch readiness,
ancillary message transport, socketpair, and execution/inheritance policy are
separate work. Native `dup2` into an occupied native reservation returns
`EBUSY`; replacing a POSIX-owned destination returns `EOPNOTSUPP` without
closing it. Atomic replacement across owners requires a registry transaction.
`FD_CLOEXEC` storage is descriptor metadata for a process adapter
to consume; it does not implement a process execution contract by itself.

The helper refactors in `amiga_syscalls.c` and `amiga_sendrecv.c` preserve the
legacy vector signatures and implementation. The accept helper's detached mode
returns the accepted socket directly, before any legacy reservation is exposed,
and clears its task owner. Native calls allocate their own registry reservation.

# Verification

The host fixture compiles the production native adapter, POSIX dispatcher and
fd registry with modeled Exec and transport primitives. It checks creator-thread
exit, blocked read/close/number reuse, duplicate identities and flags, concurrent
clones, refused withdrawal, missing hook registration, context/allocation
failures, count bounds and reference balance under ASan/UBSan. It also executes the
production ioctl wrapper and context admission function, including the maximum
library open count and allocation/timer rollback. It does not
execute the TCP/IP stack or qualify native scheduling/network behavior.

```sh
python3 developer/debug/test/native-socket/test.py --output /path/to/new-host-output
```

The object gate uses the actual native SMP SDK and generated library headers,
compiles every changed C unit into private output, and records commands/hashes.
It never invokes Make or installs generated headers into the SDK.

```sh
python3 developer/debug/test/native-socket/cross-check.py \
  --sdk /path/to/AROS/Developer --generated /path/to/target/gen \
  --crosstools /path/to/aros-crosstools --output /path/to/new-object-output
```

`provider-check.py` takes the same inputs plus `--collect-aros /path/to/collect-aros`.
It compiles all native provider sources from `mmakefile.src`, links the AROS
command against the supplied SDK libraries, and checks unresolved symbols.
Source and object hashes accompany the exact compile/link commands. Its logs
preserve pre-existing warnings in the legacy stack. It qualifies cross-linking,
not deployment or runtime behavior. Native acceptance requires creator exit,
parallel I/O/close/reuse, cancellation, DNS, routing, UDP/IPv6, option widths and
stack withdrawal under the actual target scheduler and stack.

`posix-check.py` builds the complete POSIX library with the same SDK/tool inputs
and an additional `--genmodule /path/to/current/genmodule`. It regenerates module
sources and headers in private output from the core's current configuration.
It rejects a generator without the per-task standard-handle identity guard;
older generated bases can reuse an earlier CLI command's closed handles.
The SDK must expose the matching libinit sets ABI; the runner refuses older
symbolsets headers before creating output. A compiler-object check against an
older generated module is insufficient to
qualify the resulting library. SDK/source headers, static libraries, module
configuration and tool identities are retained and checked for changes during
this complete build. No shared SDK is updated.

The complete build also emits private `pwrite`/`pwrite64` import stubs for a
read-only SDK predating POSIX revision 0.41. The file-operation host fixtures
and Rust guest consumer are described in [native positioned I/O](../native-file-io/README.md).
When testing a replacement POSIX library, replace its boot bundle component;
a filesystem copy under `Libs/` cannot override an already resident module.
