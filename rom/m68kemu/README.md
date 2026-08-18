# m68kemu.library — Motorola 680x0 Emulation for AROS

A transparent emulation layer that allows classic AmigaOS m68k binaries to run
on any AROS port (x86_64, aarch64, etc.) by emulating the m68k CPU and routing
AmigaOS API calls to native AROS implementations.

## How It Works

### Overview

When AROS encounters an m68k hunk executable, `dos.library` detects the hunk
magic (`0x000003F3`) and routes execution through `m68kemu.library` instead of
attempting native execution. The emulator loads the binary into a contained
memory space, emulates the m68k CPU instruction by instruction, and intercepts
all AmigaOS library calls to forward them to the native AROS implementations.

```plain
┌───────────────────────────────────────────────────┐
│  m68k binary (e.g. LZX, SysInfo, DPaint)          │
│  runs inside Moira CPU emulator                   │
│                                                   │
│  JSR -offset(A6) ──► A-line trap ──► thunk        │
│                                        │          │
│  ┌────────────────────────────────┐    │          │
│  │  32 MB containment memory      │    │          │
│  │  (big-endian, m68k layout)     │    │          │
│  └────────────────────────────────┘    │          │
└────────────────────────────────────────┼──────────┘
                                         │
                  ┌──────────────────────▼────────────┐
                  │  Native AROS (x86_64/aarch64)     │
                  │                                   │
                  │  exec.library    dos.library      │
                  │  intuition.library  graphics.lib  │
                  │  ... 17 libraries, ~910 thunks    │
                  └───────────────────────────────────┘
```

### CPU Emulation

