/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Opens two windows on the Workbench screen: an ordinary one and,
          partly over it, one with WA_Opaqueness (composited layer, see
          layers.library LA_Composited). Reports the layer flags and whether
          the composited window kept its content where the two overlap,
          and that the screen's display composes while it is open (the
          window's surface reaches the display only through composition).
          It also reads the composed display where the composited window's
          content lies over the plain window's red, and checks that the two
          are blended there by the window's opacity (scaled by the content's
          alpha for ALPHA).
          ALPHASCREEN: the same on a screen whose bitmap has an alpha channel
          (ARGB32, SA_CompositingFlags COMPF_ALPHA, as Tests/graphics/
          alphascreen) instead of the Workbench screen. The composited window's
          content is drawn with a pen there, which leaves alpha 0 in the
          surface: a constant opacity window must still show at its opacity.
          ALPHAFORMAT opens them on a screen like the Workbench's but with
          an alpha pixel format (SA_PixelFormat) and checks it starts opaque
          and that pens, text and alpha-less blits land opaque in it.

    Usage: compwindow [DELAY=seconds] [OPACITY=n] [ALPHA] [ALPHASCREEN] [ALPHAFORMAT]
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/oop.h>
#include <intuition/intuition.h>
#include <hidd/gfx.h>
#include <graphics/clip.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>

OOP_AttrBase HiddBitMapAttrBase, HiddDisplayAttrBase, HiddCompositorAttrBase;

/*
 * Is the display of the screen composing? A composited layer is only seen
 * on the display through composition; -1 when this cannot be told (no HIDD
 * bitmap or no compositor).
 */
static int display_composing(struct Screen *scr)
{
    OOP_Object *bm, *display = NULL, *compositor = NULL;
    IPTR active = 0;

    if (!IS_HIDD_BM(scr->RastPort.BitMap) || !HiddBitMapAttrBase || !HiddDisplayAttrBase || !HiddCompositorAttrBase)
        return -1;
    bm = HIDD_BM_OBJ(scr->RastPort.BitMap);
    OOP_GetAttr(bm, aHidd_BitMap_Display, (IPTR *)&display);
    if (display)
        OOP_GetAttr(display, aHidd_Display_Compositor, (IPTR *)&compositor);
    if (!compositor)
        return -1;
    OOP_GetAttr(compositor, aHidd_Compositor_Active, &active);
    return active ? 1 : 0;
}

/* The compositor of the screen's display, NULL when there is none */
static OOP_Object *screen_compositor(struct Screen *scr)
{
    OOP_Object *display = NULL, *compositor = NULL;

    if (!IS_HIDD_BM(scr->RastPort.BitMap) || !HiddBitMapAttrBase || !HiddDisplayAttrBase || !HiddCompositorAttrBase)
        return NULL;
    OOP_GetAttr(HIDD_BM_OBJ(scr->RastPort.BitMap), aHidd_BitMap_Display, (IPTR *)&display);
    if (display)
        OOP_GetAttr(display, aHidd_Display_Compositor, (IPTR *)&compositor);
    return compositor;
}

/* A pixel of what the display shows (display coordinates); FALSE when it cannot be read */
static BOOL composed_rgb(OOP_Object *compositor, LONG x, LONG y, ULONG *rgb)
{
    OOP_Object *target = NULL;
    IPTR width = 0, height = 0;
    UBYTE argb[4];

    OOP_GetAttr(compositor, aHidd_Compositor_ComposedBitMap, (IPTR *)&target);
    if (!target)
        return FALSE;
    OOP_GetAttr(target, aHidd_BitMap_Width, &width);
    OOP_GetAttr(target, aHidd_BitMap_Height, &height);
    if (x < 0 || y < 0 || x >= (LONG)width || y >= (LONG)height)
        return FALSE;
    HIDD_BM_GetImage(target, argb, sizeof(argb), x, y, 1, 1, vHidd_StdPixFmt_ARGB32);
    *rgb = ((ULONG)argb[1] << 16) | ((ULONG)argb[2] << 8) | argb[3];
    return TRUE;
}

