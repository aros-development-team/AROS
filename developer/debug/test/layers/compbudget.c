/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Memory budget of composited layer surfaces and its fallback.

    layers.library lets the surfaces of a screen's composited layers take
    LAYERS_SURFACE_BUDGET_FACTOR (4) screen bitmaps together. A layer that
    does not fit is an ordinary smart refresh layer (no LAYERCOMPOSITED),
    and a composited layer whose growth does not fit is converted into one,
    content kept.

    Phase 1: screen sized composited layers are created until one falls
    back; the first FACTOR ones must be composited, the next one not, each
    one's content must read back through its RastPort, and the fallback
    one, being on top and in the screen bitmap, must show on the screen.

    Phase 2: FACTOR-1 screen sized layers plus two half height ones fill
    the budget exactly; growing the last one by a row converts it: no
    LAYERCOMPOSITED, new height, content kept, visible in the screen
    bitmap, the other layers untouched.

    Usage: compbudget [DELAY=seconds] [FACTOR=n]
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/layersext.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#define MAXLAYERS 8

static int failures = 0;
static LONG factor = 4;

static const ULONG colours[MAXLAYERS] =
{
    0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF, 0xFF8000, 0x8000FF
};

static void result(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static ULONG read_rgb(struct RastPort *rp, LONG x, LONG y)
{
    UBYTE pix[4] = { 0, 0, 0, 0 };

    /* RECTFMT_ARGB is a byte order (A, R, G, B), not a native ULONG */
    ReadPixelArray(pix, 0, 0, 4, rp, x, y, 1, 1, RECTFMT_ARGB);
    return ((ULONG)pix[1] << 16) | ((ULONG)pix[2] << 8) | pix[3];
}

static void check_rgb(const char *what, struct RastPort *rp, LONG x, LONG y, ULONG want)
{
    ULONG got = read_rgb(rp, x, y);
    char buf[160];

    snprintf(buf, sizeof(buf), "%s at %ld,%ld = %06lx%s", what, (long)x, (long)y, (unsigned long)got,
             got == want ? "" : " (wanted ");
    if (got != want)
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%06lx)", (unsigned long)want);
    result(got == want, buf);
}

static struct Layer *make(struct Layer_Info *li, struct BitMap *bm, LONG x0, LONG y0, LONG x1, LONG y1, int i)
{
    struct TagItem tags[] =
    {
        { LA_Composited, TRUE },
        { TAG_DONE            }
    };
    struct Layer *l = CreateUpfrontLayerTagList(li, bm, x0, y0, x1, y1, LAYERSMART, tags);

    if (l)
    {
        FillPixelArray(l->rp, 0, 0, l->Width, l->Height, colours[i]);
        printf("layer %d: %ldx%ld at %ld,%ld flags 0x%x %s\n", i, (long)l->Width, (long)l->Height, (long)x0, (long)y0,
               l->Flags, (l->Flags & LAYERCOMPOSITED) ? "COMPOSITED" : "not composited (fallback)");
    }
    else
        printf("layer %d: not created\n", i);
    return l;
}

