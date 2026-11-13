/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: cybergraphics.library/WritePixelArrayAlpha() global alpha test.

    Opens a white screen in the Workbench mode and blends five 64x64 blocks
    onto it, 100 pixels apart starting at (20,20):

        1. opaque black, global alpha 0xFFFFFFFF  -> black
        2. opaque black, global alpha 0x80000000  -> mid grey (127)
        3. opaque black, global alpha 0x40000000  -> light grey (191)
        4. opaque black, global alpha 0           -> untouched (white)
        5. red at alpha 128, global 0x80000000    -> effective alpha 64

    Before that it blends into off-screen RGB24 and ARGB32 bitmaps, reads
    the result back and reports each case as PASS or FAIL on the debug
    output and on stdout. Those formats have no dedicated blending loop in
    the chunky bitmap class, so they exercise the generic read-blend-write
    path, including a destination with an alpha channel of its own. The
    rows are 37 pixels wide so that vector code and its scalar tail both
    get used.

    With BENCH it also times WritePixelArrayAlpha() of a 512x512 source
    into off-screen bitmaps and reports megapixels per second on the debug
    output and stdout.

    Usage: wpaaglobal [DELAY=seconds] [BENCH]
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <sys/time.h>

#define BLOCK 64

static void fill_block(UBYTE *img, UBYTE a, UBYTE r, UBYTE g, UBYTE b)
{
    ULONG i;

    for (i = 0; i < BLOCK * BLOCK; i++)
    {
        img[i * 4 + 0] = a;
        img[i * 4 + 1] = r;
        img[i * 4 + 2] = g;
        img[i * 4 + 3] = b;
    }
}

#define OFFW 37
#define OFFH 3

static void fill_rect(UBYTE *img, UBYTE a, UBYTE r, UBYTE g, UBYTE b)
{
    ULONG i;

    for (i = 0; i < OFFW * OFFH; i++)
    {
        img[i * 4 + 0] = a;
        img[i * 4 + 1] = r;
        img[i * 4 + 2] = g;
        img[i * 4 + 3] = b;
    }
}

/*
 * Put 'dst' into an off-screen bitmap of the given format, blend 'src'
 * over it with the given global alpha and compare what comes back.
 * 'checkalpha' is FALSE for formats that have no alpha channel.
 */
static BOOL offscreen_case(struct BitMap *friend, ULONG pixfmt, ULONG depth, CONST_STRPTR name,
                           const UBYTE *dst, const UBYTE *src, ULONG galpha,
                           const UBYTE *expect, BOOL checkalpha)
{
    static UBYTE img[OFFW * OFFH * 4], out[OFFW * OFFH * 4];
    struct RastPort rp;
    struct BitMap *bm;
    BOOL ok = TRUE;
    ULONG i, bad = 0;

    bm = AllocBitMap(OFFW, OFFH, depth, BMF_SPECIALFMT | SHIFT_PIXFMT(pixfmt) | BMF_CLEAR, friend);
    if (!bm)
    {
        bug("[wpaaglobal] %s: cannot allocate bitmap\n", name);
        printf("%s: cannot allocate bitmap\n", name);
        return FALSE;
    }

    InitRastPort(&rp);
    rp.BitMap = bm;

    fill_rect(img, dst[0], dst[1], dst[2], dst[3]);
    WritePixelArray(img, 0, 0, OFFW * 4, &rp, 0, 0, OFFW, OFFH, RECTFMT_ARGB);

    fill_rect(img, src[0], src[1], src[2], src[3]);
    WritePixelArrayAlpha(img, 0, 0, OFFW * 4, &rp, 0, 0, OFFW, OFFH, galpha);

    ReadPixelArray(out, 0, 0, OFFW * 4, &rp, 0, 0, OFFW, OFFH, RECTFMT_ARGB);

    for (i = 0; i < OFFW * OFFH; i++)
    {
        const UBYTE *p = out + i * 4;
        LONG c;

        for (c = checkalpha ? 0 : 1; c < 4; c++)
        {
            LONG diff = (LONG)p[c] - (LONG)expect[c];

            if (diff < -1 || diff > 1)
                ok = FALSE;
        }
        if (!ok && !bad)
            bad = i + 1;
    }

    bug("[wpaaglobal] %s: expect %u,%u,%u,%u got %u,%u,%u,%u (pixel 0) %u,%u,%u,%u (last)%s: %s\n", name,
        expect[0], expect[1], expect[2], expect[3], out[0], out[1], out[2], out[3],
        out[(OFFW * OFFH - 1) * 4 + 0], out[(OFFW * OFFH - 1) * 4 + 1],
        out[(OFFW * OFFH - 1) * 4 + 2], out[(OFFW * OFFH - 1) * 4 + 3],
        checkalpha ? "" : " [alpha ignored]", ok ? "PASS" : "FAIL");
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
    if (!ok)
        bug("[wpaaglobal] %s: first bad pixel %lu\n", name, (unsigned long)(bad - 1));

    FreeBitMap(bm);

    return ok;
}

