/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Composited layers test.

    Opens a truecolor screen like the Workbench one and creates, with
    CreateUpfrontLayerTagList() directly (no windows), an ordinary smart
    refresh layer A and a composited layer B (LA_Composited: B renders into
    a surface of its own that the display's compositor presents). Both get a
    recognisable pattern; B is then moved, depth arranged and resized, then
    replaced by a half transparent one (LA_Opacity 128) over A.

    After every step the program prints what it did and PASS/FAIL lines for
    what can be checked from inside: the screen bitmap where A is uncovered
    (ReadPixelArray on the screen RastPort) and B's own content (read
    through B's RastPort, i.e. from its surface). The composed result
    (B on screen, blending) is only in the composed bitmap the display
    shows, so it is checked from screen captures taken DELAY seconds apart.

        A: red, 20 pixel yellow border, white vertical stripe
           at layer x 150..169, 300x200 at screen 100,100
        B: green, 10 pixel magenta border, blue cross
           250x200 at screen 250,200 (over the lower right of A)

    Usage: complayer [DELAY=seconds] [OPACITY=n] [NOCOMP]
        DELAY   seconds to wait after each step (default 3)
        OPACITY opacity of the second composited layer (default 128)
        NOCOMP  create B as an ordinary smart layer (reference run)
*/

#include <exec/types.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>

#define ARG_TEMPLATE "DELAY/N,OPACITY/N,NOCOMP/S"
enum { ARG_DELAY, ARG_OPACITY, ARG_NOCOMP, NUM_ARGS };

#define A_X0 100
#define A_Y0 100
#define A_W  300
#define A_H  200
#define B_X0 250
#define B_Y0 200
#define B_W  250
#define B_H  200

#define RED     0xFF0000
#define YELLOW  0xFFFF00
#define WHITE   0xFFFFFF
#define GREEN   0x00FF00
#define MAGENTA 0xFF00FF
#define BLUE    0x0000FF

static LONG delay = 3;
static int failures = 0;

static void pause_step(const char *what)
{
    printf("STEP: %s\n", what);
    fflush(stdout);
    Delay(delay * 50);
}

static ULONG read_rgb(struct RastPort *rp, LONG x, LONG y)
{
    UBYTE pix[4] = { 0, 0, 0, 0 };

    /* RECTFMT_ARGB is a byte order (A, R, G, B), not a native ULONG */
    ReadPixelArray(pix, 0, 0, 4, rp, x, y, 1, 1, RECTFMT_ARGB);
    return ((ULONG)pix[1] << 16) | ((ULONG)pix[2] << 8) | pix[3];
}

static void check(const char *what, struct RastPort *rp, LONG x, LONG y, ULONG want)
{
    ULONG got = read_rgb(rp, x, y);

    if (got == want)
        printf("PASS: %s at %ld,%ld = %06lx\n", what, (long)x, (long)y, (unsigned long)got);
    else
    {
        printf("FAIL: %s at %ld,%ld = %06lx, wanted %06lx\n", what, (long)x, (long)y,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void paint_a(struct RastPort *rp)
{
    FillPixelArray(rp, 0, 0, A_W, A_H, YELLOW);
    FillPixelArray(rp, 20, 20, A_W - 40, A_H - 40, RED);
    FillPixelArray(rp, 150, 20, 20, A_H - 40, WHITE);
}

static void paint_b(struct RastPort *rp, LONG w, LONG h)
{
    FillPixelArray(rp, 0, 0, w, h, MAGENTA);
    FillPixelArray(rp, 10, 10, w - 20, h - 20, GREEN);
    FillPixelArray(rp, w / 2 - 10, 10, 20, h - 20, BLUE);
    FillPixelArray(rp, 10, h / 2 - 10, w - 20, 20, BLUE);
}

static struct Layer *make_b(struct Layer_Info *li, struct BitMap *bm, LONG x0, LONG y0,
                            BOOL comp, ULONG opacity)
{
    struct TagItem tags[] =
    {
        { comp ? LA_Composited : TAG_IGNORE, TRUE    },
        { comp ? LA_Opacity    : TAG_IGNORE, opacity },
        { TAG_DONE                                   }
    };
    struct Layer *l = CreateUpfrontLayerTagList(li, bm, x0, y0, x0 + B_W - 1, y0 + B_H - 1,
                                                LAYERSMART, tags);

    if (l)
    {
        printf("B: layer %p flags 0x%x %s (opacity %lu)\n", l, l->Flags,
               (l->Flags & LAYERCOMPOSITED) ? "COMPOSITED" : "not composited",
               (unsigned long)opacity);
        paint_b(l->rp, B_W, B_H);
    }
    return l;
}

int main(void)
{
    IPTR args[NUM_ARGS] = { 0 };
    struct RDArgs *rda;
    struct Screen *scr;
    struct Layer_Info *li;
    struct BitMap *bm;
    struct Layer *a, *b;
    ULONG opacity = 128;
    BOOL comp = TRUE;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (rda)
    {
        if (args[ARG_DELAY])   delay   = *(LONG *)args[ARG_DELAY];
        if (args[ARG_OPACITY]) opacity = *(LONG *)args[ARG_OPACITY];
        if (args[ARG_NOCOMP])  comp    = FALSE;
    }

    scr = OpenScreenTags(NULL,
                         SA_LikeWorkbench, TRUE,
                         SA_Title,         (IPTR)"complayer",
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
    printf("screen %p %dx%d depth %lu, bitmap %p, composited layers %s\n", scr, scr->Width, scr->Height,
           (unsigned long)GetBitMapAttr(bm, BMA_DEPTH), bm, comp ? "requested" : "off (NOCOMP)");

    /* grey background so black backfill shows */
    FillPixelArray(&scr->RastPort, 0, 0, scr->Width, scr->Height, 0x606060);

    a = CreateUpfrontLayerTagList(li, bm, A_X0, A_Y0, A_X0 + A_W - 1, A_Y0 + A_H - 1, LAYERSMART, NULL);
    if (!a)
    {
        printf("FAIL: cannot create layer A\n");
        CloseScreen(scr);
        return 20;
    }
    paint_a(a->rp);
    printf("A: layer %p flags 0x%x at %d,%d\n", a, a->Flags, A_X0, A_Y0);

    b = make_b(li, bm, B_X0, B_Y0, comp, 255);
    if (!b)
    {
        printf("FAIL: cannot create layer B\n");
        DeleteLayer(0, a);
        CloseScreen(scr);
        return 20;
    }
    check("A uncovered (red)", &scr->RastPort, A_X0 + 60, A_Y0 + 60, RED);
    check("A white stripe", &scr->RastPort, A_X0 + 155, A_Y0 + 60, WHITE);
    check("B content (magenta border)", b->rp, 2, 2, MAGENTA);
    check("B content (green)", b->rp, 30, 30, GREEN);
    pause_step("0: A at 100,100 (300x200), B at 250,200 (250x200) over A's lower right");

    MoveLayer(0, b, 300, 0);
    check("A where B was (red)", &scr->RastPort, A_X0 + 200, A_Y0 + 150, RED);
    check("B content after move (blue cross)", b->rp, B_W / 2, 30, BLUE);
    pause_step("1: MoveLayer B +300,0 -> 550,200, A fully uncovered");

    MoveLayer(0, b, -100, 150);
    check("B content after move 2 (green)", b->rp, 30, 30, GREEN);
    pause_step("2: MoveLayer B -100,+150 -> 450,350");

    MoveLayer(0, b, -200, -150);
    check("A uncovered (red)", &scr->RastPort, A_X0 + 60, A_Y0 + 60, RED);
    check("B content after move 3 (magenta)", b->rp, 2, 2, MAGENTA);
    pause_step("3: MoveLayer B -200,-150 -> back to 250,200 over A");

    BehindLayer(0, b);
    check("A on top where B overlaps (red)", &scr->RastPort, A_X0 + 200, A_Y0 + 150, RED);
    check("B content behind A (green)", b->rp, 30, 30, GREEN);
    pause_step("4: BehindLayer B: A covers B's upper left, B visible below/right of A");

    UpfrontLayer(0, b);
    check("B content in front again (magenta)", b->rp, 2, 2, MAGENTA);
    pause_step("5: UpfrontLayer B");

    SizeLayer(0, b, 50, 50);
    check("B content kept after SizeLayer (blue cross)", b->rp, B_W / 2, 30, BLUE);
    check("B content kept after SizeLayer (green)", b->rp, 30, 30, GREEN);
    pause_step("6: SizeLayer B +50,+50 (new area black = LAYERS_BACKFILL)");

    DeleteLayer(0, b);
    check("A after B deleted (red)", &scr->RastPort, A_X0 + 200, A_Y0 + 150, RED);
    b = make_b(li, bm, B_X0, B_Y0, comp, opacity);
    if (b)
        check("B2 content (green)", b->rp, 30, 30, GREEN);
    pause_step("7: B deleted; B2 with LA_Opacity over A: A shows through B2");

    if (b)
    {
        MoveLayer(0, b, 100, 50);
        check("B2 content after move (magenta)", b->rp, 2, 2, MAGENTA);
        check("A under translucent B2 old place (red)", &scr->RastPort, A_X0 + 200, A_Y0 + 150, RED);
    }
    pause_step("8: MoveLayer B2 +100,+50 -> 350,250");

    if (b)
        DeleteLayer(0, b);
    DeleteLayer(0, a);
    pause_step("9: all layers deleted, grey screen");

    CloseScreen(scr);
    if (rda)
        FreeArgs(rda);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 5 : 0;
}