static BOOL near(ULONG a, ULONG b)
{
    return (a > b ? a - b : b - a) <= 3;
}

/*
 * Green (the composited window's content) with coverage 'cov' over red (the
 * plain window's content) must be on the display at x,y. Composition follows
 * rendering asynchronously, so the display is polled for a second.
 */
static int check_display_blend(struct Screen *scr, LONG x, LONG y, ULONG cov)
{
    OOP_Object *compositor = screen_compositor(scr);
    ULONG want = ((255 - cov) << 16) | (cov << 8), got = 0;
    int i;

    if (!compositor)
    {
        printf("display blend: cannot tell (no compositor)\n");
        return 0;
    }
    for (i = 0; i < 50; i++)
    {
        if (!composed_rgb(compositor, scr->LeftEdge + x, scr->TopEdge + y, &got))
        {
            printf("display blend: cannot tell (composed display not readable)\n");
            return 0;
        }
        if (near(got >> 16, want >> 16) && near((got >> 8) & 0xFF, cov) && near(got & 0xFF, 0))
        {
            printf("display blend at %ld,%ld: %06lx (wanted about %06lx)\n",
                   (long)x, (long)y, (unsigned long)got, (unsigned long)want);
            return 0;
        }
        WaitTOF();
    }
    printf("FAIL: display at %ld,%ld shows %06lx, wanted about %06lx (green at coverage %lu over red)\n",
           (long)x, (long)y, (unsigned long)got, (unsigned long)want, (unsigned long)cov);
    return 1;
}

/* The alpha format of a 32-bit screen pixel format, or -1 */
static LONG alpha_pixfmt(ULONG pf)
{
    switch (pf)
    {
    case PIXFMT_BGR032: case PIXFMT_BGRA32: return PIXFMT_BGRA32;
    case PIXFMT_RGB032: case PIXFMT_RGBA32: return PIXFMT_RGBA32;
    case PIXFMT_0RGB32: case PIXFMT_ARGB32: return PIXFMT_ARGB32;
    }
    return -1;
}

/* Alpha of one pixel of 'rp', in its own coordinates */
static UBYTE alpha_at(struct RastPort *rp, WORD x, WORD y)
{
    UBYTE pix[4];

    ReadPixelArray(pix, 0, 0, 4, rp, x, y, 1, 1, RECTFMT_ARGB);
    return pix[0];
}

/* Pens, text and an alpha-less blit drawn into the alpha screen through window 'w' */
static int check_screen(struct Window *w)
{
    struct RastPort *rp = w->RPort;
    struct BitMap *plain;
    WORD x0 = w->BorderLeft + 20, y0 = w->BorderTop + 20;
    int fails = 0;
    UBYTE a;

    SetAPen(rp, 1);
    RectFill(rp, x0, y0, x0 + 19, y0 + 19);
    if ((a = alpha_at(rp, x0 + 10, y0 + 10)) != 0xFF)
    {
        printf("FAIL: pen 1 RectFill in the alpha screen has alpha %02x, wanted ff\n", a);
        fails++;
    }

    SetAPen(rp, 1);
    SetBPen(rp, 2);
    SetDrMd(rp, JAM2);
    Move(rp, x0 + 30, y0 + rp->TxBaseline);
    Text(rp, "Ag", 2);
    if ((a = alpha_at(rp, x0 + 31, y0 + 1)) != 0xFF)
    {
        printf("FAIL: JAM2 Text in the alpha screen has alpha %02x, wanted ff\n", a);
        fails++;
    }

    plain = AllocBitMap(16, 16, 8, BMF_CLEAR, NULL);
    if (plain)
    {
        struct RastPort prp;

        InitRastPort(&prp);
        prp.BitMap = plain;
        SetAPen(&prp, 1);
        RectFill(&prp, 0, 0, 15, 15);
        BltBitMapRastPort(plain, 0, 0, rp, x0 + 60, y0, 16, 16, 0xC0);
        if ((a = alpha_at(rp, x0 + 68, y0 + 8)) != 0xFF)
        {
            printf("FAIL: alpha-less blit into the alpha screen has alpha %02x, wanted ff\n", a);
            fails++;
        }
        FreeBitMap(plain);
    }
    printf("pens, text and alpha-less blits into the alpha screen: %s\n", fails ? "FAIL" : "opaque");
    return fails;
}

