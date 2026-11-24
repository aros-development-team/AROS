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
when no check failed. Then it builds and runs the comparison with Moira
(below), for two million instructions; `make check COMPARE=n` sets another
number.

`include/exec/types.h` stands in for the AROS header of that name on the
host.

## What is checked

28 groups. A stub host owns the guest memory (1 MiB in most groups, 16 or
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
| `growth` | the code memory, the block table and the guest bytes doubling up to their largest sizes and no further, with each allocation of the next size made to fail once; no growth for a program that only translates the same code again |
| `word-division`, `long-division`, `zero-divisor-state`, `div-host-read` | the word and the long divisions against an oracle worked out in the checks, their overflows among them; a zero divisor; and a divisor the engine would have to read through the host |
| `dbcc-targets`, `full-index`, `encoder`, `output-buffers-and-counters` | DBcc and its branch targets, the full index addressing modes, the self test of the AArch64 encoder, the exit and counter buffers |

## The comparison with Moira

`m68kjit_moira_compare` runs one instruction at a time on the engine and on
Moira, the interpreter of `m68kemu.library`, set up as that library sets it
up (68LC040), and compares D0-D7, A0-A7, the PC, the status register and
every byte of memory. Each case has a random opcode with random extension
words, random registers and flags, and random memory. Half of the opcodes
are drawn from all 65536 words, half from the groups whose results are
easy to get wrong: shifts and rotates, bit fields, ADDX, SUBX and NEGX, CAS
and CAS2, the long and the word multiplications and divisions, MOVEP, the
decimal instructions, the bit instructions, MOVEM, CHK and CMP2, Scc and
DBcc.

- Where Moira completes the instruction, the engine either completes it
  with the same result or declines it with nothing changed, which the
  interface allows: the host then executes it.
- Where Moira takes an exception or stops, the engine must not have
  executed anything.
- **The divisions are the exception.** Moira, built as `m68kemu.library`
  builds it (`MOIRA_MIMIC_MUSASHI`), departs from the 68040 there: after
  DIVU.W and DIVS.W it sets Z when the quotient is not zero, on the
  overflow of any division it leaves C as it was, and it does not report
  the overflow of 0x80000000 / -1. The engine follows the manual. For the
  divisions the program works the expected flags out itself, from the
  quotient Moira gives; its source says how, and its output counts them.

`m68kjit_moira_compare [cases [seed [examples [names]]]]` prints the counts
and returns 0 when no case differs. It links Moira from
`rom/m68kemu/Moira`, so it needs that submodule.

## On a Mac: x18

The translator keeps a temporary in x18, an ordinary caller-saved register
in the AROS ABI. Darwin does not keep x18 across a context switch: on the
Mac of these runs a value left in x18 was found changed 2451 times in 8
seconds of a loop that was preempted. So on a Mac the Makefile builds the
translator from a copy with that one temporary in x23, a register the
trampolines save and the translator does not use. On Linux it builds the
file as it is. What a Mac cannot show, the native probe does: its
callbacks change x16, x17 and x18.

## Result

On an AArch64 Mac with Apple clang 21.0.0, 21 November 2026:

- the checks: 71636 checks, 0 failed, in each of the two builds;
- the comparison, two million cases with its own seed: 1196645 the same,
  325637 declined by the engine with nothing changed, 477558 exceptions or
  stops that the engine did not execute either, 160 not usable, 0 differ.
  50957 divisions had their flags from the manual;
- the comparison, ten million cases with each of the seeds 11 to 15: 0
  differ.

The comparison was first run on the engine as it was before it: it found
the missing overflow of DIVS.L and the C flag left set on the overflow of
a long division, about 500 cases in each million, and nothing else.

Not shown by these checks: a build without the test seams, another
compiler, the engine on AROS (the native probe in
`developer/debug/test/m68kjit` does that), and `m68kemu.library`
using the engine.
