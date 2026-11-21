# Intuition ScreenPosition host regression

Copyright (C) 2026, The AROS Development Team. All rights reserved.

This fixture compiles the complete `ScreenPosition()` and `MoveScreen()`
definitions extracted from `rom/intuition/screenposition.c` and
`rom/intuition/movescreen.c`. Compiler and sanitizer diagnostics retain the
production source locations, and `SPOS_*` and `SF_Draggable` constants are read
from the current Intuition headers.

From the repository root, run:

```
make -C developer/debug/test/intuition_screenposition_host
make -C developer/debug/test/intuition_screenposition_host test-sanitize
```

Only Python 3 and a C11 compiler are required. `AROS_ROOT`, `BUILD_DIR`, `CC`,
and `PYTHON` can be overridden; no AROS SDK is used. The sanitizer target
enables ASan and UBSan.

The scenarios exercise:

* `SPOS_MAKEVISIBLE` on already-visible rectangles (including classic editor
  caret boxes on a non-oversized screen, full-display rectangles, and interior
  or boundary-touching rectangles on scrolled oversized screens), verifying that
  the screen does not shift.
* `SPOS_MAKEVISIBLE` off each edge (left, right, top, bottom, and diagonal
  corners) and oversized rectangles wider or taller than the visible display
  clip, plus reversed rectangles (`x2 < x1`, `y2 < y1`) and extreme `LONG`
  coordinates (`INT32_MIN`, `INT32_MAX`).
* `SPOS_ABSOLUTE`, `SPOS_RELATIVE`, and `MoveScreen()` semantics: preserve
  established driver behavior by forwarding target offsets directly to
  `ScrollVPort()` without pre-clamping in `ScreenPosition()`, handle draggable
  versus non-draggable screens with and without `SPOS_FORCEDRAG`, and reject
  mutually exclusive `SPOS_ABSOLUTE | SPOS_MAKEVISIBLE` flags.
* `SPOS_MAKEVISIBLE` desktop and screen bounding (`[min_x, max_x]` and
  `[min_y, max_y]`: no further than the position that shows the far edge of
  the screen, and no further back than 0 or the present position, while
  preserving existing driver-approved positions outside those minima and
  positive placements when the requested rectangle is already visible),
  `ViewPortExtra` `DisplayClip`
  resolution via `GfxLookUp()` and `ColorMap->cm_vpe` (using the normalized
  `Max - Min + 1` visible span as `OpenScreen()`, `MakeScreen()`,
  `gfx_displayclass`, and `inputhandler.c` do), and `ScrollVPort()` offset
  feedback.
* `NULL` screen safety and `LockIBase(0)` / `UnlockIBase()` ownership,
  including nested calls when the caller already holds `IBaseLock`.

## Native AROS regression

The companion program in `developer/debug/test/intuition/screenposition`
uses public Intuition and Graphics APIs. From a configured AROS build
directory, build `test-intuition-screenposition`; when the SDK is already
built, `test-intuition-screenposition-quick` builds just this test. Run
`SYS:Developer/Debug/Tests/intuition/screenposition/screenposition` on AROS.

The program opens temporary custom screens behind Workbench, uses the
actual display mode and visible screen intersection for no-op checks, and
compares scrolling with driver-resolved absolute positions. It reports
checks, failures and skipped geometry capabilities separately, and exits
with code 20 on failure. A successful host test or cross-build does not
establish native execution or editor acceptance.
