<!--
Copyright (C) 2026, The AROS Development Team. All rights reserved.
-->

# Native palette-screen regression

Build with the normal AROS mmake target `test-hidd-gfx-palette`, or through
`test` / `test-quick`. The executable is installed as
`Tests/hidds/gfx/gfx_palette`. Run it from an AROS Shell on an otherwise idle
graphics display. It opens and closes custom screens; save work in other
applications before running it.

The test uses the public screen's display and searches its ordinary graphics
mode database for a matching LUT8 mode. On a truecolor-only driver the generic
implementation must expose one; lack of LUT8 enumeration is a failure. An
existing native LUT8 screen is distinguished by its `Displayable` attribute.
The test prints which path actually ran.

On a generic software LUT8 screen it checks that a single screen covering the
whole display still has a non-NULL `ComposedBitMap`. It reads ARGB32 pixels from
that actual target, then changes the palette through `SetRGB32` and
`LoadRGB32`, without drawing again. The target must converge within twelve
`UNIT_VBLANK` timer waits. `ComposedBitMap` is a read-only observation; the test
never requests a flush or calls `RethinkDisplay` during these checks.

A temporary sixteen-entry HIDD colormap makes growth past entry 16 observable
rather than depending on how Intuition initialized the screen palette. It is
attached with the public `SetColorMap` method, grown to pen 200, changed by a
256-pen `SetRGB32` burst, then detached before disposal. The test separately
checks raw `Native32` source pixels, so a conversion that overwrites pen
indices cannot pass by only producing the right displayed color.

Two stacked screens use the same pen index with different palettes. Dragging
the front screen exposes the back screen; changing either palette must update
only its corresponding output. If the display exposes native indexed scanout,
a covering screen must preserve passthrough. If that hardware also composes a
dragged indexed stack, the same output checks run; hardware that puts the
stack on separate planes reports that case as skipped. Displays without a
HIDD or without relevant palette/composition support also report explicit
skips. A zero return code is therefore accompanied by check and skip counts;
it does not establish that every hardware path ran.

Finally, the test closes a screen immediately after palette invalidation and
waits three VBlanks. This exercises ordinary pending-work cleanup. Native
output reads prove the running OS path only: there is no host-method extraction
or simulated compositor in this test.

The 256-pen burst checks output convergence, not an exact redraw-count bound.
Per-frame redraw-count instrumentation, compositor/display object disposal,
and other architectures remain separate validation. Deterministic redraw-region
allocation failure has the optional fault-build test described below. Run this test with both baseline and changed gfx modules, recording
the actual module version, executable hash, Shell return code, and output.

## Optional allocation-failure retry test

Build `gfx.hidd` with the existing test flag:

```
make kernel-hidd-gfx COMPOSITOR_FAULT_INJECTION=yes
```

Install that test module in the target system, then run separately:

```
Tests/hidds/gfx/gfx_palette NOMEM
```

Run this case exclusively: no concurrent `compflicker NOMEM`, other
`gfx_palette NOMEM`, or other user of `compositor.fail.redrawregion` is allowed.
The test checks the name is unused before adding its own marker, verifies the
returned node identity, and removes only its own node. The ordinary
`Forbid`/`FindSemaphore`/`AddSemaphore` check-and-add is atomic on UP AROS;
`Forbid` is CPU-local on SMP, so this is not an SMP admission guarantee.
An existing marker causes a failure without removing the existing node.

With one covering software LUT8 screen already displaying green, `NOMEM`
changes only its sample pen to blue while the fault marker is present. The
composed target must remain green during twelve VBlank waits. After the test
removes its marker, blue must appear within the ordinary twelve-frame bound,
without another setter, draw, or display-stack change. This exercises retention
and retry of pending palette invalidation, and preserves source pen indices.
The helper removes its marker before returning, including on timer failure.

Normal execution does not need fault injection. `NOMEM` is a required test,
not a skip: it fails if the software screen path cannot run, if an ordinary
uninstrumented module displays the new blue pixel while the marker is present,
or if the pending update is lost after allocation becomes available again.
Keep the normal and `NOMEM` outputs as separate target results.
