/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Superbitmap layer behaviour, as documented for AmigaOS:
          rastport coordinates are superbitmap coordinates, the layer shows
          the superbitmap from (Scroll_X, Scroll_Y), ScrollLayer() adds its
          deltas to those, rendering into covered parts goes into the
          superbitmap, and covering/uncovering/sizing preserves the content.

          A superbitmap window is opened partly covered by a second window.
          A position coded pattern is rendered through graphics.library
          (WriteRGBPixel and RectFill) across the visible and covered parts,
          then checked in the superbitmap (SyncSBitMap), on screen after
          uncovering, after covering again and scrolling, after uncovering
          again, and after enlarging the window.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>

#include <exec/types.h>
#include <graphics/clip.h>
#include <graphics/rpattr.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>

#include <stdio.h>

#define SBW 480
#define SBH 360
#define WINW 320
#define WINH 200

static int failures = 0;
static ULONG salt = 0x40;   /* distinguishes the bands painted at different stages */

static ULONG codeof(LONG x, LONG y)
{
    return ((x & 0xFF) << 16) | ((y & 0xFF) << 8) | salt;
}

/* rect is in rastport (superbitmap) coordinates */
static void paint(struct RastPort *rp, struct Rectangle *rect)
{
    LONG x, y;

    /* the top half pixel by pixel, the bottom half with RectFills */
    for (y = rect->MinY; y <= rect->MaxY; y++)
    {
        if (y < (rect->MinY + rect->MaxY) / 2)
        {
            for (x = rect->MinX; x <= rect->MaxX; x++)
                WriteRGBPixel(rp, x, y, codeof(x, y));
        }
        else
        {
            for (x = rect->MinX; x <= rect->MaxX; x += 4)
            {
                LONG x1 = (x + 3 > rect->MaxX) ? rect->MaxX : x + 3;
                /* 4 pixels wide blocks share the code of their first pixel */
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, codeof(x, y), TAG_DONE);
                RectFill(rp, x, y, x1, y);
            }
        }
    }
}

static ULONG expected(struct Rectangle *rect, LONG x, LONG y)
{
    if (y < (rect->MinY + rect->MaxY) / 2)
        return codeof(x, y);
    return codeof(rect->MinX + ((x - rect->MinX) & ~3), y);
}

/*
 * Compare pixels read with 'reader' against the pattern. The pixel at
 * rastport coordinate (x,y) is read at (x + dx, y + dy) of 'rp'. Only
 * pixels inside 'limit' (rastport coordinates) are checked.
 */
static void check(const char *stage, struct RastPort *rp, LONG dx, LONG dy,
                  struct Rectangle *rect, struct Rectangle *limit)
{
    LONG x, y, bad = 0, shown = 0, checked = 0;

    for (y = rect->MinY; y <= rect->MaxY; y++)
        for (x = rect->MinX; x <= rect->MaxX; x++)
        {
            ULONG want = expected(rect, x, y), got;
            if (x < limit->MinX || x > limit->MinX + (limit->MaxX - limit->MinX) || y < limit->MinY || y > limit->MaxY)
                continue;
            checked++;
            got = ReadRGBPixel(rp, x + dx, y + dy) & 0xFFFFFF;
            if (got != want)
            {
                bad++;
                if (shown++ < 3)
                    printf("    %s: at %ld,%ld got %06lx want %06lx\n", stage, (long)x, (long)y,
                           (unsigned long)got, (unsigned long)want);
            }
        }
    printf("  %-52s %s (%ld pixels checked, %ld bad)\n", stage, bad ? "FAIL" : "ok", (long)checked, (long)bad);
    if (bad) failures++;
}

