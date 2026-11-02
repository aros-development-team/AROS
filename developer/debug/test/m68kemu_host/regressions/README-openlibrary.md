# OpenLibrary version regression

Run from the repository root:

```sh
make -C developer/debug/test/m68kemu_host test-openlibrary-versions
make -C developer/debug/test/m68kemu_host test-openlibrary-versions-sanitize
```

The ordinary target is also a prerequisite of the host Makefile's `test`
target. Set `CXX` and `PYTHON` to select the tools. `OPENLIBRARY_ARGS`
passes runner options such as `--build-dir /tmp/openlibrary`.

The runner compiles the production lookup helper and classic entry thunks
verbatim with recording native/class/disk-library collaborators. It reads
the library-version field offset from the production header. The fixture
uses 32-bit guest registers and tests dirty D0 high bits, cached version
failure, normalized reopen arguments, OldOpenLibrary's independence from
D0, NULL/native fast paths, unchanged CPU registers and separate task tail
states. The two task states are fixture collaborators, not running AROS
tasks or a concurrency test.

The same fixture can test an unmodified source tree:

```sh
python3 developer/debug/test/m68kemu_host/regressions/run_openlibrary_versions.py \
  --source-root /path/to/unmodified/AROS
```

These function-level checks need Python 3.9+ and a C++17 compiler. They do
not require an SDK, Moira checkout or VM and do not establish a complete
host-suite, linked library, AROS runtime or application-level pass.
The optional sanitizer target uses ASan/UBSan; leak detection is disabled
by default and can be configured with `ASAN_OPTIONS`.
