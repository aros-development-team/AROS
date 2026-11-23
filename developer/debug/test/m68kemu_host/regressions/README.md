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


## Thunk tables

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-thunk-tables
```

The check is also a prerequisite of the host Makefile's `test` target. It
needs Python 3.9+ and no compiler; `PYTHON` selects the interpreter, and
`--source-root` checks another AROS source tree when the runner is invoked
directly.

A 68k program calls a library function by its offset, and the thunk tables
of `m68kemu.library` map each offset to the code that converts the call. The
runner reads every `M68KThunkEntry` table in `rom/m68kemu` and requires each
offset to be a positive multiple of 6. For the hand-written dos.library
table it also requires, at each offset, the function that `rom/dos/dos.conf`
has there, and it lets each thunk read only the registers that function
takes (A6 and A7 aside). `dos.conf` follows the AmigaOS dos.library layout,
the one 68k programs are built for. Before it reads the tree, the runner
checks itself on a small table with one fault of each kind.

The register check reads the body of each thunk, not the helpers it calls,
and the name and register checks cover the dos.library table only: the
`.conf` files of some other libraries do not follow the AmigaOS offsets for
every function. A pass says nothing about what a thunk does with the values
it reads.

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


## Pointers returned by native functions

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-native-returns
make -C developer/debug/test/m68kemu_host test-native-returns-sanitize
```

The check is also a prerequisite of the host Makefile's `test` target. It
needs Python 3.9+ and a C99 compiler, with ASan/UBSan for the sanitizer
target. `PYTHON` and `CC` select them; `NATIVE_RETURNS_ARGS` passes runner
options such as `--build-dir /tmp/native-returns` or `--static-only`.

A native pointer does not fit a 32-bit register on every target, and on a
target whose memory lies below 4 GB, such as raspi-aarch64, it fits but can
look like an address of the program. The dispatcher's test of the upper 32
bits cannot tell such a pointer from a number, so the thunks translate the
pointers they return with `m68k_native_out()`.

The runner first reads `m68kemu_thunks_gen.c`: no generated thunk whose
native function returns a pointer, by the return type in the library's
`.conf`, may return the value of the native call as it is. The functions
that `returns_arg_pointer()` in `m68kemu_moira.cpp` lists are left to the
dispatcher. It checks itself first on two thunks that are right and one
that is not. On a tree before this check, 120 of the 155 such thunks return
the native value as it is.

It then compiles `m68k_native_out()`, `m68k_bptr_out()`, `m68k_bptr_in()`,
`m68k_to_host()` and `m68k_to_host_or_shadow()`, taken verbatim from the
sources, with `test_native_returns.c`: a pointer into the program's memory
becomes its address; native pointers at 0x003FEC70, 0x7FFF0000, just past
the program's memory and above 4 GB become handles that lead back to them;
the same pointer gives the same handle. No pointer is dereferenced. The
check does not run a thunk or a 68k program.

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


## 68k library loading bounds

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-library-loading-bounds
make -C developer/debug/test/m68kemu_host test-library-loading-bounds-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target; the
sanitizer target is optional. `PYTHON` and `CXX` select the tools and
`LIBRARY_LOADING_BOUNDS_ARGS` passes runner options such as
`--build-dir /tmp/loading-bounds` or `--prepare-only`, which writes the
extracted sources and stops before the compiler.

The runner extracts `M68KEmu_LoadM68KLibrary`, `m68k_init_struct` and
`m68k_library_abandon` from `m68kemu_m68klib.c`, `M68KEmu_ImageHunk` and
`M68KEmu_FreeImage` from `m68kemu_memory.c`, the memory accessors from
`m68kemu_intern.h` and the `M68K_*` constants they use from
`m68kemu_offsets.h`. DOS, the heap and the placement of the hunks of a
library image are recording fixtures; no CPU runs. The 27 cases load
well-formed libraries (relative function tables, a Resident in the second
hunk, every init table command, a base filled to its last byte, a function
table cut at 1000) and malformed ones: data sizes that wrap or exceed
containment, init tables and items outside the base or containment, an
init command with size code 3, pointer and function tables that run out of
containment, a Resident that is not whole inside its hunk or lies between
two hunks of the image, and each refusal on the way, which must give back
exactly the blocks of the attempt and nothing of the running program. The
program's entry point, segment start and load span must be its own again
after a library was loaded or refused. The fixture prints `ok` or `FAIL`
for each case and `m68kemu 68k library loading bounds: <cases> cases,
<n> failed, <checks> checks`; it exits with 1 when a case failed.

