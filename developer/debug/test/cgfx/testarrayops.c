/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    testarrayops: cybergraphics' pixel array operations.

    Usage: testarrayops [CHECK] [QUICK] [SCREEN]

    Without CHECK: a demonstration and benchmark of ProcessPixelArray() on a
    picture (SYS:Developer/Debug/Tests/Datatypes/png/SamplePNGImage_1mb.png),
    shown in a window until it is closed.

    CHECK: exact checks of FillPixelArray(), ProcessPixelArray() and
    graphics' RPTAG_FgColor/RPTAG_BgColor on every kind of destination:
    off-screen bitmaps with and without alpha (ARGB32, BGRA32, RGBA32,
    0RGB32, RGB24, RGB16), a composited window with an alpha surface
    (LA_AlphaSurface on a screen without alpha) and an ordinary window partly
    covered by another one. Every result is compared with a reference
    computed here on the bytes of RECTFMT_ARGB (A, R, G, B in memory). The
    generic C implementations of the ProcessPixelArray() operations are
    compiled in as well and checked the same way, so a CPU whose library uses
    optimised versions (x86_64: SSE2/AVX2) also tests the generic ones.
    QUICK checks fewer rectangle sizes. The checks run on the default public
    screen when it is a truecolour one, otherwise (and with SCREEN always) on
    a truecolour screen of their own. Returns 0 when all checks pass.
*/

#include <clib/alib_protos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <aros/debug.h>
#include <graphics/rpattr.h>
#include <graphics/gfxmacros.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <intuition/screens.h>

#include <stdio.h>
#include <string.h>


/* The generic operations, as built for CPUs without optimised versions */
#include "processpixelarray_opbrightness.c"
#include "processpixelarray_opsetalpha.c"
#include "processpixelarray_optint.c"
#include "processpixelarray_opblur.c"

#define RW  80      /* test region */
#define RH  40

struct Dest
{
    const char      *name;
    struct RastPort *rp;
    LONG             x0, y0;    /* region origin in the RastPort */
    BOOL             alpha;     /* holds alpha; without, alpha reads back undefined */
    UBYTE            tol;       /* per channel tolerance of a stored colour */
    struct BitMap   *bm;        /* off-screen bitmap we allocated */
    struct RastPort  ownrp;
};

static ULONG checks, fails;
static BOOL quick;
static struct Screen *testscr;    /* the screen the checks run on */
static LONG testpen = -1;           /* a pen of colour 0x2266AA: no component equal to another */

#define CHECK(cond, ...) \
    do { checks++; if (!(cond)) { fails++; if (fails <= 80) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } } while (0)

static UBYTE region[RW * RH * 4], before[RW * RH * 4], after[RW * RH * 4], expect[RW * RH * 4];

static ULONG rnd_state;

static ULONG rnd(void)
{
    rnd_state ^= rnd_state << 13;
    rnd_state ^= rnd_state >> 17;
    rnd_state ^= rnd_state << 5;
    return rnd_state;
}

static void put_region(struct Dest *d, const UBYTE *img)
{
    WritePixelArray((APTR)img, 0, 0, RW * 4, d->rp, d->x0, d->y0, RW, RH, RECTFMT_ARGB);
}

static void get_region(struct Dest *d, UBYTE *img)
{
    ReadPixelArray(img, 0, 0, RW * 4, d->rp, d->x0, d->y0, RW, RH, RECTFMT_ARGB);
}

/* Random pixels, alpha included */
static void random_region(ULONG seed)
{
    ULONG i;

    rnd_state = seed ? seed : 1;
    for (i = 0; i < RW * RH; i++)
    {
        ULONG v = rnd();
        region[i * 4 + 0] = v >> 24;
        region[i * 4 + 1] = v >> 16;
        region[i * 4 + 2] = v >> 8;
        region[i * 4 + 3] = v;
    }
}

/* How a stored colour reads back from the destination */
static BOOL same_pixel(struct Dest *d, const UBYTE *got, const UBYTE *want)
{
    LONG c;

    if (d->alpha && got[0] != want[0])
        return FALSE;

    for (c = 1; c < 4; c++)
    {
        LONG diff = (LONG)got[c] - (LONG)want[c];
        if (diff < -(LONG)d->tol || diff > (LONG)d->tol)
            return FALSE;
    }
    return TRUE;
}

/* Compare a whole region; returns the number of differing pixels */
static ULONG compare_region(struct Dest *d, const UBYTE *got, const UBYTE *want, LONG *firstx, LONG *firsty)
{
    ULONG bad = 0, i;

    for (i = 0; i < RW * RH; i++)
    {
        if (!same_pixel(d, got + i * 4, want + i * 4))
        {
            if (!bad)
            {
                *firstx = i % RW;
                *firsty = i / RW;
            }
            bad++;
        }
    }
    return bad;
}

static void argb_bytes(ULONG argb, UBYTE *p)
{
    p[0] = argb >> 24;
    p[1] = argb >> 16;
    p[2] = argb >> 8;
    p[3] = argb;
}

/*****************************************************************************/
/* References, on RECTFMT_ARGB bytes                                         */

static UBYTE clampb(LONG v)
{
    return (v < 0) ? 0 : ((v > 255) ? 255 : v);
}