The m68k CPU is emulated by [Moira](https://github.com/dirkwhoffmann/Moira),
a cycle-accurate Motorola 680x0 emulator written by Dirk W. Hoffmann. Moira is
integrated as a git submodule and provides instruction-level emulation of the
M68040 instruction set including the built-in FPU. The emulator runs in M68040
mode with a proper `Supervisor()` implementation that executes user functions in
supervisor mode, enabling CPU detection code used by programs like SysInfo.

**Not emulated**: Interrupts, DMA.

### Containment Memory Model

All m68k code and data lives inside a 32 MB memory block ("containment memory").
This block uses big-endian byte order, matching the m68k architecture. A
first-fit heap allocator with coalescing handles `AllocMem`/`FreeMem` requests
from emulated code. The m68k stack, hunk segments, and all dynamically allocated
memory reside within this block.

```plain
Address Map (32 MB):
0x000000 - 0x0003FF  Exception vectors (1 KB)
0x000400 - 0x0103FF  Library base region (64 KB) — fake jump tables
0x010400 - 0x01FEFFFF  Heap (grows upward, first-fit with coalescing)
0x01FF0000 - 0x01FFFFFF  Stack (64 KB, grows downward)
```

### Custom Chip Register Emulation

Programs that poll Amiga custom chip registers (e.g., busy-waiting on the video
beam position via VHPOSR at `$DFF006`) are supported through a minimal custom
chip emulation layer. The emulator intercepts reads/writes to the `$DFF000-$DFFFFF`
address range and provides:

- **VPOSR/VHPOSR** (`$DFF004`/`$DFF006`): PAL beam position counter (313 lines ×
  227 color clocks per frame) with LOF bit and PAL Agnus chip ID
- **VPOSW/VHPOSW** (`$DFF02A`/`$DFF02C`): Writable beam position
- **POTGOR** (`$DFF016`): Returns `$FF00` (no buttons pressed)
- **DMACONR, INTREQR, INTENA, ADKCONR**: Return 0

The beam counter advances by 2 color clocks per custom chip register read,
providing changing values for polling loops and RNG seeding. There is no
per-instruction cycle counting — the beam only advances on register access.

### A-Line Trap Mechanism

AmigaOS programs call library functions via `JSR -offset(A6)`, where A6 holds
the library base address. In the emulator, each library's jump table contains
`A-line` opcodes (`0xA000`). When the m68k CPU executes one of these, Moira
raises a Line-A exception. The exception handler:

1. Determines which library and function (LVO) was called based on the PC
2. Pops the JSR return address from the m68k stack
3. Looks up the thunk: manual thunks first, then auto-generated (manual override)
4. Executes the corresponding native thunk function
5. Returns control to the m68k code via an RTE instruction

Unimplemented LVOs return 0 with a debug log message.

### Thunk System

Thunks bridge the gap between the m68k calling convention (registers D0-D7/A0-A6)
and native AROS C functions. There are two kinds:

- **Auto-generated thunks** (810 across 17 libraries): Created by
  `m68kemu_thunkgen.py` from Amiga FD files and NDK clib prototype headers.
  The generator parses C prototypes for full type information, automatically
  emitting correct pointer translation code: `m68k_to_host_or_shadow` for
  struct pointers (256 uses), `m68k_to_host` for string pointers (133 uses),
  `m68k_to_native_taglist` for TagItem arrays (73 uses), `resolve_rp` for
  RastPort resolution (63 uses), and `shadow_create_by_name` for return values
  that need shadow wrapping (34 uses).

- **Manual thunks** (~100) for functions needing special handling:
  - **exec**: `OpenLibrary` (fake base creation + m68k library fallback),
    `AllocMem`/`FreeMem`/`AllocVec`/`FreeVec` (containment heap),
    `CreatePool`/`AllocPooled`/`FreePooled` (fake pool handle),
    `FindTask` (Process shadow with pr_CLI), `RawDoFmt` (custom %s/%d),
    `WaitPort`/`GetMsg`/`ReplyMsg` (IDCMP shadow management),
    `Supervisor` (68020 exception frame trampoline),
    `Forbid`/`Permit`/semaphores (no-ops — single-threaded)
  - **dos**: `Open`/`Close`/`Read`/`Write`/`Seek` (BPTR pass-through),
    `ReadArgs`/`FreeArgs` (host-side array translation),
    `Lock`/`UnLock`/`CurrentDir`, `AllocDosObject`/`FreeDosObject`,
    `Execute`/`SystemTagList`/`RunCommand`, `VPrintf`/`VFPrintf`,
    `GetProgramName` (from context), `MatchFirst`/`MatchNext`/`MatchEnd`
  - **intuition**: `OpenScreen`/`CloseScreen` (NewScreen translation + shadow),
    `OpenWindow`/`OpenWindowTagList`/`CloseWindow` (NewWindow translation +
    RPort/UserPort/WScreen sub-shadows), `AutoRequest` (IntuiText chain
    translation)
  - **graphics**: `OpenFont` (TextAttr translation), `LoadRGB4` (big-endian
    byte-swap), `Move`/`Draw`/`Text`/`RectFill`/`SetAPen`/`SetBPen`/`SetDrMd`/
    `SetABPenDrMd`/`ScrollRaster`/`TextLength`/`SetFont` (RastPort shadow
    resolution via `resolve_rp`)

### Device I/O

Device I/O is supported through the exec thunks. The full chain works:

1. `CreateMsgPort` → native MsgPort, returns shadow address
2. `CreateIORequest` → native IORequest, returns shadow address
3. `OpenDevice` → resolves IORequest shadow, calls native OpenDevice
4. `DoIO`/`SendIO`/`WaitIO`/`CheckIO`/`AbortIO` → resolve shadow, call native
5. `CloseDevice`/`DeleteIORequest`/`DeleteMsgPort` → cleanup

Shadow layouts exist for IORequest, IOStdReq, IOExtSer, IOExtTD, IOAudio,
IODRPReq, IODRPTagsReq, and several printer IO structs.

**Known limitation**: After `DoIO`/`WaitIO` complete, the native IORequest
fields (io_Error, io_Actual, etc.) are not synced back to the m68k shadow.
Programs reading these fields from m68k space will see stale values. See
[INTERNALS.md](INTERNALS.md) for details.

### C Startup Library Auto-Open

Classic Amiga C compilers (SAS/C, Lattice, Aztec, DICE) embed a table of
library base pointer slots followed by library name strings in the binary.
`M68KEmu_AutoOpenLibs()` scans containment memory after hunk loading for this
pattern and pre-fills the base pointer slots with fake library bases, enabling
multi-hunk C programs to find their libraries without executing the startup
code's OpenLibrary loop.

### Shadow Struct System

The central challenge: m68k structs have different field offsets and sizes than
native AROS structs (32-bit vs 64-bit pointers, different alignment). When a
native function returns a struct pointer (e.g., `OpenWindow` returns a
`Window *`), the m68k code expects to read fields at m68k offsets.

The shadow system solves this by maintaining parallel m68k-layout copies of
native structs in containment memory. When a native function returns a struct
pointer, a shadow is created: a block of containment memory with fields copied
from the native struct at the correct m68k offsets. Pointer fields are
recursively resolved.

**297 struct layouts** with **2,918 field mappings** are auto-generated from the
NDK and AROS headers by the shadow layout toolchain. This covers virtually all
public AmigaOS structs across exec, dos, intuition, graphics, utility, workbench,
devices, libraries, resources, and datatypes. The offset extractor recurses into
embedded sub-structs (Node, Message, IORequest, Task, etc.) to map every scalar
leaf field at its actual offset. The most thoroughly mapped structs include
GfxBase (65 fields), Screen (62), Process (56), Window (26), ColorMap (19),
and IOStdReq (16). See [INTERNALS.md](INTERNALS.md) for the complete struct list.

The `OpenScreen` and `OpenWindow` thunks create shadows with properly wired
embedded structs: the Screen shadow includes registered ViewPort (offset 44) and
RastPort (offset 84) sub-shadows, and the Window shadow includes RPort, UserPort
(for IDCMP WaitPort/GetMsg), and WScreen back-pointer.

The shadow registry is a flat array (max 64 entries) with linear scan lookup.
Type tags distinguish shadow kinds: 1=Window, 3=RastPort, 4=MsgPort, 5=Screen,
6=ViewPort, 17=List, 20+=auto-generated layouts.

### m68k Library Loading

Libraries without native AROS equivalents (e.g., `reqtools.library`) are loaded
as m68k hunk files. The loader searches in this order:

1. `LIBS68K:<name>` — dedicated assign for m68k libraries
2. `LIBS:m68k/<name>` — subdirectory, no assign needed
3. `LIBS:<name>` — fallback for compatibility

This keeps m68k libraries separate from native AROS libraries, avoiding
name conflicts. Users can set up `LIBS68K:` in their Startup-Sequence:

```
Assign LIBS68K: SYS:Libs/m68k
```

The loader:

1. Parses the hunk file (CODE/DATA/BSS/RELOC32)
2. Scans for `RTC_MATCHWORD` (`0x4AFC`) and reads the `RTF_AUTOINIT` function table
3. Builds a JMP absolute long jump table — m68k-to-m68k calls work with no traps
4. Handles both absolute and relative function pointers in the init table

Non-AUTOINIT libraries are not supported.

### Threading

GUI programs that block on `WaitPort` would freeze AROS if run in the main
process. `RunFile` uses `CreateNewProc(NP_Synchronous=TRUE)` to run the emulator
in a child process, allowing the AROS event loop to continue.

### dos.library Integration

To run m68k binaries transparently, two functions in `dos.library` need patching:

- **RunCommand**: Checks `GetSegListInfo(GSLI_68KHUNK)` — if the seglist is an
  m68k hunk binary, opens `m68kemu.library` and calls `RunHunk` instead of
  native execution.
- **CreateNewProc**: Same detection — m68k processes created via `CreateNewProc`
  are also routed through the emulator.

This makes m68k binary execution fully transparent: no special commands needed.

### Execution Lifecycle

1. `dos.library` detects hunk binary, calls `RunHunk(segList, stackSize, argPtr, argSize)`
2. `RunHunk` creates a 32 MB containment buffer and heap
3. Hunk segments are loaded into containment memory (with relocations applied)
4. exec.library and dos.library fake bases are set up with A-line trap jump tables
5. ExecBase shadow is initialized at address 4 (AttnFlags set to 68040+FPU)
6. `AutoOpenLibs` scans for C startup library tables and pre-fills them
7. Sentinel address (`0x00DEAD00`) is pushed as return address
8. Moira CPU starts executing at the entry point in M68040 mode (with FPU)
9. Library calls trigger A-line traps → thunk dispatch → native AROS calls
10. When the program does RTS to the sentinel, execution stops
11. Context is destroyed, exit code returned to AROS

## Supported Libraries

The following 17 libraries are natively thunked (m68k calls → native AROS):

| Library | Auto | Manual | Total | Key Coverage |
|---------|------|--------|-------|-------------|
| exec.library | 81 | 33 | 114 | Memory, tasks, signals, semaphores, messages, devices, libraries |
| dos.library | 112 | 47 | 159 | File I/O, locks, CLI, pattern matching, ReadArgs, packets |
| intuition.library | 130 | 8 | 138 | Windows, screens, gadgets, menus, IDCMP, requesters |
| graphics.library | 172 | 12 | 184 | Rendering, text, blitting, layers, fonts, colors |
| layers.library | 40 | — | 40 | Layer management |
| utility.library | 42 | — | 42 | TagItem processing, hooks, date functions |
| gadtools.library | 18 | — | 18 | GadTools gadget toolkit |
| asl.library | 8 | — | 8 | File/font/screenmode requesters |
| commodities.library | 29 | — | 29 | Input event handling |
| cybergraphics.library | 24 | — | 24 | RTG (retargetable graphics) |
| datatypes.library | 30 | — | 30 | Multimedia data type system |
| diskfont.library | 3 | — | 3 | Disk-based font loading |
| expansion.library | 22 | — | 22 | Expansion board management |
| icon.library | 21 | — | 21 | Workbench icon handling |
| iffparse.library | 40 | — | 40 | IFF file parsing |
| keymap.library | 4 | — | 4 | Keyboard mapping |
| locale.library | 34 | — | 34 | Localization |
| **Total** | **810** | **~100** | **~910** | |

Libraries not in this list are loaded as m68k code and executed
within the emulator.

## Tested Programs

| Program | Size | Type | Status |
|---------|------|------|--------|
| LZX | 83 KB | CLI archiver (commercial) | ✅ Full functionality |
| olha | 291 KB | SFX archiver (6 hunks, relocations) | ✅ LH5 decompression, CreateDir |
| ROLEX | 13 KB | Clock (custom screen, GUI loop) | ✅ Opens screen, event loop works |
| SysInfo 4.4 | 62 KB | System info (7 hunks, custom screen) | ✅ Renders full UI (title, menus, buttons, text) |
| Directory Opus 4.12 | 328 KB | File manager (GUI, m68k libraries) | ✅ Loads dopus.library, shows AutoRequest dialog, responds to button click |
| AmigaTerm 1.0 | 82 KB | Terminal emulator (GUI) | ✅ Opens window, clean exit |
| Textcraft Plus | 208 KB | Word processor (GUI, custom screen) | ✅ Opens screen/window, sets colors, clean exit |
| reqtools.library | 56 KB | m68k shared library (29 functions) | ✅ |

## Known Limitations

### CPU

- No interrupt delivery to m68k code (INTREQR/INTENA return 0)
- Single-threaded: one m68k instruction at a time, no cycle counting

### Memory

- Fixed 32 MB containment, fixed 64 KB stack (RunHunk ignores stackSize parameter)
- No MEMF_CHIP/MEMF_FAST distinction (requirements parameter ignored)
- No memory protection within containment

### Thunks

- `RawDoFmt` only handles %s, %d, and %ld; other specifiers get basic fallback
- `CreatePool` returns fake handle; pool semantics not preserved
- `Forbid`/`Permit`/semaphores are no-ops (single-threaded assumption)

### Device I/O

- Endianness issues for structured (non-byte) io_Data buffers

### Shadow System

- Max 64 simultaneous shadows (linear scan lookup)
- Max 32 libraries, max 16 library name mappings
- 1 of 298 structs lacks a shadow layout (RegionRectangle — no scalar fields)

### Custom Chips

- Only beam position (VPOSR/VHPOSR) and POTGOR are functional
- No blitter, copper, audio, sprites, or DMA emulation
- Beam advances only on register reads, not per-instruction

### Hunk Loader

- Max 100 hunks per binary
- No overlay hunks (HUNK_OVERLAY, HUNK_BREAK)
- No HUNK_DEBUG or HUNK_SYMBOL support
- Non-AUTOINIT m68k libraries not supported

## File Structure

### Core

| File | Description |
|------|-------------|
| `m68kemu.conf` | AROS library definition (LVO 5: RunHunk, LVO 6: RunFile) |
| `m68kemu_intern.h` | Context struct, fake lib bases, inline helpers, memory accessors |
| `m68kemu_moira.cpp` | Moira subclass, A-line exception handler, sentinel detection |
| `m68kemu_run.c` | RunHunk/RunFile entry points, process creation, ExecBase shadow init |
| `m68kemu_memory.c` | Containment memory allocation, heap allocator, hunk loaders |
| `m68kemu_init.c` | Library init/open/close |
| `m68kemu_stubs.c` | AROS library stubs |
| `mmakefile.src` | Build rules (C++20 for Moira, links stdc++/stdcio/stdc/posixc) |

### Thunks

| File | Description |
|------|-------------|
| `m68kemu_thunks.h` | THUNK_D/A/PTR/DPTR/SET_D/SET_A macros, thunk entry/table structs |
| `m68kemu_thunks.c` | ~100 manual thunks, shadow registration, OpenLibrary with auto-registration |
| `m68kemu_thunks_gen.c` | 810 auto-generated type-aware thunks across 17 libraries |
| `m68kemu_thunkgen.py` | Generator: reads FD files + NDK clib protos → emits C thunks with correct pointer translation |

### Shadow Struct System

| File | Description |
|------|-------------|
| `m68kemu_shadow.h` | Field type enums (SF_BYTE/WORD/LONG/PTR), M68KFieldMap, M68KStructLayout |
| `m68kemu_shadow.c` | Generic shadow engine: create/sync/destroy with recursive sub-layouts |
| `m68kemu_shadow_layouts.h` | **Generated**: 297 struct layouts, 2,918 field mappings |

### Shadow Layout Toolchain

| File | Description |
|------|-------------|
| `gen_offset_extractor.py` | Scans NDK headers → emits C file with `offsetof()` + `sizeof()` for every struct field |
| `gen_shadow_layouts.py` | Reads m68k + AROS offset files → emits `m68kemu_shadow_layouts.h` |
| `m68k_offsets.txt` | Static m68k struct offsets (3,017 fields, 298 structs) — checked in, never changes |

#### Regenerating Shadow Layouts

The m68k offsets are fixed (AmigaOS binary compatibility) and checked into the
repository. Only the AROS offsets need to be regenerated when targeting a
different architecture:

```bash
# Generate AROS offsets for the current target (done at build time)
python3 gen_offset_extractor.py > offsets_src.c
sed 's/"n"/"i"/g; s/%0/%c0/g; s/%1/%c1/g' offsets_src.c > offsets_aros.c
gcc -S -o aros_offsets.s offsets_aros.c -isystem $AROS_SDK/include ...
grep -oP '(OFF|SIZ)\|[^"]*' aros_offsets.s > aros_offsets.txt

# Merge into header
python3 gen_shadow_layouts.py m68k_offsets.txt aros_offsets.txt > m68kemu_shadow_layouts.h
```

To add new structs to the m68k side (requires `m68k-amigaos-gcc`):

```bash
python3 gen_offset_extractor.py > offsets_src.c
m68k-amigaos-gcc -S -o m68k.s offsets_src.c -I $NDK_INCLUDE
grep -oP '(OFF|SIZ)\|[^"]*' m68k.s | sed 's/#//g' > m68k_offsets.txt
```

### m68k Library Loader

| File | Description |
|------|-------------|
| `m68kemu_m68klib.c` | Loads .library hunk files from LIBS:, scans for RTC_MATCHWORD, builds JMP table |

### Moira Submodule

| Path | Description |
|------|-------------|
| `Moira/` | Git submodule: [github.com/dirkwhoffmann/Moira](https://github.com/dirkwhoffmann/Moira) |

### Tests

| File | Description |
|------|-------------|
| `tests/m68kemu_host/test_m68kemu.cpp` | Host-side unit tests: context, heap, endian, pointer translation, lib bases |
| `tests/m68kemu_host/test_moira_exec.cpp` | Host-side Moira execution tests: MOVEQ/RTS, ADD, loops, memory access |
| `tests/m68kemu_host/test_hello_thunks.cpp` | Host-side end-to-end: A-line trap → thunk dispatch → return |
| `developer/debug/test/m68kemu/test_m68kemu.c` | AROS-side test: opens m68kemu.library (execution test incomplete) |

## Building

### Prerequisites

- AROS source tree with the `emu68-hosted` branch
- GCC 14 (GCC 15 crashes the AROS linux-x86_64 hosted build)
- C++20 support (for Moira)
- Python 3 (for code generators)

### Build Commands

```bash
cd $AROS_BUILD_DIR
rm -rf bin/linux-x86_64/gen/rom/m68kemu
make kernel-m68kemu
```

The built library appears at `bin/linux-x86_64/AROS/Libs/m68kemu.library`.

### Installation

Copy `m68kemu.library` to `AROS:Libs/`. With the `emu68-hosted` branch,
`dos.library` automatically detects m68k hunk binaries and routes them through
the emulator — no configuration needed.

Without the dos.library patch, use the `RunM68K` CLI tool:

```
RunM68K <m68k-binary> [arguments]
```

### Running Tests

```bash
# Host-side unit tests (no AROS required)
cd tests/m68kemu_host
make
./test_m68kemu

# Moira execution tests (manual build)
g++ -std=c++20 -I../../rom/m68kemu/Moira -o test_moira_exec \
    test_moira_exec.cpp -DNO_INLINE_STDARG

# AROS-side test (built with AROS)
make test-m68kemu
```

## Dependencies

| Dependency | Version | License | Purpose |
|------------|---------|---------|---------|
| [Moira](https://github.com/dirkwhoffmann/Moira) | HEAD | MIT | M68000 CPU emulation core |
| AROS SDK | — | MPL | Target platform headers and libraries |
| GCC | 14.x | GPL | C/C++ compiler (C++20 for Moira) |
| Python | 3.x | PSF | Code generators (thunks, shadow layouts) |
| m68k-amigaos-gcc | 15.x | GPL | m68k cross-compiler (only for regenerating m68k_offsets.txt) |
| AmigaOS NDK | 3.2 | Hyperion | m68k struct definitions (via m68k-amigaos-gcc sysroot) |

## Further Reading

- [INTERNALS.md](INTERNALS.md) — Deep technical reference: memory model, shadow
  engine internals, complete struct layout list, thunk generator details, device
  I/O analysis, and known bugs.

## Acknowledgments

This project would not be possible without:

- **Dirk W. Hoffmann** — Author of [Moira](https://github.com/dirkwhoffmann/Moira),
  the cycle-accurate Motorola 68000 emulator at the heart of this project. Moira
  is also the CPU core of [vAmiga](https://dirkwhoffmann.github.io/vAmiga/),
  Dirk's outstanding Amiga 500/1000/2000 emulator. His meticulous work on
  accurate m68k emulation made this entire effort possible.

- **The AROS Development Team** — For building and maintaining AROS, the open-source
  AmigaOS-compatible operating system that serves as the host platform.

- **Hyperion Entertainment** — For the AmigaOS NDK 3.2, which provides the
  authoritative struct definitions used to generate the shadow layout tables.

- **The Amiga community** — For keeping the platform alive for over 40 years and
  producing the software that this emulator aims to preserve.

## License

`m68kemu.library` is part of AROS and licensed under the
[AROS Public License (APL)](LICENSE), the same license as AROS itself.

The Moira CPU core is licensed under the
[MIT License](https://opensource.org/licenses/MIT).
See `Moira/LICENSE` for details.
