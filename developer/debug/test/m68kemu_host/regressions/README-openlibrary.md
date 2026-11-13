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


## First-load version checks with Moira

Initialize the same submodule used by the full host suite, then run:

```sh
git submodule update --init rom/m68kemu/Moira
make -C developer/debug/test/m68kemu_host test-library-loading-versions
make -C developer/debug/test/m68kemu_host test-library-loading-versions-sanitize
```

The test compiles the production loader, InitStruct interpreter and memory
accessors with recording DOS/heap/hunk-placement collaborators. The loader
itself generates the init/Open sequences; the checked-out Moira executes
those actual bytes, the fixture's 68k init function and its Open vector.
Native file I/O and hunk parsing are not tested by these collaborators.
The CPU is an M68000 and this is a host instruction test, not an AROS VM.

Cases cover no init function, InitStruct version overrides, init raising
or lowering the version, failed init, unsigned 16-bit boundaries, A6/stack
restoration and reopening an initialized library after an excessive first
request was rejected. Rejection occurs before calling Open, after init has
established the actual version. A full-width internal request above 65535
is refused before native file or heap activity, without truncation.
The entry-point fixture also checks version rejection on native-provided
libraries, including zero and high-bit version values.

`LIBRARY_LOADING_ARGS` forwards options such as `--build-dir /tmp/loading`.
`--source-root` selects a control tree and `--moira-root` can provide its
matching initialized core separately. The ordinary CPU regression is also
a prerequisite of the normal host `test` target. These checks remain
separate from a full linked library or application acceptance run.