static void fill(struct Window *w, ULONG rgb, ULONG border)
{
    struct RastPort *rp = w->RPort;
    WORD x0 = w->BorderLeft, y0 = w->BorderTop;
    WORD x1 = w->Width - w->BorderRight - 1, y1 = w->Height - w->BorderBottom - 1;

    FillPixelArray(rp, x0, y0, x1 - x0 + 1, y1 - y0 + 1, border);
    FillPixelArray(rp, x0 + 12, y0 + 12, x1 - x0 - 23, y1 - y0 - 23, rgb);
}

/*
 * Per pixel alpha: the content is written as ARGB. Left half alpha 0x40,
 * right half opaque, a fully transparent hole in the middle. (Pens and
 * alpha-less blits render opaque into an alpha surface.)
 */
static void fill_alpha(struct Window *w)
{
    WORD x0 = w->BorderLeft, y0 = w->BorderTop;
    WORD cw = w->Width - w->BorderLeft - w->BorderRight;
    WORD ch = w->Height - w->BorderTop - w->BorderBottom;
    UBYTE *row = AllocVec(cw * 4, MEMF_ANY);
    WORD x, y;

    if (!row)
        return;
    for (y = 0; y < ch; y++)
    {
        for (x = 0; x < cw; x++)
        {
            BOOL hole = (x > cw / 2 - 30 && x < cw / 2 + 30 && y > ch / 2 - 30 && y < ch / 2 + 30);
            BOOL edge = (x < 12 || y < 12 || x >= cw - 12 || y >= ch - 12);
            row[x * 4 + 0] = hole ? 0x00 : (x < cw / 2 ? 0x40 : 0xFF);  /* A */
            row[x * 4 + 1] = edge ? 0xFF : 0x00;                           /* R */
            row[x * 4 + 2] = edge ? 0x00 : 0xFF;                           /* G */
            row[x * 4 + 3] = edge ? 0xFF : 0x00;                           /* B */
        }
        WritePixelArray(row, 0, 0, cw * 4, w->RPort, x0, y0 + y, cw, 1, RECTFMT_ARGB);
    }
    FreeVec(row);
}

/* A screen like the Workbench one, on a custom ARGB32 bitmap the display blends by its alpha */
static struct Screen *open_alpha_screen(struct BitMap **bmp)
{
    struct Screen *wb = LockPubScreen(NULL), *scr = NULL;
    struct BitMap *bm = NULL;
    ULONG modeid;

    *bmp = NULL;
    if (!wb)
        return NULL;
    modeid = GetVPModeID(&wb->ViewPort);
    bm = AllocBitMap(wb->Width, wb->Height, 32, BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_ARGB32) | BMF_CLEAR,
                     wb->RastPort.BitMap);
    if (bm && GetCyberMapAttr(bm, CYBRMATTR_PIXFMT) == PIXFMT_ARGB32)
        scr = OpenScreenTags(NULL,
            SA_DisplayID, modeid, SA_Width, wb->Width, SA_Height, wb->Height, SA_Depth, 32,
            SA_BitMap, (IPTR)bm, SA_CompositingFlags, COMPF_ALPHA,
            SA_Title, (IPTR)"compwindow ALPHASCREEN", SA_ShowTitle, FALSE, SA_Quiet, TRUE,
            SA_SharePens, TRUE, TAG_DONE);
    UnlockPubScreen(NULL, wb);
    if (!scr)
    {
        if (bm)
            FreeBitMap(bm);
        return NULL;
    }
    *bmp = bm;
    return scr;
}

