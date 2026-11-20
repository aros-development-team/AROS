/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Compile the complete, unmodified ScreenPosition and MoveScreen bodies from
    rom/intuition/screenposition.c and rom/intuition/movescreen.c. The fixture
    substitutes controlled native collaborators (LockIBase, UnlockIBase,
    GfxLookUp, ScrollVPort) to verify mode dispatch, rectangle visibility,
    desktop/screen bounds, and lock ownership.
*/
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "production_constants.h"

typedef uint32_t ULONG;
typedef int32_t LONG;
typedef int16_t WORD;
typedef uint16_t UWORD;
typedef int64_t QUAD;
typedef uintptr_t IPTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define D(value) do { } while (0)
#define AROS_LIBFUNC_INIT
#define AROS_LIBFUNC_EXIT
#define AROS_LHA(type, name, reg) type name
#define AROS_LH3(ret, name, a1, a2, a3, bt, bn, lvo, lib) \
    static ret production_##name(a1, a2, a3, bt bn)
#define AROS_LH6(ret, name, a1, a2, a3, a4, a5, a6, bt, bn, lvo, lib) \
    static ret production_##name(a1, a2, a3, a4, a5, a6, bt bn)

struct Rectangle {
    WORD MinX, MinY, MaxX, MaxY;
};

struct ViewPortExtra {
    struct Rectangle DisplayClip;
};

struct ColorMap {
    struct ViewPortExtra *cm_vpe;
};

struct ViewPort {
    WORD DWidth, DHeight;
    WORD DxOffset, DyOffset;
    struct ColorMap *ColorMap;
};

struct Screen {
    WORD LeftEdge, TopEdge;
    WORD Width, Height;
    struct ViewPort ViewPort;
};

struct IntScreen {
    struct Screen Screen;
    UWORD SpecialFlags;
};

struct GfxBase {
    int identity;
};

struct IntuitionBase {
    int identity;
};

struct IntIntuitionBase {
    struct IntuitionBase IntuitionBase;
    struct GfxBase *GfxBase;
};

static unsigned checks, failures;
static const char *case_name;

static void check(BOOL ok, const char *description)
{
    ++checks;
    if (!ok)
    {
        ++failures;
        printf("FAIL [%s] %s\n", case_name, description);
    }
}

static struct {
    struct IntIntuitionBase ibase;
    struct GfxBase gfx_base;
    struct IntScreen screen;
    struct IntScreen null_screen;
    struct ColorMap colormap;
    struct ViewPortExtra lookup_vpe;
    struct ViewPortExtra cm_vpe;
    struct ViewPortExtra *lookup_result;
    unsigned initial_lock_depth;
    unsigned lock_depth;
    unsigned lock_calls;
    unsigned unlock_calls;
    unsigned lookup_calls;
    unsigned scroll_calls;
    unsigned priv_screen_calls;
    ULONG expected_lock_token;
    WORD last_requested_dx;
    WORD last_requested_dy;
    BOOL driver_adjust;
    WORD driver_min_dx, driver_max_dx;
    WORD driver_min_dy, driver_max_dy;
    char events[32];
    unsigned event_count;
} f;

static void event(char kind)
{
    if (f.event_count + 1 < sizeof(f.events))
    {
        f.events[f.event_count++] = kind;
        f.events[f.event_count] = '\0';
    }
}

#define SANITY_CHECK(x) if (!((IPTR)(x))) return;
#define GetPrivIBase(base) ((struct IntIntuitionBase *)(base))

static struct IntScreen *fixture_GetPrivScreen(struct Screen *scr)
{
    ++f.priv_screen_calls;
    check(scr == &f.screen.Screen, "GetPrivScreen receives the active Screen");
    check(f.lock_depth == f.initial_lock_depth + 1,
          "GetPrivScreen runs while IBaseLock is held");
    return scr ? (struct IntScreen *)scr : &f.null_screen;
}
#define GetPrivScreen(scr) fixture_GetPrivScreen(scr)

