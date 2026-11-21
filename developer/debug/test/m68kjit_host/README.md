# m68kjit: host checks of the engine

These checks compile the engine of `workbench/libs/m68kjit`
(`m68kjit_engine.c`, `m68kjit_translate.c`) for the host, with its test seams
(`M68KJIT_TEST_SEAMS`), and exercise it through the interface of
`rom/m68kemu/m68kemu_engine.h`. From `m68kjit_intern.h` they take only the
table of operations, the test seams and the self test of the encoder. The
host must be AArch64 (macOS or Linux): the translator emits AArch64 code
and the checks run it. `m68kjit_platform_host.c` is the host's side of
`m68kjit_platform.h`: memory, executable memory, its write window and cache
flush.

## Running

```
make check
```

builds the checks twice, at `-O1` with the address and undefined behaviour
sanitizers and at `-O2`, both with `-std=c99 -Wall -Wextra -Werror`, and
runs both. Each run prints a line per group and a summary, and returns 0
when no check failed.

`include/exec/types.h` stands in for the AROS header of that name on the
host.

## What is checked

26 groups. A stub host owns the guest memory (1 MiB in most groups, 16 or
32 MiB where a check needs it), answers the accesses
the engine sends it as `m68kemu.library` does for its ranges (CIA and
custom chip pages, the low-memory guard, the bitplane mirror, reads outside
memory as 0), and dispatches the few library calls the test programs make.

| Group | What it checks |
| --- | --- |
| `discovery`, `size-and-version` | the table of operations, and every size and version check of the interface |
| `lifecycle-and-leaks`, `allocation-failure` | creation and destruction in every order the interface allows, with every allocation made to fail once, and nothing left allocated |
| `registers` | reading and writing every register, and the values the status register may hold |
| `program`, `a-line`, `budget`, `block-limits` | 68k programs with known results (a checksum, a sort); the exits at A-line opcodes; slices and their instruction counts |
| `sentinels-and-fetch`, `faults`, `exceptions-and-unsupported` | a PC outside memory, an odd PC, and the instructions the engine reports without executing them |
| `memory-routing`, `custom-page`, `endian`, `map-change-fetch` | which accesses go to the host and which do not, at every width and alignment, and a changed map |
| `invalidation`, `code-changes`, `code-reset` | changed code after `invalidate`, and more blocks than the engine's code memory keeps, with a nested call on a second CPU |
| `word-division`, `zero-divisor-state`, `div-host-read` | divisions, a zero divisor, and a divisor the engine would have to read through the host |
| `dbcc-targets`, `full-index`, `encoder`, `output-buffers-and-counters` | DBcc and its branch targets, the full index addressing modes, the self test of the AArch64 encoder, the exit and counter buffers |

## Result

On an AArch64 Mac with Apple clang 21.0.0, 21 November 2026: 40861 checks,
0 failed, in each of the two builds.

Not shown by these checks: a build without the test seams, another
compiler, the engine on AROS (the native probe in
`developer/debug/test/m68kjit` does that), and `m68kemu.library`
using the engine.
