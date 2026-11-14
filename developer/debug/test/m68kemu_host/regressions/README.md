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


## Requested root-stack reservation

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-requested-stack
make -C developer/debug/test/m68kemu_host test-requested-stack-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `REQUESTED_STACK_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/requested-stack`.
Use a fresh build directory; the default temporary directory is removed.

The runner extracts the actual context constructors, eight global heap
functions, memory accessors and task-shadow stack writer. It reads their
constants and the native allocation flags from the same source tree. The
recording C fixture supplies controlled context fields and native allocation,
initialization and task-identity collaborators; guest `ULONG` is 32-bit.

The cases cover the legacy constructor, zero/small requests, 1 MiB, four-byte
rounding, the maximum reservation and wrapping/oversized values, both native
allocation failures, heap accounting and reservation around the CIA/custom
holes and ROM, absolute allocation refusal and the published task bounds.
Requests must be rejected before allocation if no terminal free chunk would
remain above the ROM. The fixtures preserve the stack and ROM sentinels and
check allocation cleanup explicitly. Sanitized runs disable leak detection
by default; `ASAN_OPTIONS` can override it.

`--prepare-only --build-dir /tmp/requested-stack` emits the actual production
includes without compiling or executing a test. These focused checks do not
compile the native launch translation unit, model native struct layouts or
locks, execute the CPU, or qualify a complete library or application.


## Process streams

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-process-streams
make -C developer/debug/test/m68kemu_host test-process-streams-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `PROCESS_STREAMS_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/process-streams`.
Use a fresh build directory; the default temporary directory is removed.
The fixture is built with `-std=c99 -Wall -Wextra -Werror`, so a compiler
warning fails the target; the sanitizer target adds ASan and UBSan without
recovery.

The runner extracts the actual launch function `setup_and_execute`, the
BPTR handle translators with `m68k_bptr_drop`, the Process stream helper
and the selector helper when the tree has them, `m68k_process_of`, the
shadow registry (`shadow_register`, `shadow_lookup`, `shadow_remove`), the
complete initial Process block of the ExecBase shadow, the `Input` and
`Output` thunks, and the thunks registered for `SelectInput` (LVO -294) and
`SelectOutput` (LVO -300): the manual ones when the dos.library table
registers both, else the generated ones. When the tree has it, it also
extracts the handling of a new process at an ended one's native address
(`m68k_process_forget`). It checks in the dispatcher's source that manual
thunks are tried before generated ones. It reads the handle and registry
limits, the Process and CLI offsets and the `NP_` and tag constants from
the same source tree and its public headers.
The offset of `pr_CES` is fixed in the runner and checked against the
generated shadow layout, so it does not come from the helper under test.
The recording C fixture supplies the native task and its streams, the
native selectors, the layouts, the allocations and the tag capture; guest
`ULONG` is 32-bit and native `BPTR` is 64-bit. The `Input` and `Output`
thunks do not use their CPU argument: the fixture switches the
unused-parameter warning off around that one include and nowhere else.

The fixture runs 105 checks in three groups. The 32 launch checks cover a
child on the synchronous route that inherits the caller's 64-bit input,
output and error streams, also when the error stream is NULL or is the
output stream, with the lifecycle tags unchanged and no stream marked for
closing, so that every stream stays borrowed from the caller; a failed
child creation, which returns failure, releases both directory locks and
closes no caller stream; the direct route for a zero stack, with its
startup message, window pointer and directory handling unchanged; and an
early setup failure, which starts neither route. The 32 projection checks
cover the three stream fields of the guest Process as translated handles
that convert back to the exact 64-bit native value: for the initial
Process, after an owner refresh that follows a changed stream, for a new
other Process with its own shadow, and for that Process seen again, which
keeps its guest address. An aliased output and error stream share one
handle, and a plain Task stays supported and exposes no stream field. The
fixture models the documented close tags only; no native child runs.

