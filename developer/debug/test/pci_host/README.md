# sizePCIBaseReg() host behaviour test

Run from the repository root:

```sh
make -C developer/debug/test/pci_host test
```

`run_sizebar.py` extracts `sizePCIBaseReg()` from
`<source-root>/rom/hidds/pci/pciutil.c` verbatim — signature line to the
matching closing brace, and it fails loudly if it cannot find exactly one —
and generates one translation unit: the real
`rom/hidds/pci/include/hardware/pci.h` (included, not copied), `fixture.h`,
the extracted function, `test_sizebar.c`. It compiles and runs that at
`-m64` and `-m32`. The behaviour table answers for both widths; a 32-bit
size may differ from the 64-bit one only where it does not fit in 32 bits,
and the runner compares the two tables by that rule (differential). Any
failure makes the run exit non-zero. A `-m32` width this host cannot build
(no multilib compiler) is reported `SKIPPED` and still fails the run;
`--allow-skip-m32` accepts it only where that is intended. No silent pass.

Requirements: Python 3, and a host C compiler; `-m32` additionally wants a
multilib build of it with a 32-bit C library (checked by linking a probe
program; reported as `SKIPPED` otherwise).
`CC`, `PYTHON` and `SIZEBAR_ARGS` override as in the m68kemu host test Makefile;
`--source-root` tests another AROS source tree (the behaviour table then runs
that tree's copy of the function). The function-level check needs no AROS
SDK, NDK or target build.

The fixture supplies only what the function uses: the exec scalar types
(`ULONG` 32-bit at every width, `IPTR` the pointer width), opaque
`OOP_Object`/`struct pci_staticdata`, a `struct DeviceData` with just
`isBridge`, `Class` and `SubClass`, the four `HIDD_PCIDriver_Read/WriteConfig{Word,Long}`
macros routed to a scripted config space, and `__WORDSIZE` derived from
`__SIZEOF_POINTER__` when the host headers lack it. It never copies
production code or register constants — the BAR values, size masks and
expected answers in `test_sizebar.c` are all test inputs. The bridge and
host-bridge rows exercise `DeviceData` bookkeeping, not PCI wiring; nothing
here constitutes a driver acceptance test or an AROS boot.
