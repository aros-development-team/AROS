# lstat final-component lookup

Run `python3 developer/debug/test/posix-lstat/test.py --output /new/output --before-ref PARENT_COMMIT`.
The fixture compiles the actual lstat and lstat64 entry points and shared helper under ASan/UBSan for both classic and fast BSTR configurations. It expects the preceding Lock-based implementation to fail and the proposed implementation to pass.

DOS parent resolution, locate packets and fallback enumeration are modeled. Tests cover valid/dangling links, a soft-linked parent, raw leaf bytes, volume roots, relative entries, absent entries/parents, allocation failures, preserved errors and balanced locks/allocations. These tests do not qualify real handler metadata or task scheduling. Deploy the matching POSIX resident and run the native Rust/C metadata probe separately.