static UBYTE ref_mul255(UBYTE c, UBYTE t)
{
    /* round(c * t / 255), computed differently from the implementations */
    ULONG q = (ULONG)c * t;
    ULONG r = q / 255, rem = q % 255;

    return (rem * 2 >= 255) ? r + 1 : r;
}

static void ref_kernel(float *kernel, int radius)
{
    int n = 2 * radius;
    long double coeff = 1.0, sum = 0.0;
    int k;

    for (k = 0; k <= n; k++)
    {
        if (k == 0) coeff = 1.0;
        else coeff = coeff * (n - k + 1) / k;
        kernel[k] = (float)coeff;
        sum += coeff;
    }
    for (k = 0; k <= n; k++)
        kernel[k] /= (float)sum;
}

static UBYTE ref_round(float v)
{
    if (v <= 0.0f) return 0;
    if (v >= 255.0f) return 255;
    return (UBYTE)(v + 0.5f);
}

/* Blur of the w x h rectangle at (sx, sy) of img (RW wide), in place */
static void ref_blur(UBYTE *img, LONG sx, LONG sy, LONG w, LONG h, LONG value)
{
    static UBYTE tmp[RW * RH * 4], src[RW * RH * 4];
    float kernel[65];
    int radius;
    LONG x, y, c, k;

    if (value < 0) value = 0;
    if (value > 255) value = 255;
    radius = 1 + (value * 31) / 255;
    ref_kernel(kernel, radius);

    for (y = 0; y < h; y++)
        memcpy(src + y * w * 4, img + ((sy + y) * RW + sx) * 4, w * 4);

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            for (c = 0; c < 4; c++)
            {
                float acc = 0.0f;
                for (k = -radius; k <= radius; k++)
                {
                    LONG n = x + k;
                    if (n < 0) n = 0;
                    if (n >= w) n = w - 1;
                    acc += src[(y * w + n) * 4 + c] * kernel[k + radius];
                }
                tmp[(y * w + x) * 4 + c] = ref_round(acc);
            }

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            for (c = 0; c < 4; c++)
            {
                float acc = 0.0f;
                for (k = -radius; k <= radius; k++)
                {
                    LONG n = y + k;
                    if (n < 0) n = 0;
                    if (n >= h) n = h - 1;
                    acc += tmp[(n * w + x) * 4 + c] * kernel[k + radius];
                }
                img[((sy + y) * RW + sx + x) * 4 + c] = ref_round(acc);
            }
}

enum { OP_SETALPHA, OP_BRIGHTEN, OP_DARKEN, OP_TINT, OP_BLUR };

static const char *opname[] = { "SETALPHA", "BRIGHTEN", "DARKEN", "TINT", "BLUR" };

/* Expected result of an operation on a region read back from d */
static void ref_op(struct Dest *d, UBYTE *img, int op, LONG value, LONG sx, LONG sy, LONG w, LONG h)
{
    LONG x, y;
    ULONG bppix = GetCyberMapAttr(d->rp->BitMap, CYBRMATTR_BPPIX);

    if (op == OP_BLUR)
    {
        ref_blur(img, sx, sy, w, h, value);
        return;
    }

    for (y = sy; y < sy + h; y++)
        for (x = sx; x < sx + w; x++)
        {
            UBYTE *p = img + (y * RW + x) * 4;

            switch (op)
            {
            case OP_SETALPHA:
                if (bppix >= 4)
                    p[0] = value;
                break;
            case OP_BRIGHTEN:
                p[1] = clampb(p[1] + value); p[2] = clampb(p[2] + value); p[3] = clampb(p[3] + value);
                break;
            case OP_DARKEN:
                p[1] = clampb(p[1] - value); p[2] = clampb(p[2] - value); p[3] = clampb(p[3] - value);
                break;
            case OP_TINT:
                p[1] = ref_mul255(p[1], (value >> 16) & 0xFF);
                p[2] = ref_mul255(p[2], (value >> 8) & 0xFF);
                p[3] = ref_mul255(p[3], value & 0xFF);
                break;
            }
        }
}

/*****************************************************************************/
/* ProcessPixelArray()                                                        */

static void run_generic(struct Dest *d, int op, LONG value, LONG sx, LONG sy, LONG w, LONG h)
{
    struct Rectangle r;

    r.MinX = d->x0 + sx;
    r.MinY = d->y0 + sy;
    r.MaxX = r.MinX + w - 1;
    r.MaxY = r.MinY + h - 1;

    switch (op)
    {
    case OP_SETALPHA: ProcessPixelArrayAlphaFunc(d->rp, &r, value, CyberGfxBase); break;
    case OP_BRIGHTEN: ProcessPixelArrayBrightnessFunc(d->rp, &r, value, CyberGfxBase); break;
    case OP_DARKEN:   ProcessPixelArrayBrightnessFunc(d->rp, &r, -value, CyberGfxBase); break;
    case OP_TINT:     ProcessPixelArrayTintFunc(d->rp, &r, value, CyberGfxBase); break;
    case OP_BLUR:     ProcessPixelArrayBlurFunc(d->rp, &r, value, CyberGfxBase); break;
    }
}

