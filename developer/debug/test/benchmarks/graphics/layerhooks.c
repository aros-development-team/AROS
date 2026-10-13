/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Timings for the operations layer hook functions typically do
          (RastPort cloning, layer locking, walking the ClipRect list,
          WritePixelArray() through a layered and a layerless RastPort),
          plus decorator.library's shade hook end to end. The rear window
          is partly covered so that its layer has hidden ClipRects.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>
#include <proto/decorator.h>
#include <proto/arossupport.h>

#include <aros/asmcall.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>
#include <libraries/decorator.h>

#include <stdio.h>
#include <stdlib.h>

static ULONG t_start_s, t_start_u;

static void tstart(void)
{
    CurrentTime(&t_start_s, &t_start_u);
}

/* print microseconds per iteration */
static void tend(const char *what, ULONG iters)
{
    ULONG s, u;
    double us;

    CurrentTime(&s, &u);
    us = (double)(s - t_start_s) * 1000000.0 + ((double)u - (double)t_start_u);
    printf("  %-58s %9.3f us/iter  (%lu iters, %.0f ms)\n", what, us / iters, (unsigned long)iters, us / 1000.0);
}

AROS_UFH3(void, EmptyHook,
    AROS_UFHA(struct Hook *,     h,   A0),
    AROS_UFHA(struct RastPort *, rp,  A2),
    AROS_UFHA(APTR,              msg, A1))
{
    AROS_USERFUNC_INIT
    AROS_USERFUNC_EXIT
}

int main(void)
{
    struct Window *w1, *w2;
    struct Rectangle rect, visrect;
    struct RastPort temprp;
    struct Hook hook;
    struct DecorImage *di;
    ULONG *src;
    ULONG i, n, w, h;
    LONG  crs = 0;
    struct ClipRect *cr;

    w1 = OpenWindowTags(NULL, WA_Left, 60, WA_Top, 60, WA_Width, 320, WA_Height, 200,
            WA_Title, (IPTR)"layerhooks rear", WA_SmartRefresh, TRUE, WA_DragBar, TRUE,
            WA_Activate, FALSE, TAG_DONE);
    w2 = OpenWindowTags(NULL, WA_Left, 220, WA_Top, 180, WA_Width, 320, WA_Height, 200,
            WA_Title, (IPTR)"layerhooks front", WA_SmartRefresh, TRUE, WA_DragBar, TRUE,
            WA_Activate, FALSE, TAG_DONE);
    if (!w1 || !w2) { printf("could not open the windows\n"); goto done; }
    Delay(10);

    /* band in the rear window's inner area, spanning its visible and covered parts */
    rect.MinX = w1->BorderLeft + 2;
    rect.MaxX = w1->Width - w1->BorderRight - 3;
    rect.MaxY = w1->Height - w1->BorderBottom - 3;
    rect.MinY = rect.MaxY - 19;
    w = rect.MaxX - rect.MinX + 1;
    h = rect.MaxY - rect.MinY + 1;

    /* the part of it that is on screen: left of the front window */
    visrect = rect;
    visrect.MaxX = (w2->LeftEdge - w1->LeftEdge) - 1;

    for (cr = w1->RPort->Layer->ClipRect; cr; cr = cr->Next) crs++;
    printf("layerhooks: rect %dx%d, rear layer has %ld ClipRects, depth %lu\n", (int)w, (int)h, (long)crs,
           (unsigned long)GetBitMapAttr(w1->RPort->BitMap, BMA_DEPTH));

    n = 200000;
    tstart();
    for (i = 0; i < n; i++)
    {
        struct RastPort *c = CloneRastPort(w1->RPort);
        c->Layer = NULL;
        FreeRastPort(c);
    }
    tend("CloneRastPort + FreeRastPort", n);

    tstart();
    for (i = 0; i < n; i++)
    {
        temprp = *w1->RPort;
        temprp.Layer = NULL;
        asm volatile("" :: "r"(&temprp) : "memory");
    }
    tend("RastPort copy on the stack", n);

    tstart();
    for (i = 0; i < n; i++)
    {
        LockLayer(0, w1->RPort->Layer);
        UnlockLayer(w1->RPort->Layer);
    }
    tend("LockLayer + UnlockLayer", n);

    tstart();
    for (i = 0; i < n; i++)
    {
        struct BitMap *want = w1->RPort->BitMap;
        LONG found = 0;
        for (cr = w1->RPort->Layer->ClipRect; cr; cr = cr->Next)
            if (cr->BitMap == want) found++;
        asm volatile("" :: "r"(found) : "memory");
    }
    tend("walk the layer's ClipRect list", n);

    hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(EmptyHook);
    hook.h_Data  = NULL;
    n = 20000;
    tstart();
    for (i = 0; i < n; i++)
        DoHookClipRects(&hook, w1->RPort, &rect);
    tend("DoHookClipRects with an empty hook", n);

    src = AllocVec(w * h * 4, MEMF_ANY);
    if (!src) goto done;
    for (i = 0; i < w * h; i++) src[i] = 0xFF000000 | (i * 2654435761u);

    n = 2000;
    w = visrect.MaxX - visrect.MinX + 1;
    tstart();
    for (i = 0; i < n; i++)
        WritePixelArray(src, 0, 0, w * 4, w1->RPort, visrect.MinX, visrect.MinY, w, h, RECTFMT_ARGB);
    tend("WritePixelArray, layered RastPort (visible part)", n);

    temprp = *w1->RPort;
    temprp.Layer = NULL;
    tstart();
    for (i = 0; i < n; i++)
        WritePixelArray(src, 0, 0, w * 4, &temprp, visrect.MinX + w1->LeftEdge, visrect.MinY + w1->TopEdge, w, h, RECTFMT_ARGB);
    tend("WritePixelArray, layerless RastPort copy (same area)", n);
    w = rect.MaxX - rect.MinX + 1;

    /* decorator.library's shade hook over the whole band (visible + hidden) */
    di = DNewDecorImageContainer(64, 64);
    if (di)
    {
        for (i = 0; i < 64 * 64; i++) di->data[i] = 0xFF000000 | (i * 2654435761u);
        di->ok = TRUE;

        n = 500;
        tstart();
        for (i = 0; i < n; i++)
            DShadeLine(1, TRUE, FALSE, w1->RPort, di, 0, 256, 0, &rect);
        tend("DShadeLine over the band (visible + hidden ClipRects)", n);

        /* and a 1 pixel wide vertical line, the other buffered path */
        {
            struct Rectangle vline = rect;
            vline.MaxX = vline.MinX;
            vline.MinY = w1->BorderTop + 2;
            n = 5000;
            tstart();
            for (i = 0; i < n; i++)
                DShadeLine(1, TRUE, FALSE, w1->RPort, di, 0, 256, 0, &vline);
            tend("DShadeLine, 1 pixel wide vertical line", n);
        }
        DDisposeImageContainer(di);
    }
    else
        printf("  no DecorImage\n");

    FreeVec(src);
done:
    if (w2) CloseWindow(w2);
    if (w1) CloseWindow(w1);
    return 0;
}
