/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Check the message DoHookClipRects() hands to a layer hook for
          visible and hidden (backed up) ClipRects of a smart refresh layer.

          Two overlapping smart refresh windows are opened. A hook is run over
          a rectangle of the rear window that spans its visible part and the
          part covered by the front window. The hook logs the message and
          paints every pixel with a code derived from its position relative
          to the rectangle, using one of three offset formulas. The front
          window is then closed, which brings the backed up pixels onto the
          screen, and the rectangle is read back: a correct formula gives a
          continuous pattern across the visible/hidden boundary.

          SUPER/S opens the rear window as a superbitmap window, SCROLL/S
          scrolls its layer by 7,5 first (ScrollLayer) so Scroll_X/Y are
          non-zero.

          FORMULA/K:
            OFFSET  (default) position = msg->Offset - rect origin
                              (the layer relative offset, as documented)
            DECOR   the per-ClipRect lookup decorator.library's RectShadeFunc
                    used to do (right only for unscrolled smart refresh layers)
            BOUNDS  position = msg->Bounds - layer bounds
                              (what the muimaster hooks do)
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>

#include <aros/asmcall.h>
#include <exec/types.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>

#include <stdio.h>
#include <string.h>

/* The message as layers.library builds it (rom/hyperlayers/basicfuncs.c) */
struct layerhookmsg
{
    struct Layer *l;
    WORD MinX, MinY, MaxX, MaxY;
    LONG OffsetX, OffsetY;
};

enum { F_OFFSET, F_DECOR, F_BOUNDS };

struct HookData
{
    struct BitMap *rpbm;    /* the layer rastport's bitmap when the hook was started */
    struct Layer  *layer;
    WORD           x0, y0;  /* rectangle origin, layer coordinates */
    int            formula;
    BOOL           truecolor;
    int            calls, hiddencalls;
};

/* Code a position relative to the rectangle origin into a pixel value */
static ULONG codeof(LONG rx, LONG ry, BOOL truecolor)
{
    if (truecolor)
        return ((rx & 0xFF) << 16) | ((ry & 0xFF) << 8) | 0x80;
    return 1 + ((rx + ry) & 1);
}