## Hunk file loader

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-hunk-file-loader
make -C developer/debug/test/m68kemu_host test-hunk-file-loader-sanitize
```

`HUNK_FILE_LOADER_ARGS` passes runner options as above. The runner extracts
`M68KEmu_LoadHunksFromMemory` and `rd32` from `m68kemu_memory.c`, the
accessors from `m68kemu_intern.h` and the `HUNK_*` numbers from
`compiler/include/dos/doshunks.h` and `m68kemu_offsets.h`. The heap is a
recording fixture; no CPU runs. The 33 cases read well-formed hunk files
(relocations between hunks and on the last longword, the 16-bit and
relative relocation forms, empty, attributed, symbol and debug blocks,
headers that number their hunks from 1) and malformed ones: hunks larger
than containment or with sizes that wrap, relocations to no hunk or past
their hunk, blocks after the last hunk ended, unsupported blocks, files cut
inside a block and a heap that answers outside containment. Every refusal
must give back every block of the load, write nothing outside the blocks
the heap handed out and leave the context's fields as they were. The
fixture prints one line for each case and
`m68kemu hunk file loader: <cases> cases, <n> failed, <checks> checks`.

## Seglist copy

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-seglist-copy
make -C developer/debug/test/m68kemu_host test-seglist-copy-sanitize
```

`SEGLIST_COPY_ARGS` passes runner options as above. The runner extracts
`M68KEmu_LoadHunks` and, where the tree has them, `M68KEmu_ImageHunk` and
`M68KEmu_FreeHunks` from `m68kemu_memory.c`, the accessors from
`m68kemu_intern.h`, and `Tag`, `struct TagItem`, `TAG_DONE`, `TAG_USER`,
`GSLI_68KRELOCS` and the relocation list from `compiler/include`. A double
of `GetSegListInfo()` answers the relocation list; the heap is a recording
fixture; no CPU runs. The 22 cases copy seglists in order (relocations of
the three kinds, an empty segment, several groups) and refuse ones that are
not: a segment larger than containment or smaller than its own two words,
more segments than containment holds, a seglist dos has no node or list
for, a lost list, an unknown kind, a group or target the seglist does not
have, an offset past its segment, a bad group after a good one; a failed
allocation gives back what was placed. Two cases give a copy back, one of
more than a hundred segments. The fixture prints one line for each case
and `m68kemu seglist copy: <cases> cases, <n> failed, <checks> checks`.

These three fixtures, like the others here, are function-level host tests:
they do not run a CPU, load a real file or seglist through dos, or qualify
a complete library, emulator or application.

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

## Generated shadow layouts

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-shadow-layouts
make -C developer/debug/test/m68kemu_host test-shadow-layouts-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `SHADOW_LAYOUTS_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/shadow-layouts`.
Use a fresh build directory; the default temporary directory is removed.
The fixture is built with `-std=c99 -Wall -Wextra -Werror`, so a compiler
warning fails the target.

`m68kemu_shadow_layouts.h` writes the m68k offset of a field as a number and
leaves the native side to the compiler of the target. The runner checks that
in three steps.

The checked-in header must be, byte for byte, what `gen_shadow_layouts.py`
writes from `m68k_offsets.txt`, and every row of its tables must be a
`SHADOW_FIELD()` row or the end entry, so that no native offset is a number.

`gen_offset_extractor.py`, run over the sample header as its NDK, must name
in each probe the member it probes and in each size line the header of the
struct. These are the two columns the generator reads the native side from.

