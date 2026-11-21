<!--
Copyright (C) 2026, The AROS Development Team. All rights reserved.
-->

# m68kemu.library: the interface of an optional execution engine, version 1

Version 1.0 of the interface. The host side is in this directory
(`m68kemu_engine.c` and the slice in `m68kemu_moira.cpp`); section 10 here
and `INTERNALS.md` describe it.

The header `m68kemu_engine.h` beside this file declares every name used
here. The host is `m68kemu.library`; an engine is a separate, optional
library. The interpreter (Moira) stays the default, and `RunHunk`/`RunFile`
keep their LVOs, arguments and results.

## 1. Design in one paragraph

The engine executes 68k instructions on guest memory the host owns and
returns to the host at every point where the host must act: an A-line
opcode, a PC the engine cannot fetch, an exception, an instruction it does
not execute, or the end of a bounded slice. It never calls the host except
for single guest memory accesses the host has asked to see, and those calls
never call back into the engine. Library calls, nested 68k calls, child
tasks, interrupts and the program lock therefore stay entirely in
`m68kemu.library`, where they are today. The private `M68KEmuContext`
layout is not part of the interface: the engine sees a memory map, two leaf
callbacks and opaque cookies.

## 2. Discovery, versions and sizes

1. The host opens the engine library by its name, version 1 or later. NULL:
   the engine is absent. `m68kemu.library` looks for one name, the one in
   `m68kemu_engine.c`: another engine takes that place by being installed
   under that name or by a change of that line. There is no choice among
   several engines at run time.
2. It calls the library's first function after the standard vectors. In the
   library's `.conf` it is the first entry of the function list, declared
   `APTR <name>(ULONG abiMajor) (D0)`, which the build puts at LVO -30 on
   every target. The interface gives the function no name. The host passes
   `M68KENG_ABI_MAJOR` and gets a `const struct M68KEngOps *`, or NULL: no
   engine for ABI major 1.
3. It checks the returned `struct M68KEngOps`: `size_of >=
   M68KENG_V1_SIZE_OPS`, `abi_major == 1`, `abi_minor >=` the minor it
   needs (its own `M68KENG_ABI_MINOR`, 0), `caps & M68KENG_CAP_BASE`,
   `1 <= max_block_insns <= M68KENG_MAX_BLOCK_INSNS`, `name` and each of the
   nine operations not NULL. Any failure: incompatible.
4. `domain_create` makes the engine check the host's side (section 7 gives
   the order): `host->size_of >= M68KENG_V1_SIZE_HOST`, `host->abi_major ==
   1`, `read` and `write` not NULL, `map->size_of >= M68KENG_V1_SIZE_MAP`, and
   that `required_caps` is a subset of `caps`.

