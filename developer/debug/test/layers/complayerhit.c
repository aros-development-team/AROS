/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Alpha aware hit test of composited layers: WhichLayer() and
          IsLayerHitAt().

    On a truecolor screen like the Workbench one: smart refresh layer A
    (red, 300x200 at 100,100), composited layer B with a per pixel alpha
    surface (LA_AlphaSurface, 250x200 at 250,200, over A's lower right):
    opaque green with two fully transparent holes, one over A (layer
    relative 50..99 x 50..99) and one outside A (200..229 x 150..179), and
    a quarter transparent (alpha 0x40) patch at 10..39 x 150..179. C is a
    composited layer with constant opacity 128 (blue, 150x150 at 600,100).

    Expected: WhichLayer() returns B on its opaque and its quarter
    transparent pixels, A in the hole over A, neither in the hole outside
    A (falls through to the root layer / nothing), C anywhere in C.
    IsLayerHitAt() agrees. The screen's SA_CompositingLayers,
    SA_OpacitySupport and SA_SourceAlphaSupport are printed and checked
    against what the layers got.

    Usage: complayerhit [DELAY=seconds]
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

#define A_X0 100
#define A_Y0 100
#define A_W  300
#define A_H  200
#define B_X0 250
#define B_Y0 200
#define B_W  250
#define B_H  200
#define C_X0 600
#define C_Y0 100
#define C_W  150
#define C_H  150

static int failures = 0;

static void result(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static const char *lname(struct Layer *l, struct Layer *a, struct Layer *b, struct Layer *c)
{
    if (l == a) return "A";
    if (l == b) return "B";
    if (l == c) return "C";
    if (l == NULL) return "NULL";
    return "other (root)";
}

static void expect_which(struct Layer_Info *li, LONG x, LONG y, struct Layer *want, const char *what,
                         struct Layer *a, struct Layer *b, struct Layer *c)
{
    struct Layer *got;
    char buf[160];

    LockLayerInfo(li);
    got = WhichLayer(li, x, y);
    UnlockLayerInfo(li);

    snprintf(buf, sizeof(buf), "WhichLayer(%ld,%ld) = %s, wanted %s (%s)", (long)x, (long)y,
             lname(got, a, b, c), lname(want, a, b, c), what);
    result(got == want, buf);
}

static void expect_hit(struct Layer *l, LONG x, LONG y, BOOL want, const char *what,
                       struct Layer *a, struct Layer *b, struct Layer *c)
{
    BOOL got;
    char buf[160];

    LockLayerInfo(l->LayerInfo);
    got = IsLayerHitAt(l, x, y);
    UnlockLayerInfo(l->LayerInfo);

    snprintf(buf, sizeof(buf), "IsLayerHitAt(%s, %ld,%ld) = %s, wanted %s (%s)", lname(l, a, b, c),
             (long)x, (long)y, got ? "TRUE" : "FALSE", want ? "TRUE" : "FALSE", what);
    result((got ? TRUE : FALSE) == want, buf);
}

/* B's content as ARGB rows: opaque green, two alpha 0 holes, one alpha 0x40 patch */
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
            BOOL hole1 = (x >= 50 && x <= 99 && y >= 50 && y <= 99);
            BOOL hole2 = (x >= 200 && x <= 229 && y >= 150 && y <= 179);
            BOOL faint = (x >= 10 && x <= 39 && y >= 150 && y <= 179);

            row[x * 4 + 0] = (hole1 || hole2) ? 0x00 : (faint ? 0x40 : 0xFF);
            row[x * 4 + 1] = 0x00;
            row[x * 4 + 2] = (hole1 || hole2) ? 0x00 : 0xFF;
            row[x * 4 + 3] = faint ? 0xFF : 0x00;
        }
        WritePixelArray(row, 0, 0, B_W * 4, rp, 0, y, B_W, 1, RECTFMT_ARGB);
    }
    FreeVec(row);
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    LONG delay = 3;
    struct Screen *scr;
    struct Layer_Info *li;
    struct BitMap *bm;
    struct Layer *a, *b, *c;
    IPTR complayers = 0, opacity = 0, srcalpha = 0;
    BOOL bcomp, ccomp;
    char buf[160];

    struct TagItem btags[] =
    {
        { LA_Composited,   TRUE },
        { LA_AlphaSurface, TRUE },
        { TAG_DONE              }
    };
    struct TagItem ctags[] =
    {
        { LA_Composited, TRUE },
        { LA_Opacity,    128  },
        { TAG_DONE            }
    };

    rda = ReadArgs("DELAY/N", args, NULL);
    if (rda && args[0])
        delay = *(LONG *)args[0];

    scr = OpenScreenTags(NULL,
                         SA_LikeWorkbench, TRUE,
                         SA_Title,         (IPTR)"complayerhit",
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

    GetAttr(SA_CompositingLayers,  (Object *)scr, &complayers);
    GetAttr(SA_OpacitySupport,     (Object *)scr, &opacity);
    GetAttr(SA_SourceAlphaSupport, (Object *)scr, &srcalpha);
    printf("screen %dx%d depth %lu: SA_CompositingLayers %ld, SA_OpacitySupport %ld, SA_SourceAlphaSupport %ld\n",
           scr->Width, scr->Height, (unsigned long)GetBitMapAttr(bm, BMA_DEPTH),
           (long)complayers, (long)opacity, (long)srcalpha);

    FillPixelArray(&scr->RastPort, 0, 0, scr->Width, scr->Height, 0x606060);

    a = CreateUpfrontLayerTagList(li, bm, A_X0, A_Y0, A_X0 + A_W - 1, A_Y0 + A_H - 1, LAYERSMART, NULL);
    b = CreateUpfrontLayerTagList(li, bm, B_X0, B_Y0, B_X0 + B_W - 1, B_Y0 + B_H - 1, LAYERSMART, btags);
    c = CreateUpfrontLayerTagList(li, bm, C_X0, C_Y0, C_X0 + C_W - 1, C_Y0 + C_H - 1, LAYERSMART, ctags);
    if (!a || !b || !c)
    {
        printf("FAIL: cannot create layers (%p %p %p)\n", a, b, c);
        if (c) DeleteLayer(0, c);
        if (b) DeleteLayer(0, b);
        if (a) DeleteLayer(0, a);
        CloseScreen(scr);
        if (rda) FreeArgs(rda);
        return 20;
    }
    bcomp = (b->Flags & LAYERCOMPOSITED) ? TRUE : FALSE;
    ccomp = (c->Flags & LAYERCOMPOSITED) ? TRUE : FALSE;
    printf("A flags 0x%x, B flags 0x%x (%s), C flags 0x%x (%s)\n", a->Flags,
           b->Flags, bcomp ? "COMPOSITED" : "not composited", c->Flags, ccomp ? "COMPOSITED" : "not composited");

    FillPixelArray(a->rp, 0, 0, A_W, A_H, 0xFF0000);
    paint_b(b->rp);
    FillPixelArray(c->rp, 0, 0, C_W, C_H, 0x0000FF);

    /* the screen's capability attributes agree with what the layers got */
    snprintf(buf, sizeof(buf), "SA_CompositingLayers (%ld) matches LAYERCOMPOSITED of B (%d)", (long)complayers, bcomp);
    result((complayers ? TRUE : FALSE) == bcomp, buf);
    snprintf(buf, sizeof(buf), "SA_OpacitySupport (%ld) matches LAYERCOMPOSITED of C (%d)", (long)opacity, ccomp);
    result((opacity ? TRUE : FALSE) == ccomp, buf);
    /* a 24/32 bit deep composing screen has a spare byte for alpha (unless it is a packed 24 bit one) */
    if (bcomp && GetBitMapAttr(bm, BMA_DEPTH) >= 24 && srcalpha)
        result(TRUE, "SA_SourceAlphaSupport TRUE on a truecolor composing screen");
    else
        printf("NOTE: SA_SourceAlphaSupport %ld (depth %lu, composing %d; not checked)\n", (long)srcalpha,
               (unsigned long)GetBitMapAttr(bm, BMA_DEPTH), bcomp);

    /* hit tests that hold whatever the screen can do */
    expect_which(li, B_X0 + 10, B_Y0 + 10, b, "B opaque", a, b, c);
    expect_which(li, B_X0 + 50, B_Y0 + 20, b, "B opaque over A", a, b, c);
    expect_which(li, A_X0 + 60, A_Y0 + 60, a, "A uncovered", a, b, c);
    expect_which(li, C_X0 + 50, C_Y0 + 50, c, "C, constant opacity, hits everywhere", a, b, c);
    expect_hit(c, C_X0 + 50, C_Y0 + 50, TRUE, "constant opacity layer", a, b, c);
    expect_hit(a, A_X0 + 60, A_Y0 + 60, TRUE, "plain layer inside", a, b, c);
    expect_hit(a, A_X0 - 1, A_Y0 + 60, FALSE, "plain layer outside", a, b, c);
    expect_hit(b, 50, 50, FALSE, "B outside its bounds", a, b, c);
    expect_hit(b, B_X0 + 10, B_Y0 + 10, TRUE, "B opaque", a, b, c);
    expect_hit(a, B_X0 + 60, B_Y0 + 60, TRUE, "A under B's hole (layers in front ignored)", a, b, c);

    if (bcomp)
    {
        /* the alpha surface: holes miss, faint pixels hit */
        expect_which(li, B_X0 + 60, B_Y0 + 60, a, "hole in B over A -> A", a, b, c);
        expect_hit(b, B_X0 + 60, B_Y0 + 60, FALSE, "hole in B", a, b, c);
        expect_which(li, B_X0 + 20, B_Y0 + 160, b, "quarter transparent pixel of B still hits", a, b, c);
        expect_hit(b, B_X0 + 20, B_Y0 + 160, TRUE, "quarter transparent pixel of B", a, b, c);
        expect_hit(b, B_X0 + 210, B_Y0 + 160, FALSE, "hole in B outside A", a, b, c);
        {
            struct Layer *got;

            LockLayerInfo(li);
            got = WhichLayer(li, B_X0 + 210, B_Y0 + 160);
            UnlockLayerInfo(li);
            snprintf(buf, sizeof(buf), "WhichLayer in B's hole outside A = %s, wanted neither A, B nor C",
                     lname(got, a, b, c));
            result(got != a && got != b && got != c, buf);
        }
    }
    else
        printf("NOTE: screen cannot compose, alpha surface hit tests skipped\n");

    Delay(delay * 50);

    DeleteLayer(0, c);
    DeleteLayer(0, b);
    DeleteLayer(0, a);
    CloseScreen(scr);
    if (rda)
        FreeArgs(rda);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 5 : 0;
}