The generator's output for `shadow_layouts_sample.txt` is then compiled four
times around the structures of `shadow_layouts_sample.h`, without any change
to that output. The sample library node ends in a lock, as `struct Library`
ends in a `spinlock_t`, and the base that embeds it has four fields behind
it. The four builds give the lock 16 or 128 bytes and one value 4 or 8 bytes.
The fixture takes the SF_ types and the two table structs from
`m68kemu_shadow.h` and compares every row with an expectation that does not
use the generator's macros: the m68k offset, the native offset of this build,
the type, and that nothing else is set; then the end entries, the names, the
m68k sizes, the type numbers from 20 on, and the count. One struct of the
sample has no member and must get no layout; a field of 14 bytes and a field
without a member must get no row.

Across the builds the runner checks that the m68k offsets are those of the
sample file everywhere, that the four fields behind the lock lie 112 bytes
further with the larger lock while the other 25 do not move, and that the
value is `SF_PTR` with 8 bytes and `SF_LONG` with 4. A pointer member is
`SF_PTR` where the host's pointers have 8 bytes.

Each build prints `shadow layouts: 160 checks, <n> failed`; a failed check
prints a `FAILED` line and ends the run with status 1. On a tree whose
generator still takes two offset files the runner stops at the first step
and says so.

These checks compile the sample only. The real header needs the AROS headers
of a target, so that every member it names exists is shown by the build of
`m68kemu_shadow.c`, not here. The shadow engine itself does not run.


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
which exists already, must not run anything of the program. With
`NP_Seglist` naming a seglist that dos loaded as 68k code, the child's code
is a copy of that seglist in the program's memory; a cancelled child is
going to run none of it, so the copy is given back at once. A child that
runs owns its copy when dos would unload the seglist at its end
(`NP_FreeSeglist`, TRUE unless the program gives FALSE; FALSE for the old
`CreateProc`) and gives it back after its last use; otherwise the copy
stays in the program's memory, as what the child published of it may.

The runner extracts the actual `create_proc` and `child_entry` with the
record they share (`struct M68KChild`), `proc_name`, the two dos entries
`m68k_dos_CreateNewProc` and `m68k_dos_CreateProc`, the Process lookup
`m68k_process_of`, the registry (`shadow_register`, `shadow_lookup`,
`shadow_remove`), the forgetting of an ended process
(`m68k_forget_entry`, `m68k_process_forget`), the tag walker
(`item_ok`, `next_item`, `m68k_next_tag`), the copy of a seglist
(`M68KEmu_LoadHunks`) with, where the tree has them, the walk of a copy and
its release (`M68KEmu_ImageHunk`, `M68KEmu_FreeHunks`), and five memory
accessors. It reads the registry limit, the handle base and the guest
Process offsets from the same source tree, the tags and two memory flags
from its public headers, and the relocation list dos keeps for a 68k
seglist (`struct HunkRelocGroup`, the `HUNKRELOC_*` kinds and
`struct HunkRelocTable`) from `compiler/include/dos/doshunks.h`. The
recording C fixture supplies the native side: a `CreateNewProc` that makes
a native process and does not run it, the program's lock with its owner,
the guest heap and the native heap with every block and its frees, the
layout lookup, a native seglist of two segments laid out as `LoadSeg()`
leaves it, with one longword dos relocated, a `GetSegListInfo()` double
that answers whether it is 68k code and its relocation list, and a stand-in
for the run of guest code that records each call and runs nothing. The test itself calls
the native entry that `create_proc` handed to `CreateNewProc`, after
`create_proc` has returned, as the scheduler runs the child once the parent
lets the lock go. That order is modelled cooperatively, one task at a
time; it is not a test of real SMP or of the native runtime. Guest `ULONG`
is 32-bit and native pointers are 64-bit.

The fixture runs 151 checks in seven groups. The 18 creation checks follow
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

The 13 seglist checks follow a child whose code is a copy of the seglist:
the copy, one block for each segment, chained and relocated for the
program's memory, dos asked once for each answer and the handle translated
once, the child started, run on the copy as itself, and its stack, count,
hold and record given back once, while the native seglist and the parent's
own segment fields stay as they were. A seglist that cannot be copied is refused before any native
process or allocation. When the guest stack or the native process cannot
be made after the copy, the copy is given back once with everything else.
A seglist that is not 68k code starts a native program with its own exit
hook and makes no copy, guest stack or 68k child.