Size rule. Every versioned record of the interface (`struct M68KEngOps`,
`M68KEngHost`, `M68KEngMap`, `M68KEngExit` and `M68KEngStats`) starts with
`size_of`, which its writer sets to `sizeof` of the structure as it was
compiled. A plain record embedded in one of them, such as `struct M68KEngRange`
in the memory map, has no `size_of`; the size of the record that holds it
covers it. `M68KENG_V1_SIZE_*` is the
version 1.0 prefix: the byte just past the last version 1.0 member
(`M68KENG_V1_END`, `offsetof` plus the member's size), as the target's
compiler lays it out, trailing padding not included. A later minor only
appends members after that prefix and capability bits, never moves or
re-means an existing member, and so never changes a `M68KENG_V1_SIZE_*` value
for a target. Therefore:

- a `size_of` below the prefix is `M68KENG_ERR_SIZE`, for input and output
  structures alike;
- a `size_of` at or above it is accepted: the reader uses only the members
  that lie entirely within both its own definition and the other side's
  `size_of`; a member past either is absent (NULL or 0 for an input);
- an output structure (`M68KEngExit`, `M68KEngStats`) is written only in the
  members that lie entirely within both the engine's definition and the
  caller's `size_of`; `size_of` itself and every byte past those members,
  an appended tail the engine does not know included, keep what the caller
  put there.

Host and engine are built for the same AROS target; the interface does not
cross targets or pointer widths.

The ops table and `name` belong to the engine library and stay valid while
the host keeps the library open.

## 3. Fixed widths

| Item | Width | Notes |
| --- | --- | --- |
| guest address, register, value | 32 bits (`ULONG`) | the header refuses a target where `ULONG` is not 4 bytes |
| SR | 16 significant bits in a `ULONG` | |
| access width | 1, 2 (host calls); 1, 2, 4 (direct) | |
| status | `LONG` | `M68KENG_OK` or `M68KENG_ERR_*` |
| budget | `ULONG`, 1 to `M68KENG_MAX_BUDGET` (0xFFFF0000) | |
| `max_block_insns` | `ULONG`, 1 to `M68KENG_MAX_BLOCK_INSNS` (0x10000) | |
| retired per run | `ULONG`, at most `budget + max_block_insns - 1`, which is at most 0xFFFFFFFF | exact, never clipped or wrapped |
| retired total | `UQUAD` | saturating |
| other counters | `ULONG` | saturating |
| cookies, map base | pointer width of the target | |

Guest arithmetic on addresses wraps at 32 bits before any check. The two
bounds above are what make every per-run count fit: the engine counts in
unsigned arithmetic of at least 32 bits and cannot wrap for an accepted
call. A counter of `struct M68KEngStats` stops at its largest value
(0xFFFFFFFF, or 0xFFFFFFFFFFFFFFFF for `retired`) instead of wrapping;
`cpus` and `code_bytes` are current values and saturate the same way.

## 4. Guest memory

`struct M68KEngMap` is host memory: `base` is guest address 0, `size`
bytes are guest memory (at least 0x1000), stored big-endian, and up to
`M68KENG_MAX_RANGES` sorted disjoint ranges `[lo, hi)` with `hi <= size`.

For a data access of width `w` at address `a`:

- it is **direct** when `a <= size - w` and no byte of `[a, a + w)` lies in
  a range whose flags cover the access (`M68KENG_RANGE_READ_HOST` for reads,
  `M68KENG_RANGE_WRITE_HOST` for writes). The engine then reads or writes
  `base` itself, big-endian, at any alignment (the 68040 allows misaligned
  data);
- otherwise it goes to the host: bytes as one call of width 1, words as one
  call of width 2 (the address may be odd), longwords as two word calls,
  the word at `a` first then `a + 2`, each routed by the same rule on its
  own; addresses outside memory included. A read-modify-write instruction
  is a read then a write, not atomic across the two. Multi-access
  instructions (MOVEM and the like) make their accesses in architectural
  order.

This is the canonical behaviour of `AROSMoira::read8/read16/write8/write16`
(the CIA and custom chip pages, the low-memory guard, the bitplane mirror,
out-of-bounds reads as 0) moved behind the map: the host's callbacks keep
those semantics.

Instruction fetch is always direct and never goes to the host. A fetch of
the word at `p` needs `p` even, `p <= size - 2` and `[p, p + 2)` outside
every `READ_HOST` range. An odd PC at the start of an instruction is
`M68KENG_STOP_EXCEPTION` with vector 3. Any other first word that cannot be
fetched is `M68KENG_STOP_PC_OUTSIDE` with `address == pc`; an extension word
that cannot be fetched is the same exit with `address` set to that word and
`pc` at the instruction. The host's return sentinels (`SENTINEL_ADDR`
0xFFDEAD00, `NEST_SENTINEL` 0xFFDEAE00) are outside memory and arrive this
way; the engine knows nothing about them.

Map changes: the host changes the map only between engine calls and
increments `generation`. The engine reads it at the start of every `run()`
and must honour the current map for every access of that run. The bitplane
range, which thunks create when a program draws into a screen's planes, is
the case that makes this necessary.

## 5. Registers

`get_reg`/`set_reg` take D0-D7 as 0-7 (`M68KENG_REG_D0` + n), A0-A7 as 8-15
(`M68KENG_REG_A0` + n; `M68KENG_REG_A7` is the active stack pointer),
`M68KENG_REG_PC` and `M68KENG_REG_SR`. An index of `M68KENG_REG_COUNT` (18) or more
is `M68KENG_ERR_ARG`.

Version 1.0 has one CPU mode: supervisor, no trace, master bit clear. It is
the mode the canonical host gives every CPU: the root after reset, nested
calls and tasks start with SR 0x2000, and the host's own frames keep S set.
A program that clears S itself leaves version 1.0. `set_reg(SR, v)` with
`(v & 0xF000) != 0x2000` is `M68KENG_ERR_UNSUPPORTED` and changes nothing;
otherwise SR becomes `v & M68KENG_SR_DEFINED`, interrupt mask included. RTE,
STOP, and any instruction that would clear S or set T or M (MOVE to SR,
ANDI/EORI/ORI to SR) are `M68KENG_STOP_UNSUPPORTED` before they do anything.

`cpu_create` leaves D0-D7 and A0-A7 at 0, PC at 0 and SR at
`M68KENG_SR_AT_CREATE` (0x2700). The engine has no reset vectors and no
prefetch state visible to the host: the next `run()` starts at the PC last
set.

