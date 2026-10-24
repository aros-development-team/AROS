# lstat final-component lookup

Run `python3 developer/debug/test/posix-lstat/test.py --output /new/output --before-ref PARENT_COMMIT`.
The fixture compiles the actual lstat and lstat64 entry points and shared helper under ASan/UBSan for both classic and fast BSTR configurations. It expects the preceding Lock-based implementation to fail and the proposed implementation to pass.

DOS parent resolution, locate packets and fallback enumeration are modeled. Tests cover valid/dangling links, a soft-linked parent, raw leaf bytes, volume roots, relative entries, absent entries/parents, allocation failures, preserved errors and balanced locks/allocations. These tests do not qualify real handler metadata or task scheduling. Deploy the matching POSIX resident and run the native Rust/C metadata probe separately.

## Direct native C probe

Build with an already installed matching AArch64 AROS SDK; keep its inputs frozen:

```sh
"$AROS_RUST_SDK/bin/aarch64-aros-cc" -std=gnu11 -O2 -Wall -Wextra -Werror \
  developer/debug/test/posix-lstat/native-probe.c -o /new/output/LstatProbe
```

Run on the target with an unused native RAM directory name:

```text
LstatProbe RAM:LstatDirectUniqueName
```

The probe refuses an existing root. It creates and removes its own files, valid/dangling soft links and directory, checks both `lstat` and `lstat64`, contrasts `stat`/`stat64`, preserves the referent contents and tests a raw Latin-1 link leaf. A parent-link check is conditional on handler support. Require return zero and `[LSTAT] result=PASS checks=57 failures=0 scope=native-lstat32-lstat64` together with `parent-link result=PASS` for the complete exercised case set. A handler without parent-link support reports that limit explicitly.

For the retained qualification, the compiler wrapper, C SDK headers/libraries and std generation were frozen at SDK generation `7eabce7f51176bd4cfe2aacaf88bdac024838cb3436e34850d3c4f138fa31d11`. The POSIX resident was replaced by the exact corrected generic AArch64 SMP module. Record source, wrapper/manifest, binary and resident module hashes before each run; no SDK rebuild is required by this test-only addition. The native C probe passed 57 checks in a compatible AArch64 SMP emulator host, including parent links, with command and emulator return zero. This does not qualify a complete public kernel image or physical hardware.