The 41 ownership checks cover what in the three stream fields is the
program's and what is the module's. A handle, NULL, a copy of another
field's handle and a number that is no handle, each stored by the program
in a field of its own Process, stay through refreshes made by its own task
and by another. A native change of a stream is shown at the next refresh
and replaces a stored value, also when the new stream got the handle value
of a dropped one. `SelectInput` and `SelectOutput` show the chosen stream
in the field at once, return the stream before, and win over a stored
value even when they choose the stream that is already the native one; a
value stored after them stays. A parent and its children keep their own
stored values, a new child starts with its own native streams, and a
child's selector changes nothing of its parent. A plain Task shows no
stream. `Input()` and `Output()` answer from the native Process: they
agree with the fields until the program stores a value itself, and a
stored value is kept and read back without redirecting them. A new process
at an ended one's native address gets a guest Process of its own. A
projection that got no handle because the handle table was full is made
when a handle is free again. An entry registered into a registry slot that
a removal vacated starts with no stream state, and an entry that a removal
moves keeps its state. A root Process whose registration is refused, or
that has no Task layout, is not published and its guest block is released,
as when there is no guest memory. No check of this group stands inside a
loop.

The fixture prints a `FAIL:` line for a failed check, one
`GROUP <name>: <n> failed` line per group and a final
`process streams: 105 checks, <n> failed` line. Its exit status is 1 when
any check failed, which make reports as an error. The launch and
projection groups test the behaviour that the Process stream change
introduces: on a tree without that change the runner still prepares the
sources, without the stream helper, and checks in both groups fail. The
ownership group tests the rule that a refresh leaves a stored value alone:
on a tree whose refresh rewrites the fields every time, and whose
selectors are the generated ones, checks of that group fail and the other
two groups pass. Sanitized runs disable leak detection by default;
`ASAN_OPTIONS` can override it.

`--prepare-only --build-dir /tmp/process-streams` emits the actual
production includes without compiling or executing a test. These focused
checks do not model native struct layouts or locks, execute the CPU or a
native child, or qualify a complete library or application.


## Initial Process arguments

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-initial-arguments
make -C developer/debug/test/m68kemu_host test-initial-arguments-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `INITIAL_ARGUMENTS_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/initial-arguments`.
Use a fresh build directory; the default temporary directory is removed.
The fixture is built with `-std=c99 -Wall -Wextra -Werror`, so a compiler
warning fails the target; the sanitizer target adds ASan and UBSan without
recovery.

The runner extracts the launch function `setup_and_execute`, the BPTR
handle translators, the Process stream helper when the tree has it,
`m68k_process_of` and the complete initial Process block of the ExecBase
shadow, with the constants of the Process stream regression. The fixture
has that regression's launch and projection groups, the same 64 checks,
which need the Process stream change in the same way, over a recording
registry of its own, and one more group.

The 70 argument checks run the launch through both routes, the direct one
and the synchronous child, for three argument strings, an empty string and
a NULL pointer. They check that the launch returns the execution result;
that the context holds the final guest argument pointer and the exact size,
with a zero pointer for the empty and the NULL case; that the guest Process
carries that pointer in its argument field, published by the launch itself,
since the fixture executes no guest code; that the published bytes equal
the caller's and end with a NUL; that the automatic library opening already
sees the final pointer and size; that an owner refresh keeps the Process
and its argument field; and that the child route passes the caller's native
pointer unchanged in `NP_Arguments`, while the direct route creates no
child.

The fixture prints a `FAIL:` line for a failed check, the `launch`,
`projection` and `arguments` group lines, and a final line that still
carries the stream regression's label:
`process streams: 134 checks, <n> failed`. Its exit status is 1 when any
check failed, which make reports as an error. The arguments group tests
what the initial-arguments change introduces: on a tree that has only the
Process stream change, checks of that group fail and the other two groups
pass. Sanitized runs disable leak detection by default; `ASAN_OPTIONS` can
override it.

`--prepare-only --build-dir /tmp/initial-arguments` emits the actual
production includes without compiling or executing a test. These focused
checks do not model native struct layouts or locks, execute the CPU or a
native child, or qualify a complete library or application. `GetArgStr`
and `SetArgStr` have their own regression below.


## Guest argument APIs

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-argument-apis
make -C developer/debug/test/m68kemu_host test-argument-apis-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `ARGUMENT_APIS_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/argument-apis`.
Use a fresh build directory; the default temporary directory is removed.
The fixture is built with `-std=c99 -Wall -Wextra -Werror`, so a compiler
warning fails the target; the sanitizer target adds ASan and UBSan without
recovery.