int main(void)
{
    IPTR args[5] = { 0, 0, 0, 0, 0 };
    struct RDArgs *rda;
    LONG delay = 8, opacity = 160;
    BOOL alpha = FALSE, alphascreen = FALSE, alphaformat = FALSE;
    struct Screen *scr = NULL;
    struct BitMap *scrbm = NULL;
    LONG greenpen = -1;
    struct Window *a, *b;
    int fails = 0;
    UBYTE pix[4];
    ULONG got;

    rda = ReadArgs("DELAY/N,OPACITY/N,ALPHA/S,ALPHASCREEN/S,ALPHAFORMAT/S", args, NULL);
    if (rda)
    {
        if (args[0]) delay   = *(LONG *)args[0];
        if (args[1]) opacity = *(LONG *)args[1];
        alpha = args[2] ? TRUE : FALSE;
        alphascreen = args[3] ? TRUE : FALSE;
        alphaformat = args[4] ? TRUE : FALSE;
        FreeArgs(rda);
    }

    if (alphascreen)
    {
        scr = open_alpha_screen(&scrbm);
        if (!scr)
        {
            printf("NOTE: no ARGB32 screen on this display, ALPHASCREEN not checked\n");
            return RETURN_OK;
        }
        greenpen = ObtainBestPen(scr->ViewPort.ColorMap, 0, 0xFFFFFFFF, 0, OBP_Precision, PRECISION_EXACT, TAG_END);
    }
    else if (alphaformat)
    {
        struct Screen *wb = LockPubScreen(NULL);
        LONG apf = wb ? alpha_pixfmt(GetCyberMapAttr(wb->RastPort.BitMap, CYBRMATTR_PIXFMT)) : -1;

        if (wb)
            UnlockPubScreen(NULL, wb);
        if (apf < 0)
        {
            printf("FAIL: the Workbench screen is not 32-bit\n");
            return RETURN_FAIL;
        }
        scr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_PixelFormat, apf,
                             SA_Title, (IPTR)"compwindow alpha screen", TAG_END);
        if (!scr)
        {
            printf("FAIL: no alpha screen\n");
            return RETURN_FAIL;
        }
        /* below the screen title, nothing drawn there yet */
        if ((got = alpha_at(&scr->RastPort, scr->Width - 2, scr->Height - 2)) != 0xFF)
        {
            printf("FAIL: a fresh alpha screen has alpha %02lx, wanted ff\n", (unsigned long)got);
            fails++;
        }
        /* CGX names the same format for X and A: an alpha written must be read back */
        {
            UBYTE argb[4] = { 0x40, 0x10, 0x20, 0x30 };

            WritePixelArray(argb, 0, 0, 4, &scr->RastPort, scr->Width - 2, scr->Height - 2, 1, 1, RECTFMT_ARGB);
            if ((got = alpha_at(&scr->RastPort, scr->Width - 2, scr->Height - 2)) != 0x40)
            {
                printf("FAIL: the screen keeps no alpha (read %02lx back, wrote 40)\n", (unsigned long)got);
                fails++;
            }
        }
    }

    a = OpenWindowTags(NULL,
        WA_Left, 120, WA_Top, 120, WA_Width, 360, WA_Height, 240,
        WA_Title, (IPTR)"plain window", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        scr ? WA_CustomScreen : TAG_IGNORE, (IPTR)scr,
        WA_Activate, FALSE, TAG_END);
    if (!a) { printf("FAIL: plain window\n"); if (scr) { CloseScreen(scr); FreeBitMap(scrbm); } return RETURN_FAIL; }
    if (alphaformat)
        fails += check_screen(a);
    /* explicit colours with full alpha: on an alpha screen they are what the display shows */
    fill(a, 0xFFFF0000, 0xFFFFFF00);

    b = OpenWindowTags(NULL,
        WA_Left, 300, WA_Top, 220, WA_Width, 320, WA_Height, 220,
        WA_Title, (IPTR)"composited window", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_Opaqueness, opacity,
        alpha ? WA_AlphaSurface : TAG_IGNORE, TRUE,
        scr ? WA_CustomScreen : TAG_IGNORE, (IPTR)scr,
        WA_Activate, TRUE, TAG_END);
    if (!b) { printf("FAIL: composited window\n"); CloseWindow(a); if (scr) { CloseScreen(scr); FreeBitMap(scrbm); } return RETURN_FAIL; }
    if (alpha && (b->WLayer->Flags & LAYERCOMPOSITED))
    {
        /* an alpha surface starts fully transparent (BMF_CLEAR clears to alpha 0) */
        ReadPixelArray(pix, 0, 0, 4, b->RPort, b->BorderLeft + 5, b->BorderTop + 5, 1, 1, RECTFMT_ARGB);
        if (pix[0] != 0)
        {
            printf("FAIL: fresh alpha surface has alpha %02x, wanted 00\n", pix[0]);
            fails++;
        }
    }

    if (alpha)
        fill_alpha(b);
    else
        fill(b, 0x00FF00, 0xFF00FF);
    if (alphascreen && !alpha && greenpen >= 0)
    {
        /* The content again, drawn with a pen: alpha 0 in the surface */
        SetAPen(b->RPort, greenpen);
        RectFill(b->RPort, b->BorderLeft + 12, b->BorderTop + 12,
                 b->Width - b->BorderRight - 13, b->Height - b->BorderBottom - 13);
    }

    printf("plain layer flags 0x%x, composited layer flags 0x%x (LAYERCOMPOSITED %s)\n",
           a->WLayer->Flags, b->WLayer->Flags,
           (b->WLayer->Flags & LAYERCOMPOSITED) ? "set" : "NOT set");
    if (!(b->WLayer->Flags & LAYERCOMPOSITED))
    {
        printf("NOTE: screen cannot compose; plain smart refresh window instead\n");
    }
    else
    {
        int composing;

        HiddBitMapAttrBase     = OOP_ObtainAttrBase(IID_Hidd_BitMap);
        HiddDisplayAttrBase    = OOP_ObtainAttrBase(IID_Hidd_Display);
        HiddCompositorAttrBase = OOP_ObtainAttrBase(IID_Hidd_Compositor);
        composing = display_composing(b->WScreen);
        if (composing == 0)
        {
            printf("FAIL: the display does not compose with a composited window open\n");
            fails++;
        }
        else
            printf("display composing: %s\n", composing > 0 ? "yes" : "cannot tell");
    }

    /*
     * Pens and blits from a bitmap without an alpha channel render opaque
     * into an alpha surface (the surface has its own colormap, see
     * AllocBitMap(); a source without alpha converts to full alpha).
     */
    if (alpha && (b->WLayer->Flags & LAYERCOMPOSITED))
    {
        /* without alpha: a friend of the screen has alpha where the screen does,
         * and the Workbench may be an alpha screen too */
        struct BitMap *plain = AllocBitMap(16, 16, 24, BMF_CLEAR | BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_RGB24), NULL);
        WORD x0 = b->BorderLeft, y0 = b->BorderTop;

        SetAPen(b->RPort, 1);
        RectFill(b->RPort, x0 + 20, y0 + 20, x0 + 39, y0 + 39);
        ReadPixelArray(pix, 0, 0, 4, b->RPort, x0 + 30, y0 + 30, 1, 1, RECTFMT_ARGB);
        if (pix[0] != 0xFF)
        {
            printf("FAIL: pen 1 RectFill in the alpha surface has alpha %02x, wanted ff\n", pix[0]);
            fails++;
        }

        if (plain)
        {
            struct RastPort prp;

            InitRastPort(&prp);
            prp.BitMap = plain;
            FillPixelArray(&prp, 0, 0, 16, 16, 0x0000FF);
            ReadPixelArray(pix, 0, 0, 4, &prp, 8, 8, 1, 1, RECTFMT_ARGB);
            if (pix[0] != 0xFF)
                printf("NOTE: the alpha-less source reads back alpha %02x (CGX pixfmt %lu)\n",
                       pix[0], (unsigned long)GetCyberMapAttr(plain, CYBRMATTR_PIXFMT));
            BltBitMapRastPort(plain, 0, 0, b->RPort, x0 + 50, y0 + 20, 16, 16, 0xC0);
            ReadPixelArray(pix, 0, 0, 4, b->RPort, x0 + 58, y0 + 28, 1, 1, RECTFMT_ARGB);
            got = ((ULONG)pix[1] << 16) | ((ULONG)pix[2] << 8) | pix[3];
            if (pix[0] != 0xFF || got != 0x0000FF)
            {
                printf("FAIL: alpha-less blit into the alpha surface gives %02x/%06lx, wanted ff/0000ff\n",
                       pix[0], (unsigned long)got);
                fails++;
            }
            FreeBitMap(plain);
        }
        printf("pens and alpha-less blits into the alpha surface: %s\n", fails ? "FAIL" : "opaque");
    }

    /* the composited window's own content is intact under the overlap */
    ReadPixelArray(pix, 0, 0, 4, b->RPort, b->BorderLeft + 40, b->BorderTop + 40, 1, 1, RECTFMT_ARGB);
    got = ((ULONG)pix[1] << 16) | ((ULONG)pix[2] << 8) | pix[3];
    if (got != 0x00FF00)
    {
        printf("FAIL: composited window content %06lx, wanted 00ff00\n", (unsigned long)got);
        fails++;
    }
    /* the plain window's content under the composited one is intact (not clobbered) */
    ReadPixelArray(pix, 0, 0, 4, a->RPort, 300, 180, 1, 1, RECTFMT_ARGB);
    got = ((ULONG)pix[1] << 16) | ((ULONG)pix[2] << 8) | pix[3];
    if (got != 0xFF0000)
    {
        printf("FAIL: plain window content under overlap %06lx, wanted ff0000\n", (unsigned long)got);
        fails++;
    }

    /*
     * What the display shows where the composited window's content lies over
     * the plain window's red: green blended by the opacity, for ALPHA also by
     * the content's alpha (0x40 in the left half). A constant opacity surface
     * has no coverage of its own, whatever its pixels' alpha byte holds.
     */
    if (b->WLayer->Flags & LAYERCOMPOSITED)
    {
        ULONG op = (opacity < 0) ? 0 : (opacity > 255) ? 255 : opacity;
        ULONG cov = alpha ? (0x40 * op + 127) / 255 : op;

        fails += check_display_blend(b->WScreen, b->LeftEdge + b->BorderLeft + 40,
                                     b->TopEdge + b->BorderTop + 40, cov);
    }

    Delay(delay * 50);

    MoveWindow(b, -100, -60);
    Delay(delay * 50);
    WindowToBack(b);
    Delay(delay * 50);
    WindowToFront(b);
    Delay(delay * 50);

    CloseWindow(b);
    CloseWindow(a);
    if (scr)
    {
        if (greenpen >= 0)
            ReleasePen(scr->ViewPort.ColorMap, greenpen);
        CloseScreen(scr);
        FreeBitMap(scrbm);
    }
    if (HiddCompositorAttrBase) OOP_ReleaseAttrBase(IID_Hidd_Compositor);
    if (HiddDisplayAttrBase)    OOP_ReleaseAttrBase(IID_Hidd_Display);
    if (HiddBitMapAttrBase)     OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    printf("%s\n", fails ? "FAIL" : "PASS");
    return fails ? RETURN_FAIL : RETURN_OK;
}