The 33 cancelled-copy checks are two cases of a child whose code is a copy
of the seglist and whose guest Process cannot be made: no memory for it,
and a full registry. Each case has 16 checks with the case's name in front.
The copy was made and the native process waits; inside the call the copy
is given back, each block exactly once, so that of the call only the
child's stack is left in the program's heap, while the record, the stack,
the count and the hold, which the child owns, stay. Then room is made and
the child runs: it runs neither the copy nor its exit code, ends with 0,
frees its stack once and no block twice, gives back its count, its hold
and its record once, and afterwards the program's heap holds exactly the
blocks it held before the call; the parent's entries and memory, the
earlier child's and the native seglist are as before. One more check
counts the registry's report of a full table and the paths that must not
be reached.

The 17 owned-copy checks are about who gives a copy back. Through the
actual `m68k_dos_CreateNewProc`, a child of the copy without
`NP_FreeSeglist` owns it: its entry and its exit code run while the copy
is allocated, then the child gives it back, each block once, holding the
program's lock, before its stack, and the heap holds what it held before
plus the child's guest Process. The same with `NP_FreeSeglist` TRUE, which
is passed on to dos as given; after a fault no exit code runs and the copy
is given back all the same; three children one after another leave no
copy behind. With `NP_FreeSeglist` FALSE, and through the actual
`m68k_dos_CreateProc`, the child owns nothing: after it the copy is still
allocated and an address it published inside the copy reads as before,
as an interrupt, a timer vector or a semaphore it installed would need. A
segment in the program's own memory runs in place, and a child of
`NP_Entry` owns nothing either. A child that owns its copy keeps it when
another child is cancelled, and gives it back once when it runs.

The 8 duplicate-freeseglist checks give a child two `NP_FreeSeglist`
tags, FALSE then TRUE and TRUE then FALSE, each in one list and with the
second behind `TAG_MORE`. As dos goes by the first (`FindTagItem`), so
does the child: both tags are passed on to dos in their order, the first
is the one recorded, and the child owns its copy exactly when the first is
TRUE; after it, a FALSE-first copy is still allocated with what the child
published of it, and a TRUE-first copy was given back once, holding the
lock, before the stack.

The fixture prints a `FAIL:` line for a failed check, the `creation`,
`refusal`, `projection`, `seglist`, `cancelled-copy`, `owned-copy` and
`duplicate-freeseglist` group lines and a final
`child projection: 151 checks, <n> failed` line. Its exit status is 1
when any check failed, which make reports as an error. On a tree whose
`create_proc` returns 0 in these cases and leaves the child as it was, the
runner prepares the same functions, the creation and refusal groups pass,
and checks of the projection group fail: the child runs its entry and its
exit code and makes a guest Process for itself. On a tree whose
`create_proc` copies a seglist and never gives the copy back, the check of
each refusal after the copy and the two checks of each cancelled-copy case
that look at the copy in the heap fail: its blocks stay allocated in the
program's heap. On a tree whose child never gives its copy back, the
owned-copy checks that expect the release fail with the copy still
allocated; on one whose child gives back every copy whatever
`NP_FreeSeglist` says, the checks of the FALSE and `CreateProc` children
fail: their copy is gone. Sanitized runs disable leak detection by default;
`ASAN_OPTIONS` can override it.

`--prepare-only --build-dir /tmp/child-projection` emits the actual
production includes without compiling or executing a test. These focused
checks do not create a native process, take a real lock, execute the CPU or
any guest code, or qualify a complete library or application.

## BOOPSI class storage

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-class-storage
make -C developer/debug/test/m68kemu_host test-class-storage-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target; the
sanitizer target is optional. `PYTHON` and `CXX` select the tools and
`CLASS_STORAGE_ARGS` passes runner options such as `--build-dir`.