static const ULONG popcode[] = { POP_SETALPHA, POP_BRIGHTEN, POP_DARKEN, POP_TINT, POP_BLUR };

static void op_case(struct Dest *d, int op, LONG value, LONG sx, LONG sy, LONG w, LONG h, ULONG seed)
{
    int impl;

    /* impl 0: the library (optimised where there is a version), 1: generic C */
    for (impl = 0; impl < 2; impl++)
    {
        LONG fx = 0, fy = 0;
        ULONG bad;

        random_region(seed);
        put_region(d, region);
        get_region(d, before);              /* as the destination stores it */

        if (impl == 0)
            ProcessPixelArray(d->rp, d->x0 + sx, d->y0 + sy, w, h, popcode[op], value, NULL);
        else
            run_generic(d, op, value, sx, sy, w, h);

        get_region(d, after);
        memcpy(expect, before, sizeof(expect));
        ref_op(d, expect, op, value, sx, sy, w, h);

        bad = compare_region(d, after, expect, &fx, &fy);
        CHECK(bad == 0, "%s: %s %s(%ld) at %ld,%ld %ldx%ld: %lu pixels differ, first at %ld,%ld "
              "got %02x%02x%02x%02x want %02x%02x%02x%02x (before %02x%02x%02x%02x)",
              d->name, impl ? "generic" : "library", opname[op], (long)value,
              (long)sx, (long)sy, (long)w, (long)h, (unsigned long)bad, (long)fx, (long)fy,
              after[(fy * RW + fx) * 4], after[(fy * RW + fx) * 4 + 1], after[(fy * RW + fx) * 4 + 2], after[(fy * RW + fx) * 4 + 3],
              expect[(fy * RW + fx) * 4], expect[(fy * RW + fx) * 4 + 1], expect[(fy * RW + fx) * 4 + 2], expect[(fy * RW + fx) * 4 + 3],
              before[(fy * RW + fx) * 4], before[(fy * RW + fx) * 4 + 1], before[(fy * RW + fx) * 4 + 2], before[(fy * RW + fx) * 4 + 3]);
    }
}

static void test_ops(struct Dest *d)
{
    static const LONG widths_full[]  = { 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 33, 64, 74 };
    static const LONG heights_full[] = { 1, 2, 3, 5, 9, 33 };
    static const LONG widths_quick[]  = { 1, 3, 8, 9, 17 };
    static const LONG heights_quick[] = { 1, 5 };
    static const LONG alphas[]  = { 0, 0x80, 0xFF };
    static const LONG amounts[] = { 0, 1, 37, 200, 255 };
    static const LONG tints[]   = { 0x00FFFFFF, 0x00000000, 0x000080FF, 0x0080C0FF, 0x12345678, 0x00FF0000 };
    static const LONG blurs[]   = { 0, 1, 8, 64, 128, 255 };
    const LONG *widths  = quick ? widths_quick : widths_full;
    const LONG *heights = quick ? heights_quick : heights_full;
    int nw = quick ? 5 : 15, nh = quick ? 2 : 6;
    ULONG before_fails = fails, before_checks = checks;
    int wi, hi, i;
    ULONG seed = 0x1234567;

    for (wi = 0; wi < nw; wi++)
        for (hi = 0; hi < nh; hi++)
        {
            LONG w = widths[wi], h = heights[hi];
            LONG sx = 3, sy = 2;

            if (sx + w > RW) sx = RW - w;
            if (sy + h > RH) sy = RH - h;

            for (i = 0; i < 3; i++)
                op_case(d, OP_SETALPHA, alphas[i], sx, sy, w, h, seed++);
            for (i = 0; i < 5; i++)
                op_case(d, OP_BRIGHTEN, amounts[i], sx, sy, w, h, seed++);
            for (i = 0; i < 5; i++)
                op_case(d, OP_DARKEN, amounts[i], sx, sy, w, h, seed++);
            for (i = 0; i < 6; i++)
                op_case(d, OP_TINT, tints[i], sx, sy, w, h, seed++);
            for (i = 0; i < 6; i++)
                op_case(d, OP_BLUR, blurs[i], sx, sy, w, h, seed++);
        }

    /* A flat area stays flat through a blur, every component value */
    for (i = 0; i < 256; i += (quick ? 51 : 1))
    {
        ULONG k, bad = 0;
        LONG fx = 0, fy = 0;

        for (k = 0; k < RW * RH; k++)
        {
            region[k * 4 + 0] = i;
            region[k * 4 + 1] = i;
            region[k * 4 + 2] = 255 - i;
            region[k * 4 + 3] = i;
        }
        put_region(d, region);
        get_region(d, before);
        ProcessPixelArray(d->rp, d->x0, d->y0, RW, RH, POP_BLUR, 64, NULL);
        get_region(d, after);
        bad = compare_region(d, after, before, &fx, &fy);
        CHECK(bad == 0, "%s: BLUR of flat %02x/%02x changed %lu pixels, first at %ld,%ld got %02x%02x%02x%02x",
              d->name, i, 255 - i, (unsigned long)bad, (long)fx, (long)fy,
              after[(fy * RW + fx) * 4], after[(fy * RW + fx) * 4 + 1], after[(fy * RW + fx) * 4 + 2], after[(fy * RW + fx) * 4 + 3]);
    }

    printf("  %-28s ProcessPixelArray: %lu checks, %lu failed\n", d->name,
           (unsigned long)(checks - before_checks), (unsigned long)(fails - before_fails));
}