static ULONG fixture_LockIBase(ULONG lock_number, struct IntuitionBase *base)
{
    check(base == &f.ibase.IntuitionBase, "LockIBase uses IntuitionBase");
    check(lock_number == 0, "LockIBase locks IBaseLock (0)");
    ++f.lock_calls;
    ++f.lock_depth;
    event('L');
    f.expected_lock_token = 0x49424100UL + f.lock_calls;
    return f.expected_lock_token;
}
#define LockIBase(num) fixture_LockIBase((num), IntuitionBase)

static void fixture_UnlockIBase(ULONG ilock, struct IntuitionBase *base)
{
    check(base == &f.ibase.IntuitionBase, "UnlockIBase uses IntuitionBase");
    check(f.lock_depth == f.initial_lock_depth + 1,
          "UnlockIBase balances an active LockIBase");
    check(ilock == f.expected_lock_token,
          "UnlockIBase receives the exact token from LockIBase");
    ++f.unlock_calls;
    --f.lock_depth;
    event('U');
}
#define UnlockIBase(lock) fixture_UnlockIBase((lock), IntuitionBase)

static struct ViewPortExtra *fixture_GfxLookUp(struct ViewPort *vp, struct GfxBase *gfx)
{
    check(gfx == &f.gfx_base, "GfxLookUp uses GfxBase from IntuitionBase");
    check(vp == &f.screen.Screen.ViewPort, "GfxLookUp receives screen ViewPort");
    check(f.lock_depth == f.initial_lock_depth + 1,
          "GfxLookUp runs while IBaseLock is held");
    ++f.lookup_calls;
    return f.lookup_result;
}
#define GfxLookUp(vp) fixture_GfxLookUp((vp), GfxBase)

static void fixture_ScrollVPort(struct ViewPort *vp, struct GfxBase *gfx)
{
    check(gfx == &f.gfx_base, "ScrollVPort uses GfxBase from IntuitionBase");
    check(vp == &f.screen.Screen.ViewPort, "ScrollVPort receives screen ViewPort");
    check(f.lock_depth == f.initial_lock_depth + 1,
          "ScrollVPort runs while IBaseLock is held");
    ++f.scroll_calls;
    event('S');
    f.last_requested_dx = vp->DxOffset;
    f.last_requested_dy = vp->DyOffset;
    if (f.driver_adjust)
    {
        if (vp->DxOffset < f.driver_min_dx)
            vp->DxOffset = f.driver_min_dx;
        if (vp->DxOffset > f.driver_max_dx)
            vp->DxOffset = f.driver_max_dx;
        if (vp->DyOffset < f.driver_min_dy)
            vp->DyOffset = f.driver_min_dy;
        if (vp->DyOffset > f.driver_max_dy)
            vp->DyOffset = f.driver_max_dy;
    }
}
#define ScrollVPort(vp) fixture_ScrollVPort((vp), GfxBase)

#define ScreenPosition(scr, fl, x1, y1, x2, y2) \
    production_ScreenPosition((scr), (fl), (x1), (y1), (x2), (y2), IntuitionBase)

#include "production_screenposition.inc"

#undef ScreenPosition

static void reset_counters(void)
{
    f.lock_depth = f.initial_lock_depth;
    f.lock_calls = 0;
    f.unlock_calls = 0;
    f.lookup_calls = 0;
    f.scroll_calls = 0;
    f.priv_screen_calls = 0;
    f.expected_lock_token = 0;
    f.last_requested_dx = 0;
    f.last_requested_dy = 0;
    f.events[0] = '\0';
    f.event_count = 0;
}

static void prepare(const char *description, WORD width, WORD height,
                    WORD dwidth, WORD dheight, BOOL draggable)
{
    case_name = description;
    memset(&f, 0, sizeof(f));
    f.gfx_base.identity = 0x47465800;
    f.ibase.IntuitionBase.identity = 0x494E5400;
    f.ibase.GfxBase = &f.gfx_base;
    f.screen.Screen.Width = width;
    f.screen.Screen.Height = height;
    f.screen.Screen.ViewPort.DWidth = dwidth;
    f.screen.Screen.ViewPort.DHeight = dheight;
    f.screen.Screen.ViewPort.ColorMap = &f.colormap;
    f.lookup_vpe.DisplayClip.MinX = 0;
    f.lookup_vpe.DisplayClip.MinY = 0;
    f.lookup_vpe.DisplayClip.MaxX = (WORD)(dwidth - 1);
    f.lookup_vpe.DisplayClip.MaxY = (WORD)(dheight - 1);
    f.cm_vpe = f.lookup_vpe;
    f.colormap.cm_vpe = &f.cm_vpe;
    f.lookup_result = &f.lookup_vpe;
    f.screen.SpecialFlags = draggable ? SF_Draggable : 0;
}