The runner extracts `class_slot`, `class_room`, `class_drop` and `class_rec`
from `m68kemu_boopsi.c` and the two class block constants from
`m68kemu_intern.h`. The records of the classes a 68k program makes live in a
first block in the context and in more blocks allocated as the program makes
more classes. The fixture makes classes past the first block and up to the
limit, checks that a record never moves while the table grows (its native
class keeps the record's address in its dispatcher hook), that freeing a
class moves the last record into the freed slot together with its hook, that
an allocation failure and the limit refuse cleanly, and that the blocks are
given back. No CPU and no native BOOPSI run. It prints `m68kemu class
storage: <checks> checks, <n> failed` and exits with 1 when a check failed.

## ExecBase lists and scheduler fields of the program

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-execbase-private
make -C developer/debug/test/m68kemu_host test-execbase-private-sanitize
```

The ordinary fixture is part of the host Makefile's `test` target. It needs
Python 3.9+ and a C99 compiler; ASan/UBSan are optional. `PYTHON` and `CC`
select those tools. `EXECBASE_PRIVATE_ARGS` passes runner options such as
`--source-root /path/to/AROS` and `--build-dir /tmp/execbase-private`.
Use a fresh build directory; the default temporary directory is removed.
The fixture is built with `-std=c99 -Wall -Wextra -Werror`, so a compiler
warning fails the target; the sanitizer target adds ASan and UBSan without
recovery.

On an SMP build exec keeps `IntrList`, `PortList`, `TaskReady`, `TaskWait`
and `SemaphoreList` behind private locks, the running tasks on a private
list, and `ThisTask`, `Quantum`, `Elapsed`, `IDNestCnt`, `TDNestCnt`,
`IdleCount`, `DispCount`, `SysFlags` and `AttnResched` per CPU or for its
scheduler alone. A 68k program takes none of those locks, so its ExecBase
gets none of this from the native one: the five lists are empty lists in its
own memory, the scheduler fields hold values the emulator owns, the two
nesting counts follow the program's own `Forbid()` and `Disable()`, and
`FindName()` on `PortList` and `SemaphoreList` goes through `FindPort()` and
`FindSemaphore()`.

The runner extracts the actual `m68k_nest_counts_at`, `m68k_nest_counts` and
`m68k_execbase_private` from `m68kemu_shadow.c`, the `Forbid`, `Permit`,
`Disable`, `Enable` and `FindName` thunks from `m68kemu_thunks.c`,
`m68k_gil_obtain` from `m68kemu_tasks.c`, five accessors from
`m68kemu_intern.h`, the ExecBase offsets and the `ML_NATIVE` test. It also
reads `shadow_init_execbase` and stops when that function still registers one
of the five lists as a shadow. The fixture supplies the context fields these
functions use and records what they would ask of the bridge and of exec: the
shadow lookup, `FindPort`, the `FindSemaphore` thunk, the native `FindName`,
a shadow made for a result, and a pointer handed to a native call.

The 31 layout checks cover the five lists (head, tail, tail predecessor, node
type, and no shadow registered for the header), the six scheduler fields, the
two nesting counts, and that nothing else in the ExecBase or around it is
written. The 24 nesting checks cover eight combinations of the two counters,
including a `Disable()` alone and a `Permit()` too many under `Disable()`, a
sequence of twelve calls through the four thunks, a task that takes the
program's lock with its own nesting and one without, and a context without
an ExecBase. The 14 lookup checks cover `PortList` with a known name, an
unknown name and no name, `SemaphoreList`, the three lists that stay empty,
and the unchanged cases: `LibList`, `DeviceList` and `ResourceList`, and a
list in the program's memory.

The fixture prints a `FAIL:` line for a failed check, the `layout`, `nesting`
and `lookup` group lines and a final `execbase private: 69 checks, <n> failed`
line. Its exit status is 1 when any check failed, which make reports as an
error. On a tree that maps the five lists to the native ExecBase, the runner
prepares the functions that exist, the fixture stands in for the missing ones
the way that tree behaves (the lists registered as shadows, nothing written,
no count kept), and 62 of the 69 checks fail. Sanitized runs disable leak
detection by default; `ASAN_OPTIONS` can override it.

`--prepare-only --build-dir /tmp/execbase-private` emits the actual
production includes without compiling or executing a test. These focused
checks do not set up a whole ExecBase, run the CPU or any guest code, take a
real lock, call exec, or qualify a complete library or application; in
particular they do not show what a program does with the empty lists.
