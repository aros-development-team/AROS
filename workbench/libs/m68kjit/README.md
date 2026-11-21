# m68kjit.library

An optional 68k execution engine for `m68kemu.library`. It translates 68k
code to AArch64 code and runs it. `m68kemu.library` uses it when the
variable `M68KEMU_CPU` asks for it and works without it: the interpreter
(Moira) stays the default and the reference.

The library is built for AArch64 targets only, because the translator emits
AArch64 code.

## What it is

The interface between the two libraries is the one `m68kemu.library` has
for an engine, version 1.0: `rom/m68kemu/m68kemu_engine.h`, specified in
`rom/m68kemu/ENGINE.md`. The library has one function, `M68KJit_GetEngine()`
at LVO -30, which returns a table of nine operations.

The engine executes 68k instructions on guest memory the host owns and
returns to the host wherever the host has to act: an A-line opcode (a
library call), a PC it cannot fetch from, an exception, an instruction it
does not execute, the end of a bounded slice. It never calls the host
except for single accesses to memory ranges the host asked to see. Library
calls, tasks, interrupts and locking stay in `m68kemu.library`.

`m68kemu.library` runs the engine under its interpreter: each of its
execution loops tries an engine slice first, and whatever instruction the
engine stops at without executing it is one step of the interpreter.
`rom/m68kemu/INTERNALS.md` describes that side.

## Files

| File | What it is |
| --- | --- |
| `m68kjit.conf` | The library: one function |
| `m68kjit_library.c` | That function |
| `m68kjit_engine.c` | The nine operations, the memory map and the counters |
| `m68kjit_translate.c` | The AArch64 translator |
| `m68kjit_intern.h` | The private interface between the two engine files, and the test seams |
| `m68kjit_platform.h` | What the engine asks of the system it runs on: memory, code memory, a dead end |
| `m68kjit_platform.c` | The answers for AROS |

The checks are in `developer/debug/test/m68kjit_host` (on a host) and
`developer/debug/test/m68kjit` (a native probe).

## What it covers

Version 1.0 is for one processor, in supervisor mode with the interrupt
stack, the mode `m68kemu.library` gives every CPU. It has no FPU, no MMU
and no interrupts of its own. An instruction outside what it translates
ends the run before anything of it is done, and the host executes it.

The engine declines a division whose divisor it would have to read through
the host (a divisor in a range the host asked to see, in whole or in part,
or outside guest memory): DIVU and DIVS, word and long, with the divisor at
`(An)`, `(An)+`, `-(An)`, `d16(An)`, `d8(An,Xn)`, an absolute long address
or `d16(PC)`. The host executes such a division.

Code that a program stores into memory it already ran, without a cache
call (`CacheClearU()`, `CacheClearE()`, `CacheControl()`, CINV, CPUSH), is
not seen: the engine behaves as a 68040 with its instruction cache on.

## Memory

A domain (one for each program) starts with about 1.5 MiB: 1 MiB of code
memory, a table for 2048 blocks (112 KiB), 256 KiB for the guest bytes of
the translated blocks, and about 105 KiB of fixed buffers. When the code
memory, the block table or the guest bytes are full, all three double and
the translations made so far are dropped (a code reset). They double four
times at most, to 16 MiB of code, 32768 blocks and 4 MiB of guest bytes:
about 22 MiB. While they double, the old and the new ones are held
together. They double only when at least half of the blocks are still in
use: a program that changes a little code again and again fills them with
translations nothing finds any more, and for it they are only emptied. If
there is no memory for the next size, the engine also keeps the size it
has and only drops the translations.

The engine allocates and frees this memory inside `run()`. A host calls
`run()` from a task, as `m68kemu.library` does under its program lock, not
from an interrupt.

A CPU takes about 200 bytes and one byte for each 4 KiB page of guest
memory.

## Checks

- **On a host.** `developer/debug/test/m68kjit_host` compiles the two
  engine files with test seams and runs 28 groups of checks through the
  interface (and, from `m68kjit_intern.h`, the table of operations, the seams
  and the self test of the encoder). On an AArch64 Mac (Apple clang 21.0.0): 71636 checks, 0
  failed, at `-O1` with the address and undefined behaviour sanitizers and
  at `-O2`.
- **Native probe.** `developer/debug/test/m68kjit/m68kjitprobe` opens
  the library and runs small 68k programs through the interface, without
  `m68kemu.library`: 22 checks.
- **With m68kemu.library.** See `rom/m68kemu/INTERNALS.md`, The Optional
  Engine.

Not covered: more than one processor, physical hardware, applications, and
any measurement of speed beyond one loop.