static void call_pos(ULONG flags, LONG x1, LONG y1, LONG x2, LONG y2)
{
    reset_counters();
    production_ScreenPosition(&f.screen.Screen, flags, x1, y1, x2, y2,
                              &f.ibase.IntuitionBase);
    check(f.lock_calls == 1 && f.unlock_calls == 1 &&
          f.lock_depth == f.initial_lock_depth,
          "ScreenPosition balances LockIBase and UnlockIBase");
}

static void call_move(WORD dx, WORD dy)
{
    reset_counters();
    production_MoveScreen(&f.screen.Screen, dx, dy, &f.ibase.IntuitionBase);
    check(f.lock_calls == 1 && f.unlock_calls == 1 &&
          f.lock_depth == f.initial_lock_depth,
          "MoveScreen balances LockIBase and UnlockIBase");
}

static void expect_pos(WORD x, WORD y, const char *what)
{
    check(f.scroll_calls == 1 && !strcmp(f.events, "LSU"),
          "scrollable ScreenPosition call runs ScrollVPort under IBaseLock");
    check(f.screen.Screen.LeftEdge == x && f.screen.Screen.TopEdge == y &&
          f.screen.Screen.ViewPort.DxOffset == x &&
          f.screen.Screen.ViewPort.DyOffset == y,
          what);
}

static void test_makevisible_already_visible(void)
{
    /* Classic editor caret on a 1024x768 Workbench screen at (0, 0):
       repeated SPOS_MAKEVISIBLE calls for the visible caret must not shift
       the screen on open or on each typed character. */
    prepare("makevisible-already-visible-workbench", 1024, 768, 1024, 768, TRUE);
    for (LONG cx = 40; cx <= 120; cx += 8)
    {
        call_pos(SPOS_MAKEVISIBLE, cx, 82, cx + 8, 90);
        expect_pos(0, 0, "visible caret box does not shift a 1024x768 screen");
    }

    /* Native framebuffer probe rectangle (0, 22)-(width - 1, height - 1) and
       exact full-display rectangle (0, 0)-(width - 1, height - 1). */
    call_pos(SPOS_MAKEVISIBLE, 0, 22, 1023, 767);
    expect_pos(0, 0, "visible (0,22)-(1023,767) rectangle stays at origin");
    call_pos(SPOS_MAKEVISIBLE, 0, 0, 1023, 767);
    expect_pos(0, 0, "exact full-display rectangle stays at origin");
    call_pos(SPOS_MAKEVISIBLE, 0, 0, 0, 0);
    expect_pos(0, 0, "top-left corner pixel stays at origin");
    call_pos(SPOS_MAKEVISIBLE, 1023, 767, 1023, 767);
    expect_pos(0, 0, "bottom-right corner pixel stays at origin");

    /* Oversized 2048x1536 screen already scrolled to (-300, -200) on a
       1024x768 display: visible screen region is [300, 1323] x [200, 967]. */
    prepare("makevisible-already-visible-scrolled", 2048, 1536, 1024, 768, TRUE);
    call_pos(SPOS_ABSOLUTE, -300, -200, 0, 0);
    expect_pos(-300, -200, "initial scroll to (-300, -200)");
    call_pos(SPOS_MAKEVISIBLE, 450, 350, 900, 700);
    expect_pos(-300, -200, "interior rectangle on scrolled screen does not shift");
    call_pos(SPOS_MAKEVISIBLE, 300, 200, 1323, 967);
    expect_pos(-300, -200, "exact boundary rectangle on scrolled screen does not shift");

    /* Screen previously pulled down to TopEdge = 22: [0, 745] is still visible,
       while [0, 767] extends 22 pixels below the bottom of the display. */
    prepare("makevisible-pulled-down-screen", 1024, 768, 1024, 768, TRUE);
    call_pos(SPOS_ABSOLUTE, 0, 22, 0, 0);
    expect_pos(0, 22, "screen pulled down to TopEdge = 22");
    call_pos(SPOS_MAKEVISIBLE, 0, 10, 200, 745);
    expect_pos(0, 22, "visible rectangle on pulled-down screen stays at TopEdge = 22");
    call_pos(SPOS_MAKEVISIBLE, 0, 746, 200, 767);
    expect_pos(0, 0, "bottom-clipped rectangle scrolls pulled-down screen back to 0");
}

