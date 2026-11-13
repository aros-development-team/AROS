/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Input shapes (LA_InputShape, ChangeLayerInputShape()): where a
          layer takes input, apart from where it is seen.

    On a screen like the Workbench one: smart refresh layer A (300x200 at
    100,100); in front of it layer B (240x160 at 130,120), composited with
    a per pixel alpha surface where the screen can: a 30 pixel translucent
    rim (alpha 0x50, a soft shadow) around an opaque inner part, created
    with an input shape of only the inner part (layer relative 30..209 x
    30..129). D is an ordinary layer (no composition, 100x100 at 500,100)
    with an input shape of its left half; E is a composited constant
    opacity layer (100x100 at 500,250) with an empty input shape.

    Expected:
    - B's rim (translucent, alpha not 0) goes to A: the input shape decides.
    - B's inner part hits B.
    - ChangeLayerInputShape(B, NULL) restores the default: the rim hits B
      again (alpha not 0); a new inner-only shape takes it away again.
    - The caller's region may be disposed after the call: B keeps its copy.
    - After SizeLayer() B keeps its input shape in layer coordinates: the
      new area outside it goes to A.
    - D (no composition) behaves the same: right half misses, left half hits.
    - E takes no input anywhere (empty region).

    Usage: layerinputshape [DELAY=seconds]
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/layersext.h>
#include <graphics/rastport.h>
#include <graphics/regions.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>

#define A_X0 100
#define A_Y0 100
#define B_X0 130
#define B_Y0 120
#define B_W  240
#define B_H  160
#define RIM  30

static int failures = 0;

