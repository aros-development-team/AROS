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