Materialization: whenever `run()` has returned, every register holds the
architectural value at the instruction boundary given by the exit, and a
`set_reg` takes effect at the next `run()`. Native calls happen only
between runs, so a thunk sees current registers and its writes, including
D0, CCR and PC, are what the guest resumes with. During a host `read` or
`write` callback the registers are not available (`M68KENG_ERR_STATE`).

## 6. Execution

`run(cpu, budget, exit)` checks, in this order: `cpu` or `exit` NULL is
`M68KENG_ERR_ARG`; `exit->size_of < M68KENG_V1_SIZE_EXIT` is `M68KENG_ERR_SIZE`;
`budget` 0 or above `M68KENG_MAX_BUDGET` is `M68KENG_ERR_ARG`; a call while
another `run()` of the domain is active is `M68KENG_ERR_STATE`. On any error
no instruction is executed, the CPU and the domain are unchanged and `exit`
is untouched. Otherwise it returns `M68KENG_OK` with `exit` written as section
2 says. It retires at most `budget + max_block_insns - 1` instructions:
it may stop at any instruction boundary, it never starts a block once
`budget` instructions have retired, and a block retires at most
`max_block_insns`. `exit->retired` is exact for every resumable reason and
at most the true count for the two terminal ones. An engine that runs code
it has generated makes sure, before it enters that code in each run, that
the processor executes it as published (on AArch64, an
instruction-synchronization barrier), also when it was published from
another core.

| reason | state | host may resume |
| --- | --- | --- |
| `STOP_BUDGET` | after the last retired instruction | yes, unchanged |
| `STOP_ALINE` | PC at the A-line opcode (`opcode`), nothing of it done | yes, after setting PC |
| `STOP_PC_OUTSIDE` | PC at the instruction whose fetch failed (`address`) | yes, after changing PC |
| `STOP_EXCEPTION` | PC at the instruction; it raises `vector`; nothing done | yes, after emulating or changing state |
| `STOP_UNSUPPORTED` | PC at the instruction (`opcode`); nothing done; `text` says why | yes, after emulating it |
| `STOP_HOST` | a host access returned 1; some boundary at or after the start of that instruction | no: read registers, then destroy or set every register |
| `STOP_FAULT` | engine failure (`text`) | no, as `STOP_HOST` |

`STOP_EXCEPTION` is reported only when the engine has determined the
architectural exception of the 68040 integer unit: illegal opcodes and
ILLEGAL (4), F-line opcodes that are not integer-unit instructions on the
LC040 model (11), TRAP #n (32 + n), TRAPV with V set (7), CHK/CHK2 out of
bounds (6), division by zero (5), odd PC (3). The engine never delivers
an exception itself in version 1.0. For any instruction it does not
execute it may answer `STOP_UNSUPPORTED`; that is always a legal answer.
A-line opcodes always end the run with `STOP_ALINE`, inside or outside the
host's library tables.

## 7. Calls, ownership and concurrency

| Operation | Allowed when | Effect and failure |
| --- | --- | --- |
| `domain_create(host, host_domain, map, required_caps, &domain)` | before any guest instruction of the domain | `OK`: a domain with no CPU; else `NULL` stored (when `domain` is not NULL) and nothing allocated. First failing check wins: `host`, `map` or `domain` NULL `ERR_ARG`; `host->size_of` `ERR_SIZE`; `host->abi_major` `ERR_VERSION`; `read` or `write` NULL `ERR_ARG`; `map->size_of` `ERR_SIZE`; map contents (base NULL, size below 0x1000, more than 16 ranges, unsorted, overlapping, empty, `hi > size`, no flag) `ERR_ARG`; `required_caps` `ERR_CAPS`; no usable machine `ERR_NOHOST`; allocation `ERR_NOMEM` |
| `cpu_create(domain, host_cpu, &cpu)` | no `run()` of the domain active | `OK`; else `NULL` stored (when `cpu` is not NULL) and nothing allocated. Checks in order: `domain` or `cpu` NULL `ERR_ARG`; a `run()` active `ERR_STATE`; allocation `ERR_NOMEM` |
| `set_reg`, `get_reg` | the CPU is not inside `run()` | as section 5 |
| `run(cpu, budget, exit)` | no other `run()` of the domain active, not from a host callback | as section 6, check order included |
| `invalidate(domain, addr, size)` | no `run()` of the domain active | `OK`; afterwards no CPU of the domain executes code derived from the old bytes of `[addr, addr + size)`; `size` 0 changes nothing; a range past 0xFFFFFFFF ends at the top of the address space, it does not wrap; `addr` 0 with `M68KENG_INVALIDATE_ALL` is all memory; the engine may drop more |
| `cpu_destroy(cpu)` | the CPU is not inside `run()` | `OK`; everything the engine held for that CPU is released |
| `get_stats(domain, stats)` | no `run()` of the domain active | checks in order: `domain` or `stats` NULL `ERR_ARG`; `stats->size_of < M68KENG_V1_SIZE_STATS` `ERR_SIZE`; a `run()` active `ERR_STATE`; on error `stats` untouched; on `OK` written as section 2 says |
| `domain_destroy(domain)` | every CPU of the domain destroyed | `OK`: the engine holds nothing of the domain any more (memory, executable code, references into guest memory); `ERR_STATE` with CPUs alive, nothing freed |

