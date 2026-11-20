# Graphics TextLength and TextExtent host regression

Copyright (C) 2026, The AROS Development Team. All rights reserved.

This fixture compiles the complete `TextLength()` and `TextExtent()` definitions
extracted from `rom/graphics/textlength.c` and `rom/graphics/textextent.c`.
Compiler and sanitizer diagnostics retain the production source locations, and
`FS_*`, `FSF_*`, `FPF_*`, and `NUMCHARS` macros are read from the graphics
headers and source.

From the repository root, run:

```
make -C developer/debug/test/graphics_text_host
make -C developer/debug/test/graphics_text_host test-sanitize
```

Only Python 3 and a C99 compiler are required. `AROS_ROOT`, `BUILD_DIR`, `CC`,
and `PYTHON` can be overridden; no AROS SDK is used. The sanitizer target
enables ASan and UBSan.

The scenarios exercise hand-calculated font metrics across:

* Fonts with both `tf_CharKern` and `tf_CharSpace`, with `tf_CharKern` only,
  with `tf_CharSpace` only, and with neither table (both fixed-width and
  `FPF_PROPORTIONAL`).
* Characters outside `tf_LoChar..tf_HiChar` (below `tf_LoChar`, above
  `tf_HiChar`, and `0xFF`) selecting the default glyph at `NUMCHARS(tf) - 1`.
* Negative `TxSpacing` (both smaller and larger in magnitude than the character
  advance), glyphs wider than `tf_XSize` or `tf_CharSpace`, and `FSF_BOLD` and
  `FSF_ITALIC` algorithmic style adjustments.
* `count == 0` and negative `count` values without reading past the input
  buffer, extents exceeding `32767` and `-32768` (including `QUAD` sums beyond
  32-bit `LONG`) clamping at `32767` and `-32768`, `te_Width` matching
  `TextLength()`, and unchanged `te_Height`, `MinY`, and `MaxY`.