int main(void)
{
    struct Screen *scr;
    struct BitMap *superbm = NULL;
    struct Window *w1 = NULL, *w2 = NULL;
    struct Layer *L;
    struct RastPort sbrp;
    struct Rectangle rect, limit;

    scr = LockPubScreen(NULL);
    if (!scr) return RETURN_FAIL;
    if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) < 15)
    {
        printf("superbitmap: needs a true colour screen\n");
        UnlockPubScreen(NULL, scr);
        return RETURN_WARN;
    }
    superbm = AllocBitMap(SBW, SBH, GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH), BMF_CLEAR, scr->RastPort.BitMap);
    UnlockPubScreen(NULL, scr);
    if (!superbm) return RETURN_FAIL;
    InitRastPort(&sbrp);
    sbrp.BitMap = superbm;

    w1 = OpenWindowTags(NULL, WA_Left, 60, WA_Top, 60, WA_Width, WINW, WA_Height, WINH,
            WA_Title, (IPTR)"superbitmap", WA_SuperBitMap, (IPTR)superbm,
            WA_DragBar, TRUE, WA_Activate, FALSE, WA_MaxWidth, SBW, WA_MaxHeight, SBH, TAG_DONE);
    w2 = OpenWindowTags(NULL, WA_Left, 220, WA_Top, 180, WA_Width, WINW, WA_Height, WINH,
            WA_Title, (IPTR)"cover", WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_Activate, FALSE, TAG_DONE);
    if (!w1 || !w2) { printf("could not open the windows\n"); goto done; }
    Delay(10);
    L = w1->RPort->Layer;

    printf("superbitmap: window %dx%d borders l%d t%d r%d b%d, layer flags 0x%lx, scroll %d,%d\n",
           w1->Width, w1->Height, w1->BorderLeft, w1->BorderTop, w1->BorderRight, w1->BorderBottom,
           (unsigned long)L->Flags, L->Scroll_X, L->Scroll_Y);

    /* the band to paint: inner area, spanning visible and covered parts */
    rect.MinX = w1->BorderLeft + 2;
    rect.MaxX = w1->Width - w1->BorderRight - 3;
    rect.MaxY = w1->Height - w1->BorderBottom - 3;
    rect.MinY = rect.MaxY - 39;
    limit = rect;

    /* 1. render through graphics.library with the window partly covered */
    paint(w1->RPort, &rect);

    /* 2. the superbitmap must hold it at rastport coordinates, visible part after SyncSBitMap */
    LockLayer(0, L);
    SyncSBitMap(L);
    UnlockLayer(L);
    check("superbitmap after SyncSBitMap (rastport coords)", &sbrp, 0, 0, &rect, &limit);

    /* 3. uncover: the screen must show it all */
    CloseWindow(w2); w2 = NULL;
    Delay(5);
    check("screen after uncovering", w1->RPort, 0, 0, &rect, &limit);

    /* 4. cover again, scroll by 40,30: Scroll_X/Y must become 40,30 */
    w2 = OpenWindowTags(NULL, WA_Left, 220, WA_Top, 180, WA_Width, WINW, WA_Height, WINH,
            WA_Title, (IPTR)"cover", WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_Activate, FALSE, TAG_DONE);
    if (!w2) goto done;
    Delay(5);
    ScrollLayer(0, L, 40, 30);
    printf("  after ScrollLayer(40,30): Scroll_X/Y = %d,%d %s\n", L->Scroll_X, L->Scroll_Y,
           (L->Scroll_X == 40 && L->Scroll_Y == 30) ? "ok" : "FAIL");
    if (!(L->Scroll_X == 40 && L->Scroll_Y == 30)) failures++;

    /* Through the rastport everything must still read back: the uncovered part
       from the screen, the covered part and the part now outside the window
       from the superbitmap */
    check("scrolled, whole band through the rastport", w1->RPort, 0, 0, &rect, &limit);
    /* On the screen, rastport (x,y) is at (x + 60 - 40, y + 60 - 30): check the
       inner area left of the cover window */
    limit.MinX = 40 + w1->BorderLeft; limit.MaxX = 220 - 60 + 40 - 1;
    limit.MinY = rect.MinY; limit.MaxY = rect.MaxY;
    {
        struct Screen *s = LockPubScreen(NULL);
        if (s)
        {
            check("scrolled, uncovered part read from the screen", &s->RastPort, 60 - 40, 60 - 30, &rect, &limit);
            UnlockPubScreen(NULL, s);
        }
    }

    /* 5. uncover while scrolled */
    CloseWindow(w2); w2 = NULL;
    Delay(5);
    {
        struct Screen *s = LockPubScreen(NULL);
        limit.MaxX = rect.MaxX;
        if (s)
        {
            check("scrolled, uncovered area read from the screen", &s->RastPort, 60 - 40, 60 - 30, &rect, &limit);
            UnlockPubScreen(NULL, s);
        }
    }
    limit = rect;
    check("scrolled, whole band through the rastport again", w1->RPort, 0, 0, &rect, &limit);

    /* 6. render into the scrolled layer while covered, then scroll back and uncover */
    w2 = OpenWindowTags(NULL, WA_Left, 220, WA_Top, 180, WA_Width, WINW, WA_Height, WINH,
            WA_Title, (IPTR)"cover", WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_Activate, FALSE, TAG_DONE);
    if (!w2) goto done;
    Delay(5);
    /* a band that is inside the window in both scroll states, partly covered now */
    salt = 0x80;
    rect.MinY = 150; rect.MaxY = 189;
    limit = rect;
    paint(w1->RPort, &rect);
    /* and one outside the scrolled window (rows 230.. are below it): must land in the superbitmap */
    {
        struct Rectangle outside = rect;
        outside.MinY = 232; outside.MaxY = 241;
        paint(w1->RPort, &outside);
        check("band outside the window goes to the superbitmap", &sbrp, 0, 0, &outside, &outside);
        check("and reads back through the rastport", w1->RPort, 0, 0, &outside, &outside);
    }
    ScrollLayer(0, L, -40, -30);
    printf("  after ScrollLayer(-40,-30): Scroll_X/Y = %d,%d %s\n", L->Scroll_X, L->Scroll_Y,
           (L->Scroll_X == 0 && L->Scroll_Y == 0) ? "ok" : "FAIL");
    if (!(L->Scroll_X == 0 && L->Scroll_Y == 0)) failures++;
    CloseWindow(w2); w2 = NULL;
    Delay(5);
    check("band painted while scrolled and covered, on screen", w1->RPort, 0, 0, &rect, &limit);
    LockLayer(0, L);
    SyncSBitMap(L);
    UnlockLayer(L);
    check("same band in the superbitmap", &sbrp, 0, 0, &rect, &limit);

    /* 7. enlarge the window: the new area must show the superbitmap, which we fill first */
    {
        struct Rectangle grow;
        salt = 0xC0;
        grow.MinX = WINW; grow.MaxX = WINW + 60 - 1;
        grow.MinY = 60; grow.MaxY = 99;
        paint(&sbrp, &grow);                 /* straight into the superbitmap */
        SizeWindow(w1, 60, 0);
        Delay(10);
        printf("  window now %dx%d\n", w1->Width, w1->Height);
        if (w1->Width == WINW + 60)
        {
            /* up to the new right border, which Intuition redraws */
            struct Rectangle inner = grow;
            inner.MaxX = w1->Width - w1->BorderRight - 1;
            check("area exposed by SizeWindow shows the superbitmap", w1->RPort, 0, 0, &grow, &inner);
        }
        else
            printf("  SizeWindow did not grow the window, skipped\n");
    }

    printf("RESULT: %s\n", failures ? "FAIL" : "PASS");
done:
    if (w2) CloseWindow(w2);
    if (w1) CloseWindow(w1);
    if (superbm) FreeBitMap(superbm);
    return failures ? RETURN_WARN : RETURN_OK;
}
