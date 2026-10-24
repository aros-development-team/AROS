<!-- Copyright (C) 2026 John KNIPPER <code@jkn.me>. -->
<!-- Licensed under the AROS Public License (APL), Version 1.1. -->

# Native socket batch poll

AROS owned sockets support batch readiness through operation
`AROS_SOCKET_POLL_BATCH` (17) on the existing native vector 139. The operation
keeps the existing request size and uses `buffer` for a `length`-entry array of
`struct aros_socket_pollfd`: a signed 32-bit descriptor and signed 16-bit
`events` and `revents` fields. Its layout matches Rust libc's `pollfd`, and the
C adapter accepts an unsigned 32-bit `nfds` argument. Each ready array entry
counts once, including duplicate descriptors. Negative descriptors are ignored
and their output is cleared. Invalid descriptors report `POLLNVAL` immediately;
other entries are still scanned with a zero timeout.

One native wait context owns the whole array and a single timeout. The shared
selection engine registers both socket buffers and sleeps through native stack
notifications, maintaining the same deadline over spurious wakes. A zero-entry
array is a real timed or interruptible wait. Timeout zero performs one scan;
`-1` waits indefinitely, other negative values return `EINVAL`. Resource bounds
are checked before allocation. Native task interruption returns `EINTR`.

`POLLIN`, `POLLOUT` and urgent-data `POLLPRI` follow the native socket state.
`POLLERR` inspects the pending socket error without clearing `SO_ERROR` and is
reported even when no events were requested. `POLLHUP` denotes a fully closed
stream; it can coexist with readable EOF but never with `POLLOUT`. A read-only
half-close is readable EOF. A queued zero-length datagram is readable and does
not denote stream hangup. Other requested event bits do not imply readiness.

Each socket reservation is leased throughout the wait. Closing a descriptor
can return while its admitted poll still retains the old socket. Reusing the
number cannot redirect that wait to a different object. Concurrent close does
not cancel an admitted wait: callers use a bounded timeout or task interruption
for cancellation. Provider withdrawal remains excluded until leases finish.

This is a **socket-only poll implementation**. Valid POSIX files, DOS pipes and
legacy descriptor owners return `EOPNOTSUPP`; they are neither falsely marked
invalid nor reported ready. The adapter also distinguishes task-local standard
streams from unknown global numbers. General POSIX regular-file readiness and
pipe-based event-loop wakers need separate native providers. No Linux syscall,
host library, socket number tag, polling loop or successful stub is involved.

The common wait engine keeps legacy select's bitset strides and public entry
point. Its following selection storage is aligned separately on LP64. Timers
are armed after selection-buffer allocation succeeds, so allocation failure
cannot leave a timeout pending.

## Qualification

```sh
python3 developer/debug/test/native-poll/test.py --output /new/host-output
python3 developer/debug/test/native-socket/test.py --output /new/lifetime-output
python3 developer/debug/test/native-socket/cross-check.py \
  --sdk /path/to/Developer --generated /path/to/target/gen \
  --crosstools /path/to/crosstools --output /new/cross-output
```

The poll fixture extracts the actual production scan/admission, selection
structures, common wait loop, legacy `selscan` and `soo_select`. Exec timers,
transport and notification delivery are modeled; host sleeps and deadline
measurements use the monotonic clock. ASan/UBSan cover ownership, close/reuse,
allocation rollback, event masks, error preservation, duplicate/invalid entries,
empty waits, interrupted waits, spurious wakes and legacy select output strides.
It does not qualify guest scheduling or the actual TCP/IP stack. Run
`mutations.py --fixture /host-output/fixture.c --output /new/mutation-output`
to verify that eight separately compiled semantic mutants fail at runtime.

`tools/crosstools/rust/unmodified/ABI/network-abi.py` compares native poll records
with the actual std or application libc rlib, including layouts, scalar widths
and readiness constants. It consumes a private copy of the source native header
(default, or `--native-header`) while leaving the SDK unchanged.

`probe-build.py` builds `RustPollProbe` in fresh private output with the updated
C adapter and a reused precompiled std. It requires `--sdk`, `--std-build`,
`--crosstools` and `--output`. The std and SDK inputs remain read-only. This
binary still requires a matching batch-capable AROSTCP provider at runtime;
link success alone does not qualify it. The probe checks UDP batch readiness,
negative/invalid descriptors, real empty and unrequested-event deadlines,
stream hangup, preservation of a pending loopback UDP error and explicit pipe
refusal. Success requires all six case markers,
its final `AROS_POLL result=PASS cases=6` and a clean supervised return.