static void test_makevisible_each_edge(void)
{
    prepare("makevisible-each-edge", 2048, 1536, 1024, 768, TRUE);

    /* Right edge: x2 = 1200 -> new_x = 1024 - 1 - 1200 = -177 */
    call_pos(SPOS_MAKEVISIBLE, 900, 100, 1200, 200);
    expect_pos(-177, 0, "rectangle off right edge shifts minimally left");

    /* Left edge from (-177, 0): x1 = 50 -> new_x = -50 */
    call_pos(SPOS_MAKEVISIBLE, 50, 100, 300, 200);
    expect_pos(-50, 0, "rectangle off left edge shifts minimally right");

    /* Bottom edge from (-50, 0): y2 = 1000 -> new_y = 768 - 1 - 1000 = -233 */
    call_pos(SPOS_MAKEVISIBLE, 100, 700, 200, 1000);
    expect_pos(-50, -233, "rectangle off bottom edge shifts minimally up");

    /* Top edge from (-50, -233): y1 = 120 -> new_y = -120 */
    call_pos(SPOS_MAKEVISIBLE, 100, 120, 200, 300);
    expect_pos(-50, -120, "rectangle off top edge shifts minimally down");

    /* Simultaneous bottom-right shift from (-50, -120):
       x2 = 1400 -> new_x = 1023 - 1400 = -377
       y2 = 1100 -> new_y = 767 - 1100 = -333 */
    call_pos(SPOS_MAKEVISIBLE, 1100, 800, 1400, 1100);
    expect_pos(-377, -333, "bottom-right rectangle shifts both axes minimally");

    /* Simultaneous top-left shift from (-377, -333):
       x1 = 20 -> new_x = -20, y1 = 30 -> new_y = -30 */
    call_pos(SPOS_MAKEVISIBLE, 20, 30, 100, 120);
    expect_pos(-20, -30, "top-left rectangle shifts both axes minimally");

    /* Return to origin when making (0, 0)-(50, 50) visible. */
    call_pos(SPOS_MAKEVISIBLE, 0, 0, 50, 50);
    expect_pos(0, 0, "origin rectangle returns oversized screen to (0, 0)");
}