static void offscreen_checks(struct BitMap *friend)
{
    static const UBYTE white[4]  = { 0xFF, 0xFF, 0xFF, 0xFF };
    static const UBYTE white80[4]= { 0x80, 0xFF, 0xFF, 0xFF };
    static const UBYTE clear[4]  = { 0x00, 0x00, 0x00, 0x00 };
    static const UBYTE black[4]  = { 0xFF, 0x00, 0x00, 0x00 };
    static const UBYTE black80[4]= { 0x80, 0x00, 0x00, 0x00 };
    static const UBYTE red80[4]  = { 0x80, 0xFF, 0x00, 0x00 };
    static const UBYTE e_grey7f[4]  = { 0xFF, 127, 127, 127 };
    static const UBYTE e_greybf[4]  = { 0xFF, 191, 191, 191 };
    static const UBYTE e_black[4]   = { 0xFF, 0, 0, 0 };
    static const UBYTE e_red80[4]   = { 0x80, 0xFF, 0, 0 };
    static const UBYTE e_over[4]    = { 192, 85, 85, 85 };
    static const UBYTE e_white80[4] = { 0x80, 0xFF, 0xFF, 0xFF };

    /* No alpha channel in the destination: always opaque */
    offscreen_case(friend, PIXFMT_RGB24, 24, "RGB24 white + black, global 0x80000000",
                   white, black, 0x80000000, e_grey7f, FALSE);
    offscreen_case(friend, PIXFMT_RGB24, 24, "RGB24 white + black a128, global 0x80000000",
                   white, black80, 0x80000000, e_greybf, FALSE);
    offscreen_case(friend, PIXFMT_RGB24, 24, "RGB24 white + black, global full",
                   white, black, 0xFFFFFFFF, e_black, FALSE);

    /* Destination with alpha: opaque, empty and half transparent */
    offscreen_case(friend, PIXFMT_ARGB32, 32, "ARGB32 opaque white + black a128",
                   white, black80, 0xFFFFFFFF, e_grey7f, TRUE);
    offscreen_case(friend, PIXFMT_ARGB32, 32, "ARGB32 empty + red a128",
                   clear, red80, 0xFFFFFFFF, e_red80, TRUE);
    offscreen_case(friend, PIXFMT_ARGB32, 32, "ARGB32 white a128 + black a128",
                   white80, black80, 0xFFFFFFFF, e_over, TRUE);
    offscreen_case(friend, PIXFMT_ARGB32, 32, "ARGB32 white a128 + black a0",
                   white80, clear, 0xFFFFFFFF, e_white80, TRUE);
}

#define BW    512
#define BH    512
#define BITER 100

static ULONG now_us(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (ULONG)tv.tv_sec * 1000000UL + (ULONG)tv.tv_usec;
}

/* pixfmt 0 means "same format as the friend bitmap" */
static void bench_case(struct BitMap *friend, ULONG pixfmt, ULONG depth, CONST_STRPTR name,
                       UBYTE *srcimg, UBYTE *white, ULONG galpha)
{
    struct RastPort rp;
    struct BitMap *bm;
    ULONG t0, t1, us, i;

    if (pixfmt)
        bm = AllocBitMap(BW, BH, depth, BMF_SPECIALFMT | SHIFT_PIXFMT(pixfmt) | BMF_CLEAR, friend);
    else
        bm = AllocBitMap(BW, BH, depth, BMF_CLEAR, friend);
    if (!bm)
    {
        bug("[wpaaglobal] bench %s: cannot allocate bitmap\n", name);
        return;
    }

    InitRastPort(&rp);
    rp.BitMap = bm;
    WritePixelArray(white, 0, 0, BW * 4, &rp, 0, 0, BW, BH, RECTFMT_ARGB);

    t0 = now_us();
    for (i = 0; i < BITER; i++)
        WritePixelArrayAlpha(srcimg, 0, 0, BW * 4, &rp, 0, 0, BW, BH, galpha);
    t1 = now_us();

    us = t1 - t0;
    if (us == 0)
        us = 1;
    {
        /* megapixels per second, one decimal */
        ULONG mpx10 = (ULONG)(((UQUAD)BW * BH * BITER * 10) / us);

        bug("[wpaaglobal] bench %s: %lu us for %d x %dx%d, %lu.%lu Mpix/s\n", name,
            (unsigned long)us, BITER, BW, BH, (unsigned long)(mpx10 / 10), (unsigned long)(mpx10 % 10));
        printf("bench %s: %lu.%lu Mpix/s\n", name, (unsigned long)(mpx10 / 10), (unsigned long)(mpx10 % 10));
    }

    FreeBitMap(bm);
}