int main(void)
{
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda;
    LONG delay = 3;
    struct Screen *scr;
    struct Layer_Info *li;
    struct BitMap *bm;
    struct Layer *l[MAXLAYERS];
    LONG w, h, depth, bpp, hh, grow;
    int i, n, ncomp, nfull;
    char buf[160];

    rda = ReadArgs("DELAY/N,FACTOR/N", args, NULL);
    if (rda)
    {
        if (args[0]) delay  = *(LONG *)args[0];
        if (args[1]) factor = *(LONG *)args[1];
    }
    if (factor < 1 || factor > MAXLAYERS - 2)
        factor = 4;

    scr = OpenScreenTags(NULL,
                         SA_LikeWorkbench, TRUE,
                         SA_Title,         (IPTR)"compbudget",
                         SA_ShowTitle,     FALSE,
                         SA_Quiet,         TRUE,
                         TAG_DONE);
    if (!scr)
    {
        printf("FAIL: cannot open screen\n");
        if (rda) FreeArgs(rda);
        return 20;
    }
    li = &scr->LayerInfo;
    bm = scr->RastPort.BitMap;
    w = GetBitMapAttr(bm, BMA_WIDTH);
    h = GetBitMapAttr(bm, BMA_HEIGHT);
    depth = GetBitMapAttr(bm, BMA_DEPTH);
    bpp = (depth <= 8) ? 1 : (depth <= 16) ? 2 : 4;   /* as layers.library counts */
    printf("screen %ldx%ld depth %ld (%ld bytes/pixel, %ld bytes), budget factor %ld -> %ld bytes, "
           "largest free block %lu\n", (long)w, (long)h, (long)depth, (long)bpp, (long)(w * h * bpp),
           (long)factor, (long)(factor * w * h * bpp), (unsigned long)AvailMem(MEMF_ANY | MEMF_LARGEST));

    FillPixelArray(&scr->RastPort, 0, 0, w, h, 0x606060);

    /* Phase 1: screen sized layers until the budget is used up */
    printf("Phase 1: %ld screen sized composited layers, then one more\n", (long)factor);
    n = 0;
    ncomp = 0;
    for (i = 0; i < factor + 1; i++)
    {
        l[i] = make(li, bm, 0, 0, w - 1, h - 1, i);
        if (!l[i])
            break;
        n++;
        if (l[i]->Flags & LAYERCOMPOSITED)
            ncomp++;
        snprintf(buf, sizeof(buf), "layer %d content (own RastPort)", i);
        check_rgb(buf, l[i]->rp, 10, 10, colours[i]);
    }
    if (0 == ncomp)
    {
        printf("NOTE: screen cannot compose (first layer not composited), budget not testable here\n");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%d of %d screen sized layers composited, wanted %ld (the budget)", ncomp, n,
                 (long)factor);
        result(ncomp == factor, buf);
        result(n == factor + 1 && !(l[factor]->Flags & LAYERCOMPOSITED),
               "the layer beyond the budget is an ordinary smart refresh layer (no LAYERCOMPOSITED)");
        for (i = 0; i < ncomp; i++)
            if (!(l[i]->Flags & LAYERCOMPOSITED))
            {
                snprintf(buf, sizeof(buf), "layer %d within the budget is composited", i);
                result(FALSE, buf);
            }
        if (n == factor + 1)
            check_rgb("fallback layer on top shows in the screen bitmap", &scr->RastPort, 10, 10, colours[factor]);
    }
    Delay(delay * 50);
    for (i = n - 1; i >= 0; i--)
        DeleteLayer(0, l[i]);
    /* no SA_BackFill: nothing repaints the screen behind deleted layers, do it here */
    FillPixelArray(&scr->RastPort, 0, 0, w, h, 0x606060);
    check_rgb("screen repainted after deleting all layers", &scr->RastPort, 10, 10, 0x606060);

    if (0 == ncomp)
        goto done;

    /*
     * Phase 2: fill the budget exactly (factor-1 screen sized layers, two
     * of half the height, overlapping by 10 rows), then grow the last one.
     */
    hh = h / 2;
    grow = (h % 2) + 1;   /* one row more than fits */
    nfull = factor - 1;
    printf("Phase 2: %d screen sized layers + two %ldx%ld layers fill the budget; grow the last one by %ld row(s)\n",
           nfull, (long)w, (long)hh, (long)grow);
    n = 0;
    for (i = 0; i < nfull; i++)
    {
        l[i] = make(li, bm, 0, 0, w - 1, h - 1, i);
        if (!l[i]) break;
        n++;
    }
    if (n == nfull)
    {
        l[n] = make(li, bm, 0, 0, w - 1, hh - 1, n);
        if (l[n]) n++;
    }
    if (n == nfull + 1)
    {
        l[n] = make(li, bm, 0, hh - 10, w - 1, hh - 10 + hh - 1, n);
        if (l[n]) n++;
    }
    if (n != nfull + 2)
    {
        result(FALSE, "phase 2 layers created");
    }
    else
    {
        struct Layer *last = l[n - 1], *prev = l[n - 2];
        LONG oldh = last->Height;

        result((last->Flags & LAYERCOMPOSITED) && (prev->Flags & LAYERCOMPOSITED),
               "both half height layers composited (budget exactly full)");

        SizeLayer(0, last, 0, grow);
        snprintf(buf, sizeof(buf), "grown layer height %ld, wanted %ld", (long)last->Height, (long)(oldh + grow));
        result(last->Height == oldh + grow, buf);
        result(!(last->Flags & LAYERCOMPOSITED), "grown layer converted to an ordinary smart refresh layer");
        result((prev->Flags & LAYERCOMPOSITED) != 0, "the other half height layer is still composited");
        check_rgb("grown layer content kept (own RastPort)", last->rp, 10, 10, colours[n - 1]);
        check_rgb("grown layer content kept near the old bottom", last->rp, 10, oldh - 2, colours[n - 1]);
        check_rgb("grown layer shows in the screen bitmap", &scr->RastPort, 10, hh - 10 + 10, colours[n - 1]);
        check_rgb("still composited layer content", prev->rp, 10, 10, colours[n - 2]);

        /* a grown ordinary layer keeps working: shrink, grow again, paint */
        SizeLayer(0, last, 0, -grow);
        SizeLayer(0, last, 0, grow);
        FillPixelArray(last->rp, 0, 0, last->Width, last->Height, 0xFFFFFF);
        check_rgb("converted layer repainted (own RastPort)", last->rp, 10, last->Height - 1, 0xFFFFFF);
        check_rgb("converted layer repainted (screen bitmap)", &scr->RastPort, 10, hh - 10 + 10, 0xFFFFFF);
    }
    Delay(delay * 50);
    for (i = n - 1; i >= 0; i--)
        DeleteLayer(0, l[i]);
    FillPixelArray(&scr->RastPort, 0, 0, w, h, 0x606060);
    check_rgb("screen repainted after phase 2", &scr->RastPort, 10, 10, 0x606060);

done:
    CloseScreen(scr);
    if (rda)
        FreeArgs(rda);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 5 : 0;
}
