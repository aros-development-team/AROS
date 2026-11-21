# kaffeine-jit68.library

An optional 68k execution engine for `m68kemu.library`. It translates 68k
code to AArch64 code and runs it. `m68kemu.library` uses it when the
variable `M68KEMU_CPU` asks for it and works without it: the interpreter
(Moira) stays the default and the reference.

The library is built for AArch64 targets only, because the translator emits
AArch64 code.

## What it is

The interface between the two libraries is version 1.0 of
[`kaffeine-jit68-abi-v1.h`](kaffeine-jit68-abi-v1.h), specified in
[`kaffeine-jit68-abi-v1.md`](kaffeine-jit68-abi-v1.md). The library has one
function, `KJ68_GetEngine()` at LVO -30, which returns a table of nine
operations.

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
| `kaffeine-jit68-abi-v1.h` | The interface: every name the host and the engine share |
| `kaffeine-jit68-abi-v1.md` | Its specification; where a comment in the header is shorter, the specification governs |
| `kaffeine-jit68.conf` | The library: one function |
| `kj68_library.c` | That function, and two C library functions the translator calls that a library module does not get |
| `kj68_engine.c` | The nine operations, the memory map and the counters |
| `kj68_translate.c` | The AArch64 translator |
| `kj68_intern.h` | The private interface between the two engine files, and the test seams |

The checks are in `developer/debug/test/kaffeine_jit68_host` (on a host) and
`developer/debug/test/kaffeine-jit68` (a native probe).

## Author and origin

The library, its interface and its checks were written by Michele Dipace
(@kaffeine1) in the Kaffeine project (https://github.com/kaffeine1/kaffeine).

The translator is the one of `kaffeine.library`, the earlier library of
that project, adapted to this interface: guest memory through the host's
map, exits to the host where that library dispatched calls itself,
translations shared by the CPUs of a program. A script in that repository
generates `kj68_translate.c` from that library's translator and records
which definitions it keeps, adapts, adds and leaves out. It contains no
Emu68 code.

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

## Checks

- **On a host.** `developer/debug/test/kaffeine_jit68_host` compiles the two
  engine files with test seams and runs 26 groups of checks through the
  interface only. On an AArch64 Mac (Apple clang 21.0.0): 40861 checks, 0
  failed, at `-O1` with the address and undefined behaviour sanitizers and
  at `-O2`.
- **Native probe.** `developer/debug/test/kaffeine-jit68/kj68probe` opens
  the library and runs small 68k programs through the interface, without
  `m68kemu.library`: 20 checks.
- **With m68kemu.library.** See `rom/m68kemu/INTERNALS.md`, The Optional
  Engine.

Not covered: more than one processor, physical hardware, applications, and
any measurement of speed beyond one loop.
