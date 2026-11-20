/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native regression for Intuition ScreenPosition() and MoveScreen():
    captures Workbench's ModeID via GetVPModeID() while locked, binds
    SA_DisplayID on opened test screens, verifies actual opened dimensions
    and DisplayClip span, checks already-visible no-op preservation from
    captured initial LeftEdge/TopEdge, and validates edge/oversized
    SPOS_MAKEVISIBLE results against an SPOS_ABSOLUTE driver reference.
*/
#include <aros/debug.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/modeid.h>
#include <graphics/view.h>
#include <intuition/screens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

static ULONG checks;
static ULONG failures;
static ULONG skips;

static void check(BOOL ok, CONST_STRPTR what)
{
    checks++;
    if (!ok)
    {
        bug("SCREENPOSITION FAIL: %s\n", what);
        failures++;
    }
    Printf("%s %s\n", ok ? "ok  " : "FAIL", what);
}

static void skip(CONST_STRPTR what)
{
    skips++;
    Printf("SKIP %s\n", what);
}

/* Intersect the screen with the visible aperture in screen coordinates.
   A driver may open a screen at a positive or negative initial offset. */
static BOOL visible_rect(struct Screen *scr, LONG vw, LONG vh,
                         struct Rectangle *rect)
{
    LONG left = (scr->LeftEdge < 0) ? -(LONG)scr->LeftEdge : 0;
    LONG top = (scr->TopEdge < 0) ? -(LONG)scr->TopEdge : 0;
    LONG right = vw - 1 - (LONG)scr->LeftEdge;
    LONG bottom = vh - 1 - (LONG)scr->TopEdge;

    if (right >= scr->Width)
        right = (LONG)scr->Width - 1;
    if (bottom >= scr->Height)
        bottom = (LONG)scr->Height - 1;
    if (vw <= 0 || vh <= 0 || right < left || bottom < top)
        return FALSE;

    rect->MinX = left;
    rect->MinY = top;
    rect->MaxX = right;
    rect->MaxY = bottom;
    return TRUE;
}

static void visible_span(struct Screen *scr, LONG *vis_w, LONG *vis_h)
{
    struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(&scr->ViewPort);
    LONG w = (scr->ViewPort.DWidth > 0) ? (LONG)scr->ViewPort.DWidth : (LONG)scr->Width;
    LONG h = (scr->ViewPort.DHeight > 0) ? (LONG)scr->ViewPort.DHeight : (LONG)scr->Height;

    if (!vpe && scr->ViewPort.ColorMap)
        vpe = scr->ViewPort.ColorMap->cm_vpe;
    if (vpe)
    {
        LONG cw = (LONG)vpe->DisplayClip.MaxX - (LONG)vpe->DisplayClip.MinX + 1;
        LONG ch = (LONG)vpe->DisplayClip.MaxY - (LONG)vpe->DisplayClip.MinY + 1;

        if (cw > 0 && (vpe->DisplayClip.MinX != 0 || vpe->DisplayClip.MaxX != 0))
            w = cw;
        if (ch > 0 && (vpe->DisplayClip.MinY != 0 || vpe->DisplayClip.MaxY != 0))
            h = ch;
    }
    *vis_w = w;
    *vis_h = h;
}

/* Query what the active driver/compositor accepts for (req_x, req_y) via
   SPOS_ABSOLUTE, then restore the caller's starting position. */
static void probe_abs_reference(struct Screen *scr, ULONG force_flag,
                                LONG req_x, LONG req_y,
                                WORD *exp_x, WORD *exp_y)
{
    WORD saved_x = scr->LeftEdge;
    WORD saved_y = scr->TopEdge;

    ScreenPosition(scr, SPOS_ABSOLUTE | force_flag, req_x, req_y, 0, 0);
    *exp_x = scr->LeftEdge;
    *exp_y = scr->TopEdge;
    ScreenPosition(scr, SPOS_ABSOLUTE | force_flag, saved_x, saved_y, 0, 0);
}