/*****************************************************************************/
/* FillPixelArray(), RPTAG_FgColor/RPTAG_BgColor                              */

#define FX  3       /* fill rectangle inside the region */
#define FY  2
#define FW  6
#define FH  5

/* background, then a filled rectangle: check inside and outside */
static void check_fill(struct Dest *d, const char *what, ULONG argb, LONG fx, LONG fy, LONG fw, LONG fh)
{
    UBYTE want[4];
    LONG x, y;
    ULONG bad = 0;
    LONG bx = 0, by = 0;

    argb_bytes(argb, want);
    get_region(d, after);
    for (y = 0; y < RH; y++)
        for (x = 0; x < RW; x++)
        {
            const UBYTE *g = after + (y * RW + x) * 4;
            BOOL inside = (x >= fx && x < fx + fw && y >= fy && y < fy + fh);
            BOOL ok = inside ? same_pixel(d, g, want) : (memcmp(g, before + (y * RW + x) * 4, 4) == 0);

            if (!ok && !bad++)
            {
                bx = x;
                by = y;
            }
        }
    CHECK(bad == 0, "%s: %s %08lx: %lu pixels wrong, first at %ld,%ld got %02x%02x%02x%02x",
          d->name, what, (unsigned long)argb, (unsigned long)bad, (long)bx, (long)by,
          after[(by * RW + bx) * 4], after[(by * RW + bx) * 4 + 1], after[(by * RW + bx) * 4 + 2], after[(by * RW + bx) * 4 + 3]);
}

/* A colour from GetRPAttrs(): alpha only counts where the destination keeps it */
static BOOL same_argb(struct Dest *d, IPTR got, ULONG want)
{
    UBYTE g[4], w[4];

    argb_bytes(got, g);
    argb_bytes(want, w);
    /* nothing above the 32 bits of ARGB (no sign extension into a 64-bit IPTR) */
    return ((got & ~(IPTR)0xFFFFFFFF) == 0) && same_pixel(d, g, w);
}

static void background(struct Dest *d)
{
    ULONG i;

    for (i = 0; i < RW * RH; i++)
        argb_bytes(0xFF406080, region + i * 4);
    put_region(d, region);
    get_region(d, before);
}

static ULONG pen_argb(struct Dest *d, ULONG pen)
{
    ULONG rgb[3] = { 0, 0, 0 };

    GetRGB32(testscr->ViewPort.ColorMap, pen, 1, rgb);
    return (rgb[0] & 0xFF000000) >> 8 | (rgb[1] & 0xFF000000) >> 16 | (rgb[2] & 0xFF000000) >> 24;
}

