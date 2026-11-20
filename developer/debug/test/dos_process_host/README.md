# DOS process-entry host regression

Copyright (C) 2026, The AROS Development Team. All rights reserved.

This fixture compiles the complete `DosEntry()` and `streamSane()` definitions
extracted from `rom/dos/createnewproc.c`. It does not maintain an alternate
implementation of their dispatch or cleanup. Compiler and sanitizer diagnostics
retain the production source locations. DOS constants and process flags are
read from the public headers.

From the repository root, run:

```
make -C developer/debug/test/dos_process_host
make -C developer/debug/test/dos_process_host test-sanitize
```

Only Python 3 and a C99 compiler are required. `AROS_ROOT`, `BUILD_DIR`, `CC`,
and `PYTHON` can be overridden; no AROS SDK is used. The sanitizer target
enables ASan/UBSan. LeakSanitizer is disabled on Darwin, where the toolchain
does not support it. Cleanup-call assertions still verify exact release
ownership.

The scenarios exercise:

* A recognized 68k module (`GSLI_68KHUNK`) when `m68kemu.library` cannot be
  opened: never call the native entry, report `RETURN_FAIL` to `pr_ExitCode`,
  set child `IoErr` to `ERROR_INVALID_RESIDENT_LIBRARY`, and run the normal
  cleanup after `skip_native_entry`.
* Successful `m68kemu.library` opening: call `RunHunk` once, retain arguments
  and stack size, pass through results -1, 0, 7 and 20, balance `OpenLibrary`
  and `CloseLibrary`, and avoid native fallback even when `RunHunk` returns -1.
* Ordinary native entry, untracked module and no-module native entry, including
  unchanged argument, result and error behavior.
* Owned versus borrowed startup resources, no exit callback, derived entry
  address, saved startup streams, CLI disposal, synchronous completion and
  death notification order. A cleanup collaborator deliberately changes `IoErr`
  to verify that the failure reaches `pr_ExitCode` before cleanup.

The native APIs are controlled collaborators. An erroneous `CallEntry` is
recorded without jumping into fake 68k bytes; the unfixed production source
therefore fails assertions safely rather than crashing the host. This is
control-flow and cleanup evidence, not proof of native ABI, native scheduling,
or application startup.

`DosEntry` already runs inside the created child. This fix cannot retroactively
make the parent's `CreateNewProc` return `NULL`. Its result goes to the
existing exit callback; no new promise is made about the dying process's
`IoErr` after later cleanup calls, which may change it.