AROS_UFH3(void, HookFunc,
    AROS_UFHA(struct Hook *,         h,   A0),
    AROS_UFHA(struct RastPort *,     rp,  A2),
    AROS_UFHA(struct layerhookmsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct HookData *d = h->h_Data;
    struct RastPort  tmp = *rp;      /* as the autodoc says: a copy with Layer == NULL */
    struct Layer    *L   = d->layer;
    BOOL  hidden = (rp->BitMap != d->rpbm);
    LONG  offx = 0, offy = 0;        /* position of msg->Min relative to rect origin */
    LONG  decx = 0, decy = 0, boundsx, boundsy;
    LONG  x, y;

    tmp.Layer = NULL;
    d->calls++;
    if (hidden) d->hiddencalls++;

    /* The three candidate formulas */
    boundsx = msg->MinX - L->bounds.MinX - d->x0;
    boundsy = msg->MinY - L->bounds.MinY - d->y0;

    if (hidden)
    {
        struct ClipRect *cr;
        for (cr = L->ClipRect; cr; cr = cr->Next)
        {
            if (cr->BitMap == rp->BitMap)
            {
                decx = msg->MinX + (cr->bounds.MinX - L->bounds.MinX) - ALIGN_OFFSET(cr->bounds.MinX);
                decy = msg->MinY + (cr->bounds.MinY - L->bounds.MinY);
            }
        }
    }
    else
    {
        decx = msg->OffsetX;
        decy = msg->OffsetY;
    }
    decx -= d->x0;
    decy -= d->y0;

    printf("  hook call %d: %s bounds %d,%d-%d,%d offset %ld,%ld rp->Layer %s\n",
           d->calls, hidden ? "HIDDEN " : "visible",
           msg->MinX, msg->MinY, msg->MaxX, msg->MaxY,
           (long)msg->OffsetX, (long)msg->OffsetY,
           rp->Layer == L ? "= layer" : (rp->Layer ? "OTHER" : "NULL"));
    printf("      rect relative start: OFFSET %ld,%ld  DECOR %ld,%ld  BOUNDS %ld,%ld\n",
           (long)(msg->OffsetX - d->x0), (long)(msg->OffsetY - d->y0),
           (long)decx, (long)decy, (long)boundsx, (long)boundsy);

    switch (d->formula)
    {
    case F_OFFSET: offx = msg->OffsetX - d->x0; offy = msg->OffsetY - d->y0; break;
    case F_DECOR:  offx = decx;                 offy = decy;                 break;
    case F_BOUNDS: offx = boundsx;              offy = boundsy;              break;
    }

    for (y = msg->MinY; y <= msg->MaxY; y++)
    {
        for (x = msg->MinX; x <= msg->MaxX; x++)
        {
            ULONG code = codeof(offx + x - msg->MinX, offy + y - msg->MinY, d->truecolor);

            if (d->truecolor)
                WriteRGBPixel(&tmp, x, y, code);
            else
            {
                SetAPen(&tmp, code);
                WritePixel(&tmp, x, y);
            }
        }
    }

    AROS_USERFUNC_EXIT
}

/*
 * Compare the rectangle's pixels with the expected code. 'where' selects the
 * class of pixel to count: 0 = visible part, 1 = part covered by the front
 * window. Returns the number of bad pixels and prints the first few.
 */
static LONG checkpixels(struct Window *w1, struct HookData *data, struct Rectangle *rect,
                        struct Rectangle *covered, int where, const char *stage)
{
    LONG x, y, bad = 0, shown = 0, bx0 = 0, by0 = 0, bx1 = -1, by1 = -1;

    for (y = rect->MinY; y <= rect->MaxY; y++)
    {
        for (x = rect->MinX; x <= rect->MaxX; x++)
        {
            LONG  sx = x - data->layer->Scroll_X, sy = y - data->layer->Scroll_Y;
            ULONG want = codeof(x - rect->MinX, y - rect->MinY, data->truecolor);
            ULONG got;
            BOOL  iscovered = (sx >= covered->MinX && sy >= covered->MinY);

            if (sx < 0 || sy < 0 || sx >= w1->Width || sy >= w1->Height)
                continue;   /* scrolled out of the layer, nothing was drawn there */
            if ((where == 1) != iscovered)
                continue;

            got = data->truecolor ? (ReadRGBPixel(w1->RPort, x, y) & 0xFFFFFF)
                                  : ReadPixel(w1->RPort, x, y);
            if (got != want)
            {
                if (bad == 0) { bx0 = bx1 = x; by0 = by1 = y; }
                if (x < bx0) bx0 = x; if (x > bx1) bx1 = x;
                if (y < by0) by0 = y; if (y > by1) by1 = y;
                bad++;
                if (shown++ < 4)
                    printf("    %s %s: mismatch at %ld,%ld got %06lx want %06lx\n",
                           stage, where ? "covered" : "visible", (long)x, (long)y,
                           (unsigned long)got, (unsigned long)want);
            }
        }
    }
    if (bad)
        printf("    %s %s: %ld bad pixels within %ld,%ld-%ld,%ld\n", stage,
               where ? "covered" : "visible", (long)bad, (long)bx0, (long)by0, (long)bx1, (long)by1);
    return bad;
}

int main(int argc, char **argv)
{
    static const char *fnames[] = { "OFFSET", "DECOR", "BOUNDS" };
    struct Window  *w1 = NULL, *w2 = NULL;
    struct HookData data;
    struct Hook     hook;
    struct Rectangle rect;
    struct Rectangle covered;       /* part of w1 under w2, w1 coordinates */
    IPTR args[3] = { 0, 0, 0 };
    struct BitMap *superbm = NULL;
    BOOL super = FALSE, scroll = FALSE;
    struct RDArgs *rda;
    int formula = F_OFFSET;
    int rc = RETURN_FAIL;

    rda = ReadArgs("FORMULA/K,SUPER/S,SCROLL/S", args, NULL);
    if (rda)
    {
        super  = args[1] ? TRUE : FALSE;
        scroll = args[2] ? TRUE : FALSE;
        if (args[0])
        {
            int i;
            for (i = 0; i < 3; i++)
                if (!stricmp((char *)args[0], fnames[i])) formula = i;
        }
        FreeArgs(rda);
    }

    if (super)
    {
        struct Screen *scr = LockPubScreen(NULL);
        if (scr)
        {
            superbm = AllocBitMap(320 + 16, 200 + 16, GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH),
                                  BMF_CLEAR, scr->RastPort.BitMap);
            UnlockPubScreen(NULL, scr);
        }
        if (!superbm) { printf("could not allocate the superbitmap\n"); goto done; }
    }

    w1 = OpenWindowTags(NULL,
            WA_Left, 60, WA_Top, 60, WA_Width, 320, WA_Height, 200,
            WA_Title, (IPTR)"hookoffsets rear",
            super ? WA_SuperBitMap : TAG_IGNORE, (IPTR)superbm,
            super ? TAG_IGNORE : WA_SmartRefresh, TRUE,
            WA_DragBar, TRUE, WA_DepthGadget, TRUE,
            WA_Activate, FALSE, TAG_DONE);
    if (!w1) { printf("could not open rear window\n"); goto done; }

    w2 = OpenWindowTags(NULL,
            WA_Left, 220, WA_Top, 180, WA_Width, 320, WA_Height, 200,
            WA_Title, (IPTR)"hookoffsets front",
            WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
            WA_Activate, FALSE, TAG_DONE);
    if (!w2) { printf("could not open front window\n"); goto done; }

    Delay(10);

    if (scroll)
        ScrollLayer(0, w1->RPort->Layer, 7, 5);

    memset(&data, 0, sizeof(data));
    data.rpbm      = w1->RPort->BitMap;
    data.layer     = w1->RPort->Layer;
    data.formula   = formula;
    data.truecolor = GetBitMapAttr(w1->RPort->BitMap, BMA_DEPTH) >= 15;

    /* A band across the bottom of the rear window's inner area. It must stay
       clear of the borders: Intuition redraws those when the front window goes. */
    rect.MinX = w1->BorderLeft + 2 + data.layer->Scroll_X;
    rect.MaxX = w1->Width - w1->BorderRight - 3 + data.layer->Scroll_X;
    rect.MaxY = w1->Height - w1->BorderBottom - 3 + data.layer->Scroll_Y;
    rect.MinY = rect.MaxY - 19;
    data.x0 = rect.MinX;
    data.y0 = rect.MinY;

    covered.MinX = w2->LeftEdge - w1->LeftEdge;
    covered.MinY = w2->TopEdge  - w1->TopEdge;
    covered.MaxX = w1->Width  - 1;
    covered.MaxY = w1->Height - 1;

    printf("hookoffsets: formula %s%s%s, depth %lu, rear window %d,%d %dx%d, layer bounds %d,%d-%d,%d scroll %d,%d\n",
           fnames[formula], super ? " SUPER" : "", scroll ? " SCROLL" : "",
           (unsigned long)GetBitMapAttr(w1->RPort->BitMap, BMA_DEPTH),
           w1->LeftEdge, w1->TopEdge, w1->Width, w1->Height,
           data.layer->bounds.MinX, data.layer->bounds.MinY,
           data.layer->bounds.MaxX, data.layer->bounds.MaxY,
           data.layer->Scroll_X, data.layer->Scroll_Y);
    printf("  rect (layer coords) %d,%d-%d,%d; covered by front window from x=%d, y=%d\n",
           rect.MinX, rect.MinY, rect.MaxX, rect.MaxY, covered.MinX, covered.MinY);

    hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(HookFunc);
    hook.h_Data  = &data;

    DoHookClipRects(&hook, w1->RPort, &rect);

    printf("  %d hook calls, %d for hidden ClipRects; borders l%d t%d r%d b%d\n",
           data.calls, data.hiddencalls,
           w1->BorderLeft, w1->BorderTop, w1->BorderRight, w1->BorderBottom);

    /* Sanity: the visible part must already hold the pattern */
    {
        LONG bad = checkpixels(w1, &data, &rect, &covered, 0, "before uncover");
        printf("  before uncovering: %ld bad pixels in the visible part\n", (long)bad);
    }

    if (superbm)
    {
        /* What the hook put into the superbitmap, read in superbitmap coordinates */
        struct RastPort srp;
        LONG x, y, bad = 0, shown = 0;

        InitRastPort(&srp);
        srp.BitMap = superbm;
        for (y = rect.MinY; y <= rect.MaxY; y++)
            for (x = rect.MinX; x <= rect.MaxX; x++)
            {
                LONG sx = x - data.layer->Scroll_X, sy = y - data.layer->Scroll_Y;
                ULONG want, got;
                if (!(sx >= covered.MinX && sy >= covered.MinY)) continue;
                want = codeof(x - rect.MinX, y - rect.MinY, data.truecolor);
                got  = ReadRGBPixel(&srp, x, y) & 0xFFFFFF;   /* superbitmap x == layer x? */
                if (got != want)
                {
                    bad++;
                    if (shown++ < 3)
                        printf("    superbitmap at layer coord %ld,%ld: got %06lx want %06lx\n",
                               (long)x, (long)y, (unsigned long)got, (unsigned long)want);
                }
            }
        printf("  superbitmap, covered part read at layer coordinates: %ld bad pixels\n", (long)bad);
    }

    /* Uncover the rear window: layers copies the backed up pixels to the screen */
    CloseWindow(w2);
    w2 = NULL;
    Delay(10);

    {
        LONG bad_vis = checkpixels(w1, &data, &rect, &covered, 0, "after uncover");
        LONG bad_hid = checkpixels(w1, &data, &rect, &covered, 1, "after uncover");

        printf("  read back: %ld bad pixels in the visible part, %ld in the part that was hidden\n",
               (long)bad_vis, (long)bad_hid);

        if (data.hiddencalls == 0)
            printf("RESULT: INCONCLUSIVE (no hidden ClipRect was visited)\n");
        else if (bad_vis == 0 && bad_hid == 0)
        {
            printf("RESULT: PASS (%s)\n", fnames[formula]);
            rc = RETURN_OK;
        }
        else
        {
            printf("RESULT: FAIL (%s)\n", fnames[formula]);
            rc = RETURN_WARN;
        }
    }

done:
    if (w2) CloseWindow(w2);
    if (w1) CloseWindow(w1);
    if (superbm) FreeBitMap(superbm);
    return rc;
}