static void test_fill(struct Dest *d)
{
    ULONG before_fails = fails, before_checks = checks;
    ULONG colours[8];
    ULONG ncol = 0, i;
    struct RastPort *rp = d->rp;
    IPTR got = 0;
    LONG count;

    colours[ncol++] = 0x80112233;
    colours[ncol++] = 0xFF112233;
    colours[ncol++] = 0x00112233;
    colours[ncol++] = 0x7F000000;
    colours[ncol++] = 0x80FFFFFF;
    /* The FgPen's colour with another alpha must not be taken for the pen */
    colours[ncol++] = pen_argb(d, rp->FgPen) | 0x80000000;
    colours[ncol++] = pen_argb(d, rp->BgPen) | 0x40000000;

    for (i = 0; i < ncol; i++)
    {
        background(d);
        count = FillPixelArray(rp, d->x0 + FX, d->y0 + FY, FW, FH, colours[i]);
        CHECK(count == FW * FH, "%s: FillPixelArray %08lx returned %ld, wanted %d",
              d->name, (unsigned long)colours[i], (long)count, FW * FH);
        check_fill(d, "FillPixelArray", colours[i], FX, FY, FW, FH);
    }

    /* A fill clipped at the region's right and bottom edge */
    background(d);
    FillPixelArray(rp, d->x0 + RW - 4, d->y0 + RH - 3, 4, 3, 0x80ABCDEF);
    check_fill(d, "FillPixelArray (edge)", 0x80ABCDEF, RW - 4, RH - 3, 4, 3);

    /* Explicit colours */
    background(d);
    SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, 0x80445566, TAG_DONE);
    SetDrMd(rp, JAM1);
    RectFill(rp, d->x0 + FX, d->y0 + FY, d->x0 + FX + FW - 1, d->y0 + FY + FH - 1);
    check_fill(d, "RPTAG_FgColor RectFill", 0x80445566, FX, FY, FW, FH);

    GetRPAttrs(rp, RPTAG_FgColor, (IPTR)&got, TAG_DONE);
    CHECK(same_argb(d, got, 0x80445566), "%s: GetRPAttrs(RPTAG_FgColor) = %08lx, wanted 80445566", d->name, (unsigned long)got);

    /* FillPixelArray() uses its own colour and leaves the RastPort's alone */
    background(d);
    FillPixelArray(rp, d->x0 + FX, d->y0 + FY, FW, FH, 0x20445566);
    check_fill(d, "FillPixelArray after RPTAG_FgColor", 0x20445566, FX, FY, FW, FH);
    background(d);
    RectFill(rp, d->x0 + FX, d->y0 + FY, d->x0 + FX + FW - 1, d->y0 + FY + FH - 1);
    check_fill(d, "RPTAG_FgColor RectFill after FillPixelArray", 0x80445566, FX, FY, FW, FH);

    /* Background colour: JAM2 with a pattern of whole rows, set and clear */
    {
        static UWORD pat[2] = { 0xFFFF, 0x0000 };
        UBYTE fg[4], bg[4];
        LONG y, x, nfg = 0, nbg = 0, badrows = 0;

        SetRPAttrs(rp, RPTAG_BgColor, 0x40AABBCC, TAG_DONE);
        GetRPAttrs(rp, RPTAG_BgColor, (IPTR)&got, TAG_DONE);
        CHECK(same_argb(d, got, 0x40AABBCC), "%s: GetRPAttrs(RPTAG_BgColor) = %08lx, wanted 40AABBCC", d->name, (unsigned long)got);

        background(d);
        SetDrMd(rp, JAM2);
        SetAfPt(rp, pat, 1);
        RectFill(rp, d->x0 + FX, d->y0 + FY, d->x0 + FX + FW - 1, d->y0 + FY + FH - 1);
        SetAfPt(rp, NULL, 0);
        SetDrMd(rp, JAM1);
        get_region(d, after);

        argb_bytes(0x80445566, fg);
        argb_bytes(0x40AABBCC, bg);
        for (y = FY; y < FY + FH; y++)
        {
            BOOL rowfg = TRUE, rowbg = TRUE;

            for (x = FX; x < FX + FW; x++)
            {
                const UBYTE *g = after + (y * RW + x) * 4;
                if (!same_pixel(d, g, fg)) rowfg = FALSE;
                if (!same_pixel(d, g, bg)) rowbg = FALSE;
            }
            if (rowfg) nfg++;
            else if (rowbg) nbg++;
            else badrows++;
        }
        CHECK(badrows == 0 && nfg && nbg, "%s: RPTAG_BgColor JAM2 pattern: %ld fg rows, %ld bg rows, %ld wrong "
              "(row %d got %02x%02x%02x%02x)", d->name, (long)nfg, (long)nbg, (long)badrows, FY,
              after[(FY * RW + FX) * 4], after[(FY * RW + FX) * 4 + 1], after[(FY * RW + FX) * 4 + 2], after[(FY * RW + FX) * 4 + 3]);
    }

    /*
     * Back to pens: opaque, the pen's colour, in the destination's own pixel
     * layout (a friend in another layout must not lend it its pixel values).
     * The pen's components differ, so a component order mix-up shows.
     */
    background(d);
    SetRPAttrs(rp, RPTAG_PenMode, TRUE, TAG_DONE);
    SetAPen(rp, testpen);
    RectFill(rp, d->x0 + FX, d->y0 + FY, d->x0 + FX + FW - 1, d->y0 + FY + FH - 1);
    check_fill(d, "test pen RectFill", 0xFF2266AA, FX, FY, FW, FH);

    /* A palette change on the screen reaches the destination's colours */
    SetRGB32(&testscr->ViewPort, testpen, 0x99999999, 0x33333333, 0x55555555);
    background(d);
    RectFill(rp, d->x0 + FX, d->y0 + FY, d->x0 + FX + FW - 1, d->y0 + FY + FH - 1);
    check_fill(d, "test pen RectFill after SetRGB32", 0xFF993355, FX, FY, FW, FH);
    SetRGB32(&testscr->ViewPort, testpen, 0x22222222, 0x66666666, 0xAAAAAAAA);
    SetAPen(rp, 1);

    printf("  %-28s FillPixelArray/RPTAG colours: %lu checks, %lu failed\n", d->name,
           (unsigned long)(checks - before_checks), (unsigned long)(fails - before_fails));
}

/*****************************************************************************/

static BOOL offscreen(struct Dest *d, const char *name, ULONG pixfmt, ULONG depth, BOOL alpha, UBYTE tol,
                      struct BitMap *friend)
{
    memset(d, 0, sizeof(*d));
    d->name  = name;
    d->alpha = alpha;
    d->tol   = tol;
    d->bm = AllocBitMap(RW + 8, RH + 6, depth, BMF_SPECIALFMT | SHIFT_PIXFMT(pixfmt) | BMF_CLEAR, friend);
    if (!d->bm)
    {
        printf("  %-28s SKIP: cannot allocate\n", name);
        return FALSE;
    }
    if (GetCyberMapAttr(d->bm, CYBRMATTR_PIXFMT) != pixfmt)
    {
        printf("  %-28s SKIP: got pixel format %lu\n", name, (unsigned long)GetCyberMapAttr(d->bm, CYBRMATTR_PIXFMT));
        FreeBitMap(d->bm);
        d->bm = NULL;
        return FALSE;
    }
    InitRastPort(&d->ownrp);
    d->ownrp.BitMap = d->bm;
    d->rp = &d->ownrp;
    d->x0 = 5;
    d->y0 = 3;
    return TRUE;
}

