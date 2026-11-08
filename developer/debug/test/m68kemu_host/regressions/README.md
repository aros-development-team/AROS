# Focused host regressions

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-memory-bounds
make -C developer/debug/test/m68kemu_host test-memory-bounds-sanitize
```

The ordinary bounds regression also runs with the host Makefile's `test`
target. The sanitizer target is optional. `PYTHON` and `CXX` select the
interpreter and compiler; `MEMORY_BOUNDS_ARGS` passes runner options, such as
`--build-dir /tmp/m68kemu-memory-bounds`, to either target. The runner can
also be invoked directly as `python3 regressions/run_memory_bounds.py` from
the host test directory.

These tests require Python 3.9+ and a C++17 compiler, with ASan/UBSan for
`--sanitize`. `--cxx` selects the compiler; `--source-root` tests another
AROS source tree. By default, generated files and executables use a temporary
directory that is removed afterwards; `--build-dir` retains them for inspection.

The runner compiles four accessors from `m68kemu_intern.h` and four memory
callbacks from `m68kemu_moira.cpp`, extracted verbatim. The fixture supplies
only the context fields they use, CPU method declarations and recording
custom-register/guard collaborators. It does not copy their implementations.
Guest `ULONG` addresses are explicitly 32-bit; the range oracle uses a
64-bit sum independently of the production checks.

The corpus covers zero/tiny arenas, the last complete word/longword,
partial accesses, the arena end and wrapping addresses near `UINT32_MAX`.
Rejected writes must leave all bytes unchanged. Special-address controls
preserve custom-register dispatch, return sentinels and, where present,
the low-vector guard and nested callback sentinel.

No AROS SDK, NDK, application binary or Moira submodule is needed for these
function-level host tests. They do not constitute a full library build,
CPU instruction test or AROS target acceptance. Leak detection is disabled
by default for sanitized runs; it can be configured with `ASAN_OPTIONS`.

## Classic private MemHeader regression

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-memheader
make -C developer/debug/test/m68kemu_host test-memheader-sanitize
```

The ordinary regression is also a prerequisite of the host Makefile's
`test` target. It needs Python 3.9+ and a C99 compiler; the optional sanitizer
target needs ASan/UBSan. `PYTHON` and `CC` select those tools, and
`MEMHEADER_ARGS` passes runner options such as `--build-dir /tmp/memheader`.

The existing absolute source/include paths also allow focused targets to run
from another directory with `make -f /path/to/m68kemu_host/Makefile`.
A passing focused regression is separate from a passing complete host
suite: these fixtures do not supply the general native API/CPU stub set
used by the three older C++ binaries.

The runner extracts the complete classic `Allocate()`/`Deallocate()` handler
block, the 32-bit accessors and shared low-write policy verbatim from production sources. It obtains
the classic field offsets from `m68kemu_offsets.h`. The C fixture supplies
only the context fields, guest registers and low-write trap collaborator
those functions use. No SDK, CPU core, application binary or C++ runtime
is required. Guest `ULONG` values are explicitly 32-bit.

The tests cover first-fit allocation, rounding, exact removal and splits,
ordinary failure, coalescence on both sides and outward rounding of unaligned
frees. Malformed inputs include partial or wrapped headers and spans,
misaligned chunks, zero or invalid sizes, cycles, descending or overlapping
lists, double frees, understated `mh_Free`, late corruption after an early
fit, and protected low-vector metadata writes. Every refusal must preserve
the complete arena without even attempting a discarded low-vector write.
A caller may overstate `mh_Free`, including on an empty list. Successful
operations retain that surplus; an understated count or overflow on free
is refused. Allocation and free find and retain their neighbours during
one complete validation scan, so neither searches the list a second time.
The scan still costs O(n), even for an early fit: checking a possibly
corrupt tail before mutation is intentional. This is not a timing claim.
A fragmented 1,024-node fixture records production 32-bit reads and bounds
them to two per node plus constant overhead, for both calls.
The memory-bounds regression independently checks the shared write rule
at every start address around the protected low-memory region.

An independent bitmap/interval oracle checks 12,000 deterministic randomized
operations. A valid list with more than one million chunks verifies that
traversal is bounded by arena capacity rather than an arbitrary node limit.

The native Exec private-pool example in `rom/exec/allocate.c` initializes
`mh_First` and `mh_Free` without initializing `mh_Lower` or `mh_Upper`.
The handlers therefore validate the complete actual chunk spans in the
guest arena and a free-byte sum no larger than `mh_Free`, without requiring those bound
fields, node type or attributes. They preserve the classic eight-byte
chunk alignment and `Deallocate()`'s outward rounding documented in
`rom/exec/deallocate.c` and implemented by `stdAlloc()`/`stdDealloc()` in
`rom/exec/memory.c`. Adjacent input chunks are accepted. The bridge can
reject overlap with an existing free chunk, but a private caller remains
responsible for freeing only memory it owns; this change adds no allocation
ownership ledger. All metadata is validated before committing any write.

The separate global heap still rounds allocations to four bytes
(`M68KEmu_HeapAlloc()` in `m68kemu_memory.c`); this change preserves the
private handlers' eight-byte contract and does not fix that global policy.
A caller must align private pool chunks to eight bytes. A four-byte-aligned
chunk could previously be allocated unchanged and then rounded down on
free, overwriting bytes before that chunk. The validated handlers refuse
such a malformed list before changing it. Repairing global heap alignment
is a separate compatibility change.

To test a frozen earlier implementation without changing the checkout:

```sh
python3 developer/debug/test/m68kemu_host/regressions/run_memheader.py \
  --thunks /path/to/earlier/m68kemu_thunks.c
```

To run the exact C fixture on another target, prepare its production includes
without compiling or executing a host binary:

```sh
python3 developer/debug/test/m68kemu_host/regressions/run_memheader.py \
  --prepare-only --build-dir /tmp/memheader-target
```

Compile `regressions/test_memheader.c` with that directory on the target C
compiler's include path. It uses only the C library and needs about 17 MB
for its largest case. These function-level checks are separate from a full
library build, instruction emulation and application or AROS acceptance.
Sanitized runs default to disabling leak detection; `ASAN_OPTIONS` can
override that choice.