static void test_makevisible_oversized_and_degenerate(void)
{
    prepare("makevisible-oversized-rectangles", 2048, 1536, 1024, 768, TRUE);

    /* Rectangle wider than display (150..1500, width 1351 > 1024):
       aligns upper-left x1 = 150 with display left edge (new_x = -150). */
    call_pos(SPOS_MAKEVISIBLE, 150, 100, 1500, 300);
    expect_pos(-150, 0, "wider-than-display rectangle aligns x1 with left edge");

    /* Same wide rectangle when starting from (-900, 0) also aligns x1 = 150. */
    call_pos(SPOS_ABSOLUTE, -900, 0, 0, 0);
    expect_pos(-900, 0, "scroll to (-900, 0)");
    call_pos(SPOS_MAKEVISIBLE, 150, 100, 1500, 300);
    expect_pos(-150, 0, "wider-than-display rectangle from (-900, 0) aligns x1");

    /* Rectangle taller than display (220..1200, height 981 > 768):
       aligns upper-left y1 = 220 with display top edge (new_y = -220). */
    call_pos(SPOS_MAKEVISIBLE, 200, 220, 500, 1200);
    expect_pos(-150, -220, "taller-than-display rectangle aligns y1 with top edge");

    /* Both wider and taller than display: (180, 240)-(1600, 1300). */
    call_pos(SPOS_MAKEVISIBLE, 180, 240, 1600, 1300);
    expect_pos(-180, -240, "oversized rectangle in both axes aligns (x1, y1)");

    /* Oversized rectangle near bottom-right of screen where -x1 < min_x and
       -y1 < min_y: clamped to min_x = -1024, min_y = -768. */
    call_pos(SPOS_MAKEVISIBLE, 1500, 1200, 2800, 2200);
    expect_pos(-1024, -768, "oversized rectangle near far corner clamps to screen limits");

    /* Degenerate reversed rectangle (x2 < x1, y2 < y1): treated as (x1, y1)
       without shifting in the wrong direction. */
    call_pos(SPOS_ABSOLUTE, 0, 0, 0, 0);
    expect_pos(0, 0, "reset to origin");
    call_pos(SPOS_MAKEVISIBLE, 200, 200, 100, 100);
    expect_pos(0, 0, "visible reversed rectangle does not shift");
    call_pos(SPOS_MAKEVISIBLE, 1200, 900, 100, 100);
    expect_pos(-177, -133, "offscreen reversed rectangle shifts to bring (x1, y1) inside");

    /* Extreme 32-bit LONG values must not overflow signed arithmetic under UBSan. */
    call_pos(SPOS_MAKEVISIBLE, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX);
    expect_pos(0, 0, "INT32_MIN..INT32_MAX rectangle clamps without signed overflow");
    call_pos(SPOS_MAKEVISIBLE, INT32_MAX - 10, INT32_MAX - 10, INT32_MAX, INT32_MAX);
    expect_pos(-1024, -768, "near-INT32_MAX rectangle clamps to minimum offsets");
}