static void run(struct Dest *d)
{
    test_fill(d);
    test_ops(d);
}

/* A truecolour screen of our own, the deepest there is */
static struct Screen *open_truecolour_screen(void)
{
    static const ULONG depths[] = { 32, 24, 16 };
    ULONG i;

    for (i = 0; i < 3; i++)
    {
        struct TagItem modetags[] =
        {
            { CYBRBIDTG_Depth,         depths[i] },
            { CYBRBIDTG_NominalWidth,  640       },
            { CYBRBIDTG_NominalHeight, 480       },
            { TAG_DONE,                0         }
        };
        ULONG id = BestCModeIDTagList(modetags);
        struct Screen *s;

        if (id == INVALID_ID)
            continue;
        s = OpenScreenTags(NULL,
            SA_DisplayID, id, SA_Depth, depths[i], SA_Width, 640, SA_Height, 480,
            SA_Title, (IPTR)"testarrayops CHECK", SA_ShowTitle, TRUE, SA_LikeWorkbench, TRUE,
            TAG_DONE);
        if (s)
            return s;
    }
    return NULL;
}

static int run_checks(BOOL ownscreen)
{
    struct Screen *scr, *pub;
    struct Window *aw = NULL, *pw = NULL, *cover = NULL;
    struct Dest d;
    struct BitMap *friend;

    pub = LockPubScreen(NULL);
    if (!ownscreen && pub && GetCyberMapAttr(pub->RastPort.BitMap, CYBRMATTR_ISCYBERGFX) &&
        GetCyberMapAttr(pub->RastPort.BitMap, CYBRMATTR_BPPIX) >= 2)
        scr = pub;
    else
    {
        if (pub)
            UnlockPubScreen(NULL, pub);
        pub = NULL;
        scr = open_truecolour_screen();
    }
    if (!scr)
    {
        printf("testarrayops CHECK: no truecolour screen\n");
        return RETURN_FAIL;
    }
    testscr = scr;
    friend = scr->RastPort.BitMap;

    /* Exclusive, so that the test may change its colour (SetRGB32()) */
    testpen = ObtainPen(scr->ViewPort.ColorMap, -1, 0x22222222, 0x66666666, 0xAAAAAAAA, PEN_EXCLUSIVE);
    if (testpen < 0)
    {
        printf("testarrayops CHECK: no pen for the test colour\n");
        if (pub)
            UnlockPubScreen(NULL, pub);
        else
            CloseScreen(scr);
        return RETURN_FAIL;
    }
    printf("testarrayops CHECK: screen %ldx%ld, pixel format %lu, %lu bytes per pixel%s\n",
           (long)scr->Width, (long)scr->Height,
           (unsigned long)GetCyberMapAttr(friend, CYBRMATTR_PIXFMT),
           (unsigned long)GetCyberMapAttr(friend, CYBRMATTR_BPPIX), quick ? " (QUICK)" : "");

    if (offscreen(&d, "ARGB32 bitmap", PIXFMT_ARGB32, 32, TRUE, 0, friend))  { run(&d); FreeBitMap(d.bm); }
    if (offscreen(&d, "BGRA32 bitmap", PIXFMT_BGRA32, 32, TRUE, 0, friend))  { run(&d); FreeBitMap(d.bm); }
    if (offscreen(&d, "RGBA32 bitmap", PIXFMT_RGBA32, 32, TRUE, 0, friend))  { run(&d); FreeBitMap(d.bm); }
    if (offscreen(&d, "0RGB32 bitmap", PIXFMT_0RGB32, 32, FALSE, 0, friend)) { run(&d); FreeBitMap(d.bm); }
    if (offscreen(&d, "RGB24 bitmap", PIXFMT_RGB24, 24, FALSE, 0, friend))   { run(&d); FreeBitMap(d.bm); }
    if (offscreen(&d, "RGB16 bitmap", PIXFMT_RGB16, 16, FALSE, 8, friend))   { run(&d); FreeBitMap(d.bm); }

    /* A composited window with an alpha surface */
    aw = OpenWindowTags(NULL,
        WA_Left, 40, WA_Top, 40, WA_InnerWidth, RW, WA_InnerHeight, RH,
        WA_Title, (IPTR)"testarrayops alpha", WA_DragBar, TRUE,
        WA_AlphaSurface, TRUE, WA_Activate, FALSE, WA_CustomScreen, (IPTR)scr, TAG_END);
    if (aw && (aw->WLayer->Flags & LAYERCOMPOSITED))
    {
        memset(&d, 0, sizeof(d));
        d.name = "alpha surface window";
        d.rp = aw->RPort;
        d.x0 = aw->BorderLeft;
        d.y0 = aw->BorderTop;
        d.alpha = TRUE;
        run(&d);
    }
    else
        printf("  %-28s SKIP: no composited alpha surface on this screen\n", "alpha surface window");
    if (aw)
        CloseWindow(aw);

    /* An ordinary smart refresh window, partly covered: visible and backing store cliprects */
    pw = OpenWindowTags(NULL,
        WA_Left, 40, WA_Top, 160, WA_InnerWidth, RW, WA_InnerHeight, RH,
        WA_Title, (IPTR)"testarrayops plain", WA_DragBar, TRUE, WA_SmartRefresh, TRUE,
        WA_Activate, FALSE, WA_CustomScreen, (IPTR)scr, TAG_END);
    if (pw)
        cover = OpenWindowTags(NULL,
            WA_Left, 40 + pw->BorderLeft + RW / 2, WA_Top, 160 + pw->BorderTop + RH / 3,
            WA_Width, RW, WA_Height, RH,
            WA_Title, (IPTR)"cover", WA_SmartRefresh, TRUE,
            WA_Activate, FALSE, WA_CustomScreen, (IPTR)scr, TAG_END);
    if (pw)
    {
        LONG tol = (GetCyberMapAttr(friend, CYBRMATTR_BPPIX) == 2) ? 8 : 0;

        memset(&d, 0, sizeof(d));
        d.name = cover ? "plain window, partly covered" : "plain window";
        d.rp = pw->RPort;
        d.x0 = pw->BorderLeft;
        d.y0 = pw->BorderTop;
        /* A screen's 32-bit format may be reported as ...A32 for a pad byte: no alpha assumed */
        d.alpha = FALSE;
        d.tol = tol;
        run(&d);
    }
    if (cover)
        CloseWindow(cover);
    if (pw)
        CloseWindow(pw);

    ReleasePen(scr->ViewPort.ColorMap, testpen);
    if (pub)
        UnlockPubScreen(NULL, pub);
    else
        CloseScreen(scr);

    printf("testarrayops CHECK: %lu checks, %lu failed: %s\n", (unsigned long)checks, (unsigned long)fails,
           fails ? "FAIL" : "PASS");
    return fails ? RETURN_ERROR : RETURN_OK;
}

