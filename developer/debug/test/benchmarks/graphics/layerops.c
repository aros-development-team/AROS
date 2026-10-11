/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Timings of layers.library operations with a stack of overlapping
          smart refresh windows: moving, depth arranging, clip regions,
          BeginUpdate/EndUpdate, ScrollRaster and pixel/rect rendering into
          a window with many ClipRects.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <graphics/clip.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>

#include <stdio.h>

#define NWINMAX 12
static int NWIN = NWINMAX;

static ULONG t_s, t_u;
static void tstart(void) { CurrentTime(&t_s, &t_u); }
static void tend(const char *what, ULONG iters)
{
    ULONG s, u; double us;
    CurrentTime(&s, &u);
    us = (double)(s - t_s) * 1000000.0 + ((double)u - (double)t_u);
    printf("  %-56s %9.2f us/iter (%lu iters)\n", what, us / iters, (unsigned long)iters);
}

static LONG countcr(struct Layer *L)
{
    struct ClipRect *cr; LONG n = 0;
    LockLayer(0, L);
    for (cr = L->ClipRect; cr; cr = cr->Next) n++;
    UnlockLayer(L);
    return n;
}

int main(int argc, char **argv)
{
    struct Window *w[NWINMAX];
    struct Window *rear;
    struct Region *reg;
    struct Rectangle rect;
    ULONG i, n, k;

    if (argc > 1) { NWIN = argv[1][0] - '0'; if (argv[1][1]) NWIN = NWIN * 10 + argv[1][1] - '0'; if (NWIN > NWINMAX) NWIN = NWINMAX; }

    /* a rear window covered by a grid of small windows: many ClipRects */
    rear = OpenWindowTags(NULL, WA_Left, 20, WA_Top, 20, WA_Width, 600, WA_Height, 400,
            WA_Title, (IPTR)"layerops rear", WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_Activate, FALSE, TAG_DONE);
    for (i = 0; i < NWIN; i++)
        w[i] = OpenWindowTags(NULL, WA_Left, 40 + (i % 4) * 150, WA_Top, 50 + (i / 4) * 120,
                WA_Width, 120, WA_Height, 90, WA_Title, (IPTR)"cover", WA_SmartRefresh, TRUE,
                WA_DragBar, TRUE, WA_Activate, FALSE, TAG_DONE);
    if (!rear) return 20;
    Delay(10);
    {
        struct Layer *L; LONG total = 0, hidden = 0; struct ClipRect *cr;
        LockLayerInfo(&rear->WScreen->LayerInfo);
        for (L = rear->WScreen->LayerInfo.top_layer; L; L = L->back)
            for (cr = L->ClipRect; cr; cr = cr->Next) { total++; if (cr->BitMap) hidden++; }
        UnlockLayerInfo(&rear->WScreen->LayerInfo);
        printf("layerops: %d covering windows; rear window has %ld ClipRects; screen total %ld ClipRects, %ld with a backing bitmap\n",
               NWIN, (long)countcr(rear->RPort->Layer), (long)total, (long)hidden);
    }

    /* the primitives layers uses per hidden ClipRect */
    {
        struct BitMap *bm; ULONG depth = GetBitMapAttr(rear->RPort->BitMap, BMA_DEPTH);
        n = 2000;
        tstart();
        for (i = 0; i < n; i++) { bm = AllocBitMap(136, 90, depth, BMF_CLEAR, rear->RPort->BitMap); FreeBitMap(bm); }
        tend("AllocBitMap(136x90, BMF_CLEAR, friend) + FreeBitMap", n);
        tstart();
        for (i = 0; i < n; i++) { bm = AllocBitMap(136, 90, depth, 0, rear->RPort->BitMap); FreeBitMap(bm); }
        tend("AllocBitMap(136x90, no clear, friend) + FreeBitMap", n);
        bm = AllocBitMap(136, 90, depth, 0, rear->RPort->BitMap);
        tstart();
        for (i = 0; i < n; i++) BltBitMap(rear->RPort->BitMap, 40, 50, bm, 8, 0, 120, 90, 0xC0, 0xFF, NULL);
        tend("BltBitMap 120x90 screen -> backing bitmap", n);
        tstart();
        for (i = 0; i < n; i++) BltBitMap(bm, 8, 0, rear->RPort->BitMap, 40, 50, 120, 90, 0xC0, 0xFF, NULL);
        tend("BltBitMap 120x90 backing bitmap -> screen", n);
        FreeBitMap(bm);
    }
    {
        struct Region *a = NewRegion(), *b = NewRegion(); struct Rectangle rr;
        LockLayer(0, rear->RPort->Layer);
        OrRegionRegion(rear->RPort->Layer->VisibleRegion, a);
        UnlockLayer(rear->RPort->Layer);
        rr.MinX = 100; rr.MinY = 80; rr.MaxX = 500; rr.MaxY = 380; OrRectRegion(b, &rr);
        n = 20000;
        tstart();
        for (i = 0; i < n; i++) { struct Region *c = AndRegionRegionND(a, b); DisposeRegion(c); }
        tend("AndRegionRegionND(rear VisibleRegion, rect region)", n);
        tstart();
        for (i = 0; i < n; i++) { struct Region *c = NewRegion(); OrRegionRegion(a, c); DisposeRegion(c); }
        tend("copy of the rear VisibleRegion (NewRegion+OrRegionRegion)", n);
        DisposeRegion(a); DisposeRegion(b);
    }

    n = 200;
    tstart();
    for (i = 0; i < n; i++)
        MoveLayer(0, w[NWIN / 2]->RPort->Layer, (i & 1) ? -37 : 37, (i & 1) ? -23 : 23);
    tend("MoveLayer of a middle window (both directions)", n);

    tstart();
    for (i = 0; i < n; i++)
        MoveLayer(0, rear->RPort->Layer, (i & 1) ? -5 : 5, 0);
    tend("MoveLayer of the covered rear window", n);

    tstart();
    for (i = 0; i < n; i++) { BehindLayer(0, w[NWIN / 2]->RPort->Layer); UpfrontLayer(0, w[NWIN / 2]->RPort->Layer); }
    tend("BehindLayer + UpfrontLayer of a middle window", n);

    tstart();
    for (i = 0; i < n; i++) { UpfrontLayer(0, rear->RPort->Layer); BehindLayer(0, rear->RPort->Layer); }
    tend("UpfrontLayer + BehindLayer of the rear window", n);

    reg = NewRegion();
    rect.MinX = 30; rect.MinY = 30; rect.MaxX = 400; rect.MaxY = 300;
    OrRectRegion(reg, &rect);
    rect.MinX = 100; rect.MinY = 100; rect.MaxX = 200; rect.MaxY = 200;
    ClearRectRegion(reg, &rect);
    tstart();
    for (i = 0; i < n; i++) { InstallClipRegion(rear->RPort->Layer, reg); InstallClipRegion(rear->RPort->Layer, NULL); }
    tend("InstallClipRegion install + remove on rear window", n);

    tstart();
    for (i = 0; i < n; i++) { BeginUpdate(rear->RPort->Layer); EndUpdate(rear->RPort->Layer, TRUE); }
    tend("BeginUpdate + EndUpdate on rear window", n);

    tstart();
    for (i = 0; i < n; i++)
        ScrollRaster(rear->RPort, (i & 1) ? -3 : 3, 0, 20, 30, 580, 380);
    tend("ScrollRaster 560x350 in the covered rear window", n);

    n = 20000;
    SetAPen(rear->RPort, 1);
    tstart();
    for (i = 0; i < n; i++)
        WritePixel(rear->RPort, 25 + (i % 550), 40 + (i % 350));
    tend("WritePixel into the covered rear window", n);

    tstart();
    for (i = 0; i < n; i++)
        WritePixel(w[0]->RPort, 10 + (i % 100), 20 + (i % 60));
    tend("WritePixel into a small uncovered window", n);

    n = 5000;
    tstart();
    for (i = 0; i < n; i++)
        RectFill(rear->RPort, 25 + (i % 500), 40 + (i % 300), 25 + (i % 500) + 15, 40 + (i % 300) + 7);
    tend("RectFill 16x8 into the covered rear window", n);

    tstart();
    for (i = 0; i < n; i++)
        RectFill(w[0]->RPort, 10 + (i % 90), 20 + (i % 50), 10 + (i % 90) + 15, 20 + (i % 50) + 7);
    tend("RectFill 16x8 into a small uncovered window", n);

    n = 100000;
    tstart();
    for (i = 0; i < n; i++) { LockLayer(0, rear->RPort->Layer); UnlockLayer(rear->RPort->Layer); }
    tend("LockLayer + UnlockLayer", n);

    n = 100000;
    tstart();
    for (i = 0; i < n; i++)
        k = (ULONG)(IPTR)WhichLayer(&rear->WScreen->LayerInfo, 25 + (i % 500), 40 + (i % 300));
    tend("WhichLayer over the stack", n);
    (void)k;

    DisposeRegion(reg);
    for (i = 0; i < NWIN; i++) if (w[i]) CloseWindow(w[i]);
    CloseWindow(rear);
    return 0;
}