The runner reads the dos.library thunk table. When manual thunks are
registered for `GetArgStr` (LVO -534) and `SetArgStr` (LVO -540) it extracts
them with their two helpers; when neither is registered it extracts the
generated thunks; it refuses a table that registers only one of them. It
checks in the dispatcher's source that manual thunks are tried before
generated ones. It also extracts the Process lookup `m68k_process_of`, the
shadow registry (`shadow_register`, `shadow_lookup`, `shadow_remove`), five
memory accessors and, when the tree has it, the handling of a new process
at an ended one's native address (`m68k_process_forget`). It reads the
registry limit, the ExecBase, Process and CLI offsets, `NT_PROCESS` and
`ERROR_BAD_NUMBER` from the same source tree and its public headers. The
recording C fixture supplies the native task that is current, the native
argument accessors, the error value, the layouts, the guest heap, the CPU
registers and a counter for the low-write trap; the Task field sync and
the stack and stream writers are empty stand-ins. Guest `ULONG` is 32-bit.
The root Process is named both ways, as a launch names it: registered for
the current native task and published in the guest `ThisTask`.

The fixture runs 112 checks in four groups over a 4 KiB guest arena. The
16 semantics checks: `GetArgStr` returns the guest Process's own pointer
and sees a direct guest change of the field; `SetArgStr` publishes the new
guest pointer and returns the exact old one, also from and to NULL and for
an unchanged replacement; successful calls preserve `IoErr`; an empty
string in the last byte of the arena and a Process field that ends exactly
at the arena boundary are accepted. The 35 bounds checks: an old or a new
string pointer at or beyond the arena end, or an unterminated string in
its last byte, is rejected; a zero, out-of-range or boundary-straddling
guest SysBase and a zero or out-of-range guest `ThisTask` do not matter,
because the calling native task selects the Process: the getter returns
and the setter writes the caller's own argument; a Process whose field
straddles the arena end and a Process field in protected low memory are
rejected. Each rejection returns zero with `ERROR_BAD_NUMBER`, a rejected
setter leaves the Process field as it was, and no case reaches the
low-write trap. The 12 ownership checks: after a getter and a setter call,
also with an invalid replacement, the native argument accessors were never
called, the native Process's argument pointer is unchanged, and the
startup argument pointer, size and bytes are as they were. The 49 task
checks: a child made as `CreateNewProc` makes one has a guest Process of
its own whose argument field starts NULL; a parent and two children read
and set their own fields only, also through direct guest writes, with the
same string in two of them, with NULL, and in every order of calls; a
guest `ThisTask` that names another Process changes nothing; a task first
seen inside the call gets one registered guest Process; a plain Task has
its own structure; without a Task layout, without guest memory or with a
full registry the calls return zero with `ERROR_BAD_NUMBER`, register
nothing, release the guest block they took and change no other task's
field; with one registry slot left a Process, which needs an entry for
itself and one for its port, is taken back whole, a plain Task still takes
the slot, and with two slots the Process and its port are registered;
without a MsgPort layout a new Process is taken back whole; a new process
at the native address of an ended one gets a new
guest Process with a NULL argument, while the ended one's structure keeps
its bytes, its registry entries are gone and every other live entry and
argument stays; no call reaches the native argument accessors. No check of
this group stands inside a loop.

The fixture prints a `FAIL:` line for a failed check, the `semantics`,
`bounds`, `ownership` and `tasks` group lines and a final
`argument APIs: 112 checks, <n> failed` line. Its exit status is 1 when
any check failed, which make reports as an error. It tests the manual
thunks that the argument API change introduces: on a tree without them the
generated thunks are extracted instead and checks in every group fail. On
a tree whose manual thunks find the Process through the guest `ThisTask`,
the checks of the task group in which the caller is not the first Process
fail, and so do the getter and setter checks of the SysBase and `ThisTask`
cases. A tree that reads the type of a new Process's port through a
missing MsgPort layout would follow a NULL pointer: the runner sees that
in the source, the fixture does not make that one call there, and its
check fails. Sanitized runs disable leak detection by default;
`ASAN_OPTIONS` can override it.

`--prepare-only --build-dir /tmp/argument-apis` emits the actual production
includes without compiling or executing a test. These focused checks do not
model native struct layouts, execute the CPU, or qualify a complete library
or application. They do not cover how the initial arguments reach the
guest Process, which has its own regression above.


## AllocBitMap result projection

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-bitmap-result
make -C developer/debug/test/m68kemu_host test-bitmap-result-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `BITMAP_RESULT_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/bitmap-result`.
Use a fresh build directory; the default temporary directory is removed.
The fixture is built with `-std=c99 -Wall -Wextra -Werror`, so a compiler
warning fails the target; the sanitizer target adds ASan and UBSan without
recovery.

