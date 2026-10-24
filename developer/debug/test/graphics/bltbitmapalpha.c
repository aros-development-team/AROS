/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BltBitMapAlpha() test (AROS extension of graphics.library).

    Blends a 64x64 ARGB source (left half red with alpha 0x80, right half
    opaque green, a fully transparent 16x16 corner) over an opaque blue
    destination in the screen's format and checks the result pixel by
    pixel, read back as bytes with ReadPixelArray(RECTFMT_ARGB). Covers
    opacity 255, 128 and 0, clipping on all four sides, a displayable
    destination (the screen bitmap) and the planar fallback.

    Usage: bltbitmapalpha [DELAY=seconds]  (time the screen stays up)
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 64
#define H 64

static int failures = 0;

static void check(const char *what, struct RastPort *rp, LONG x, LONG y, int r, int g, int b, int tol)
{
    UBYTE pix[4] = { 0, 0, 0, 0 };

    /* RECTFMT_ARGB is a byte order (A, R, G, B), never read it into a ULONG */
    ReadPixelArray(pix, 0, 0, 4, rp, x, y, 1, 1, RECTFMT_ARGB);

    if (abs(pix[1] - r) <= tol && abs(pix[2] - g) <= tol && abs(pix[3] - b) <= tol)
        printf("PASS: %s at %ld,%ld = %02x%02x%02x\n", what, (long)x, (long)y, pix[1], pix[2], pix[3]);
    else
    {
        printf("FAIL: %s at %ld,%ld = %02x%02x%02x, wanted %02x%02x%02x (+-%d)\n", what, (long)x, (long)y,
               pix[1], pix[2], pix[3], r, g, b, tol);
        failures++;
    }
}

static void fill_argb(struct RastPort *rp, LONG x0, LONG y0, LONG w, LONG h, UBYTE a, UBYTE r, UBYTE g, UBYTE b)
{
    UBYTE *row = AllocVec(w * 4, MEMF_ANY);
    LONG x, y;

    if (!row)
        return;
    for (x = 0; x < w; x++)
    {
        row[x * 4 + 0] = a;
        row[x * 4 + 1] = r;
        row[x * 4 + 2] = g;
        row[x * 4 + 3] = b;
    }
    for (y = 0; y < h; y++)
        WritePixelArray(row, 0, 0, w * 4, rp, x0, y0 + y, w, 1, RECTFMT_ARGB);
    FreeVec(row);
}

