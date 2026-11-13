# m68kemu Internals

**Author:** Fabian Schmieder ([@metaneutrons](https://github.com/metaneutrons))

Deep technical reference for the m68kemu.library implementation. For an overview,
see [README.md](README.md). Every number below names the constant or table it
comes from; the source files are in `rom/m68kemu/`.

## Table of Contents

- [Memory Model](#memory-model)
- [CPU Integration](#cpu-integration)
- [Tasks and the Program Lock](#tasks-and-the-program-lock)
- [Semaphores, Ports and Messages](#semaphores-ports-and-messages)
- [Thunk System Details](#thunk-system-details)
- [Shadow Struct Engine](#shadow-struct-engine)
- [Shadow Struct Layouts](#shadow-struct-layouts)
- [68k Libraries and Relocations](#68k-libraries-and-relocations)
- [Device I/O Path](#device-io-path)
- [Custom Chip and CIA Emulation](#custom-chip-and-cia-emulation)
- [Thunk Generator Toolchain](#thunk-generator-toolchain)
- [Diagnostics](#diagnostics)
- [Test Infrastructure](#test-infrastructure)
- [Known Bugs and Limitations](#known-bugs-and-limitations)

## Memory Model

### Containment Buffer

`M68KEmu_CreateContext()` (`m68kemu_memory.c`) allocates a single 32 MB buffer
(`M68KEMU_MEM_SIZE`) with `AllocMem(MEMF_CLEAR | MEMF_31BIT)`. All m68k addresses
are byte offsets into this buffer:

```c
host_ptr = ctx->mem + m68k_addr;    // m68k → host (m68k_to_host)
m68k_addr = host_ptr - ctx->mem;    // host → m68k (host_to_m68k)
```

`m68k_to_host()` returns NULL for address 0 and for anything at or past
`mem_size`. `m68k_to_host_or_shadow()` resolves in three steps: a native BPTR
handle (see below), then the shadow registry, then containment.

### Bounds Checking of Guest Addresses

Guest addresses are 32-bit and arrive from program data, so every accessor in
`m68kemu_intern.h` checks the whole access before adding to the base pointer:

| Accessor | Behaviour |
|----------|-----------|
| `m68k_in_mem(ctx, addr, len)` | TRUE only if `addr != 0`, `addr < mem_size` and `len <= mem_size - addr`; used by the thunks before reading a structure of `len` bytes |
| `m68k_read16/32` | 0 when the 2 or 4 bytes do not fit inside `mem_size` |
| `m68k_write16/32` | dropped when they do not fit; while `ctx->low_guard` is set, writes below 0x400 are dropped and reported by `m68k_low_write_trap()`, except to 0x10, 0x28, 0x2C, 0x6C and 0x140–0x18F (the emulator's own cells) |
| `m68k_write32_raw` | the unguarded form, for the emulator's own writes to low memory |

The CPU side (`AROSMoira::read8/read16/write8/write16` in `m68kemu_moira.cpp`)
checks the same way and, under `low_guard`, drops program writes to addresses
below 8 and to 0x100–0x17F. The regression `test_memory_bounds` compiles these
functions verbatim (see [Test Infrastructure](#test-infrastructure)).

### Native BPTR Handles

A native BPTR or opaque native pointer is 64 bits wide and cannot travel through
a 32-bit register. `m68k_bptr_out()` hands the program an index instead:
`M68KEMU_HANDLE_BASE` (0x7F000000) + 4 × slot, from a table of
`M68KEMU_MAX_HANDLES` (1024) entries allocated on first use; the same native
value always maps to the same handle. `m68k_bptr_in()` translates back, and
treats values outside the handle range as m68k BPTRs into containment
(`MKBADDR(m68k_to_host(v << 2))`). The dispatcher applies the same rule to every
thunk result whose upper 32 bits are neither 0 nor all ones: a pointer into
containment becomes its m68k address, anything else a handle.

### Address Map

```plain
0x000000 - 0x0003FF   Exception vectors (M68KEMU_VECTORS_SIZE)
  0x000004              AbsExecBase (restored every instruction if a program changes it)
  0x000028              LINEA vector (M68K_VEC_LINEA) → RTE_ADDR
  0x00006C              Level 3 autovector → M68KEMU_VBL_STUB
  0x000100              RTE instruction (RTE_ADDR)
  0x000108              Supervisor trampoline: ADDQ.L #8,SP; JMP (A5) (SV_TRAMP_ADDR)
  0x000140 - 0x000161   Vertical blank stub (M68KEMU_VBL_STUB, 34 bytes)
  0x000170 / 0x000174   is_Data / is_Code cells of the VBL server
  0x000180              addq.l #4,2(sp); rte (M68KEMU_SKIP4_STUB)
  0x000188              A-line $A0FF, the "unhandled exception" marker (M68KEMU_FAULT_STUB)
0x000400 - 0x0103FF   Library base region (M68KEMU_LIBBASE_REGION, M68KEMU_LIBBASE_SIZE 64 KB)
                        Each library: num_vectors × 6 bytes (A-line + 2×NOP) below its base
0x010400 - 0x1FFFFF   Heap, "chip memory" region (M68KEMU_HEAP_BASE .. M68KEMU_CHIP_END)
0x200000 - 0x1FEFFFF  Heap, "expansion fast memory" region (M68KEMU_CHIP_END .. stack_bottom)
  0xF80000 - 0xFFFFFF   Kickstart ROM area, left out of the Fast region (M68KEMU_ROM_BASE/SIZE)
0x01FF0000 - 0x01FFFFFF Supervisor stack of the program (M68KEMU_STACK_SIZE 64 KB, grows downward)
0x7F000000 + 4n       Native BPTR handles (M68KEMU_HANDLE_BASE)
0xFFDEAD00            Return address of the program (SENTINEL_ADDR), outside memory
0xFFDEAE00            Return address of a nested call (NEST_SENTINEL), outside memory
```

Exception vectors 2–47, except LINEA (10) and the level 3 autovector (27), point
at `M68KEMU_FAULT_STUB`. The ROM area holds a ROM header (`0x11144ef9`,
`0x00f80002`, `0x0000ffff`, exec's `lib_Version`/`lib_Revision`), the ROM size at
`M68KEMU_ROM_END - 20` and a copy of exec's `lib_IdString` at `ROM_BASE + 0x100`,
so programs that read the Kickstart version find one.

`M68KEmu_Execute()` also allocates from the heap a 16 KB user stack (USP), an
8 KB interrupt stack (`irq_stack_top`), and, on first use by `M68KEmu_Call68k()`
without a 68k caller, a 32 KB callback stack (`cb_stack_top`).

### Heap Allocator

Two regions (`ctx->rgn[RGN_CHIP]`, `ctx->rgn[RGN_FAST]`), each with its own free
list and its own 68k `MemHeader` in the ExecBase `MemList` ("chip memory",
priority -10; "expansion fast memory", priority 0). The free lists are the
MemHeaders' chunk lists, so a program that walks `MemList` sees the real state;
`heap_publish()` updates `mh_First`/`mh_Free` after every operation.

- **Free block**: an Amiga `MemChunk` (`FB_NEXT` 0, `FB_SIZE` 4)
- **Allocated block**: 8-byte header (`M68KEMU_HEAP_HDR`) holding the size
- **Alignment**: 4 bytes (`(size + 3) & ~3`)
- **Strategy**: first fit; a block is split when the remainder is at least
  `M68KEMU_HEAP_MIN_SPLIT` (16) bytes, allocating from the end of the free block
- **Requirements**: `MEMF_CHIP` allocates from the Chip region only, `MEMF_FAST`
  from the Fast region only, anything else tries Fast first, then Chip;
  `MEMF_CLEAR` is honoured
- **Free**: `M68KEmu_HeapFree()` ignores addresses outside the heap or in the ROM
  area, blocks with a bad header, and blocks already on the free list (the
  program's `FreeMem()` takes any pointer); then address-ordered insertion with
  forward and backward coalescing
- **AvailMem**: `M68KEmu_HeapAvail()` handles `MEMF_CHIP`, `MEMF_FAST`,
  `MEMF_TOTAL` and `MEMF_LARGEST`
- **AllocAbs**: `M68KEmu_HeapAllocAbs()` carves a block at a fixed address if that
  range is free
- **Allocate/Deallocate**: `thunk_exec_Allocate_m`/`thunk_exec_Deallocate_m`
  work on a `MemHeader` in the program's memory
- **Pools**: `CreatePool` returns a 12-byte header in containment
  (`requirements, 'POOL', first`); every `AllocPooled` block carries a 16-byte
  header (`POOL_HDR`) linking it into the pool, so `DeletePool` frees them all

### Limits

| Resource | Limit | Constant / location |
|----------|-------|---------------------|
| Containment memory | 32 MB | `M68KEMU_MEM_SIZE` |
| Program stack | 64 KB | `M68KEMU_STACK_SIZE` |
| Chip RAM region | 0x010400–0x1FFFFF | `M68KEMU_CHIP_END` |
| Library bases | 64 | `M68KEMU_MAX_LIBS` |
| LVOs per library | 512 | `M68KEMU_MAX_LVO` |
| Shadow entries | 1024 | `M68KEMU_MAX_SHADOWS` |
| Native BPTR handles | 1024 | `M68KEMU_MAX_HANDLES` |
| Tasks in the fixed table | 32, further ones on `more_tasks` | `M68KEMU_MAX_TASKS` |
| Nested 68k calls per task | 8 | `M68KEMU_MAX_NEST` |
| Methods in flight per task | 16 | `M68KTaskState.msgstack` |
| Gadget map | 256 | `M68KEMU_MAX_GADGETS` |
| Menu/MenuItem map | 512 | `M68KEMU_MAX_MENUITEMS` |
| IORequests | 64 | `M68KEMU_MAX_IOREQS` |
| BOOPSI objects / classes / lists | 512 / 64 / 64 | `M68KEMU_MAX_BOBJS`, `M68KEMU_MAX_BCLASSES`, `M68KEMU_MAX_BLISTS` |
| Class libraries | 32 | `M68KEMU_MAX_CLIBS` |
| Classes written in 68k code | 32 | `ctx->m68kclasses` |
| 68k libraries loaded from disk | 16 | `ctx->m68klibs` |
| Public 68k semaphores | 16 | `ctx->pubsems` |
| Native processes started from seglists | 8 | `ctx->native_procs` |
| ReadArgs result blocks | 16 | `M68KEMU_MAX_RDARGS` |
| Pattern-match anchors / FIBs | 16 / 16 | `ctx->anchors`, `ctx->fibs` |
| File notifications | 64 | `ctx->notify` |
| Icons in 68k layout | 64 | `ctx->icons` |
| Grown windows | 32 | `M68KEMU_MAX_GROWN` |
| Hunks per file | 100 | `M68KEmu_LoadHunksFromMemory()` |
| PC history / library call history | 512 / 2048 | `ctx->pc_ring`, `ctx->callring` |

## CPU Integration

### Moira Subclass

`AROSMoira` (`m68kemu_moira.cpp`) extends `moira::Moira` with:

- **Memory callbacks**: `read8/read16/write8/write16` intercept the CIA range,
  the custom chip range (`CUSTOM_BASE` $DFF000, `CUSTOM_MASK` $FFF000), the two
  sentinel addresses and out-of-bounds accesses; `write8/write16` also call
  `m68k_planar_touch()` while a program screen has bitplanes in containment
- **`read16OnReset` / `read16Dasm`**: plain bounds-checked reads
- **Exception dispatch**: `willExecute()` handles LINEA (thunk dispatch) and the
  other exceptions (below)
- **`didChangeCACR`**: keeps only bits 31 and 15 of CACR, as a 68040 does
- **Halt handler**: `cpuDidHalt()` calls `m68k_program_stop(ctx, TRUE)`

The CPU model is `Model::M68LC040` for every CPU the emulator creates: Moira
executes no FPU instructions, so F-line instructions trap as on an LC040 and a
program's own FPU emulator can take them. `AttnFlags` is set to
`M68K_ATTNFLAGS_040FPU` (0x000F: 68010–68040, no FPU bit).

### Run Loop

```plain
M68KEmu_Execute(ctx):
  1. Start the watchdog if M68KEMU_TRACE is set; m68k_ts_main() makes this task tasks[0]
  2. Construct AROSMoira on the stack, model M68LC040
  3. Write stack_top → vector 0, entry_point → vector 4; cpu.reset()
  4. Restore AbsExecBase at address 4; clear the interrupt mask in SR
  5. Allocate a 16 KB user stack and set USP
  6. Plant RTE at 0x100, the Supervisor trampoline at 0x108, LINEA vector → 0x100
  7. Push SENTINEL_ADDR as the program's return address
  8. A0 = argptr, D0 = argsize, A6 = m68k SysBase
  9. Plant the VBL stub (0x140), SKIP4 (0x180) and FAULT (0x188) stubs,
     point vectors 2–47 (except 10 and 27) at the FAULT stub, allocate the 8 KB interrupt stack
  10. running = TRUE, low_guard = TRUE, m68k_gil_obtain()
  11. while (ctx->running):
        record PC in pc_ring[512]
        raise the vertical blank (level 3) when due and not Disable()d
        put AbsExecBase back if the program overwrote it
        stop with a message if PC is outside memory
        cpu.execute()
        every 65536 instructions: m68k_gil_yield()
  12. exit_code = D0; m68k_ts_drop(); m68k_gil_release(); stop the watchdog
```

`setup_and_execute()` in `m68kemu_run.c` runs this either inline (`stackSize ==
0`) or in a child process created with `CreateNewProc(NP_Synchronous)`, named
"m68kemu", inheriting Input/Output, current directory, `PROGDIR:` (RunFile) and
the caller's CLI status. `stackSize` only selects between the two; the 68k stack
is always `M68KEMU_STACK_SIZE`.

### Other CPUs

- `M68KEmu_RunTask()` runs a process the program created on a CPU of its own
  (heap-allocated `AROSMoira`), returning to `NEST_SENTINEL`, yielding the
  program lock every 65536 instructions, until its code returns, `Exit()` is
  called or the program faults.
- `M68KEmu_Call68k()` calls 68k code from native code (hooks, BOOPSI
  dispatchers, the library-init tail of a 68k library). Each task keeps one CPU
  per nesting level (`nest_cpu[M68KEMU_MAX_NEST]`), made once; the code runs on a
  stack 256 bytes below the calling CPU's SP, or on the callback stack when there
  is no 68k caller, with D0–D7/A0–A6 from `regs[15]` and D0 as the result.
- `M68KEmu_StopCpu()` implements dos `Exit()`: a nested CPU finishes, the main
  CPU stops the program.

### Termination

- **Sentinel**: an RTS to `SENTINEL_ADDR` makes `read16()` return the STOP opcode
  (`M68K_OP_STOP` 0x4E72) and clear `ctx->running`; `NEST_SENTINEL` does the same
  for a nested CPU (`nested_done`)
- **Fault**: `m68k_program_stop(ctx, TRUE)` on HALT, on an unhandled exception,
  on a jump outside memory, or on `dos Exit()` with `fault = FALSE`; a faulted
  program takes its child processes with it (`ctx->faulted`)

### A-Line Dispatch (willExecute, LINEA)

1. Restore the LINEA vector to `RTE_ADDR` (a Supervisor() call may have moved it)
2. If PC0 is `M68KEMU_FAULT_STUB`: log "unhandled exception N at PC", dump
   registers and the last instructions when tracing, stop the program
3. Scan `ctx->libs[]` for the jump table containing PC; LVO = `lib->m68k_addr - pc`
4. `m68k_sync_windowptr()` copies the program's `pr_WindowPtr` to the native process
5. Search the manual table (`lib->thunks[]`), then the generated one
   (`lib->gen_thunks[]`); the first entry with that LVO and a non-NULL function wins
6. Pop the JSR return address, record `ts->cur_cpu`, call the thunk, record the
   call in `callring` when `M68KEMU_TRACE=end`
7. If `ts->sv_redirect` is set: `dispatchSupervisor()` (below) instead of returning
8. Translate a 64-bit result (containment pointer → address, else handle); for a
   generated thunk in `returns_arg_pointer()` (FGets, FilePart, PathPart, IDtoStr,
   NextPubScreen, BumpRevision) a pointer into containment becomes its address
9. Set D0; apply `ts->ccr_val` if a thunk set condition codes (math libraries)
10. If `ts->tail_pc` is set: push the return address and continue in 68k code at
    `tail_pc` with `tail_d0/tail_a0/tail_a6` (68k library init, character hooks),
    whose RTS returns to the caller
11. Set PC0/PC so Moira's exception frame returns to the caller through the RTE
12. Unimplemented LVO: pop the return address, D0 = 0, `m68k_log()` reports
    "`<lib>` function -N is not supported yet"

### Other Exceptions (willExecute, non-LINEA)

The vector is read through VBR (programs may install their own table).

- **ILLEGAL on MOVEC** (`0x4E7A`/`0x4E7B`) with no program handler: the
  instruction is skipped and the destination register reads 0, as an MMU that is
  switched off would answer (SysInfo reads the 68040 TT/MMU registers)
- **LINEF**: `emulate040()` executes CINV and CPUSH as no-ops and MOVE16 as a
  16-byte copy (both the `(Ax)+,(Ay)+` and the absolute forms); other F-line
  instructions go to the program's handler, or to the fault stub
- Any other exception with vector 0 is pointed at `M68KEMU_FAULT_STUB`, so the
  next instruction fetch reports it as unhandled

### Supervisor() Support

`thunk_exec_Supervisor` stores the user function address in `ts->sv_redirect`.
After the thunk returns, `willExecute` calls `dispatchSupervisor(ret_addr)`:

1. Pushes a 68020-format frame (vector offset 0, return PC, SR = the caller's
   current SR) onto the stack; the program already runs in supervisor mode from
   reset, so an RTE into user mode would hand it the user stack pointer
2. Redirects the LINEA vector to `SV_TRAMP_ADDR`
3. Sets PC0/PC to the user function, so Moira's own frame lands on the
   trampoline, which skips it (`ADDQ.L #8,SP`) and does `JMP (A5)`
4. The user function (typically `MOVE.W SR,D0; RTE`) returns through the
   manually pushed frame to the caller

`SuperState()` answers with the current stack pointer and `UserState()` does
nothing, for the same reason. `StackSwap()` exchanges SP with `stk_Pointer`.

### Vertical Blank Interrupt

`AddIntServer(INTB_VERTB, is)` (`thunk_exec_AddIntServer`) records `is_Data` and
`is_Code` in `ctx->vbl_data/vbl_code` and in the cells at 0x170/0x174; other
interrupt numbers are ignored. `m68k_vbl_due()` opens `timer.device`
(`UNIT_ECLOCK`) once and compares `ReadEClock()` with the last tick: the server
runs at 50 Hz (`vbl_efreq / 50`). The run loop checks every 1024 instructions
while `ctx->disable` is 0, switches to the interrupt stack when the CPU is in
user mode, and raises IPL 3; the stub at 0x140 saves D0–D1/A0–A1/A5–A6, loads
A0 = $DFF000, A1 = `is_Data`, A5 = `is_Code`, A6 = SysBase, calls the server and
returns with RTE. `RemIntServer(INTB_VERTB)` clears the server. ExecBase's
`VBlankFrequency` and `PowerSupplyFrequency` read 50.

## Tasks and the Program Lock

A program is no longer one task: it may `CreateNewProc()` processes of its own,
and native code may call back into it (hooks, dispatchers). Every task that runs
the program's code has a `struct M68KTaskState` (`m68kemu_intern.h`,
`m68kemu_tasks.c`):

| Field | Purpose |
|-------|---------|
| `task` | the native task; NULL marks a free slot |
| `cur_cpu`, `nest_cpu[8]`, `nest_depth` | the CPU whose library call is running, and the CPUs for nested calls into 68k code |
| `msgstack[16]`, `msgdepth` | BOOPSI methods passed to 68k dispatchers, so a superclass call gets the native method back |
| `tail_pc`, `tail_d0/a0/a6` | 68k code to run after a thunk, as if the caller had called it |
| `ccr_valid`, `ccr_val` | condition codes set by a thunk |
| `sv_redirect` | pending `Supervisor()` user function |
| `forbid`, `disable` | the task's Forbid()/Disable() nesting while it does not hold the lock |
| `semsig` | the signal a released 68k semaphore wakes it with |
| `running` | cleared when a process of the program ends (`Exit()`) |
| `stash[16]`, `nstash` | tag data translated for a BOOPSI object not made yet |

`m68k_ts()` returns the calling task's state, creating it on first use: before
`emu_task` is set (library setup done by whichever task), `tasks[0]`; then a
free slot of `tasks[M68KEMU_MAX_TASKS]`, or an `AllocMem()`ed state on
`more_tasks`, or `ctx->orphan` when even that fails. `m68k_ts_drop()` frees the
nested CPUs, the signal and the stash when a task is done with the program.

### The Program Lock

The emulator's state (memory, shadow registry, maps) is shared, so a CPU runs
68k code only while its task holds `ctx->gil`, a native `SignalSemaphore`.

- `m68k_gil_obtain()` takes the lock and moves the task's saved Forbid()/Disable()
  nesting into `ctx->forbid/disable`; `m68k_gil_release()` saves it back. Exec
  keeps a task's own nesting while it waits; the program's nesting travels with
  the task that owns it, so the other tasks are not forbidden by it.
- `m68k_gil_yield()` releases and re-obtains the lock between instructions
  (every 65536) when another task is queued (`ss_QueueCount > 0`) and the program
  has not called Forbid() or Disable().
- Calls that wait release the lock around the native call: `Wait`, `WaitPort` on
  a native port, `Delay`, `DoIO`, `WaitIO`, `ObtainSemaphore` on a native
  semaphore, and the 68k semaphore and port waits.
- `Forbid`/`Permit` count `ctx->forbid`; `Disable`/`Enable` count both
  `ctx->forbid` and `ctx->disable`. `Disable()` also suspends the vertical blank.
  Native interrupts are never turned off for the program.

### Processes of the Program

`m68k_dos_CreateNewProc` (`m68kemu_tasks.c`) reads the tag list from the
program's memory (up to 12 tags are passed on):

- `NP_Entry`: 68k code. A native process (stack 65536, `child_entry`) obtains the
  lock, gives itself an m68k Process stand-in (`m68k_process_of`) and runs the
  entry on its own CPU with `M68KEmu_RunTask()`, on a 68k stack from the heap
  (`NP_StackSize` rounded up to 16, at least 16384). `NP_ExitCode`/`NP_ExitData`
  run as 68k code afterwards unless the program faulted.
- `NP_Seglist`: a seglist the program got from `LoadSeg()`. If
  `GetSegListInfo(GSLI_68KHUNK)` says it is a hunk file, `M68KEmu_LoadHunks()`
  copies it into this program's memory and it runs as above. Otherwise it is a
  native program (Directory Opus starting the WBStartup programs): it is started
  natively with `NP_Seglist`, a stack of at least 65536, `NP_ExitCode =
  m68k_native_proc_exit`, and recorded in `ctx->native_procs[8]`.
- `NP_Name` is copied; `NP_Input/Output/Error/CurrentDir/HomeDir` are translated
  with `m68k_bptr_in()`; `NP_Priority`, `NP_Close*`, `NP_Cli` and
  `NP_FreeSeglist` pass through.

The parent counts the child in `ctx->num_children` before it runs and takes a
hold on the context (below). `Cli()` returns the `pr_CLI` of the caller's m68k
Process; `FindTask()` returns one m68k Process stand-in per native task, re-synced
on every call, with `pr_MsgPort` registered as a shadow of the native port and a
68k CLI structure when the task has one.

### Context Lifetime

`ctx->alive` counts the users of a context: the caller of `RunHunk`/`RunFile`
(taken by `M68KEmu_CreateContext()`), each process of the program and each native
program it started. `m68k_context_hold()` increments it and the library's
`lib_OpenCnt`; `m68k_context_release()` decrements, and the last release calls
`M68KEmu_DestroyContext()`, which closes the VBL timer, frees all CPUs and task
states, ends file notifications, frees FIBs, pattern-match anchors (`MatchEnd`),
handles, BOOPSI state, the `gfx_owned` list, native RastPort twins and the memory
buffer. The library stays open while any of them runs, so a child process can
still execute the library's code after `RunHunk` has returned.

## Semaphores, Ports and Messages

### 68k Semaphores

A `SignalSemaphore` the program initialises lives in its memory in 68k layout
(`ss_NestCount` 14, `ss_WaitQueue` 16, `ss_Owner` 40, `ss_QueueCount` 44) and is
operated on there by `m68k_sem_init/obtain/release/list()` in `m68kemu_tasks.c`,
under the program lock, so the fields change as if in Forbid(). `sem_native()`
decides: an address that is inside containment and not a shadow is a 68k
semaphore; anything else (a shadow or a handle) is a native semaphore and is
used natively, with the lock released while waiting.

- `ObtainSemaphore`/`ObtainSemaphoreShared`: taken at once when free, owned by
  the caller, or shared with no owner and no queue; otherwise a 12-byte
  `SemaphoreRequest` (`sr_Waiter` = the caller's m68k Process, bit 0 set for a
  shared request) is queued and the task waits on its `semsig` with the lock
  released. `AttemptSemaphore`/`AttemptSemaphoreShared` return FALSE instead.
- `ReleaseSemaphore`: when the nest count reaches 0, `sem_handover()` grants the
  head of the queue (one exclusive request, or every shared request in a row)
  and signals those tasks.
- `ObtainSemaphoreList`/`ReleaseSemaphoreList` walk the program's list (guard 1024).
- `AddSemaphore`/`RemSemaphore` keep up to 16 public 68k semaphores
  (`ctx->pubsems`) for `FindSemaphore`, which looks there by `ln_Name` first and
  then returns a native `FindSemaphore()` result as a handle.

### Ports and Messages

A message the program makes lives in its memory in 68k layout. `m68k_deliver()`
(`PutMsg`, `ReplyMsg`) distinguishes two kinds of port:

- a **native port** (a shadow: `CreateMsgPort`, a window's `UserPort`, a process's
  `pr_MsgPort`): the message travels in a small native carrier
  (`struct M68KCarrier`, `ln_Name` "m68kemu carrier") holding the m68k address and
  type; `m68k_msg_from_native()` unwraps it in `GetMsg`
- a **port in the program's memory** (`m68k_port_is_68k()`: inside containment,
  no shadow): the message is queued on `mp_MsgList` as exec queues it and the
  port's task (found through the shadow of `mp_SigTask`) is signalled when
  `mp_Flags` says `PA_SIGNAL`; `WaitPort` waits on `1 << mp_SigBit` with the lock
  released

A port the program built itself and passes to a native call (a reply port for
`OpenDevice`, a commodities `CX_SEND` object) gets a native twin on the same task
and signal bit (`m68k_port_twin()`), registered as its shadow. A `WBStartup`
message sent to a native program the program started is rebuilt natively with
its lock, segment and up to 64 `WBArg`s (`deliver_wbstartup()`), and the reply
comes back as the program's own message.

The exec list functions (`Insert`, `AddHead`, `AddTail`, `Remove`, `RemHead`,
`RemTail`, `Enqueue`, `FindName`) operate on 32-bit big-endian links in the
program's memory, unless the list is a shadow of a native list, which is left to
exec. `GetMsg` on a native port also recognises replied I/O requests
(`m68k_ioreq_of`), file notifications (`NotifyMessage`) and IntuiMessages.

## Thunk System Details

### Calling Convention

A thunk has the signature `IPTR thunk(struct M68KEmuContext *ctx, void *cpu)` and
is registered as `struct M68KThunkEntry { ULONG lvo; thunk }` (`m68kemu_thunks.h`).
Register access macros:

| Macro | Purpose |
|-------|---------|
| `THUNK_D(n)` | Read data register Dn as ULONG |
| `THUNK_A(n)` | Read address register An as ULONG (raw m68k address) |
| `THUNK_PTR(n)` | Read An and translate via `m68k_to_host_or_shadow` |
| `THUNK_DPTR(n)` | Read Dn and translate via `m68k_to_host_or_shadow` |
| `THUNK_SET_D(n, v)` | Write data register Dn |
| `THUNK_SET_A(n, v)` | Write address register An |

The return value is placed in D0 by the dispatcher, after the handle/address
translation described under [A-Line Dispatch](#a-line-dispatch-willexecute-linea).
`M68KEmu_GetSR/SetSR/SetPC` exist for the few thunks that need them.

### Library Registration

`exec.library` (lib_id 0) and `dos.library` (lib_id 1) are set up before the
program starts, with `M68KEMU_MAX_LVO` vectors each. Every other base is made on
the program's first `OpenLibrary()` by `M68KEmu_OpenLibrary()`
(`m68kemu_thunks.c`), in this order:

1. an existing base of that name (`M68KEmu_FindLibBase`)
2. `workbench.library`, `rexxsyslib.library` and the six math libraries: manual
   tables only
3. one of the 17 generated libraries (`m68kemu_all_gen_libs[]`), paired with its
   manual table where one exists (intuition, graphics, gadtools, diskfont,
   utility, icon, locale, commodities)
4. a native BOOPSI class library (`.class`, `.gadget`, `.image`;
   `m68k_open_class_library`)
5. a 68k library already loaded from disk (re-opened through its own Open vector)
6. a 68k library loaded from disk (`M68KEmu_LoadM68KLibrary`)

`M68KEmu_SetupLibBase()` sizes the jump table from the highest LVO in both tables
(`m68k_lib_vectors()`: `max/6 + 4`, capped at `M68KEMU_MAX_LVO`), fills every slot
with `A-line, NOP, NOP`, reports the native library's `lib_Version`/`lib_Revision`,
and gives `graphics.library`, `intuition.library` and `dos.library` their full
m68k base layout synced from the native base (`GfxBase`, `IntuitionBase`,
`DosLibrary`), since programs read `ActiView`, `FirstScreen` and `dl_Root` from
them. `m68k_publish_base()` names the base and links it into the ExecBase
`LibList`, `DeviceList` or `ResourceList`. Devices get a base with 16 vectors at
`OpenDevice()`, class libraries one with 24, the BOOPSI pseudo-library one with 8.

### Manual Thunk Priority

When both tables hold the same LVO, the manual thunk wins: `willExecute` searches
`lib->thunks[]` before `lib->gen_thunks[]`. Within a table the first matching
entry wins (dos registers LVO -198 twice; the first entry, `thunk_dos_Delay_m`,
is used).

### Manual Thunk Tables

Counts exclude the `{ 0, NULL }` terminators, which the dispatcher skips.

| Table | Library | Entries | File |
|-------|---------|---------|------|
| `m68kemu_thunks_exec` | exec.library | 71 | `m68kemu_thunks.c` |
| `m68kemu_thunks_dos` | dos.library | 66 | `m68kemu_thunks.c` |
| `m68kemu_thunks_intuition_manual` | intuition.library | 48 | `m68kemu_thunks.c` |
| `m68kemu_thunks_graphics_manual` | graphics.library | 42 | `m68kemu_thunks.c` |
| `m68kemu_thunks_gadtools_manual` | gadtools.library | 15 | `m68kemu_thunks.c` |
| `m68kemu_thunks_utility_manual` | utility.library | 20 | `m68kemu_thunks.c`, `m68kemu_tags.c` |
| `m68kemu_thunks_locale_manual` | locale.library | 3 | `m68kemu_thunks.c` |
| `m68kemu_thunks_diskfont_manual` | diskfont.library | 1 | `m68kemu_thunks.c` |
| `m68kemu_icon_manual` | icon.library | 15 | `m68kemu_icon.c` |
| `m68kemu_cx_manual` | commodities.library | 2 | `m68kemu_math.c` |
| `wb_tab` | workbench.library | 15 | `m68kemu_math.c` |
| `rexx_tab` | rexxsyslib.library | 10 | `m68kemu_rexx.c` |
| `ffp_tab`, `sb_tab`, `db_tab` | mathffp, mathieeesingbas, mathieeedoubbas | 12 each | `m68kemu_math.c` |
| `fft_tab`, `st_tab`, `dt_tab` | mathtrans, mathieeesingtrans, mathieeedoubtrans | 17 each | `m68kemu_math.c` |
| `dev_std_tab`, `dev_timer_tab`, `dev_console_tab` | device bases | 2, 7, 3 | `m68kemu_devices.c` |
| `boopsi_tab` | "boopsi" pseudo-library | 1 | `m68kemu_boopsi.c` |
| `clib_plain`, `clib_layout`, `clib_nodes`, `clib_listbrowser` | class libraries | 1, 3, 5, 6 | `m68kemu_boopsi.c` |

The library tables together hold 395 entries.

### Handling Families

**exec.library** (`m68kemu_thunks.c`, `m68kemu_tasks.c`, `m68kemu_devices.c`)

- Memory: `AllocMem`/`FreeMem`/`AllocVec`/`FreeVec`/`AllocAbs`/`AvailMem` on the
  containment heap; `Allocate`/`Deallocate` on a program `MemHeader`;
  `AllocEntry`/`FreeEntry`; pools as described above; `TypeOfMem` answers from
  the region's `MemHeader` attributes (0 for the ROM area and outside memory);
  `CopyMem`; `CacheClearU` no-op; `CacheControl` keeps the bits in `ctx->cacr`
- Lists: `Insert`..`Enqueue`, `FindName` in the program's memory (`FindName` on
  the ExecBase `LibList` opens the named library and returns its base; on
  `DeviceList`/`ResourceList` it returns 0, since those have no m68k base)
- Tasks: `FindTask`, `Forbid`/`Permit`/`Disable`/`Enable`, `Wait`, `SetSignal`,
  `StackSwap`, `SuperState`/`UserState`, `Supervisor`
- Messages: `PutMsg`, `GetMsg`, `ReplyMsg`, `WaitPort`; `AddPort`/`RemPort` no-ops
- Semaphores: 11 functions (see above)
- Libraries: `OpenLibrary`, `OldOpenLibrary`, `CloseLibrary` (no-op),
  `OpenResource` (only a base that already has thunks; a native resource base
  cannot be called from 68k code)
- Interrupts: `AddIntServer`/`RemIntServer`, vertical blank only
- Devices: `OpenDevice`, `CloseDevice`, `DoIO`, `SendIO`, `CheckIO`, `WaitIO`,
  `AbortIO` (see [Device I/O Path](#device-io-path))
- `RawDoFmt`: the m68k datastream is repacked by specifier
  (`translate_fmt_args`); flags, width, precision and `l` are accepted; the result
  is formatted into a 1024-byte buffer and copied to the `PutChData` buffer (A3).
  The `PutChProc` (A2) is not called.
- `AVL_AddNode`, `AVL_RemNodeByAddress`, `AVL_FindNode`: stubs returning 0

**dos.library** (`m68kemu_thunks.c`, `m68kemu_tasks.c`)

- File handles, locks and seglists travel as native BPTR handles
  (`Open`, `Close`, `Read`, `Write`, `Seek`, `Lock`, `UnLock`, `CurrentDir`,
  `LoadSeg`, `UnLoadSeg`, `NewLoadSeg`, `Input`, `Output`)
- `FileInfoBlock` translation: `Examine`, `ExNext`, `ExamineFH` keep a native FIB
  behind the program's (`ctx->fibs[16]`); `ExAll`/`ExAllEnd` with an
  `ExAllControl` shadow and per-type record sizes; `AllocDosObject(DOS_FIB)`
  returns a shadow
- Pattern matching: `MatchFirst`/`MatchNext`/`MatchEnd` with native `AnchorPath`s
  (`ctx->anchors[16]`)
- `DosList` entries: `NextDosEntry`/`FindDosEntry` hand out 44-byte m68k copies
  (one per native entry) including `FileSysStartupMsg` and `DosEnvec`; `Info`
- Notifications: `StartNotify`/`EndNotify` with a native `NotifyRequest` copy;
  the `NotifyMessage` arrives as a shadow through `GetMsg`
- Processes: `CreateNewProc`, `CreateProc`, `Cli`, `Exit`, `Delay`,
  `GetProgramName`, `SetProgramName`, `SetPrompt`, `SetCurrentDirName`,
  `CliInitNewcli`/`CliInitRun`, `Execute`, `SystemTagList`, `RunCommand`
- Formatting: `VPrintf`, `VFPrintf`, `VFWritef` through
  `m68k_to_native_fmt_stream()`; `FPuts`, `PutStr`; `StrToLong`
- Dates: `DateStamp`, `CompareDates`, `DateToStr`, `StrToDate`, `SetFileDate`
- `ReadArgs`/`FreeArgs`: the result array is built natively and copied into a
  containment block (`rdargs_blocks[M68KEMU_MAX_RDARGS]`); strings and arrays are
  copied by template kind
- Packets: `DoPkt`, `SendPkt`, `WaitPkt`, `ReplyPkt`, `GetDeviceProc`/`FreeDeviceProc`

**intuition.library** (`m68kemu_thunks.c`, `m68kemu_boopsi.c`)

- Screens and windows: `OpenScreen`/`OpenScreenTagList`/`CloseScreen`,
  `OpenWindow`/`OpenWindowTagList`/`CloseWindow`, `ModifyIDCMP`, `MoveWindow`,
  `SizeWindow`. A program screen opens in the Workbench screen's mode, size and
  depth (`open_screen()`), gets a 346-byte Screen shadow with registered ViewPort
  and RastPort, an 8-plane `sc_BitMap` with real planes in containment (writes
  are mirrored onto the native screen by `m68k_planar_touch()`), and is noted in
  `IntuitionBase->ActiveScreen/FirstScreen` and `GfxBase->ActiView`. A window
  gets a 148-byte shadow (`M68K_SIZEOF_WINDOW_SHADOW`) with RPort, WLayer,
  UserPort and WScreen; with `classic_borders` (default, `M68KEMU_CLASSICBORDERS=0`
  turns it off) a window sized from the outside grows by what the decoration
  takes beyond the AmigaOS 3 borders, and the program sees the size it asked for
- Gadgets: `AddGList`, `AddGadget`, `RemoveGList`, `RemoveGadget`,
  `RefreshGList`, `RefreshGadgets`, `ModifyProp`, `NewModifyProp`, `OnGadget`,
  `OffGadget`. The program's 44-byte m68k Gadgets are translated into native
  copies kept in `ctx->gadmap`, so `IntuiMessage.IAddress` names the program's own
  gadget; `GFLG_SELECTED`/`GFLG_DISABLED` and prop pots are copied back when a
  message arrives
- Menus: `SetMenuStrip`, `ResetMenuStrip`, `ClearMenuStrip`, `ItemAddress`,
  `OnMenu`, `OffMenu`. The m68k Menu/MenuItem strip is copied into native
  structures (`ctx->menumap`); `MENUPICK` syncs `Flags` and `NextSelect` back
- Images, borders, text: `DrawImage`, `DrawBorder`, `DrawImageState`,
  `PrintIText`, `AutoRequest`, `EasyRequestArgs`; `struct Image`, `Border` and
  `IntuiText` chains are translated per call; plain images handed to native
  classes are cached for the run (`ctx->imgcache[64]`)
- BOOPSI: `NewObjectA`, `DisposeObject`, `SetAttrsA`, `GetAttr`,
  `SetGadgetAttrsA`, `DoGadgetMethodA`, `MakeClass`, `AddClass`, `RemoveClass`,
  `FreeClass`, `NextObject`, `FindClass` (see below)
- Misc: `CurrentTime`, `QueryOverscan`, `GetScreenData`, `GetScreenDrawInfo`/
  `FreeScreenDrawInfo`

**BOOPSI** (`m68kemu_boopsi.c`)

A native object is handed to the program as an m68k stand-in: a 12-byte
`_Object` header whose class is an m68k `IClass` (52 bytes + the class ID), and a
64-byte body mirroring the Gadget fields. Every m68k `IClass` has its dispatcher
hook pointing at vector -30 of the "boopsi" pseudo-library, so amiga.lib
`DoMethod()` lands in `thunk_boopsi_dispatch`, which runs the method on the native
object: `do_method()` handles `OM_SET`, `OM_UPDATE`, `OM_GET`, `OM_DISPOSE`,
`OM_ADDTAIL`, `OM_REMOVE`, the window class methods `WM_OPEN`, `WM_CLOSE`,
`WM_HANDLEINPUT`, `WM_NEWPREFS`, `WM_ICONIFY`, `WM_RETHINK`, and the listbrowser
methods `LBM_ADDNODE`, `LBM_EDITNODE`, `LBM_REMNODE`, `LBM_SHOWCHILDREN`,
`LBM_HIDECHILDREN`; a method that is passed on to a superclass is handled in
`thunk_boopsi_dispatch` itself (`OM_NEW`, `OM_SET`, `OM_UPDATE`, `OM_NOTIFY`,
`OM_GET`, `OM_DISPOSE`). `MakeClass()` gives the program an m68k
`IClass` whose dispatcher it fills in; behind it is a native class whose
dispatcher (`m68k_class_hook`) runs that hook as 68k code with
`M68KEmu_Call68k()`, the method passed in 68k layout; a method the 68k code hands
to its superclass is mapped back to the native method it came from
(`ts->msgstack`). Class libraries (`layout.gadget`, `chooser.gadget`,
`clicktab.gadget`, `radiobutton.gadget`, `speedbar.gadget`, `listbrowser.gadget`
and any other `.class/.gadget/.image`) get m68k bases whose `GetClass`,
`Alloc/Free/Set/GetXxxNode`, `RethinkLayout`, `PAGE_GetClass` and
`FreeListBrowserList` are thunked; label nodes are m68k Nodes standing in for the
native ones, and a native copy of a list of them is built when it is passed to
a gadget.

**graphics.library** (`m68kemu_thunks.c`)

- RastPorts: `resolve_rp()` returns the native RastPort behind a shadow, or
  gives a RastPort in the program's own memory (`InitRastPort` on its own BitMap)
  a native twin (`SHADOW_OWNRP`, `ctx->own_rps[32]`) whose fields are copied in
  before each call and back afterwards (`rp_state_out()`: pens, draw mode, pen
  position, font, text metrics)
- BitMaps: `m68k_bitmap_in()` accepts a native BitMap (shadow or handle), a
  screen's `sc_BitMap`, or planes in the program's memory, for which a native
  planar BitMap is built; `BltBitMap`, `BltBitMapRastPort`,
  `BltMaskBitMapRastPort`, `ClipBlit`, `BltTemplate`, `AllocBitMap`, `InitBitMap`,
  `InitRastPort`, `GetBitMapAttr`
- Text and fonts: `Text`, `TextLength`, `TextExtent`, `TextFit`, `SetFont`,
  `OpenFont`, `AskFont` (and diskfont `OpenDiskFont`); `TextAttr` is read from
  the program's memory, `TextExtent` written back
- Area fill: `InitArea`, `InitTmpRas`, `AreaMove`, `AreaDraw`, `AreaEllipse`,
  `AreaEnd`, `Flood` with native `AreaInfo`/`TmpRas` twins; `PolyDraw`
- Regions: `AndRectRegion`, `OrRectRegion`, `XorRectRegion`, `ClearRectRegion`
- Colours: `LoadRGB4`, `LoadRGB32`, `GetRGB32`; display info: `GetVPModeID`,
  `FindDisplayInfo`, `GetDisplayInfoData`
- `Move`, `Draw`, `RectFill`, `ScrollRaster`, `SetAPen`, `SetBPen`, `SetDrMd`,
  `SetABPenDrMd`

**gadtools.library**: `CreateGadgetA`, `FreeGadgets`, `CreateContext`,
`GT_SetGadgetAttrsA`, `GT_GetGadgetAttrsA` build native gadgets and give the
program a 44-byte m68k mirror of each (in `gadmap`, flagged `gt`);
`CreateMenusA`, `FreeMenus`, `LayoutMenusA`, `LayoutMenuItemsA` build the strip
natively and mirror it item by item with the `UserData` slots
(`ctx->gtmenus[16]`); `GT_GetIMsg`, `GT_ReplyIMsg`, `GT_FilterIMsg`,
`GT_PostFilterIMsg`; `GetVisualInfoA`/`FreeVisualInfo` return handles.

**utility.library**: the tag item functions (`FindTagItem` .. `FilterTagItems`,
12 functions in `m68kemu_tags.c`) work directly on tag lists in the program's
memory; `CallHookPkt` calls a 68k hook; `SDivMod32`, `UDivMod32`, `SMult64`,
`UMult64` return their second result in D1; `Amiga2Date`, `Date2Amiga`,
`CheckDate` translate `ClockData`.

**locale.library**: `FormatDate` uses the character-hook mechanism (the native
call collects the characters; a 68k stub run through `tail_pc` calls the
program's hook once per character, NUL included); `GetCatalogStr`/`GetLocaleStr`
copy the native string into containment (`ctx->strcache[64]`).

**icon.library** (`m68kemu_icon.c`): a native `DiskObject` is given to the
program in 68k layout (78 bytes plus gadget images, tool types, drawer data) and
its changes go back to the native icon on `PutDiskObject`/`PutDefDiskObject`/
`PutIconTagList`; 15 functions including `GetDiskObject(New)`, `DupDiskObjectA`,
`IconControlA`, `DrawIconStateA`, `GetIconRectangleA`, `LayoutIconA`,
`FindToolType`.

**commodities.library** (`m68kemu_math.c`): `CreateCxObj` and `CxBroker`
translate `NewBroker` and the per-type arguments (`CX_FILTER`, `CX_TYPEFILTER`,
`CX_SEND` through a port twin, `CX_SIGNAL` to the program's task, `CX_TRANSLATE`
as a NULL translation, `CX_DEBUG`); the rest is generated.

**workbench.library** (`m68kemu_math.c`): `AddAppWindowA`, `AddAppIconA`,
`AddAppMenuItemA`, `AddAppWindowDropZoneA` return a token, the `Remove*`
functions TRUE, `OpenWorkbenchObjectA` is real, the others return FALSE.

**rexxsyslib.library** (`m68kemu_rexx.c`): `CreateArgstring`, `DeleteArgstring`,
`LengthArgstring`, `CreateRexxMsg`, `DeleteRexxMsg`, `ClearRexxMsg`,
`FillRexxMsg`, `IsRexxMsg` build argstrings and RexxMsgs in the program's memory
in 68k layout; `LockRexxBase`/`UnlockRexxBase` are no-ops.

**Math libraries** (`m68kemu_math.c`): mathffp/mathtrans (Motorola FFP),
mathieeesingbas/trans (IEEE single in D0) and mathieeedoubbas/trans (IEEE double
in D0/D1) are computed natively; the Z and N condition codes the caller expects
are set through `ts->ccr_val`.

### Auto-Generated Thunk Type Translations

`m68kemu_thunks_gen.c` holds 789 thunks for 17 libraries
(`m68kemu_all_gen_libs_count`): asl 8, commodities 29, cybergraphics 24,
datatypes 30, diskfont 3, dos 112, exec 79, expansion 22, gadtools 17, graphics
167, icon 21, iffparse 40, intuition 127, keymap 4, layers 40, locale 25,
utility 41. The generator (`m68kemu_thunkgen.py`) classifies each parameter from
the NDK prototype:

| Classification | Translation | Used for |
|---------------|-------------|----------|
| shadow | `m68k_to_host_or_shadow()`; `resolve_rp()` for `RastPort` | the 21 types in `SHADOW_LOOKUP_TYPES` (Window, Screen, MsgPort, IORequest, Gadget, Menu, TextFont, BitMap, Layer, Region, DiskObject, IFFHandle, ...) |
| string | `m68k_to_host()` | `STRPTR`, `CONST_STRPTR`, `char *`, `UBYTE *` |
| taglist | `m68k_to_native_taglist()` + `FreeVec` | `struct TagItem *` |
| bptr | `m68k_bptr_in()` | `BPTR`, `BSTR` |
| rawarg | `m68k_to_native_fmt_stream()` | a `RAWARG` datastream following a format string |
| input_struct | no thunk generated | `NewWindow`, `NewScreen`, `EasyStruct`, `ExtNewScreen` |
| scalar | `THUNK_D(n)` as ULONG | `ULONG`, `LONG`, `WORD`, `BOOL`, `Tag`, `IPTR` ... |
| pointer | `THUNK_PTR(n)` / `THUNK_DPTR(n)` | remaining pointer types; `DPTR_OVERRIDES` names D-register pointers when no prototype is known |

Return values: the 425 names in `VOID_FUNCTIONS` return 0; the 21 functions in
`SHADOW_RETURNS` (`CreateMsgPort`, `CreateIORequest`, `OpenFont`, `AllocBitMap`,
`LockPubScreen`, `CreateGadgetA`, `CreateMenusA`, ...) wrap the result with
`shadow_create_by_name()`; a `BPTR` result goes through `m68k_bptr_out()`;
anything else is cast to IPTR and translated by the dispatcher. `SKIP_LIBRARIES`
(78 names) and `SKIP_FUNCTIONS` (153 names, the manually implemented and the
private ones) are left out.

## Shadow Struct Engine

### API

| Function | Purpose |
|----------|---------|
| `shadow_create(ctx, layout, native)` | One shadow per native structure: returns the existing one for the same `(native, shadow_type)`, otherwise allocates `layout->m68k_size` bytes and registers it; then syncs native→m68k. A library base keeps its 14-byte node. Runs the extras for `Screen`, `DrawInfo` and `TextFont`. |
| `shadow_sync(ctx, layout, m68k_addr, native)` | Re-sync all fields native→m68k. |
| `shadow_destroy(ctx, layout, m68k_addr)` | Remove from the registry, free the m68k block. |
| `shadow_find_layout(name)` | Linear scan of `shadow_gen_layouts[]`. |
| `shadow_create_by_name(ctx, name, native)` | `shadow_find_layout` + `shadow_create`. |
| `shadow_init_execbase(ctx, m68k_base, native)` | ExecBase init: syncs the layout, sets `AttnFlags`, `VBlankFrequency`/`PowerSupplyFrequency` = 50, `ThisTask` → a 228-byte Process stand-in with a 64-byte CLI (`cli_DefaultStack` 4096, `cli_CommandName`), `pr_CIS`/`pr_COS`, `ex_EClockFrequency`, `lib_IdString` → ROM copy, the `MemList` with the two region MemHeaders, empty `LibList`/`DeviceList`/`ResourceList` (then publishes the bases made so far), and registers 5 embedded native lists (`IntrList`, `PortList`, `TaskReady`, `TaskWait`, `SemaphoreList`) as type 17. |
| `shadow_register(ctx, m68k_addr, native, type)` | Append to `shadow_map[]`; when full, logs once which two types fill it and returns 0. |
| `shadow_lookup(ctx, m68k_addr)` | Linear scan by m68k address, returns the native pointer or NULL. |
| `shadow_remove(ctx, m68k_addr)` | Linear scan + swap-with-last removal. |
| `m68k_to_native_taglist(ctx, m68k_addr)` | Convert an m68k TagItem list (8 bytes per entry, `TAG_MORE`/`TAG_SKIP`/`TAG_IGNORE` followed, at most 256 items) to a native one. Known flag and display-mode tags stay numbers; a value in the handle range maps to the native object; a value in `[M68KEMU_HEAP_BASE, mem_size)` is resolved with `m68k_to_host_or_shadow`. |
| `m68k_to_native_struct(ctx, name, m68k_addr)` | Reverse sync m68k→native into an `AllocMem` block of `3 × m68k_size` bytes. |
| `m68k_task_stack(ctx, mtask, task)` | Writes `tc_SPReg/tc_SPLower/tc_SPUpper` of the program's own task from the emulated stack. |
| `m68k_screen_extras(ctx, ms, scr)` | Screen fields the generated layout leaves out, `sc_Font` as a 68k `TextAttr`, ViewPort/RastPort/ColorMap registration, an AGA-like palette for the free pens of a deep screen. |
| `m68k_sync_windowptr` / `m68k_restore_windowptr` | `pr_WindowPtr` of the program → native process before each call, restored at the end. |

### Field Types

```c
enum { SF_END=0, SF_BYTE, SF_WORD, SF_LONG, SF_PTR };
```

| Type | Sync behaviour |
|------|----------------|
| SF_BYTE | direct byte copy |
| SF_WORD | 16-bit big-endian via `m68k_write16/read16` |
| SF_LONG | 32-bit big-endian via `m68k_write32/read32` |
| SF_PTR | NULL → 0; an existing shadow of the same `native_ptr` + `shadow_type` → its address; a `sub_layout` → recursive create; otherwise 0 (never a dangling native pointer) |
| SF_END | terminator |

### Type Detection Heuristic (in the generator)

| m68k size | AROS size | Classification |
|-----------|-----------|---------------|
| 1 | any | SF_BYTE |
| 2 | any | SF_WORD |
| 4 | 8 | SF_PTR (pointer widening 32→64 bit) |
| 4 | 4 | SF_LONG |
| other | other | skipped |

### Registry

A flat array in `M68KEmuContext`:

```c
struct { ULONG m68k_addr; void *native_ptr; UWORD type; } shadow_map[M68KEMU_MAX_SHADOWS];  /* 1024 */
UWORD num_shadows;
```

- **Register**: O(1) append; a full table is reported once through `m68k_log()`
  ("more than 1024 live library objects (messages not replied?)") and further
  registrations fail
- **Lookup**: O(n) scan by m68k address (`m68k_to_host_or_shadow` does the same
  scan inline)
- **Remove**: O(n) scan + swap-with-last
- **Dedup**: `shadow_create` and `sync_fields` look for an existing entry with the
  same native pointer and type

Hand-assigned type tags: 1 Window, 3 RastPort, 4 MsgPort, 5 Screen, 6 ViewPort,
9 DrawInfo pen table, 12 DosList copy, 13 font glyph block, 14 program-owned
RastPort twin, 17 ExecBase list, 30 Layer, 31 BOOPSI object, 32 BOOPSI class,
33 BOOPSI label node, 34 DiskObject, 35 ExAllControl, 36 DevProc, 40 AreaInfo,
41 TmpRas, 42 TextAttr. The generated layouts are numbered 20–316 in table order
(`gen_shadow_layouts.py` reserves 0–19), so the hand-assigned tags from 30 up
share their numbers with generated layouts (30 is also `AppWindowDropZoneMsg`,
31 `AreaInfo`, 32 `AssignList`, 33 `BadBlockBlock`, 34 `BadBlockEntry`, 40
`BootNode`, 41 `Border`, 42 `CDInfo`). Lookups by address are unaffected; the
dedup by `(native_ptr, type)` would only be confused if the same native object
were registered under both.

### Recursive Sub-Shadows

`M68KFieldMap.sub_layout` allows `sync_fields` to create and sync a pointee
recursively, but every generated layout sets `sub_layout = NULL`, so an SF_PTR
field is either an already registered shadow or 0. The embedded structures a
program needs are wired by hand: `setup_window_shadow()` (RPort, WLayer,
UserPort, WScreen), `open_screen()` and `m68k_screen_extras()` (ViewPort,
RastPort, ColorMap, BitMap), `m68k_drawinfo_extras()` (pens, font),
`m68k_textfont_extras()` (glyph data for programs that render text themselves),
`m68k_process_of()` (`pr_MsgPort`, CLI).

## Shadow Struct Layouts

`m68kemu_shadow_layouts.h` is generated and holds 297 layouts
(`SHADOW_GEN_LAYOUT_COUNT`) with 3,171 field mappings: 1,296 `SF_PTR`, 574
`SF_LONG`, 754 `SF_WORD`, 547 `SF_BYTE`. The native offsets are those of the
x86_64 build (see `mmakefile.src`). Format of the header comments: Name
(m68k size → AROS size, field count).

### Most Thoroughly Mapped

| Struct | m68k | AROS | Fields | Domain |
|--------|------|------|--------|--------|
| GfxBase | 552B | 1040B | 102 | graphics |
| ExecBase | 632B | 1320B | 65 | exec |
| Screen | 346B | 1408B | 62 | intuition |
| Process | 228B | 768B | 56 | exec |
| Preferences | 232B | 232B | 46 | intuition |
| Locale | 168B | 312B | 42 | locale |
| ConUnit | 296B | 640B | 42 | devices |
| MonitorSpec | 160B | 896B | 41 | graphics |
| PrtInfo | 128B | 208B | 34 | devices |
| PrinterData | 6842B | 7808B | 31 | devices |
| ColorTextFont | 96B | 184B | 29 | graphics |
| Task | 92B | 176B | 28 | exec |

### Complete List

The full list is the `shadow_gen_layouts[]` table at the end of
`m68kemu_shadow_layouts.h`.

### Missing Structs

`m68k_offsets.txt` describes 298 structs (3,273 field lines); 1 of them lacks a
layout: **RegionRectangle**, whose only field is `bounds` (an embedded
`struct Rectangle`) with no scalar leaf fields to map.

## 68k Libraries and Relocations

### Loading a Program

`RunHunk()` receives a seglist that `LoadSeg()` relocated to its host addresses.
`M68KEmu_LoadHunks()` copies each segment into containment, laid out as
`LoadSeg()` does (`ULONG size`, `BPTR next`, data), links the segments, and
re-applies the relocations that dos kept with the seglist:
`GetSegListInfo(GSLI_68KRELOCS)` returns a `struct HunkRelocTable` of
`struct HunkRelocGroup { hrg_Hunk, hrg_Target, hrg_Kind, hrg_Count, hrg_Offsets }`
(`compiler/include/dos/doshunks.h`), one group per relocation block and target
hunk of the file. For each offset the original value is recovered from the
host-relocated longword and re-based on the containment address of the target
hunk; `HUNKRELOC_ABS`, `HUNKRELOC_REL` (relative to the longword) and
`HUNKRELOC_BASE` (relative to the holding hunk) are handled. A table whose
`hrt_Groups` is `0xFFFFFFFF` means dos could not keep the list; loading fails with
a message, as it does for a group or offset that points outside the segments.

`RunFile()` and the 68k library loader use `M68KEmu_LoadHunksFromMemory()`
instead, a hunk file parser: `HUNK_HEADER` with resident names skipped, at most
100 hunks, memory requirement bits 30/31 (Chip, Fast, or an explicit requirement
longword) honoured with a fallback to any memory; `HUNK_CODE`, `HUNK_DATA`,
`HUNK_BSS`, `HUNK_RELOC32`, `HUNK_RELOC32SHORT`, `HUNK_DREL32`, `HUNK_RELRELOC32`
(16-bit forms padded to a longword), `HUNK_SYMBOL`, `HUNK_DEBUG` and `HUNK_NAME`
(skipped), `HUNK_END`. Any other block type fails the load with "hunk file: block
type N is not supported". `ctx->load_lo/load_hi` record the span of the file.

`M68KEmu_AutoOpenLibs()` then scans the heap for the C startup pattern (N zero
longwords followed by N consecutive `".library"` names, up to 16) and fills the
slots with fake bases.

### dos.library Integration

`rom/dos/runcommand.c` and `rom/dos/createnewproc.c` ask
`GetSegListInfo(GSLI_68KHUNK)` whether a seglist is a hunk file and, if so, open
`m68kemu.library` and call `RunHunk()` instead of jumping into the segment.

### 68k Libraries from Disk

`M68KEmu_LoadM68KLibrary()` (`m68kemu_m68klib.c`) is the last resort of
`OpenLibrary()`. It looks for a hunk file (magic `0x3F3` checked, so native
libraries in `LIBS:` are never mistaken) in `LIBS68K:`, `SYS:Libs68k/`,
`LIBS:m68k/` and `LIBS:`, or at the given path; a probe for an optional library
sets `pr_WindowPtr = -1` so no requester appears. Then:

1. Loads the hunks and scans every hunk for `RTC_MATCHWORD` (0x4AFC) whose
   `rt_MatchTag` points back at itself
2. Requires `RTF_AUTOINIT`; a Resident with an init function is reported as
   "not M68K_RTF_AUTOINIT, not supported yet"
3. Reads the init table (`dataSize`, `funcTable`, `dataInit`, `initFunc`); the
   function table may hold absolute longwords or, starting with the word -1,
   word offsets from the table (up to 1000 entries)
4. Allocates `jt_size + max(dataSize, 34)` bytes from the heap and builds a
   `JMP abs.l` jump table (`M68K_JT_SLOT_SIZE` 6 bytes per vector): m68k-to-m68k
   calls run without traps
5. Fills the `Library` node (`ln_Type`, `ln_Name`, flags, `lib_NegSize`,
   `lib_PosSize`, `lib_Version`, `lib_IdString`) and runs the `InitStruct()` table
   (`m68k_init_struct`, exec's command byte format)
6. Arranges, through `ts->tail_pc`, for the library's init function and then
   its Open vector (-6) to run as 68k code when `OpenLibrary` returns to the
   caller, with the caller's A6 kept and the Open result as the OpenLibrary
   result. A later `OpenLibrary` of the same name runs only the Open vector
   (`m68klibs[].reopen`) after checking `lib_Version` against the requested one.

Up to 16 such libraries are remembered per program (`ctx->m68klibs`).

## Device I/O Path

Device I/O lives in `m68kemu_devices.c`. The program's `IORequest` stays in its
memory in 68k layout (its own allocation, or the 32-byte shadow a generated
`CreateIORequest` returns); a native twin is kept in `ctx->ioreqs[64]`
(`struct M68KIoMap { m68k, native, kind, pending, ownport }`). Fields are copied
in before each call (`io_in()`: `io_Command`, `io_Flags`, and per kind
`tr_time`, the `IOAudio` fields, or `io_Length/io_Data/io_Offset`) and out after
it (`m68k_ioreq_out()`: `io_Error`, `io_Flags`, `io_Actual` or the time, the
audio key and length).

Only devices in `known_devices[]` open: `timer.device`, `audio.device`,
`console.device`, `input.device`, `keyboard.device`, `gameport.device`,
`trackdisk.device`, `clipboard.device`. Anything else fails with
`IOERR_OPENFAIL` and the log message "`<device>` is not supported for 68k
programs yet".

```plain
m68k: CreateMsgPort()                     generated thunk → MsgPort shadow
m68k: CreateIORequest(port, size)         generated thunk → IORequest shadow (in containment)
m68k: OpenDevice(name, unit, io, flags)   reply port → native port (shadow or twin);
                                          native request of the right size (timerequest,
                                          IOAudio or IOStdReq); native OpenDevice;
                                          io_Device := an m68k device base (16 vectors) whose
                                          BeginIO/AbortIO (and timer.device's AddTime, SubTime,
                                          CmpTime, ReadEClock, GetSysTime; console.device's
                                          RawKeyConvert) are thunks; io_Unit as a handle
m68k: DoIO / WaitIO                       io_in; lock released around the native call; io_out
m68k: SendIO / BeginIO                    io_in; native call; io_out at once for IOF_QUICK
m68k: CheckIO                             io_out when done, returns the m68k request
m68k: AbortIO                             only for a request in flight
m68k: GetMsg(replyport)                   a replied request is recognised (m68k_ioreq_of) and io_out run
m68k: CloseDevice(io)                     native CloseDevice, twin and own port freed
```

`input.device` requests are special-cased: `IND_ADDHANDLER`/`IND_REMHANDLER`
succeed but install nothing (a 68k handler cannot be called by the native
device), `IND_WRITEEVENT` translates up to 16 chained `InputEvent`s.

### IORequest Shadow Layouts

| Struct | m68k | AROS | Fields |
|--------|------|------|--------|
| IORequest | 32B | 72B | 12 |
| IOStdReq | 48B | 96B | 16 |
| timerequest | 40B | 80B | 14 |
| IOExtSer | 82B | 136B | 28 |
| IOExtPar | 62B | 112B | 21 |
| IOExtTD | 56B | 104B | 18 |
| IOAudio | 68B | 152B | 18 |
| IOClipReq | 52B | 96B | 17 |

### IDCMP Message Flow

1. `OpenWindow` creates the Window shadow; `window_userport_sync()` gives the
   native `UserPort` a 34-byte m68k MsgPort (`mp_Flags`, `mp_SigBit` copied, so
   programs can `Wait()` on `1 << mp_SigBit` themselves) at `M68K_WIN_USERPORT`
   (86); `ModifyIDCMP` makes it come and go
2. `WaitPort`/`GetMsg` resolve the port shadow; `GetMsg` creates an IntuiMessage
   shadow (`imsg_to_m68k()`), fills `MouseX/MouseY`, `Seconds/Micros`,
   `IDCMPWindow` (reverse lookup of the Window shadow) and `IAddress` (the
   program's own gadget through `gadmap`), updates the window's mouse position,
   and syncs gadget and menu state back
3. `ReplyMsg` replies natively and destroys the shadow; `GT_GetIMsg`/
   `GT_ReplyIMsg` follow the same pattern
4. A program that sets `Window->UserPort` itself (a shared port) has the native
   window follow (`window_userport_in()`)

## Custom Chip and CIA Emulation

Accesses to `$DFF000–$DFFFFF` are intercepted in `read8/read16/write8/write16`.
A byte read returns the high or low half of the word register; byte writes to
the range are ignored.

### Read Registers

| Offset | Name | Value |
|--------|------|-------|
| 0x002 | DMACONR | 0 |
| 0x004 | VPOSR | `(beam_lof << 15) \| ((beam_v >> 8) & 1)` (PAL Agnus ID 0) |
| 0x006 | VHPOSR | `((beam_v & 0xFF) << 8) \| beam_h` |
| 0x010 | ADKCONR | 0 |
| 0x016 | POTGOR | `POTGOR_DEFAULT` 0xFF00 (no buttons pressed) |
| 0x01C | INTREQR | 0 |
| 0x01E | INTENA | 0 |
| other | — | 0 |

Every read calls `beamAdvance(2)`: 2 colour clocks per register access.

### Write Registers

| Offset | Name | Effect |
|--------|------|--------|
| 0x02A | VPOSW | sets `beam_lof` from bit 15, merges V8 into `beam_v` |
| 0x02C | VHPOSW | sets `beam_v[7:0]` and `beam_h` |
| other | — | ignored |

### Beam Counter

PAL timing: `PAL_LINES_PER_FRAME` 313 × `PAL_CLOCKS_PER_LINE` 227.
`beamAdvance(ticks)` adds to `beam_h`, wraps at 227 incrementing `beam_v`, wraps
`beam_v` at 313 toggling `beam_lof`. The beam only advances on register reads,
not per instruction.

### CIA-A and CIA-B

Byte accesses to the odd addresses `$BFEx01` (CIA-A) and the even addresses
`$BFDx00` (CIA-B) are decoded by `ciaDecode()` (register = bits 8–11). Reads are
derived from the E clock of `timer.device` (`m68k_eclock_now()`, 709379 Hz when
the timer reports no frequency):

- PRA reads 0xFF (no buttons, no disk change) unless written; PRB, DDRA, DDRB
  read back what was written
- Timers A and B count down from their latch (0xFFFF when unset) at the E rate
- TOD counts at 50 Hz on CIA-A and 15625 Hz (horizontal rate) on CIA-B, plus
  `tod_base`
- SDR and ICR read 0; CRA/CRB read back what was written

Writes only keep the port, latch and control registers; TOD, SDR and ICR mask
writes are ignored. No CIA interrupt is ever raised.

## Thunk Generator Toolchain

### m68kemu_thunkgen.py

- **Input**: Amiga FD files (`*_lib.fd`) and, optionally, NDK `clib/*_protos.h`
- **Output**: `m68kemu_thunks_gen.c` — one `static IPTR thunk_<lib>_<Func>()`
  per function, a `{ lvo, thunk }` table per library terminated by `{ 0, NULL }`,
  a `_count` constant, and the `m68kemu_all_gen_libs[]` registration table
- **Classification**: see [Auto-Generated Thunk Type
  Translations](#auto-generated-thunk-type-translations); without a prototype the
  generator falls back to register class (A registers are pointers, D registers
  scalars unless listed in `DPTR_OVERRIDES`)
- **Skip lists**: `SKIP_LIBRARIES` (78), `SKIP_FUNCTIONS` (153), `VOID_FUNCTIONS`
  (425)

### gen_offset_extractor.py and gen_shadow_layouts.py

`gen_offset_extractor.py` scans the NDK headers (`exec`, `dos`, `intuition`,
`graphics`, `utility`, `workbench`, `devices`, `libraries`, `resources`,
`datatypes`; some headers, structs and fields are skipped because their layouts
differ or cannot be probed) and writes a C file that emits `OFF|Struct|field|
offset|size|` and `SIZ|Struct|size|` lines through `offsetof()`/`sizeof()`,
recursing into embedded structs so every scalar leaf field is listed. Compiled
once with the m68k compiler (`m68k_offsets.txt`, checked in) and once with the
target compiler, the two outputs feed `gen_shadow_layouts.py`, which emits
`m68kemu_shadow_layouts.h`. It treats a field as a pointer only when the native
field is 8 bytes wide, so the checked-in header is valid for 64-bit targets only
(`mmakefile.src` builds the library for x86_64, aarch64 and riscv64).

## Diagnostics

- `SetEnv M68KEMU_TRACE 1`: per-call trace lines (`CALL`/`GEN` with registers and
  the last PCs), exception dumps with the 512-entry PC history, and a watchdog
  process ("m68kemu watchdog") that prints the instruction count and PC every
  2 seconds. `M68KEMU_TRACE msg` starts the trace at the first IntuiMessage;
  `M68KEMU_TRACE end` records the last 2048 library calls (`ctx->callring`) and
  prints them when the program ends, after 10 seconds in one call, or at 60
  seconds.
- `M68KEMU_CLASSICBORDERS 0` turns off the window growth for the decoration.
- `m68k_log()` sends messages about a program (unsupported function, library
  missing, unhandled exception, shadow table full) to `log.resource`, so
  LogDump and the log viewers show them; with tracing on they also go to the
  debug output.

## Test Infrastructure

### Host-Side Tests (no AROS required)

`developer/debug/test/m68kemu_host/Makefile` builds with `-std=c++20` against the
stub headers in `stubs/` (`aros/`, `dos/`, `exec/`, `proto/`) and includes the
production sources directly.

| Test | File | What it tests |
|------|------|---------------|
| t_ctx | test_m68kemu.cpp | context create/destroy, `mem_size == M68KEMU_MEM_SIZE` |
| t_heap | test_m68kemu.cpp | heap alloc/free/reuse |
| t_stress | test_m68kemu.cpp | 1000 × 64-byte allocations, free all, then a 100 KB allocation |
| t_endian | test_m68kemu.cpp | big-endian `read16/write16/read32/write32` |
| t_xlat | test_m68kemu.cpp | pointer translation round trip |
| t_libbase | test_m68kemu.cpp | fake library base setup, A-line opcodes in the slots |
| t_exec_moveq_rts | test_moira_exec.cpp | `MOVEQ #42,D0; RTS` → 42 |
| t_exec_add | test_moira_exec.cpp | `MOVEQ #10,D0; ADDI.W #32,D0; RTS` → 42 |
| t_exec_loop | test_moira_exec.cpp | loop summing 1..10 → 55 |
| t_exec_memory_access | test_moira_exec.cpp | `MOVE.L` store/load round trip |
| (integration) | test_hello_thunks.cpp | A-line trap → `OpenLibrary` and `PutStr` thunks → return |
| memory bounds | regressions/run_memory_bounds.py, test_memory_bounds.cpp | compiles `m68k_read16/32`, `m68k_write16/32` and `AROSMoira::read16/write16/read16OnReset/read16Dasm` extracted verbatim from the sources against a fixture; arenas of 0, 1, 2, 3, 4 and 17 bytes, the last complete word/longword, partial accesses, wrapping addresses near `UINT32_MAX`; rejected writes leave memory unchanged; custom-register dispatch, both sentinels and the low-vector guard; `--sanitize` adds ASan/UBSan |

`make test` runs all of them; `make test-memory-bounds` and
`make test-memory-bounds-sanitize` run the regression alone (see
`regressions/README.md`). `make_m68k_test.py` writes minimal hunk binaries.

### AROS-Side Tests

`developer/debug/test/m68kemu/mmakefile.src` builds two programs:

- `test_m68kemu` (`test-m68kemu`): opens `m68kemu.library`, writes an embedded
  hunk binary (`MOVEQ #42,D0; RTS`) to `RAM:`, calls `RunFile` (LVO 6) and
  expects 42
- `RunM68K` (`test-m68kemu-runm68k`, installed in `C:`): runs any m68k binary
  through `RunFile` with the remaining arguments

### Test Gaps

- No tests for the custom chip registers, the CIA model or beam advancement
- No tests for `Supervisor()`, the MOVEC skip or the 68040 instruction emulation
- No tests for the generated thunk fallback or for unimplemented LVOs
- No tests for the hunk loaders, the seglist relocation path or the 68k library loader
- No tests for the heap regions, `AllocAbs`, pools or heap exhaustion
- No tests for tasks, the program lock, 68k semaphores or ports
- No tests for the shadow engine, the taglist translation or device I/O

## Known Bugs and Limitations

Messages the code prints mark the first group.

- An LVO without a thunk returns 0: "`<lib>` function -N is not supported yet"
  (`m68kemu_moira.cpp`).
- A 68k library whose Resident is not `RTF_AUTOINIT` is not loaded
  (`m68kemu_m68klib.c`).
- Hunk block types other than those listed under [Loading a
  Program](#loading-a-program) (overlays among them) fail the load
  (`m68kemu_memory.c`).
- Devices outside `known_devices[]` do not open (`m68kemu_devices.c`); a 68k
  input handler (`IND_ADDHANDLER`) is accepted but never called.
- BOOPSI methods other than those handled in `do_method()` return 0, logged once;
  `NewObjectA` with a class pointer that is not a known class fails
  (`m68kemu_boopsi.c`).
- Commodities object types that need a 68k callback (`CX_CUSTOM`) are not
  created; `CX_TRANSLATE` swallows its events (`m68kemu_math.c`).
- `WA_SuperBitMap` and `WA_BackFill` are dropped from `OpenWindowTagList`
  (`m68kemu_thunks.c`).
- `ReadArgs` with a caller-supplied `RDArgs` returns 0 (`m68kemu_thunks.c`).

Further limitations visible in the code:

- `RawDoFmt` writes the formatted text (at most 1024 bytes) into the `PutChData`
  buffer and does not call the `PutChProc`.
- `AVL_AddNode`, `AVL_RemNodeByAddress`, `AVL_FindNode` return 0; `AddPort`,
  `RemPort` and `CloseLibrary` do nothing; `OpenResource` finds only bases that
  already have thunks.
- Only the `INTB_VERTB` interrupt server runs; `AddIntServer` ignores other
  interrupts, and no CIA or custom chip interrupt is raised.
- `workbench.library` App* registrations succeed with a token, but no
  AppMessage is ever delivered.
- `FindSemaphore` on a system semaphore returns a handle whose fields the
  program cannot read.
- The 68k stack is always 64 KB; `RunHunk`'s `stackSize` only decides whether the
  program runs in a child process.
- A program screen opens in the Workbench screen's mode, size and depth; its
  `sc_BitMap` has at most 8 planes.
- `m68k_to_native_struct()` sizes the native copy as 3 × the m68k size rather
  than the real native size.
- The hand-assigned shadow type tags 30–42 overlap the generated numbering
  (see [Registry](#registry)).
- The shadow registry holds 1024 entries; a program that never replies its
  messages fills it, and further native objects are refused with a log message.
- The checked-in shadow layouts carry x86_64 native offsets; 32-bit targets
  need regenerated layouts and a generator that recognises 4-byte pointers.
- No FPU: the CPU is an LC040, and a program without its own FPU emulator
  faults on F-line instructions.