static void test_absolute_relative_and_flags(void)
{
    prepare("absolute-relative-and-flags", 2048, 1536, 1024, 768, TRUE);

    /* SPOS_ABSOLUTE ignores x2, y2 and leaves driver bounding to ScrollVPort. */
    call_pos(SPOS_ABSOLUTE, -300, -150, 9999, 9999);
    check(f.lookup_calls == 0, "SPOS_ABSOLUTE does not query ViewPortExtra");
    expect_pos(-300, -150, "SPOS_ABSOLUTE moves to (-300, -150) and ignores x2/y2");

    /* SPOS_RELATIVE adds delta to current LeftEdge, TopEdge and leaves driver
       bounding to ScrollVPort. */
    call_pos(SPOS_RELATIVE, -50, 30, 8888, 8888);
    check(f.lookup_calls == 0, "SPOS_RELATIVE does not query ViewPortExtra");
    expect_pos(-350, -120, "SPOS_RELATIVE moves by (-50, 30) and ignores x2/y2");

    /* MoveScreen delegates to ScreenPosition(screen, SPOS_RELATIVE, dx, dy, 0, 0). */
    call_move(100, -40);
    check(f.lookup_calls == 0, "MoveScreen does not query ViewPortExtra");
    expect_pos(-250, -160, "MoveScreen moves by (100, -40)");

    /* SPOS_ABSOLUTE and SPOS_RELATIVE pass arbitrary WORD positions to
       ScrollVPort without pre-clamping in ScreenPosition, preserving driver
       positioning semantics (see the screen drag in inputhandler.c). */
    call_pos(SPOS_ABSOLUTE, -1400, 900, 0, 0);
    expect_pos(-1400, 900, "SPOS_ABSOLUTE passes out-of-drag-range offsets to ScrollVPort");
    call_pos(SPOS_RELATIVE, -100, 50, 0, 0);
    expect_pos(-1500, 950, "SPOS_RELATIVE passes out-of-drag-range offsets to ScrollVPort");
    call_pos(SPOS_ABSOLUTE, 0, 0, 0, 0);
    expect_pos(0, 0, "restore screen to origin");
    call_pos(SPOS_RELATIVE, INT32_MIN, INT32_MAX, 0, 0);
    check(f.scroll_calls == 1, "extreme LONG relative delta does not trigger signed overflow");

    /* Non-draggable screen (SF_Draggable clear) ignores calls without SPOS_FORCEDRAG. */
    prepare("non-draggable-screen", 2048, 1536, 1024, 768, FALSE);
    call_pos(SPOS_ABSOLUTE, -200, -100, 0, 0);
    check(f.scroll_calls == 0 && !strcmp(f.events, "LU") &&
          f.screen.Screen.LeftEdge == 0 && f.screen.Screen.TopEdge == 0,
          "non-draggable screen ignores SPOS_ABSOLUTE without SPOS_FORCEDRAG");
    call_pos(SPOS_RELATIVE, -50, -50, 0, 0);
    check(f.scroll_calls == 0 && !strcmp(f.events, "LU") &&
          f.screen.Screen.LeftEdge == 0 && f.screen.Screen.TopEdge == 0,
          "non-draggable screen ignores SPOS_RELATIVE without SPOS_FORCEDRAG");
    call_pos(SPOS_MAKEVISIBLE, 1200, 900, 1300, 1000);
    check(f.scroll_calls == 0 && !strcmp(f.events, "LU") &&
          f.screen.Screen.LeftEdge == 0 && f.screen.Screen.TopEdge == 0,
          "non-draggable screen ignores SPOS_MAKEVISIBLE without SPOS_FORCEDRAG");
    call_move(-25, -25);
    check(f.scroll_calls == 0 && !strcmp(f.events, "LU") &&
          f.screen.Screen.LeftEdge == 0 && f.screen.Screen.TopEdge == 0,
          "non-draggable screen ignores MoveScreen");

    /* SPOS_FORCEDRAG overrides SF_Draggable == 0 across all three modes. */
    call_pos(SPOS_ABSOLUTE | SPOS_FORCEDRAG, -200, -100, 0, 0);
    expect_pos(-200, -100, "SPOS_ABSOLUTE | SPOS_FORCEDRAG moves non-draggable screen");
    call_pos(SPOS_RELATIVE | SPOS_FORCEDRAG, -50, -20, 0, 0);
    expect_pos(-250, -120, "SPOS_RELATIVE | SPOS_FORCEDRAG moves non-draggable screen");
    call_pos(SPOS_MAKEVISIBLE | SPOS_FORCEDRAG, 0, 0, 50, 50);
    expect_pos(0, 0, "SPOS_MAKEVISIBLE | SPOS_FORCEDRAG moves non-draggable screen");

    /* Mutually exclusive mode bits (SPOS_ABSOLUTE | SPOS_MAKEVISIBLE) are rejected. */
    call_pos(SPOS_ABSOLUTE | SPOS_MAKEVISIBLE | SPOS_FORCEDRAG, -300, -300, 0, 0);
    check(f.scroll_calls == 0 && !strcmp(f.events, "LU") &&
          f.screen.Screen.LeftEdge == 0 && f.screen.Screen.TopEdge == 0,
          "invalid combined SPOS_ABSOLUTE | SPOS_MAKEVISIBLE does not scroll");
}