The runner extracts the actual manual `AllocBitMap` thunk, the shadow
factory, destructor and registry (`shadow_create`, `shadow_destroy`,
`shadow_register`, `shadow_lookup`, `shadow_remove`), the bitmap
converters, five memory accessors, the DOS object code of an ExAll control
(`exall_control_new`, `exall_control` and the `FreeDosObject` thunk) and
the thunk registered for `FreeBitMap`, LVO -924: the manual one when the
graphics.library table registers it, else the generated one. It passes the
registry tag of an ExAll control on as the tree defines it, and checks
that the manual thunk is
the one registered for LVO -918 and, in the dispatcher's source, that
manual thunks are tried before generated ones. It reads the shadow and
handle limits, the Screen bitmap offset and the BitMap shadow layout from
the same source tree, and two memory flags and two DOS object types from
its public headers. The recording C fixture supplies the native allocation
and release of bitmaps and of DOS objects, the guest heap and the layout
lookup over a 32 MiB guest arena; guest `ULONG` is 32-bit and native
pointers are 64-bit. The guest heap records every live block with the size
it was allocated with and counts a free of a block that is not live, or
with another size, as bad. The native side hands out the one configured
bitmap or, for the lifecycle group, bitmaps of a pool, and counts each
one's releases.

The fixture runs 154 checks in three groups. The 105 contract checks call the
thunk with a friend bitmap given as NULL, as a registered shadow, as a
Screen's bitmap, as a handle and as a guest planar structure, each with
three native results: a host object, an address below 4 GiB and one above
it, which needs a 64-bit host. Each time the guest receives the address of
a registered guest bitmap inside the arena and never the native pointer;
the width, height, depth, flags and the converted friend reach the native
call once and unchanged; the shadow is created and synchronized once; the
native bitmap is not released; the RastPort bitmap converter resolves the
guest address to the same native bitmap; and the `FreeBitMap` thunk
registered for LVO -924 hands that native bitmap back exactly once. The 16
failure checks: a
failed native allocation returns zero with nothing to release and no
shadow; a failed guest allocation, a full shadow table and a missing BitMap
layout each return zero, release the native bitmap exactly once and
register no shadow. The full table also releases the guest block that the
factory had taken, stays unchanged and logs its refusal once; the other two
leave no guest block behind.

The 33 lifecycle checks pair every allocation with its free. In 4096
turns, four times the registry's 1024 entries, each with a distinct native
bitmap, every allocation succeeds and after every free the registry and
the guest heap, in live blocks and bytes, are at their baseline, the freed
guest address resolves to nothing, each native bitmap is released exactly
once and the full-registry refusal is never logged. The same holds for
1100 turns in which the native address returns every time. With 1000
bitmaps alive together beside another entry, freed first-in, last-in and
alternating, every live bitmap still resolves to its own native bitmap
after each free. With the registry full the next allocation is refused as
before, and after one free it succeeds. A second free of the same guest
address, NULL, a handle, a Screen's embedded bitmap that is a second
address of an allocated native bitmap, a planar structure in the guest
arena, the guest addresses of entries of other types, an address outside
the arena and one inside a guest BitMap are not passed to the native call
and change nothing; after each of them the owner's address still frees
its bitmap exactly once. Without the BitMap layout the type cannot be
verified and nothing is passed on. The entry that a free moves into the
freed slot keeps its address, native pointer, type and stream state, and
a registration into the slot that the move vacated starts with no stream
state. An ExAll control made by the actual DOS object code is not freed as
a bitmap, and a bitmap is neither found nor freed as an ExAll control;
each is then freed once by its own call, with the 16 or 40 guest bytes it
was allocated with. No check of this group stands inside a loop: a loop
feeds counters and each counter is checked once.

The fixture prints a `FAIL:` line for a failed check, the `contract`,
`failures` and `lifecycle` group lines and a final
`bitmap result: 154 checks, <n> failed` line. Its exit status is 1 when
any check failed, which make reports as an error. The contract and
failures groups test the result projection and the rollback that the
AllocBitMap change introduces: on a tree without that change the manual
thunk is already registered, the runner prepares the same functions, and
checks in both groups fail. The lifecycle group tests the paired
`FreeBitMap`: on a tree whose LVO -924 is the generated thunk, which frees
the native bitmap and keeps the guest BitMap registered, checks of that
group fail and the other two groups pass. Sanitized runs disable leak
detection by default; `ASAN_OPTIONS` can override it.