`ERR_STATE` is a host defect, never a normal outcome.

Ownership:

- The host owns guest memory, the map, the host services, the cookies, the
  exit and stats buffers and the library reference. Map and services stay
  valid until `domain_destroy` returns.
- The engine owns domains, CPUs, its counters and what it makes of the
  program's code. An engine that generates code allocates the memory for it
  itself (on native AROS `AllocMem(MEMF_PUBLIC | MEMF_EXECUTABLE)`,
  published with `CacheClearE(..., CACRF_ClearD | CACRF_ClearI)` before it
  runs) and frees all of it by `domain_destroy` at the latest. It never
  frees host memory and keeps pointers into guest memory only while the
  domain exists. Whether translations are shared by the CPUs of a domain
  is the engine's choice; the host sees no difference.
- The engine has no mutable global state outside its domains, or protects
  it with a semaphore: two programs run two domains on two tasks, and AROS
  preempts them.

Concurrency: the host makes at most one engine call per domain at a time.
In `m68kemu.library` this is the program lock: "a CPU runs 68k code only
while its task holds `ctx->gil`" (`INTERNALS.md`,
The Program Lock). Every
engine call on a domain while a CPU of it exists is made under that lock;
`domain_create` comes before the first CPU and `domain_destroy` after the
last, when no other call on the domain can happen. Calls on different
domains may run at the same time. A thunk that releases the lock to wait
leaves its CPU between runs, and another task of the program may run its
own CPU meanwhile; that is allowed.

Nesting: a native call that runs 68k code (`M68KEmu_Call68k`, the CIA
timer and `Cause()` paths, hooks) uses another CPU of the same domain and
runs it to its sentinel while the outer CPU waits between runs. The engine
is never reentered; the depth limit stays the host's (`M68KEMU_MAX_NEST`).

## 8. Invalidation

One generation per domain. `invalidate` is cheap for the caller and takes
effect for every CPU of the domain before its next instruction. The host
calls it after it writes guest code once a CPU of the domain exists: its
loaders (`M68KEmu_LoadHunks`, `M68KEmu_LoadHunksFromMemory`,
`M68KEmu_LoadM68KLibrary`), `CacheClearU`, `CacheClearE` with
`CACRF_ClearI`, a `CacheControl` change of the instruction cache bits, and
any stub it writes after the first `cpu_create`. Guest stores into code
already translated, without one of those calls, are not detected in version
1.0: the instruction cache behaves as enabled. A program that disables it
and relies on fresh stores is outside version 1.0. So a program that
changes its own code and does not flush the cache, as some programs
written for the 68000 do, works on the interpreter only.

## 9. Teardown order

For one program run:

1. A child task's CPU is destroyed when its code has returned, before the
   task releases the program lock and leaves (`child_entry`).
2. A task's nested CPUs are destroyed when the task is done with the
   program (`m68k_ts_drop`, `M68KEmu_FreeNestCpus`).
3. The root CPU is destroyed after its last run, before `m68k_ts_drop` of
   the root task returns.
4. Children and native programs may outlive the root's run; the context
   lives until the last of them releases it (`ctx->alive`,
   `INTERNALS.md`, Context Lifetime), possibly after
   `RunHunk` has returned. The last
   release calls `M68KEmu_DestroyContext`, which already frees every CPU and
   task state left: those CPUs are destroyed first, then `domain_destroy`
   is called and must return `M68KENG_OK`.
5. The host frees guest memory after that, in the same function.
6. The host closes the engine library after `domain_destroy` and after it
   drops its pointer to the ops table.

A CPU whose run ended with `STOP_HOST` or `STOP_FAULT` is destroyed by the
same rule. Nothing of the engine survives step 6 for that run.

## 10. Selection and outcomes (host behaviour)