int main(void)
{
    struct Screen *wb = LockPubScreen(NULL);
    ULONG modeid = INVALID_ID;
    LONG dwidth = 640;
    LONG dheight = 480;
    struct Rectangle exact_clip;
    struct Rectangle half_clip;
    struct Screen *exact = NULL;
    struct Screen *over = NULL;
    struct Screen *fixed = NULL;

    if (wb)
    {
        modeid = GetVPModeID(&wb->ViewPort);
        visible_span(wb, &dwidth, &dheight);
        UnlockPubScreen(NULL, wb);
    }

    exact_clip.MinX = 0;
    exact_clip.MinY = 0;
    exact_clip.MaxX = (WORD)(dwidth - 1);
    exact_clip.MaxY = (WORD)(dheight - 1);

    half_clip.MinX = 0;
    half_clip.MinY = 0;
    half_clip.MaxX = (WORD)((dwidth / 2) - 1);
    half_clip.MaxY = (WORD)((dheight / 2) - 1);

    exact = OpenScreenTags(NULL,
        (modeid != (ULONG)INVALID_ID) ? SA_DisplayID : TAG_IGNORE, modeid,
        SA_Width, dwidth,
        SA_Height, dheight,
        SA_Depth, 2,
        SA_Type, CUSTOMSCREEN,
        SA_DClip, (IPTR)&exact_clip,
        SA_Quiet, TRUE,
        SA_Behind, TRUE,
        TAG_DONE);
    check(exact != NULL, "open exact-fit custom screen behind Workbench");
    if (exact)
    {
        LONG vw = 0, vh = 0;
        WORD exp_x = 0, exp_y = 0;
        struct Rectangle visible;
        struct Rectangle pulled_visible;

        visible_span(exact, &vw, &vh);
        check(exact->Width > 0 && exact->Height > 0 && vw > 0 && vh > 0,
              "exact-fit screen has valid opened dimensions and DisplayClip span");

        if (visible_rect(exact, vw, vh, &visible))
        {
            WORD init_x = exact->LeftEdge;
            WORD init_y = exact->TopEdge;
            LONG cx1 = visible.MinX +
                ((visible.MaxX - visible.MinX > 40) ? 40 : visible.MaxX - visible.MinX);
            LONG cy1 = visible.MinY +
                ((visible.MaxY - visible.MinY > 82) ? 82 : visible.MaxY - visible.MinY);
            LONG cx2 = (cx1 + 8 < visible.MaxX) ? cx1 + 8 : visible.MaxX;
            LONG cy2 = (cy1 + 8 < visible.MaxY) ? cy1 + 8 : visible.MaxY;

            ScreenPosition(exact, SPOS_MAKEVISIBLE,
                           cx1, cy1, cx2, cy2);
            check(exact->LeftEdge == init_x && exact->TopEdge == init_y &&
                  exact->LeftEdge == exact->ViewPort.DxOffset &&
                  exact->TopEdge == exact->ViewPort.DyOffset,
                  "MAKEVISIBLE caret rectangle on exact-fit screen does not shift");
            ScreenPosition(exact, SPOS_MAKEVISIBLE,
                           visible.MinX, visible.MinY, visible.MaxX, visible.MaxY);
            check(exact->LeftEdge == init_x && exact->TopEdge == init_y,
                  "MAKEVISIBLE already-visible rectangle stays at initial position");
        }
        else
            skip("exact-fit screen has no visible intersection for no-op checks");

        ScreenPosition(exact, SPOS_ABSOLUTE, 0, 22, 0, 0);
        check(exact->LeftEdge == exact->ViewPort.DxOffset &&
              exact->TopEdge == exact->ViewPort.DyOffset,
              "SPOS_ABSOLUTE copies driver-validated ViewPort offsets to Screen");
        if (exact->TopEdge > 0 && vh > exact->TopEdge + 1 &&
            exact->Height >= vh && visible_rect(exact, vw, vh, &pulled_visible))
        {
            WORD pulled_x = exact->LeftEdge;
            WORD pulled_y = exact->TopEdge;

            ScreenPosition(exact, SPOS_MAKEVISIBLE,
                           pulled_visible.MinX, pulled_visible.MinY,
                           pulled_visible.MaxX, pulled_visible.MaxY);
            check(exact->LeftEdge == pulled_x && exact->TopEdge == pulled_y,
                  "MAKEVISIBLE preserves positive placement when rectangle is visible");

            probe_abs_reference(exact, 0, pulled_x, 0, &exp_x, &exp_y);
            ScreenPosition(exact, SPOS_MAKEVISIBLE,
                           pulled_visible.MinX, vh - pulled_y,
                           pulled_visible.MaxX, vh - 1);
            check(exact->LeftEdge == exp_x && exact->TopEdge == exp_y,
                  "MAKEVISIBLE scrolls pulled-down screen back to origin");
        }
        else
            skip("driver did not provide a pulled-down screen for visibility checks");

        probe_abs_reference(exact, 0,
                            (exact->Width > vw) ? (vw - exact->Width) : 0,
                            (exact->Height > vh) ? (vh - exact->Height) : 0,
                            &exp_x, &exp_y);
        ScreenPosition(exact, SPOS_MAKEVISIBLE, vw + 64, vh + 32,
                       vw + 96, vh + 64);
        check(exact->LeftEdge == exp_x && exact->TopEdge == exp_y,
              "MAKEVISIBLE bounds out-of-range rectangle to screen limits");

        CloseScreen(exact);
    }

    over = OpenScreenTags(NULL,
        (modeid != (ULONG)INVALID_ID) ? SA_DisplayID : TAG_IGNORE, modeid,
        SA_Width, dwidth * 2,
        SA_Height, dheight * 2,
        SA_Depth, 2,
        SA_Type, CUSTOMSCREEN,
        SA_DClip, (IPTR)&half_clip,
        SA_AutoScroll, TRUE,
        SA_Quiet, TRUE,
        SA_Behind, TRUE,
        TAG_DONE);
    check(over != NULL, "open oversized custom screen behind Workbench");
    if (over)
    {
        LONG vw = 0, vh = 0;
        WORD exp_x = 0, exp_y = 0;
        struct Rectangle visible;

        visible_span(over, &vw, &vh);
        check(over->Width > 0 && over->Height > 0 && vw > 0 && vh > 0,
              "oversized screen has valid opened dimensions and DisplayClip span");

        if (visible_rect(over, vw, vh, &visible))
        {
            WORD init_x = over->LeftEdge;
            WORD init_y = over->TopEdge;

            ScreenPosition(over, SPOS_MAKEVISIBLE,
                           visible.MinX, visible.MinY, visible.MaxX, visible.MaxY);
            check(over->LeftEdge == init_x && over->TopEdge == init_y,
                  "MAKEVISIBLE leaves already-visible rectangle on oversized screen in place");
        }
        else
            skip("oversized screen has no visible intersection for no-op check");

        if (over->Width >= vw + 64 && over->Height >= vh + 64)
        {
            ScreenPosition(over, SPOS_ABSOLUTE, 0, 0, 0, 0);

            /* Right edge: x2 = vw + 50 -> target LeftEdge = -51 */
            probe_abs_reference(over, 0, -51, 0, &exp_x, &exp_y);
            ScreenPosition(over, SPOS_MAKEVISIBLE, vw / 2, 16, vw + 50, 48);
            check(over->LeftEdge == exp_x && over->TopEdge == exp_y &&
                  over->LeftEdge == over->ViewPort.DxOffset &&
                  over->TopEdge == over->ViewPort.DyOffset,
                  "MAKEVISIBLE right-edge shift matches driver reference");

            /* Left edge from current position: x1 = 12 -> target LeftEdge = -12 */
            if (over->LeftEdge < -12)
            {
                probe_abs_reference(over, 0, -12, over->TopEdge, &exp_x, &exp_y);
                ScreenPosition(over, SPOS_MAKEVISIBLE, 12, 16, 48, 48);
                check(over->LeftEdge == exp_x && over->TopEdge == exp_y,
                      "MAKEVISIBLE left-edge shift matches driver reference");
            }

            /* Bottom edge: y2 = vh + 40 -> target TopEdge = -41 */
            probe_abs_reference(over, 0, over->LeftEdge, -41, &exp_x, &exp_y);
            ScreenPosition(over, SPOS_MAKEVISIBLE, 20, vh / 2, 48, vh + 40);
            check(over->LeftEdge == exp_x && over->TopEdge == exp_y,
                  "MAKEVISIBLE bottom-edge shift matches driver reference");

            /* Top edge from current position: y1 = 15 -> target TopEdge = -15 */
            if (over->TopEdge < -15)
            {
                probe_abs_reference(over, 0, over->LeftEdge, -15, &exp_x, &exp_y);
                ScreenPosition(over, SPOS_MAKEVISIBLE, 20, 15, 48, 48);
                check(over->LeftEdge == exp_x && over->TopEdge == exp_y,
                      "MAKEVISIBLE top-edge shift matches driver reference");
            }

            /* Oversized rectangle: (30, 25)-(vw + 60, vh + 50) -> target (-30, -25) */
            probe_abs_reference(over, 0, -30, -25, &exp_x, &exp_y);
            ScreenPosition(over, SPOS_MAKEVISIBLE, 30, 25, vw + 60, vh + 50);
            check(over->LeftEdge == exp_x && over->TopEdge == exp_y,
                  "MAKEVISIBLE oversized rectangle aligns upper-left corner per driver reference");

            /* Far out-of-range rectangle clamps to (vw - Width, vh - Height). */
            probe_abs_reference(over, 0, vw - over->Width, vh - over->Height,
                                &exp_x, &exp_y);
            ScreenPosition(over, SPOS_MAKEVISIBLE, over->Width * 2, over->Height * 2,
                           over->Width * 3, over->Height * 3);
            check(over->LeftEdge == exp_x && over->TopEdge == exp_y,
                  "MAKEVISIBLE clamps oversized screen offsets to screen bounds");
        }
        else
            skip("driver did not provide an oversized screen for edge-scroll checks");

        ScreenPosition(over, SPOS_ABSOLUTE, 0, 0, 0, 0);
        probe_abs_reference(over, 0, over->LeftEdge - 10, over->TopEdge + 8,
                            &exp_x, &exp_y);
        MoveScreen(over, -10, 8);
        check(over->LeftEdge == exp_x && over->TopEdge == exp_y &&
              over->LeftEdge == over->ViewPort.DxOffset &&
              over->TopEdge == over->ViewPort.DyOffset,
              "MoveScreen updates Screen from driver-validated ViewPort offsets");

        CloseScreen(over);
    }

    fixed = OpenScreenTags(NULL,
        (modeid != (ULONG)INVALID_ID) ? SA_DisplayID : TAG_IGNORE, modeid,
        SA_Width, dwidth * 2,
        SA_Height, dheight * 2,
        SA_Depth, 2,
        SA_Type, CUSTOMSCREEN,
        SA_DClip, (IPTR)&half_clip,
        SA_Draggable, FALSE,
        SA_Quiet, TRUE,
        SA_Behind, TRUE,
        TAG_DONE);
    check(fixed != NULL, "open non-draggable custom screen");
    if (fixed)
    {
        LONG vw = 0, vh = 0;
        WORD init_x = fixed->LeftEdge;
        WORD init_y = fixed->TopEdge;
        WORD exp_x = 0, exp_y = 0;

        visible_span(fixed, &vw, &vh);

        ScreenPosition(fixed, SPOS_ABSOLUTE, init_x - 20, init_y + 12, 0, 0);
        ScreenPosition(fixed, SPOS_MAKEVISIBLE, vw + 10, vh + 10,
                       vw + 20, vh + 20);
        check(fixed->LeftEdge == init_x && fixed->TopEdge == init_y,
              "non-draggable screen ignores moves without SPOS_FORCEDRAG");

        probe_abs_reference(fixed, SPOS_FORCEDRAG, 0, 12, &exp_x, &exp_y);
        ScreenPosition(fixed, SPOS_ABSOLUTE | SPOS_FORCEDRAG, 0, 12, 0, 0);
        check(fixed->LeftEdge == exp_x && fixed->TopEdge == exp_y &&
              fixed->LeftEdge == fixed->ViewPort.DxOffset &&
              fixed->TopEdge == fixed->ViewPort.DyOffset,
              "SPOS_FORCEDRAG overrides SA_Draggable FALSE");

        CloseScreen(fixed);
    }

    Printf("screenposition: %ld checks, %ld failures, %ld skips\n",
           (LONG)checks, (LONG)failures, (LONG)skips);
    return failures ? 20 : 0;
}