/*****************************************************************************/
/* Demonstration and benchmark                                                */

struct BitMap *LoadPictureAsBitMap(CONST_STRPTR filename, struct BitMap **testbm)
{
    Object *dto = NULL;
    struct BitMap *bm = NULL;

    dto = NewDTObject((STRPTR)filename,
						DTA_SourceType, 		DTST_FILE,
						DTA_GroupID, 			GID_PICTURE,
						OBP_Precision, 			PRECISION_EXACT,
						PDTA_FreeSourceBitMap, 	FALSE,
						PDTA_Remap, 			FALSE,
						PDTA_DestMode,   		PMODE_V43,
						TAG_END);

    if (dto)
    {
        struct BitMapHeader *bmhd;
        GetAttr(PDTA_BitMapHeader, dto, (APTR)&bmhd);
        if(bmhd)
        {
			ULONG width = bmhd->bmh_Width, height = bmhd->bmh_Height, depth = bmhd->bmh_Depth;
			struct FrameInfo fri = {0};
			DoMethod(dto, DTM_FRAMEBOX, NULL, (IPTR)&fri, (IPTR)&fri, sizeof(struct FrameInfo),0);
			if (fri.fri_Dimensions.Depth > 0)
			{
				if (DoMethod(dto, DTM_PROCLAYOUT, NULL, TRUE))
				{
					GetAttr(PDTA_DestBitMap, dto, (APTR)&bm);
					if (!bm)
						GetAttr(PDTA_BitMap, dto, (APTR)&bm);
					if (bm)
					{
						struct Screen *pubScreen = LockPubScreen(NULL);

						struct BitMap *clone2, *clone = AllocBitMap(
							width, height, depth,
							0,
							(pubScreen) ? pubScreen->RastPort.BitMap : 0);

						clone2 = AllocBitMap(
							width, height, depth,
							0,
							(pubScreen) ? pubScreen->RastPort.BitMap : 0);
						
						if (pubScreen)
							UnlockPubScreen(NULL, pubScreen);

						if (clone)
						{
							struct RastPort rp_dst;
							InitRastPort(&rp_dst);
							rp_dst.BitMap = clone;
							if (clone2) {
								BltBitMapRastPort(bm, 0, 0, &rp_dst, 0, 0, bm->BytesPerRow * 8, bm->Rows, 0xC0);
								*testbm = clone2;
							}
							else {
								FreeBitMap(clone);
								clone = NULL;
							}
						}
						else {
							if (clone2)
								FreeBitMap(clone2);
						}

						bm = clone;
					}
				}
			}
		}

        DisposeDTObject(dto);
    }

    return bm;
}