This section describes the host side (`m68kemu_engine.c`,
`m68kemu_moira.cpp`). It is the host's choice and not part of the interface.

**The engine runs under the interpreter.** Every CPU of a run stays an
interpreter CPU (`AROSMoira`): it keeps the registers the thunks read and
write, takes every exception and interrupt and makes every library call as
it does without an engine. With the engine selected, each of those objects
also has an engine CPU, and each of the host's three execution loops (the
program, a process of the program, a call from native code into 68k code)
tries an engine slice before it interprets an instruction:

- The engine takes over only at a plain instruction boundary: nothing
  pending in the interpreter core (stopped, trace, an interrupt to check),
  no interrupt level asserted or raised and not yet taken, SR inside the
  model of section 5,
  PC even, at or above 0x1000 and inside memory. Below 0x1000 are the
  vectors and the host's own stubs.
- D0-D7, A0-A7, SR and PC go to the engine with `set_reg`, it runs with a
  budget of 1024, and they come back with `get_reg`. The PC comes back
  through the interpreter's own prefetch reads, so a run that ends at one
  of the return sentinels ends the loop exactly as it does without the
  engine.
- The instruction a run stopped at without executing it is one step of the
  interpreter. An A-line opcode is a library call through the existing
  dispatcher, with its exception frame and its RTE. `STOP_EXCEPTION`,
  `STOP_UNSUPPORTED` and `STOP_PC_OUTSIDE` are handled the same way: the
  interpreter does what it does without an engine, which for an odd PC is
  to execute from it, not to raise vector 3. So
  `Supervisor()`, RTE, exceptions with a guest handler, MOVEC, CINV, CPUSH,
  MOVE16 and the vertical blank and CIA interrupts are the interpreter's
  code, unchanged.
- `STOP_HOST`, `STOP_FAULT`, or a `run()` that does not answer `M68KENG_OK`,
  stop the program as a fault (`m68k_program_stop(ctx, TRUE)`) with a
  diagnostic giving PC, opcode and text, and the result of the run is 20.
  The host's callbacks never ask for a stop, so `STOP_HOST` is an engine
  defect.

The map (section 4) has the first page as a range written through the host
(the low-memory guard and the host's stubs), the CIA and custom chip pages
as ranges read and written through the host, and the bitplane range as a
range written through the host while a program draws into a screen's
planes. `generation` is incremented when a thunk changes that last range.
The host's `read` and `write` call the interpreter's own access functions
on the CPU that makes the access.

Selection. The environment variable `M68KEMU_CPU` is read once per
`RunHunk`/`RunFile` call, before the program is loaded (local variable
first, then global):

| value | run |
| --- | --- |
| unset, empty | the interpreter, exactly as without this code, and no diagnostic |
| `interpreter` | the interpreter; one line says so |
| `jit` | the engine under the interpreter. If the library is absent, its function gives no table of interface 1.0, `domain_create` fails or the engine CPU of the program cannot be made, the call returns -1 with a diagnostic and no guest instruction runs |
| `auto` | as `jit` when all of that works, otherwise the interpreter, with the reason in the line |
| anything else, or a value too long for the host's buffer | the call returns -1 with a diagnostic |

With the variable set, one line per run says what runs the program: the
engine's `name` and interface version, or the interpreter and why. After a
run with the engine a second line gives its counters (`get_stats`) and the
number of instructions the interpreter ran, so that a run is never reported
as more of the engine's than it was. An engine CPU that cannot be made for
a later process or nested call leaves that CPU with the interpreter, with
one line.

Invalidation (section 8) is called for all memory at the three loaders,
`M68KEmu_SetupLibBase`, the two stubs that thunks write into the heap, the
exec calls `CacheClearU`, `CacheClearE` and `CacheControl`, and at CINV or
CPUSH.

Teardown follows section 9. An engine CPU is destroyed with its interpreter
CPU, the program's own before `m68k_ts_drop`. `M68KEmu_DestroyContext`
destroys the domain after the last CPU and before it frees the program's
memory, then closes the library.

Limits of this host side:

- The interpreter stays linked and needed.
- The instruction ring of `M68KEMU_TRACE` has one entry per engine slice.
- An interrupt can be late by one slice.
- Code that a program stores into memory it already ran, without a cache
  call, is stale under the engine and fresh under the interpreter (section
  8).

## 11. Scope

Version 1.0 is for one processor. It claims no SMP, MMU, FPU,
full 68040 coverage, PPC, mid-run backend switching, serialized CPU state or
live migration of a run. Capability bits for some of these are reserved
only. Unsupported instructions are reported truthfully, never skipped.