static void bench(struct BitMap *friend)
{
    UBYTE *partial = AllocVec(BW * BH * 4, MEMF_ANY);
    UBYTE *icon    = AllocVec(BW * BH * 4, MEMF_ANY);
    UBYTE *white   = AllocVec(BW * BH * 4, MEMF_ANY);
    ULONG x, y;

    if (partial && icon && white)
    {
        for (y = 0; y < BH; y++)
        {
            for (x = 0; x < BW; x++)
            {
                UBYTE *p = partial + (y * BW + x) * 4;
                UBYTE *q = icon + (y * BW + x) * 4;
                UBYTE *w = white + (y * BW + x) * 4;
                ULONG  k = (x / 16 + y / 16) % 10;

                /* every pixel partly transparent */
                p[0] = 1 + ((x * 7 + y * 13) % 254);
                p[1] = x; p[2] = y; p[3] = x ^ y;

                /* icon-like: half empty, a third opaque, the rest soft edges */
                q[0] = (k < 5) ? 0 : (k < 8) ? 255 : 1 + ((x * 5 + y * 3) % 254);
                q[1] = y; q[2] = x; q[3] = x + y;

                w[0] = w[1] = w[2] = w[3] = 0xFF;
            }
        }

        bench_case(friend, PIXFMT_RGB24,  24, "RGB24   partial alpha", partial, white, 0xFFFFFFFF);
        bench_case(friend, PIXFMT_ARGB32, 32, "ARGB32  partial alpha", partial, white, 0xFFFFFFFF);
        bench_case(friend, PIXFMT_ARGB32, 32, "ARGB32  icon-like    ", icon,    white, 0xFFFFFFFF);
        bench_case(friend, PIXFMT_ARGB32, 32, "ARGB32  partial, global 0x80", partial, white, 0x80000000);
        bench_case(friend, 0, 24,             "screen  partial alpha", partial, white, 0xFFFFFFFF);
        bench_case(friend, 0, 24,             "screen  partial, global 0x80", partial, white, 0x80000000);
    }

    FreeVec(partial);
    FreeVec(icon);
    FreeVec(white);
}

int main(void)
{
    static const ULONG galpha[4] = { 0xFFFFFFFF, 0x80000000, 0x40000000, 0x00000000 };
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda;
    struct Screen *wb, *scr;
    ULONG modeid = INVALID_ID, width = 640, height = 480, delay = 20, i;
    UBYTE *img;

    BOOL dobench = FALSE;

    rda = ReadArgs("DELAY/N,BENCH/S", args, NULL);
    if (rda)
    {
        if (args[0]) delay = *(IPTR *)args[0];
        dobench = args[1] ? TRUE : FALSE;
        FreeArgs(rda);
    }

    wb = LockPubScreen(NULL);
    if (wb)
    {
        modeid = GetVPModeID(&wb->ViewPort);
        width  = wb->Width;
        height = wb->Height;
        UnlockPubScreen(NULL, wb);
    }
    if (modeid == INVALID_ID)
        modeid = BestModeID(BIDTAG_NominalWidth, width, BIDTAG_NominalHeight, height,
                            BIDTAG_Depth, 24, TAG_DONE);

    scr = OpenScreenTags(NULL,
                         SA_DisplayID, modeid,
                         SA_Width,     width,
                         SA_Height,    height,
                         SA_Depth,     24,
                         SA_ShowTitle, FALSE,
                         SA_Quiet,     TRUE,
                         TAG_DONE);
    if (!scr)
    {
        printf("Cannot open screen\n");
        return RETURN_FAIL;
    }

    offscreen_checks(scr->RastPort.BitMap);

    if (dobench)
        bench(scr->RastPort.BitMap);

    img = AllocVec(BLOCK * BLOCK * 4, MEMF_ANY);
    if (img)
    {
        FillPixelArray(&scr->RastPort, 0, 0, width, height, 0xFFFFFF);

        fill_block(img, 0xFF, 0, 0, 0);
        for (i = 0; i < 4; i++)
        {
            ULONG n = WritePixelArrayAlpha(img, 0, 0, BLOCK * 4, &scr->RastPort,
                                           20 + i * 100, 20, BLOCK, BLOCK, galpha[i]);
            bug("[wpaaglobal] block %lu global alpha 0x%08lx: %lu pixels\n",
                (unsigned long)i, (unsigned long)galpha[i], (unsigned long)n);
        }

        fill_block(img, 128, 0xFF, 0, 0);
        WritePixelArrayAlpha(img, 0, 0, BLOCK * 4, &scr->RastPort,
                             20 + 4 * 100, 20, BLOCK, BLOCK, 0x80000000);

        bug("[wpaaglobal] painted, waiting %lu s\n", (unsigned long)delay);
        Delay(delay * TICKS_PER_SECOND);
        FreeVec(img);
    }

    CloseScreen(scr);
    return RETURN_OK;
}