static void test_desktop_and_screen_limits(void)
{
    /* Exact-fit 1024x768 screen on a 1024x768 display:
       MAKEVISIBLE horizontal and vertical bounds at origin are [0, 0], while
       ABS/REL preserve driver validation in ScrollVPort. */
    prepare("limits-exact-fit", 1024, 768, 1024, 768, TRUE);
    call_pos(SPOS_MAKEVISIBLE, -100, -50, -10, -5);
    expect_pos(0, 0, "negative MAKEVISIBLE coordinates never drag screen past (0, 0)");
    call_pos(SPOS_MAKEVISIBLE, 1200, 900, 1400, 1100);
    expect_pos(0, 0, "out-of-range MAKEVISIBLE on non-oversized screen stays at (0, 0)");

    /* Exact-size positive placement: if an exact-fit 1024x768 screen was
       positioned at positive LeftEdge/TopEdge (40, 22), MAKEVISIBLE keeps
       that positive placement for already-visible rectangles, shifts only the
       required delta when partially clipped, and returns to (0, 0) when the
       far corner (1023, 767) is requested. */
    call_pos(SPOS_ABSOLUTE, 40, 22, 0, 0);
    expect_pos(40, 22, "SPOS_ABSOLUTE places exact-size screen at (40, 22)");
    call_pos(SPOS_MAKEVISIBLE, 0, 0, 983, 745);
    expect_pos(40, 22, "visible rectangle on positively placed exact-size screen stays at (40, 22)");
    call_pos(SPOS_MAKEVISIBLE, 0, 0, 993, 750);
    expect_pos(30, 17, "partially clipped rectangle on positively placed screen shifts minimally to (30, 17)");
    call_pos(SPOS_MAKEVISIBLE, 1015, 760, 1023, 767);
    expect_pos(0, 0, "bottom-right corner of positively placed exact-size screen scrolls back to (0, 0)");

    /* Existing negative offset outside nominal autoscroll minima:
       on a 2048x1536 screen with 1024x768 display (nominal min -1024, -768),
       if a driver-approved position is (-1200, -900), an already-visible
       rectangle (1300, 1000)-(1400, 1100) (at display 100..200, 100..200)
       must stay at (-1200, -900) without jumping to (-1024, -768). */
    prepare("limits-negative-beyond-nominal-min", 2048, 1536, 1024, 768, TRUE);
    call_pos(SPOS_ABSOLUTE, -1200, -900, 0, 0);
    expect_pos(-1200, -900, "SPOS_ABSOLUTE sets driver-approved (-1200, -900)");
    call_pos(SPOS_MAKEVISIBLE, 1300, 1000, 1400, 1100);
    expect_pos(-1200, -900, "already-visible rectangle at (-1200, -900) does not jump to (-1024, -768)");

    /* Nonzero-origin DisplayClip normalized span (Max - Min + 1):
       1) Negative overscan origin (-16, -12)..(1007, 755) -> 1024x768 visible span.
       2) Positive shifted origin (32, 16)..(671, 495) -> 640x480 visible span. */
    prepare("limits-nonzero-origin-overscan-dclip", 2048, 1536, 2048, 1536, TRUE);
    f.lookup_vpe.DisplayClip.MinX = -16;
    f.lookup_vpe.DisplayClip.MinY = -12;
    f.lookup_vpe.DisplayClip.MaxX = 1007;
    f.lookup_vpe.DisplayClip.MaxY = 755;
    call_pos(SPOS_MAKEVISIBLE, 0, 0, 1023, 767);
    expect_pos(0, 0, "negative-origin overscan DisplayClip (1024x768 span) leaves (0,0)-(1023,767) in place");
    call_pos(SPOS_MAKEVISIBLE, 900, 700, 1200, 1000);
    expect_pos(-177, -233, "negative-origin overscan DisplayClip shifts against 1024x768 span");

    prepare("limits-nonzero-origin-positive-dclip", 2048, 1536, 2048, 1536, TRUE);
    f.lookup_vpe.DisplayClip.MinX = 32;
    f.lookup_vpe.DisplayClip.MinY = 16;
    f.lookup_vpe.DisplayClip.MaxX = 671;
    f.lookup_vpe.DisplayClip.MaxY = 495;
    call_pos(SPOS_MAKEVISIBLE, 0, 0, 639, 479);
    expect_pos(0, 0, "positive-origin DisplayClip (640x480 span) leaves (0,0)-(639,479) in place");
    call_pos(SPOS_MAKEVISIBLE, 500, 400, 700, 500);
    expect_pos(-61, -21, "positive-origin DisplayClip shifts against 640x480 span");

    /* DisplayClip precedence when ViewPort.DWidth/DHeight still hold full
       oversized screen dimensions (2048x1536) before MakeScreen clamping. */
    prepare("limits-vpe-precedence", 2048, 1536, 2048, 1536, TRUE);
    f.lookup_vpe.DisplayClip.MinX = 0;
    f.lookup_vpe.DisplayClip.MinY = 0;
    f.lookup_vpe.DisplayClip.MaxX = 1023;
    f.lookup_vpe.DisplayClip.MaxY = 767;
    call_pos(SPOS_MAKEVISIBLE, 900, 700, 1200, 1000);
    expect_pos(-177, -233, "GfxLookUp ViewPortExtra DisplayClip overrides unclipped DWidth/DHeight");

    /* Fallback to ColorMap->cm_vpe when GfxLookUp returns NULL. */
    f.lookup_result = NULL;
    f.cm_vpe.DisplayClip.MinX = 0;
    f.cm_vpe.DisplayClip.MinY = 0;
    f.cm_vpe.DisplayClip.MaxX = 639;
    f.cm_vpe.DisplayClip.MaxY = 479;
    call_pos(SPOS_ABSOLUTE, 0, 0, 0, 0);
    expect_pos(0, 0, "reset before cm_vpe check");
    call_pos(SPOS_MAKEVISIBLE, 500, 400, 700, 500);
    expect_pos(-61, -21, "ColorMap->cm_vpe DisplayClip (640x480) is used when GfxLookUp is NULL");

    /* Fallback to ViewPort.DWidth/DHeight when vpe DisplayClip is uninitialized. */
    f.screen.Screen.ViewPort.ColorMap = NULL;
    f.screen.Screen.ViewPort.DWidth = 800;
    f.screen.Screen.ViewPort.DHeight = 600;
    call_pos(SPOS_ABSOLUTE, 0, 0, 0, 0);
    expect_pos(0, 0, "reset before DWidth/DHeight fallback check");
    call_pos(SPOS_MAKEVISIBLE, 700, 500, 900, 700);
    expect_pos(-101, -101, "ViewPort.DWidth/DHeight (800x600) is used when vpe is absent");

    /* Driver / compositor adjustment inside ScrollVPort is copied back to Screen. */
    prepare("limits-driver-feedback", 2048, 1536, 1024, 768, TRUE);
    f.driver_adjust = TRUE;
    f.driver_min_dx = -512;
    f.driver_max_dx = 0;
    f.driver_min_dy = -384;
    f.driver_max_dy = 100;
    call_pos(SPOS_ABSOLUTE, -800, -600, 0, 0);
    check(f.last_requested_dx == -800 && f.last_requested_dy == -600,
          "ScreenPosition passes bounded (-800, -600) to ScrollVPort");
    expect_pos(-512, -384, "Screen adopts ScrollVPort driver-clamped offsets (-512, -384)");
}