static void blue(struct RastPort *rp)
{
    FillPixelArray(rp, 0, 0, W, H, 0x0000FF);
}

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    LONG delay = 3;
    struct Screen *scr;
    struct BitMap *friend, *src, *dst, *psrc, *pdst;
    struct RastPort srp, drp;

    rda = ReadArgs("DELAY/N", args, NULL);
    if (rda && args[0])
        delay = *(LONG *)args[0];

    scr = OpenScreenTags(NULL,
                         SA_LikeWorkbench, TRUE,
                         SA_Title,         (IPTR)"bltbitmapalpha",
                         SA_ShowTitle,     FALSE,
                         SA_Quiet,         TRUE,
                         TAG_DONE);
    if (!scr)
    {
        printf("FAIL: cannot open screen\n");
        if (rda) FreeArgs(rda);
        return 20;
    }
    friend = scr->RastPort.BitMap;
    printf("screen %dx%d depth %lu\n", scr->Width, scr->Height, (unsigned long)GetBitMapAttr(friend, BMA_DEPTH));

    src = AllocBitMap(W, H, 32, BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_ARGB32) | BMF_CLEAR, friend);
    dst = AllocBitMap(W, H, GetBitMapAttr(friend, BMA_DEPTH), BMF_CLEAR, friend);
    if (!src || !dst)
    {
        printf("FAIL: cannot allocate bitmaps\n");
        if (src) FreeBitMap(src);
        if (dst) FreeBitMap(dst);
        CloseScreen(scr);
        if (rda) FreeArgs(rda);
        return 20;
    }
    InitRastPort(&srp);
    srp.BitMap = src;
    InitRastPort(&drp);
    drp.BitMap = dst;

    /* the source: left half red at alpha 0x80, right half opaque green, transparent 16x16 corner */
    fill_argb(&srp, 0, 0, W / 2, H, 0x80, 0xFF, 0x00, 0x00);
    fill_argb(&srp, W / 2, 0, W / 2, H, 0xFF, 0x00, 0xFF, 0x00);
    fill_argb(&srp, 0, 0, 16, 16, 0x00, 0xFF, 0xFF, 0xFF);
    check("source red half", &srp, 8, 40, 0xFF, 0x00, 0x00, 0);
    check("source green half", &srp, 40, 40, 0x00, 0xFF, 0x00, 0);

    /* 1. opacity 255: the source's own alpha */
    blue(&drp);
    BltBitMapAlpha(src, 0, 0, dst, 0, 0, W, H, 255);
    check("opacity 255: half red over blue", &drp, 8, 40, 0x80, 0x00, 0x7F, 2);
    check("opacity 255: opaque green", &drp, 40, 40, 0x00, 0xFF, 0x00, 0);
    check("opacity 255: transparent corner untouched", &drp, 8, 8, 0x00, 0x00, 0xFF, 0);

    /* 2. opacity 128 scales the source alpha */
    blue(&drp);
    BltBitMapAlpha(src, 0, 0, dst, 0, 0, W, H, 128);
    check("opacity 128: green at 50%", &drp, 40, 40, 0x00, 0x80, 0x7F, 2);
    check("opacity 128: red at 25%", &drp, 8, 40, 0x40, 0x00, 0xBF, 3);
    check("opacity 128: transparent corner untouched", &drp, 8, 8, 0x00, 0x00, 0xFF, 0);

    /* 3. opacity 0 draws nothing */
    blue(&drp);
    BltBitMapAlpha(src, 0, 0, dst, 0, 0, W, H, 0);
    check("opacity 0: untouched", &drp, 40, 40, 0x00, 0x00, 0xFF, 0);

    /* 4. clipping: negative source corner moves the destination corner too */
    blue(&drp);
    BltBitMapAlpha(src, -32, -32, dst, 0, 0, W, H, 255);
    check("clip src -32,-32: dest 8,8 untouched", &drp, 8, 8, 0x00, 0x00, 0xFF, 0);
    check("clip src -32,-32: src 8,8 (transparent) at 40,40", &drp, 40, 40, 0x00, 0x00, 0xFF, 0);
    check("clip src -32,-32: src 4,28 (half red) at 36,60", &drp, 36, 60, 0x80, 0x00, 0x7F, 2);
    /* negative destination corner */
    blue(&drp);
    BltBitMapAlpha(src, 0, 0, dst, -(W / 2), 0, W, H, 255);
    check("clip dest -32,0: src 40,40 (green) at 8,40", &drp, 8, 40, 0x00, 0xFF, 0x00, 0);
    check("clip dest -32,0: dest 40,40 untouched", &drp, 40, 40, 0x00, 0x00, 0xFF, 0);
    /* oversized rectangle */
    blue(&drp);
    BltBitMapAlpha(src, 32, 32, dst, 32, 32, 1000, 1000, 255);
    check("clip size: src 40,40 (green) at 40,40", &drp, 40, 40, 0x00, 0xFF, 0x00, 0);
    check("clip size: dest 8,40 untouched", &drp, 8, 40, 0x00, 0x00, 0xFF, 0);

    /* 5. a displayable destination: the screen bitmap */
    FillPixelArray(&scr->RastPort, 100, 100, W, H, 0x0000FF);
    BltBitMapAlpha(src, 0, 0, friend, 100, 100, W, H, 255);
    check("screen bitmap: half red over blue", &scr->RastPort, 108, 140, 0x80, 0x00, 0x7F, 2);
    check("screen bitmap: opaque green", &scr->RastPort, 140, 140, 0x00, 0xFF, 0x00, 0);
    FillPixelArray(&scr->RastPort, 200, 100, W, H, 0x0000FF);
    BltBitMapAlpha(src, 0, 0, friend, 200, 100, W, H, 128);
    check("screen bitmap: green at 50%", &scr->RastPort, 240, 140, 0x00, 0x80, 0x7F, 2);

    /* 6. planar bitmaps: opacity 255 is a copy, anything else does nothing */
    psrc = AllocBitMap(W, H, 1, BMF_CLEAR, NULL);
    pdst = AllocBitMap(W, H, 1, BMF_CLEAR, NULL);
    if (psrc && pdst && psrc->Planes[0] && pdst->Planes[0])
    {
        memset(psrc->Planes[0], 0xFF, psrc->BytesPerRow * psrc->Rows);
        BltBitMapAlpha(psrc, 0, 0, pdst, 0, 0, W, H, 128);
        if (pdst->Planes[0][0] == 0x00)
            printf("PASS: planar, opacity 128: nothing drawn\n");
        else
        {
            printf("FAIL: planar, opacity 128: destination changed (%02x)\n", pdst->Planes[0][0]);
            failures++;
        }
        BltBitMapAlpha(psrc, 0, 0, pdst, 0, 0, W, H, 255);
        if (pdst->Planes[0][0] == 0xFF && pdst->Planes[0][pdst->BytesPerRow * (H - 1) + W / 8 - 1] == 0xFF)
            printf("PASS: planar, opacity 255: copied\n");
        else
        {
            printf("FAIL: planar, opacity 255: not copied (%02x)\n", pdst->Planes[0][0]);
            failures++;
        }
    }
    else
        printf("NOTE: no planar bitmaps, planar fallback not tested\n");
    if (psrc) FreeBitMap(psrc);
    if (pdst) FreeBitMap(pdst);

    Delay(delay * 50);

    FreeBitMap(src);
    FreeBitMap(dst);
    CloseScreen(scr);
    if (rda)
        FreeArgs(rda);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 5 : 0;
}
