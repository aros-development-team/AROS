# m68kemu Internals

Deep technical reference for the m68kemu.library implementation. For an overview,
see [README.md](README.md).

## Table of Contents

- [Memory Model](#memory-model)
- [CPU Integration](#cpu-integration)
- [Thunk System Details](#thunk-system-details)
- [Shadow Struct Engine](#shadow-struct-engine)
- [Shadow Struct Layouts](#shadow-struct-layouts)
- [Device I/O Path](#device-io-path)
- [Custom Chip Emulation](#custom-chip-emulation)
- [Thunk Generator Toolchain](#thunk-generator-toolchain)
- [Test Infrastructure](#test-infrastructure)
- [Known Bugs](#known-bugs)

## Memory Model

### Containment Buffer

`M68KEmu_CreateContext()` allocates a single 32 MB buffer with
`AllocMem(MEMF_CLEAR | MEMF_31BIT)`. All m68k addresses are byte offsets into
this buffer. Translation is trivial:

```c
host_ptr = ctx->mem + m68k_addr;    // m68k → host
m68k_addr = host_ptr - ctx->mem;    // host → m68k
```

The `m68k_to_host_or_shadow()` variant checks the shadow registry first (for
native AROS structs that appear at m68k addresses), then falls back to
containment. Addresses ≥ 32 MB return 0 on read and are silently dropped on
write.

### Address Map

```plain
0x000000 - 0x0003FF      Exception vectors (1 KB, 256 × 4 bytes)
0x000100                 RTE instruction (planted at init)
0x000108                 Supervisor trampoline: ADDQ.L #8,SP; JMP (A5)
0x000400 - 0x0103FF      Library base region (64 KB)
                         Each library: num_vectors × 6 bytes (A-line + 2×NOP)
                         Base address follows jump table (AmigaOS convention)
0x010400 - ~0x01FEFFFF   Heap (first-fit, 4-byte aligned, 8-byte headers)
~0x01FF0000 - 0x01FFFFFF Stack (64 KB, grows downward)
```

### Heap Allocator

- **Strategy**: First-fit with splitting and forward/backward coalescing
- **Alignment**: 4-byte (`(size + 3) & ~3`)
- **Header**: 8 bytes in containment memory (big-endian ULONG size + ULONG next)
- **Splitting**: If remainder ≥ 16 bytes after allocation; allocates from the END of the free block
- **Free**: Address-ordered insertion into free list with coalescing
- **No MEMF_CHIP/MEMF_FAST**: The `requirements` parameter is ignored

### Limits

| Resource | Limit | Constant |
|----------|-------|----------|
| Containment memory | 32 MB | `M68KEMU_MEM_SIZE` |
| Stack | 64 KB | `M68KEMU_STACK_SIZE` |
| Libraries | 32 | `M68KEMU_MAX_LIBS` |
| Library name map | 16 | `M68KEMU_MAX_LIBMAP` |
| LVOs per library | 512 | `M68KEMU_MAX_LVO` |
| Shadow entries | 64 | `M68KEMU_MAX_SHADOWS` |
| Hunks per binary | 100 | hardcoded in `LoadHunksFromMemory` |

## CPU Integration

### Moira Subclass

`AROSMoira` (in `m68kemu_moira.cpp`) extends `moira::Moira` with:

- **Memory callbacks**: `read8/read16/write8/write16` — intercept custom chip range (`$DFF000`), sentinel address, and out-of-bounds access
- **Exception dispatch**: `willExecute(LINEA)` — the core thunk dispatch mechanism
- **Halt handler**: `cpuDidHalt()` — sets `ctx->running = FALSE`

CPU model is set to `Model::M68040` (includes FPU).

### Run Loop

```plain
M68KEmu_Execute(ctx):
  1. Construct AROSMoira on stack, set M68040 model
  2. Write initial SP → vector 0, entry_point → vector 4
  3. cpu.reset()
  4. Restore AbsExecBase at address 4
  5. Plant RTE (0x4E73) at 0x000100
  6. Plant supervisor trampoline at 0x000108: ADDQ.L #8,SP (0x508F) + JMP (A5) (0x4ED5)
  7. Set LINEA vector (address 0x28) → 0x000100
  8. Push SENTINEL_ADDR (0x00DEAD00) as return address on stack
  9. Set registers: A0=argptr, D0=argsize, A6=m68k_sysbase
  10. while (ctx->running) cpu.execute()
  11. Return D0
```

### Termination

- **Sentinel**: When m68k code does RTS and fetches the instruction at
  `0x00DEAD00`, `read16` returns STOP opcode (`0x4E72`) and sets
  `ctx->running = FALSE`
- **HALT**: `cpuDidHalt()` sets `ctx->running = FALSE`

### A-Line Dispatch (willExecute)

1. Restore LINEA vector to `RTE_ADDR` (0x000100)
2. Get PC0 (faulting instruction address)
3. Scan `ctx->libs[]` to find which library's jump table contains PC
4. Compute LVO offset = `lib->m68k_addr - pc`
5. Search manual thunks first (`lib->thunks[]`), then generated (`lib->gen_thunks[]`)
6. If found: pop JSR return address from m68k stack, call thunk, set D0, redirect PC
7. If `ctx->sv_redirect` is set: dispatch Supervisor() trampoline instead
8. Unimplemented LVOs: pop return, set D0=0, log warning

### Supervisor() Support

`thunk_exec_Supervisor` stores the user function address in `ctx->sv_redirect`. After the thunk returns, `willExecute` detects this and calls `dispatchSupervisor()`:

1. Pushes a 68020-format exception frame (8 bytes: vector offset + PC + SR) onto SSP
2. Redirects LINEA vector to `SV_TRAMP_ADDR` (0x000108)
3. Sets PC to the user function address
4. User function runs in supervisor mode, typically does `MOVE.W SR,D0; RTE`
5. RTE pops the manually-pushed frame, returns to caller

## Thunk System Details

### Calling Convention

Thunks receive `(struct M68KEmuContext *ctx, void *cpu)` and access m68k
registers via macros:

| Macro | Purpose |
|-------|---------|
| `THUNK_D(n)` | Read data register Dn as ULONG |
| `THUNK_A(n)` | Read address register An as ULONG (raw m68k address) |
| `THUNK_PTR(n)` | Read An and translate via `m68k_to_host_or_shadow` |
| `THUNK_DPTR(n)` | Read Dn and translate via `m68k_to_host_or_shadow` |
| `SET_D(n, val)` | Write data register Dn |
| `SET_A(n, val)` | Write address register An |

Return value is placed in D0 by the dispatch code.

### Manual Thunk Priority

When both manual and generated thunks exist for the same LVO, the manual thunk
wins. The dispatch code in `willExecute` searches `lib->thunks[]` (manual)
before `lib->gen_thunks[]` (generated).

### Complete Manual Thunk List

**exec.library (33 manual thunks):**

| LVO | Function | Handling |
|-----|----------|----------|
| -30 | Supervisor | Stores user func addr, triggers 68020 exception frame trampoline |
| -132 | Forbid | No-op (single-threaded) |
| -138 | Permit | No-op (single-threaded) |
| -198 | AllocMem | Containment heap allocation |
| -210 | FreeMem | Containment heap free |
| -216 | AvailMem | Returns heap_end - heap_start |
| -276 | FindName | Shadow-aware; returns fake lib base for NT_LIBRARY/NT_DEVICE |
| -294 | FindTask | Creates 228-byte Process shadow, sets pr_CLI for CLI detection |
| -306 | SetSignal | Pass-through to native |
| -354 | AddPort | No-op |
| -360 | RemPort | No-op |
| -372 | GetMsg | Shadow-resolves port, creates IntuiMessage shadow with IDCMPWindow |
| -378 | ReplyMsg | Shadow-resolves message, calls native ReplyMsg, destroys shadow |
| -384 | WaitPort | Shadow-resolves port, calls native WaitPort |
| -408 | OldOpenLibrary | Delegates to OpenLibrary |
| -414 | CloseLibrary | No-op for fake libs |
| -522 | RawDoFmt | Custom implementation: %s (m68k pointer translation), %d (16-bit), %ld (32-bit) |
| -534 | TypeOfMem | Returns MEMF_PUBLIC always |
| -552 | OpenLibrary | Creates fake base + registers thunks; falls back to m68k library loader |
| -558 | InitSemaphore | No-op |
| -564 | ObtainSemaphore | No-op |
| -570 | ReleaseSemaphore | No-op |
| -624 | CopyMem | Direct memcpy in containment |
| -636 | CacheClearU | No-op |
| -684 | AllocVec | Containment heap allocation |
| -690 | FreeVec | Containment heap free |
| -696 | CreatePool | Returns fake handle 0x00CAFE00 |
| -708 | AllocPooled | Containment heap allocation |
| -714 | FreePooled | Containment heap free |
| -720 | DeletePool | No-op |
| -852 | AVL_AddNode | No-op stub |
| -858 | AVL_RemNodeByAddress | No-op stub |
| -864 | AVL_FindNode | No-op stub |

**dos.library (47 manual thunks):**

| LVO | Function | Handling |
|-----|----------|----------|
| -30 | Open | String translation for filename |
| -36 | Close | BPTR pass-through |
| -42 | Read | Buffer pointer translation |
| -48 | Write | Buffer pointer translation |
| -54 | Input | Pass-through |
| -60 | Output | Pass-through |
| -66 | Seek | BPTR pass-through |
| -72 | DeleteFile | String translation |
| -78 | Rename | String translation for both names |
| -84 | Lock | String translation |
| -90 | UnLock | BPTR pass-through |
| -102 | Examine | Allocates native FIB, calls Examine, copies all fields to m68k FIB |
| -108 | ExNext | Allocates native FIB, copies state from m68k, calls ExNext, copies results back |
| -126 | CurrentDir | BPTR pass-through |
| -132 | IoErr | Pass-through |
| -138 | CreateProc | String translation, returns MsgPort shadow |
| -150 | LoadSeg | String translation |
| -156 | UnLoadSeg | BPTR pass-through |
| -198 | Delay | Pass-through |
| -222 | Execute | String translation |
| -228 | AllocDosObject | TagList translation; FIB returns shadow |
| -234 | FreeDosObject | Pass-through |
| -240 | DoPkt | Pass-through |
| -246 | SendPkt | Pass-through |
| -252 | WaitPkt | Pass-through |
| -258 | ReplyPkt | Pass-through |
| -342 | FPuts | String translation |
| -348 | VFPrintf | Format string parsing with translate_fmt_args |
| -354 | VFWritef | Format string parsing with translate_fmt_args |
| -432 | ExAll | Pass-through |
| -462 | SetIoErr | Pass-through |
| -492 | CreateNewProc | TagList translation, returns Process shadow |
| -498 | RunProcess | **Stubbed** (returns 0) |
| -504 | RunCommand | String translation |
| -576 | GetProgramName | Copies from ctx->program_name |
| -604 | SystemTagList | String + TagList translation |
| -768 | NewLoadSeg | String + TagList translation |
| -798 | ReadArgs | Host-side array allocation, copies results back to m68k |
| -822 | MatchFirst | String + pointer translation |
| -828 | MatchNext | Pointer translation |
| -834 | MatchEnd | Pointer translation |
| -858 | FreeArgs | Pass-through |
| -930 | CliInitNewcli | Pass-through |
| -936 | CliInitRun | Pass-through |
| -948 | PutStr | String translation |
| -954 | VPrintf | Format string parsing, translates %s pointers and %d/%ld integers |
| -990 | ExAllEnd | Pass-through |

**intuition.library (8 manual thunks):**

| LVO | Function | Handling |
|-----|----------|----------|
| -66 | CloseScreen | Shadow-resolves screen, destroys shadow |
| -72 | CloseWindow | Shadow-resolves window, destroys shadow |
| -198 | OpenScreen | Reads m68k NewScreen fields, creates Screen shadow with ViewPort + RastPort sub-shadows |
| -204 | OpenWindow | Reads m68k NewWindow fields, creates Window shadow with RPort + UserPort + WScreen |
| -348 | AutoRequest | Translates IntuiText chains (up to 8 deep), calls native |
| -432 | RefreshGList | No-op |
| -438 | SetWindowPointerA | No-op (avoids X11 blocking) |
| -606 | OpenWindowTagList | TagList translation with WA_Title/WA_CustomScreen special handling |

**graphics.library (12 manual thunks):**

| LVO | Function | Handling |
|-----|----------|----------|
| -60 | Text | RastPort shadow resolution via `resolve_rp` |
| -66 | TextLength | RastPort shadow resolution |
| -72 | OpenFont | Reads m68k TextAttr fields, calls native OpenFont |
| -192 | LoadRGB4 | Big-endian byte-swap of UWORD color array |
| -240 | Move | RastPort shadow resolution |
| -246 | Draw | RastPort shadow resolution |
| -306 | RectFill | RastPort shadow resolution |
| -318 | ScrollRaster | RastPort shadow resolution |
| -342 | SetAPen | RastPort shadow resolution |
| -348 | SetBPen | RastPort shadow resolution |
| -354 | SetDrMd | RastPort shadow resolution |
| -864 | SetABPenDrMd | RastPort shadow resolution |

### Auto-Generated Thunk Type Translations

The generator (`m68kemu_thunkgen.py`) classifies each parameter:

| Classification | Translation | Used For |
|---------------|-------------|----------|
| shadow | `m68k_to_host_or_shadow()` | RastPort, Window, Screen, ViewPort, BitMap, ColorMap, Layer, Region, TextFont, Menu, Gadget, Task, Process, SignalSemaphore, IORequest, Library, Device, MsgPort, etc. |
| string | `m68k_to_host()` | STRPTR, CONST_STRPTR, char*, UBYTE* |
| taglist | `m68k_to_native_taglist()` + cleanup | struct TagItem* |
| input_struct | Skipped (needs manual thunk) | NewWindow, NewScreen, TextAttr, EasyStruct, NewGadget, Border, IntuiText |
| scalar | `THUNK_D(n)` as ULONG | ULONG, LONG, WORD, BOOL, BPTR, Tag, APTR |
| pointer | `THUNK_PTR(n)` or `THUNK_DPTR(n)` | Remaining pointer types |

Return values: void functions return 0; struct pointer returns get wrapped with `shadow_create_by_name()`; scalars are cast to IPTR.

## Shadow Struct Engine

### API

| Function | Purpose |
|----------|---------|
| `shadow_create(ctx, layout, native)` | Allocate m68k heap, register, sync all fields native→m68k. Returns m68k address. |
| `shadow_sync(ctx, layout, m68k_addr, native)` | Re-sync all fields from native→m68k for existing shadow. |
| `shadow_destroy(ctx, layout, m68k_addr)` | Remove from registry, free m68k heap. |
| `shadow_find_layout(name)` | Linear scan of 297-entry table (with static cache for last hit). |
| `shadow_create_by_name(ctx, name, native)` | Convenience: find_layout + create. |
| `shadow_init_execbase(ctx, m68k_base, native)` | Special ExecBase init: syncs fields, sets AttnFlags=68040+FPU, creates Task/Process shadow for ThisTask with CLI struct, registers 9 embedded lists. |
| `shadow_register(ctx, m68k_addr, native, type)` | Append to shadow_map[] (max 64). |
| `shadow_lookup(ctx, m68k_addr)` | Linear scan by m68k_addr, returns native pointer or NULL. |
| `shadow_remove(ctx, m68k_addr)` | Linear scan + swap-with-last removal. |
| `m68k_to_native_taglist(ctx, m68k_addr)` | Convert m68k TagItem list (8B/entry) to native (16B on 64-bit). Values in [0x100, mem_size) treated as pointers. |
| `m68k_to_native_struct(ctx, name, m68k_addr)` | Reverse-sync m68k→native (allocates with 3× overestimate). |

### Field Types

```c
enum { SF_END=0, SF_BYTE, SF_WORD, SF_LONG, SF_PTR };
```

| Type | Sync Behavior |
|------|--------------|
| SF_BYTE | Direct byte copy |
| SF_WORD | 16-bit big-endian via m68k_write16/read16 |
| SF_LONG | 32-bit big-endian via m68k_write32/read32 |
| SF_PTR | Complex: NULL→0, scan for existing shadow of same native_ptr+type, if sub_layout→recursive create, else→0 |
| SF_END | Terminator sentinel |

### Type Detection Heuristic (in generator)

| m68k size | AROS size | Classification |
|-----------|-----------|---------------|
| 1 | 1 | SF_BYTE |
| 2 | 2 | SF_WORD |
| 4 | 8 | SF_PTR (pointer widening 32→64 bit) |
| 4 | 4 | SF_LONG |
| other | other | Skipped |

### Registry

Flat array in `M68KEmuContext`:

```c
struct { ULONG m68k_addr; void *native_ptr; UBYTE type; } shadow_map[64];
UWORD num_shadows;
```

- **Register**: O(1) append
- **Lookup**: O(n) linear scan by m68k_addr
- **Remove**: O(n) scan + O(1) swap-with-last
- **Dedup**: sync_fields scans for matching native_ptr + shadow_type to avoid duplicates

Type tags: 0=untyped, 1=Window, 3=RastPort, 4=MsgPort, 5=Screen, 6=ViewPort, 17=List, 20+=auto-generated layout types.

### Recursive Sub-Shadows

`M68KFieldMap` has `sub_layout` (const M68KStructLayout*) for pointer fields. When `sync_fields` encounters SF_PTR with non-NULL `sub_layout`, it recursively allocates and syncs the pointee. **However**: all auto-generated layouts set `sub_layout=NULL`. The recursive capability is only exercised by manually constructed layouts (OpenScreen, OpenWindow, ExecBase init).

## Shadow Struct Layouts

297 struct layouts with 2,918 field mappings (1,237 pointers correctly detected as SF_PTR for 32→64 bit conversion). Format: Name (m68k size → AROS size, field count).

### Most Thoroughly Mapped

| Struct | m68k | AROS | Fields | Domain |
|--------|------|------|--------|--------|
| GfxBase | 552B | 1040B | 65 | graphics |
| Screen | 346B | 1408B | 62 | intuition |
| Process | 228B | 768B | 56 | exec |
| Window | 136B | 272B | 26 | intuition |
| ColorMap | 52B | 96B | 19 | graphics |
| ConUnit | 296B | 640B | 19 | devices |
| IOStdReq | 48B | 96B | 16 | exec |
| CommandLineInterface | 64B | 128B | 16 | dos |
| MonitorSpec | 160B | 896B | 14 | graphics |
| timerequest | 40B | 80B | 14 | devices |
| MsgPort | 34B | 256B | 13 | exec |

### Complete List

The full list of 297 struct layouts is in `m68kemu_shadow_layouts.h` (auto-generated).

### Missing Structs

1 of 298 structs in `m68k_offsets.txt` lacks a shadow layout:

**RegionRectangle** — its only field is `bounds` (an embedded `struct Rectangle`,
8 bytes), which has no scalar leaf fields to map.

## Device I/O Path

### Call Chain

```plain
m68k: CreateMsgPort()
  → thunk creates native MsgPort, returns shadow address

m68k: CreateIORequest(port, size)
  → thunk resolves port shadow, creates native IORequest
  → wraps result with shadow_create_by_name("IORequest", native)

m68k: OpenDevice("timer.device", 0, ioReq, 0)
  → thunk translates name string, resolves IORequest shadow
  → calls native OpenDevice with native pointers

m68k: DoIO(ioReq)
  → thunk resolves IORequest shadow, calls native DoIO
  → io_sync_back() syncs IOStdReq fields back to m68k shadow

m68k: io_Error = ioReq->io_Error   ← reads correct value

m68k: CloseDevice(ioReq)
  → thunk resolves shadow, calls native CloseDevice, removes shadow

m68k: DeleteIORequest(ioReq)
  → thunk resolves shadow, calls native DeleteIORequest, removes shadow
```

### IORequest Shadow Layouts

| Struct | m68k | AROS | Fields Mapped |
|--------|------|------|--------------|
| IORequest | 32B | 72B | io_Command, io_Flags, io_Error |
| IOStdReq | 48B | 96B | io_Command, io_Flags, io_Error, io_Actual, io_Length, io_Data, io_Offset + inherited Node/Message fields |
| IOExtSer | 82B | 136B | io_Status |
| IOExtTD | 56B | 104B | iotd_Count, iotd_SecLabel |
| IOAudio | 68B | 152B | 6 fields (AllocKey, Data, Length, Period, Volume, Cycles) |

### IDCMP Message Flow

1. `OpenWindow` creates Window shadow with UserPort sub-shadow (MsgPort at m68k offset 86)
2. m68k code reads UserPort from Window shadow, calls `WaitPort`/`GetMsg`
3. `GetMsg` thunk: shadow-resolves port → native GetMsg → creates IntuiMessage shadow
4. IntuiMessage shadow syncs: Class, Code, Qualifier, MouseX, MouseY, IDCMPWindow
5. IDCMPWindow field: reverse-lookup finds Window shadow by native pointer match
6. m68k code reads fields, calls `ReplyMsg` → destroys IntuiMessage shadow
7. `GT_GetIMsg`/`GT_ReplyIMsg` in gadtools follow the same pattern

## Custom Chip Emulation

All accesses to `$DFF000–$DFFFFF` are intercepted in `AROSMoira::read8/read16/write8/write16`.

### Read Registers

| Offset | Name | Value |
|--------|------|-------|
| 0x002 | DMACONR | 0 |
| 0x004 | VPOSR | `(beam_lof << 15) \| ((beam_v >> 8) & 1)` |
| 0x006 | VHPOSR | `((beam_v & 0xFF) << 8) \| beam_h` |
| 0x010 | ADKCONR | 0 |
| 0x016 | POTGOR | 0xFF00 (no buttons pressed) |
| 0x01C | INTREQR | 0 |
| 0x01E | INTENA | 0 |
| other | — | 0 |

Every read calls `beamAdvance(2)` — 2 color clocks per register access.

### Write Registers

| Offset | Name | Effect |
|--------|------|--------|
| 0x02A | VPOSW | Sets beam_lof from bit 15, merges V8 into beam_v |
| 0x02C | VHPOSW | Sets beam_v[7:0] and beam_h |
| other | — | Silently ignored |

Byte writes (`write8`) to the custom chip range are silently ignored.

### Beam Counter

PAL timing: 313 lines × 227 color clocks per line. `beamAdvance(ticks)` adds
ticks to beam_h, wraps at 227 (incrementing beam_v), wraps beam_v at 313
(toggling beam_lof for interlace). The 2-tick-per-read value approximates a
real 68000 register read (~4 cycles = 2 color clocks at 7.09 MHz / 3.546 MHz).

## Thunk Generator Toolchain

Two generators exist:

### m68kemu_thunkgen.py (production)

- **Input**: Amiga FD files (`*_lib.fd`) + NDK clib proto headers
- **Output**: Multi-library C source with master registration table
- **Features**: Full type classification (shadow/string/taglist/input_struct/scalar),
  shadow object support, return value wrapping, extensive skip lists (~70 libraries,
  ~160 functions, ~300 void functions)
- **Proto parsing**: Reads NDK `*_protos.h` for accurate C types; falls back to
  heuristics (arg name, register class) when no proto available

### tools/m68kthunkgen/m68kthunkgen.py (simplified)

- **Input**: AROS `.conf` files
- **Output**: Single-library C source
- **Features**: Basic pointer-vs-scalar classification only. No shadow objects,
  no taglist translation, no struct layout awareness.
- **Use case**: Generating thunks from AROS's own `.conf` format (which already
  contains type + register info)

## Test Infrastructure

### Host-Side Tests (no AROS required)

| Test | File | What It Tests |
|------|------|--------------|
| t_ctx | test_m68kemu.cpp | Context create/destroy |
| t_heap | test_m68kemu.cpp | Heap alloc/free/reuse |
| t_stress | test_m68kemu.cpp | 1000 × 64B alloc, free all, 100KB alloc |
| t_endian | test_m68kemu.cpp | Big-endian read16/write16/read32/write32 |
| t_xlat | test_m68kemu.cpp | Pointer translation round-trip |
| t_libbase | test_m68kemu.cpp | Fake lib base setup, A-line opcode verification |
| t_exec_moveq_rts | test_moira_exec.cpp | MOVEQ #42,D0; RTS → result 42 |
| t_exec_add | test_moira_exec.cpp | MOVEQ + ADDI → result 42 |
| t_exec_loop | test_moira_exec.cpp | Loop summing 1..10 → result 55 |
| t_exec_memory_access | test_moira_exec.cpp | MOVE.L store/load round-trip |
| (integration) | test_hello_thunks.cpp | Full A-line trap → thunk dispatch → return |

### AROS-Side Test

`test_m68kemu.c` — Opens `m68kemu.library`, writes an embedded m68k hunk binary
(MOVEQ #42,D0; RTS) to `RAM:`, calls `RunFile`, and verifies the result is 42.

### Test Gaps

- No tests for custom chip registers (VPOSR, VHPOSR, POTGOR, beam advancement)
- No tests for Supervisor() dispatch path
- No tests for generated thunk fallback path
- No tests for unimplemented LVO handling
- No tests for hunk loader (LoadHunksFromMemory)
- No tests for heap alignment or exhaustion
- No tests for out-of-bounds memory access
- Makefile only builds test_m68kemu; Moira-based tests require manual compilation

## Known Bugs

No known bugs at this time.
