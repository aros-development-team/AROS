# Raspberry framebuffer regression tests

The AArch64 Raspberry framebuffer HIDD offers a software LUT8 mode alongside
its original truecolor mode. The hardware remains a 32-bit RGBX surface.
Indexed bitmaps keep one byte per source pixel and a 256-entry RGB palette;
refresh expands those indices into the framebuffer. Palette changes repaint
the visible bitmap, and showing or moving a bitmap clears uncovered areas.

## Host tests

Python 3.9+ and a C++17 compiler suffice; no AROS SDK is needed:

```sh
make -C developer/debug/test/fbgfx_host
make -C developer/debug/test/fbgfx_host test-sanitize
```

`make test` extracts the current production constructor/disposal, bitmap Set,
SetColors, UpdateRect, display Show, hardware-init, clear and refresh bodies.
Only object-system, kernel-attribute, allocation and semaphore collaborators
are fixtures. The current portable refresh header is included directly.
Source line directives preserve useful compiler and sanitizer diagnostics.

The method fixture checks constructor failure cleanup, palette ranges and
null input, hidden-screen isolation, prepainted Show, positive and negative
scroll offsets, cleared uncovered bands, partial-update preservation,
hardware-geometry refusal before console detachment or clearing, canaries
and balanced recursive framebuffer acquisition. It passes 283 checks.

The independent pixel oracle visits source pixels individually rather than
repeating the production clipping algorithm. It checks LUT8 expansion and
four-byte copying, extreme offsets, strides, source/destination guards,
overlap refusal, palette changes and 32-bit backing-size boundaries. It
passes 11,078 checks, including all 11,042 original clipping/guard checks.
Both executables pass normally and with ASan/UBSan. Removing transition
clearing in a separate source copy causes nine method assertions to fail.

Select a compiler or retain generated sources and executables with:

```sh
make -C developer/debug/test/fbgfx_host CXX=clang++ \
    TEST_ARGS='--build-dir /tmp/fbgfx-host'
```

The fixture records semaphore behavior; it does not establish scheduler
concurrency, target ABI, VideoCore behavior or native CopyBox/Scale coverage.
Caller-supplied ChunkyBM buffers still have to satisfy the HIDD's backing
storage contract. Geometry validation establishes the reported span and
arithmetic limits, not the size of an arbitrary caller allocation.

## Native AROS ARM64 probe

`fbgfx_palette.c` uses native Exec, graphics, Intuition and kernel.resource.
It has no dependency on an emulated application or CPU library. With an
AROS AArch64 SDK configured, build it using:

```sh
make -C developer/debug/test/fbgfx_host native \
    NATIVE_CC=aarch64-aros-gcc BUILD_DIR=/tmp/fbgfx-arm64 \
    NATIVE_CPPFLAGS='-I/path/to/AROS/Developer/include' \
    NATIVE_LDFLAGS='-L/path/to/AROS/Developer/lib'
```

Copy the executable to an AROS system using the Raspberry RGBX framebuffer
driver. From Shell, give it sufficient stack and run it:

```text
Stack 1048576
fbgfx_palette
Run fbgfx_palette
```

The probe opens temporary truecolor and indexed screens and closes them.
It reads the kernel-reported framebuffer bytes, independently of ReadPixel:
truecolor rendering, hidden indexed updates, prepainted Show, entries 1,
2, 3 and 255, live recoloring with unchanged indices, unaffected palette
entries, and restoration of the original truecolor screen. The updated
driver passes 23 checks in each Shell and Run invocation on AROS ARM64 in
`raspi3b` QEMU. The same probe with the unmodified mainline driver reports
the missing indexed mode (3 checks, 1 failed, return code 10) in both modes.

Only the driver module was replaced in an existing AROS test image; this
does not establish a complete current-mainline image build. Host results
were recorded on macOS ARM64. Physical Raspberry Pi hardware and 32-bit or
big-endian AROS execution were not tested.