static void test_lock_ownership_and_null_screen(void)
{
    prepare("null-screen-safety", 1024, 768, 1024, 768, TRUE);
    reset_counters();
    production_ScreenPosition(NULL, SPOS_MAKEVISIBLE, 0, 0, 100, 100,
                              &f.ibase.IntuitionBase);
    check(f.lock_calls == 0 && f.unlock_calls == 0 && f.scroll_calls == 0 &&
          f.priv_screen_calls == 0 && f.lookup_calls == 0,
          "ScreenPosition(NULL, ...) returns before locking or touching collaborators");

    reset_counters();
    production_MoveScreen(NULL, 10, 20, &f.ibase.IntuitionBase);
    check(f.lock_calls == 0 && f.unlock_calls == 0 && f.scroll_calls == 0 &&
          f.priv_screen_calls == 0 && f.lookup_calls == 0,
          "MoveScreen(NULL, ...) returns before locking or touching collaborators");

    /* Nested lock ownership: int_RethinkDisplay() calls ScreenPosition while
       already holding LockIBase(0). */
    prepare("nested-ibase-lock", 2048, 1536, 1024, 768, TRUE);
    f.initial_lock_depth = 2;
    call_pos(SPOS_RELATIVE, -40, -30, 0, 0);
    expect_pos(-40, -30, "nested ScreenPosition call under existing IBaseLock succeeds");
    check(f.lock_depth == 2, "nested ScreenPosition preserves caller IBaseLock depth");
}

int main(void)
{
    test_makevisible_already_visible();
    test_makevisible_each_edge();
    test_makevisible_oversized_and_degenerate();
    test_absolute_relative_and_flags();
    test_desktop_and_screen_limits();
    test_lock_ownership_and_null_screen();

    printf("test_screenposition: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