`--prepare-only --build-dir /tmp/bitmap-result` emits the actual production
includes without compiling or executing a test. These focused checks do not
call graphics.library, model native struct layouts, execute the CPU, or
qualify a complete library or application.

## A 68k child without a guest Process

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-child-projection
make -C developer/debug/test/m68kemu_host test-child-projection-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `CHILD_PROJECTION_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/child-projection`.
Use a fresh build directory; the default temporary directory is removed.
The fixture is built with `-std=c99 -Wall -Wextra -Werror`, so a compiler
warning fails the target; the sanitizer target adds ASan and UBSan without
recovery.

`CreateNewProc` with `NP_Entry` starts a native process first and only then
makes the guest Process that the program is given. When that Process cannot
be made, the program is told that there is no child, and the native process,
which exists already, must not run anything of the program.

The runner extracts the actual `create_proc` and `child_entry` with the
record they share (`struct M68KChild`) and `proc_name`, the Process lookup
`m68k_process_of`, the registry (`shadow_register`, `shadow_lookup`,
`shadow_remove`), the forgetting of an ended process
(`m68k_forget_entry`, `m68k_process_forget`), the tag walker
(`item_ok`, `next_item`, `m68k_next_tag`) and five memory accessors. It
reads the registry limit, the handle base and the guest Process offsets
from the same source tree, and the tags and two memory flags from its
public headers. The recording C fixture supplies the native side: a
`CreateNewProc` that makes a native process and does not run it, the
program's lock with its owner, the guest heap and the native heap with
every block and its frees, the layout lookup, and a stand-in for the run of
guest code that records each call and runs nothing. The test itself calls
the native entry that `create_proc` handed to `CreateNewProc`, after
`create_proc` has returned, as the scheduler runs the child once the parent
lets the lock go. Guest `ULONG` is 32-bit and native pointers are 64-bit.

The fixture runs 80 checks in three groups. The 18 creation checks follow
one child with an exit code from the call to its end: the guest Process
returned and registered with its port, one native process with the
module's entry and the child's record, the name copied, the count, the
hold and the guest stack taken before it runs, nothing run inside the call
while the parent keeps the lock, then the entry run once by the child
itself with the lock, on its stack, finding the same guest Process, the
exit code run with the entry's result, and afterwards the stack, the
count, the hold, the record and the lock given back once; no exit code
after a fault; the two stack size rules. The 9 refusal checks are the
cases in which no native process exists: no entry, an entry outside the
program's memory, no memory for the guest stack, and a native
`CreateNewProc` that refuses, each leaving nothing behind and nothing of
the parent changed.

The 53 projection checks are three cases of a child whose native process
exists and whose guest Process cannot be made: no memory for it, a full
registry, and a registry with one slot left, where a Process needs two.
Each case has 17 checks with the case's name in front: `CreateNewProc`
returns 0; the native process waits and still owns its record, its stack,
its count and its hold, which the parent must not free; no guest Process
and no registry entry is left. Then room is made again and the child is
run: it runs neither its entry nor its exit code, looks up no guest
Process and makes none, ends with 0, and gives back its stack, its count,
its hold and its record exactly once; the entries and the memory of the
parent and of an earlier child are as before; a later `CreateNewProc`
gives a child that runs. Two more checks count the registry's own report
of a full table for each case by itself, once in the two registry cases and
never in the first, and the paths that must not be reached in any case.

The fixture prints a `FAIL:` line for a failed check, the `creation`,
`refusal` and `projection` group lines and a final
`child projection: 80 checks, <n> failed` line. Its exit status is 1 when
any check failed, which make reports as an error. On a tree whose
`create_proc` returns 0 in these cases and leaves the child as it was, the
runner prepares the same functions, the creation and refusal groups pass,
and checks of the projection group fail: the child runs its entry and its
exit code and makes a guest Process for itself. Sanitized runs disable
leak detection by default; `ASAN_OPTIONS` can override it.

`--prepare-only --build-dir /tmp/child-projection` emits the actual
production includes without compiling or executing a test. These focused
checks do not create a native process, take a real lock, execute the CPU or
any guest code, or qualify a complete library or application.