static void result(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static struct Layer *which(struct Layer_Info *li, LONG x, LONG y)
{
    struct Layer *l;

    LockLayerInfo(li);
    l = WhichLayer(li, x, y);
    UnlockLayerInfo(li);
    return l;
}

static void expect(struct Layer_Info *li, LONG x, LONG y, struct Layer *want, const char *what)
{
    char buf[160];
    struct Layer *got = which(li, x, y);

    snprintf(buf, sizeof(buf), "WhichLayer(%ld,%ld) %s (%s)", (long)x, (long)y,
             got == want ? "as wanted" : "is another layer", what);
    result(got == want, buf);
}

static struct Region *rect_region(WORD x0, WORD y0, WORD x1, WORD y1)
{
    struct Region *r = NewRegion();
    struct Rectangle rect = { x0, y0, x1, y1 };

    if (r && !OrRectRegion(r, &rect))
    {
        DisposeRegion(r);
        r = NULL;
    }
    return r;
}

/* B: translucent rim, opaque inside */
static void paint_b(struct RastPort *rp)
{
    UBYTE *row = AllocVec(B_W * 4, MEMF_ANY);
    LONG x, y;

    if (!row)
        return;
    for (y = 0; y < B_H; y++)
    {
        for (x = 0; x < B_W; x++)
        {
            BOOL inner = x >= RIM && x < B_W - RIM && y >= RIM && y < B_H - RIM;

            row[x * 4 + 0] = inner ? 0xFF : 0x50;
            row[x * 4 + 1] = inner ? 0x30 : 0x00;
            row[x * 4 + 2] = inner ? 0x80 : 0x00;
            row[x * 4 + 3] = inner ? 0xC0 : 0x00;
        }
        WritePixelArray(row, 0, 0, B_W * 4, rp, 0, y, B_W, 1, RECTFMT_ARGB);
    }
    FreeVec(row);
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    LONG delay = 2;
    struct Screen *scr;
    struct Layer_Info *li;
    struct BitMap *bm;
    struct Layer *a = NULL, *b = NULL, *d = NULL, *e = NULL;
    struct Region *inner, *left, *empty;
    BOOL bcomp;

    rda = ReadArgs("DELAY/N", args, NULL);
    if (rda && args[0])
        delay = *(LONG *)args[0];

    scr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_Title, (IPTR)"layerinputshape",
                         SA_ShowTitle, FALSE, SA_Quiet, TRUE, TAG_DONE);
    if (!scr)
    {
        printf("FAIL: cannot open screen\n");
        if (rda) FreeArgs(rda);
        return 20;
    }
    li = &scr->LayerInfo;
    bm = scr->RastPort.BitMap;
    FillPixelArray(&scr->RastPort, 0, 0, scr->Width, scr->Height, 0x606060);

    inner = rect_region(RIM, RIM, B_W - RIM - 1, B_H - RIM - 1);
    left = rect_region(0, 0, 49, 99);
    empty = NewRegion();
    if (inner && left && empty)
    {
        struct TagItem btags[] =
        {
            { LA_Composited,   TRUE          },
            { LA_AlphaSurface, TRUE          },
            { LA_InputShape,   (IPTR)inner   },
            { TAG_DONE                       }
        };
        struct TagItem dtags[] = { { LA_InputShape, (IPTR)left }, { TAG_DONE } };
        struct TagItem etags[] =
        {
            { LA_Composited, TRUE        },
            { LA_Opacity,    128         },
            { LA_InputShape, (IPTR)empty },
            { TAG_DONE                   }
        };

        a = CreateUpfrontLayerTagList(li, bm, A_X0, A_Y0, A_X0 + 299, A_Y0 + 199, LAYERSMART, NULL);
        b = CreateUpfrontLayerTagList(li, bm, B_X0, B_Y0, B_X0 + B_W - 1, B_Y0 + B_H - 1, LAYERSMART, btags);
        d = CreateUpfrontLayerTagList(li, bm, 500, 100, 599, 199, LAYERSMART, dtags);
        e = CreateUpfrontLayerTagList(li, bm, 500, 250, 599, 349, LAYERSMART, etags);
    }
    /* The layers have copies of their own */
    if (inner) DisposeRegion(inner);
    if (left) DisposeRegion(left);
    if (empty) DisposeRegion(empty);

    if (!a || !b || !d || !e)
    {
        printf("FAIL: cannot create layers\n");
        if (e) DeleteLayer(0, e);
        if (d) DeleteLayer(0, d);
        if (b) DeleteLayer(0, b);
        if (a) DeleteLayer(0, a);
        CloseScreen(scr);
        if (rda) FreeArgs(rda);
        return 20;
    }
    bcomp = (b->Flags & LAYERCOMPOSITED) ? TRUE : FALSE;
    printf("B %s, E %s\n", bcomp ? "composited" : "not composited",
           (e->Flags & LAYERCOMPOSITED) ? "composited" : "not composited");
    FillPixelArray(a->rp, 0, 0, 300, 200, 0xFF0000);
    paint_b(b->rp);
    FillPixelArray(d->rp, 0, 0, 100, 100, 0x00FF00);
    FillPixelArray(e->rp, 0, 0, 100, 100, 0x0000FF);

    /* The rim is seen (alpha 0x50 where composited) but takes no input */
    expect(li, B_X0 + 10, B_Y0 + 10, a, "B's rim, outside its input shape, goes to A");
    expect(li, B_X0 + B_W / 2, B_Y0 + B_H / 2, b, "B's inner part");
    {
        BOOL hit;

        LockLayerInfo(li);
        hit = IsLayerHitAt(b, B_X0 + 10, B_Y0 + 10);
        UnlockLayerInfo(li);
        result(!hit, "IsLayerHitAt(B, rim) is FALSE");
    }

    /* Back to the default, then the inner part again */
    result(ChangeLayerInputShape(b, NULL), "ChangeLayerInputShape(B, NULL)");
    if (bcomp)
        expect(li, B_X0 + 10, B_Y0 + 10, b, "default again: the rim (alpha not 0) hits B");
    else
        expect(li, B_X0 + 10, B_Y0 + 10, b, "default again: an ordinary layer is hit everywhere");
    {
        struct Region *r = rect_region(RIM, RIM, B_W - RIM - 1, B_H - RIM - 1);

        result(r && ChangeLayerInputShape(b, r), "ChangeLayerInputShape(B, inner part)");
        if (r) DisposeRegion(r);
    }
    expect(li, B_X0 + 10, B_Y0 + 10, a, "inner part again: rim goes to A");

    /* A size change keeps the shape in layer coordinates */
    LockLayers(li);
    result(SizeLayer(0, b, 40, 0), "SizeLayer(B, +40)");
    UnlockLayers(li);
    expect(li, B_X0 + B_W + 20, B_Y0 + B_H / 2, a, "B's new area, outside the kept input shape, goes to A");
    expect(li, B_X0 + B_W / 2, B_Y0 + B_H / 2, b, "B's inner part after the size change");

    /* No composition needed */
    expect(li, 520, 150, d, "D (not composited), left half inside its input shape");
    {
        struct Layer *got = which(li, 580, 150);
        result(got != d, "D's right half, outside its input shape, is not D");
    }

    /* Empty region: nowhere */
    {
        struct Layer *got = which(li, 550, 300);
        result(got != e, "E with an empty input shape takes no input");
    }

    Delay(delay * 50);

    DeleteLayer(0, e);
    DeleteLayer(0, d);
    DeleteLayer(0, b);
    DeleteLayer(0, a);
    CloseScreen(scr);
    if (rda)
        FreeArgs(rda);

    printf("%s: layerinputshape, %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 5 : 0;
}