int main(void)
{
    struct BitMap *bm = NULL, *testbm = NULL;
    struct Window *win = NULL;
    IPTR args[3] = { 0, 0, 0 };
    struct RDArgs *rda;
    BOOL check = FALSE, ownscreen = FALSE;

    rda = ReadArgs("CHECK/S,QUICK/S,SCREEN/S", args, NULL);
    if (rda)
    {
        check = args[0] ? TRUE : FALSE;
        quick = args[1] ? TRUE : FALSE;
        ownscreen = args[2] ? TRUE : FALSE;
        FreeArgs(rda);
    }
    if (check)
        return run_checks(ownscreen);

    /* Load the bitmap from file */
    bm = LoadPictureAsBitMap("SYS:Developer/Debug/Tests/Datatypes/png/SamplePNGImage_1mb.png", &testbm);
    if (!bm)
    {
        printf("Failed to load image\n");
        return 20;
    }

    /* Open a simple window */
    win = OpenWindowTags(NULL,
                         WA_Left,        100,
                         WA_Top,         100,
                         WA_Width,       GetBitMapAttr(bm, BMA_WIDTH),
                         WA_Height,      GetBitMapAttr(bm, BMA_HEIGHT),
                         WA_Title,       (IPTR)"Image Viewer",
                         WA_Flags,       WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_SMART_REFRESH | WFLG_ACTIVATE,
                         WA_IDCMP,       IDCMP_CLOSEWINDOW,
                         TAG_END);

    if (!win)
    {
        printf("Failed to open window\n");
        FreeBitMap(bm);
        return 20;
    }

    {
		ULONG 	width = GetBitMapAttr(bm, BMA_WIDTH),
				height = GetBitMapAttr(bm, BMA_HEIGHT);
		ULONG endpos, value;
		struct timeval tv_start, tv_end;
        struct RastPort rp_src;
        InitRastPort(&rp_src);
        rp_src.BitMap = bm;
		LONG dur;
		int i, t;

        BltBitMapRastPort(bm,
                          0, 0,
                          win->RPort,
                          0, 0,
                          width,
                          height,
                          0xC0);

		ULONG tiles = width / 138, step = (256/tiles);
		printf("Testing Brighten\n");
		for (t = i = 0; i < 256; t++, i = i + step) {
			Delay(25);
			ProcessPixelArray(&rp_src, 10 + (t * 138), 10, 128, 128, POP_BRIGHTEN, i, NULL);
			BltBitMapRastPort(bm,
				  0, 0,
				  win->RPort,
				  0, 0,
				  width,
				  height,
				  0xC0);   
		}
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		endpos = 10 + (tiles - 1 * 138);
		value = (tiles - 1) * step;
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 0, 0, 512, 128, POP_BRIGHTEN, value, NULL);
		}
		Permit();
		printf("\n Aligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 512x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, endpos, 10, 128, 128, POP_BRIGHTEN, value, NULL);
		}
		Permit();
		printf(" Missaligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 128x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);

        rp_src.BitMap = bm;		
		printf("\nTesting Darken\n");
		for (t = i = 0; i < 256; t++, i = i + step) {
			Delay(25);
			ProcessPixelArray(&rp_src, 10 + (t * 138), 148, 128, 128, POP_DARKEN, i, NULL);
			BltBitMapRastPort(bm,
				  0, 0,
				  win->RPort,
				  0, 0,
				  width,
				  height,
				  0xC0);   
		}
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		value = (tiles - 1) * step;
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 0, 0, 512, 128, POP_DARKEN, value, NULL);
		}
		Permit();
		printf("\n Aligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 512x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, endpos, 148, 128, 128, POP_DARKEN, value, NULL);
		}
		Permit();
		printf(" Missaligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 128x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);

        rp_src.BitMap = bm;
		printf("\nTesting Tint\n");
		for (t = 0; t < tiles; t++) {
			Delay(3 * 50);
			ULONG tintval = 0xFFFF2020;
			ProcessPixelArray(&rp_src, 10 + (t * 138), 296, 128, 128, POP_TINT, (LONG)tintval, NULL);
			BltBitMapRastPort(bm,
				  0, 0,
				  win->RPort,
				  0, 0,
				  width,
				  height,
				  0xC0);   
		}
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 0, 0, 512, 128, POP_TINT, (LONG)0xFFFF2020, NULL);
		}
		Permit();
		printf("\n Aligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 512x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, endpos, 296, 128, 128, POP_TINT, (LONG)0xFFFF2020, NULL);
		}
		Permit();
		printf(" Missaligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 128x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);


		Delay(3 * 50);
        rp_src.BitMap = bm;
		printf("\nTesting Blur\n");
		ProcessPixelArray(&rp_src, width / 3 , 10, width / 3, height - 20, POP_BLUR, 128, NULL);
		ProcessPixelArray(&rp_src, (width / 3) * 2 , 10, (width / 3) - 10, height - 20, POP_BLUR, 255, NULL);
		BltBitMapRastPort(bm,
			  0, 0,
			  win->RPort,
			  0, 0,
			  width,
			  height,
			  0xC0);
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 10, 10, width - 20, height - 20, POP_BLUR, 0, NULL);
		}
		Permit();
		printf("\n Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf(" Operation count  : %d (x %ux%u)\n", (int)i, width - 20, height - 20);
		printf(" Ops/sec          : %f\n", i * 1000000.0 / dur);

    }

	printf("Testing complete\n");

    BOOL done = FALSE;
    struct IntuiMessage *msg;
    while (!done)
    {
        WaitPort(win->UserPort);
        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            if (msg->Class == IDCMP_CLOSEWINDOW)
                done = TRUE;
            ReplyMsg((struct Message *)msg);
        }
    }

    /* Cleanup */
    CloseWindow(win);
    FreeBitMap(testbm);
    FreeBitMap(bm);

    return 0;
}
