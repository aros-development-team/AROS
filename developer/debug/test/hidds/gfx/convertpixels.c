/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Times the bitmap format conversion routines (ConvertPixels).
          CHECK: instead checks every conversion into a 32 or 24 bit format
          against the formats' byte layouts, for widths that exercise both the
          SIMD blocks and the scalar remainders of the optimised converters:
          the colour must survive, and the alpha byte of a 32 bit result must
          be 0xFF when the source has no alpha (and the source's alpha
          otherwise).

    Usage: convertpixels [CHECK]
*/

#define __OOP_NOATTRBASES__

#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/intuition.h>

#include <devices/timer.h>
#include <dos/rdargs.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#undef ConvertPixels

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddDisplayAttrBase;

#if AROS_BIG_ENDIAN
#define SRC_PIXFMT vHidd_StdPixFmt_ARGB32
#define DST_PIXFMT vHidd_StdPixFmt_RGB15
#define DST_PIXFMT2 vHidd_StdPixFmt_BGRA32
#define DST_PIXFMT24 vHidd_StdPixFmt_RGB24
#else
#define SRC_PIXFMT vHidd_StdPixFmt_BGRA32
#define DST_PIXFMT vHidd_StdPixFmt_RGB15_LE
#define DST_PIXFMT2 vHidd_StdPixFmt_ARGB32
#define DST_PIXFMT24 vHidd_StdPixFmt_BGR24
#endif

static void ConvertPixels(APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt,
                   APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt,
                   ULONG width, ULONG height, OOP_Object *bm)
{
    OOP_Object *display = NULL, *dmenum = NULL;
    OOP_Object *srcpf, *dstpf;
    APTR src = srcPixels;
    APTR dst = dstPixels;

    OOP_GetAttr(bm, aHidd_BitMap_Display, (IPTR *)&display);
    if (display)
        OOP_GetAttr(display, aHidd_Display_DMEnumerator, (IPTR *)&dmenum);

    if (!dmenum) {
        printf("ConvertPixels(): Failed to obtain graphics driver\n");
        return;
    }

    srcpf = HIDD_DMEnum_GetPixFmt(dmenum, srcPixFmt);
    dstpf = HIDD_DMEnum_GetPixFmt(dmenum, dstPixFmt);

    if (!srcpf || !dstpf)
    {
        printf("ConvertPixels(): Bad source (%ld) or dest (%ld) pixfmt!\n", srcPixFmt, dstPixFmt);
        return;
    }

    HIDD_BM_ConvertPixels(bm, &src, (HIDDT_PixelFormat *)srcpf, srcMod,
                          &dst, (HIDDT_PixelFormat *)dstpf, dstMod,
                          width, height, NULL);
}

#define GFXBUFPIXCNT (640 * 480)
#define GFXBUFSIZE (GFXBUFPIXCNT << 2)

/*
 * Byte layout in memory of the formats CHECK uses: offsets of R, G, B and
 * alpha (-1: none) within a pixel. 15/16 bit sources are only checked for
 * the alpha of the result.
 */
struct fmtinfo
{
    HIDDT_StdPixFmt fmt;
    const char *name;
    UBYTE bpp;
    BYTE r, g, b, a;
};

static const struct fmtinfo fmts[] =
{
    { vHidd_StdPixFmt_ARGB32,   "ARGB32",   4, 1, 2, 3,  0 },
    { vHidd_StdPixFmt_BGRA32,   "BGRA32",   4, 2, 1, 0,  3 },
    { vHidd_StdPixFmt_RGBA32,   "RGBA32",   4, 0, 1, 2,  3 },
    { vHidd_StdPixFmt_ABGR32,   "ABGR32",   4, 3, 2, 1,  0 },
    { vHidd_StdPixFmt_0RGB32,   "0RGB32",   4, 1, 2, 3, -1 },
    { vHidd_StdPixFmt_BGR032,   "BGR032",   4, 2, 1, 0, -1 },
    { vHidd_StdPixFmt_RGB032,   "RGB032",   4, 0, 1, 2, -1 },
    { vHidd_StdPixFmt_0BGR32,   "0BGR32",   4, 3, 2, 1, -1 },
    { vHidd_StdPixFmt_RGB24,    "RGB24",    3, 0, 1, 2, -1 },
    { vHidd_StdPixFmt_BGR24,    "BGR24",    3, 2, 1, 0, -1 },
    { vHidd_StdPixFmt_RGB16,    "RGB16",    2, -1, -1, -1, -1 },
    { vHidd_StdPixFmt_RGB16_LE, "RGB16_LE", 2, -1, -1, -1, -1 },
    { vHidd_StdPixFmt_BGR16,    "BGR16",    2, -1, -1, -1, -1 },
    { vHidd_StdPixFmt_BGR16_LE, "BGR16_LE", 2, -1, -1, -1, -1 },
    { vHidd_StdPixFmt_RGB15,    "RGB15",    2, -1, -1, -1, -1 },
    { vHidd_StdPixFmt_RGB15_LE, "RGB15_LE", 2, -1, -1, -1, -1 },
    { vHidd_StdPixFmt_BGR15,    "BGR15",    2, -1, -1, -1, -1 },
    { vHidd_StdPixFmt_BGR15_LE, "BGR15_LE", 2, -1, -1, -1, -1 },
};
#define NUMFMTS (sizeof(fmts) / sizeof(fmts[0]))
#define NUMDSTFMTS 10   /* the 32 and 24 bit formats lead the table */

#define CHECK_MAXW   37 /* past two 16 pixel SIMD blocks, every remainder */
#define CHECK_H      2
#define CHECK_SRCMOD (CHECK_MAXW * 4 + 8)
#define CHECK_DSTMOD (CHECK_MAXW * 4 + 12)

static ULONG check_conversions(OOP_Object *bm)
{
    static UBYTE src[CHECK_SRCMOD * CHECK_H], dst[CHECK_DSTMOD * CHECK_H];
    ULONG seed = 0x12345678, checks = 0, fails = 0, i;
    UWORD si, di, w, x, y;

    for (si = 0; si < NUMFMTS; si++)
    {
        const struct fmtinfo *sf = &fmts[si];

        for (di = 0; di < NUMDSTFMTS; di++)
        {
            const struct fmtinfo *df = &fmts[di];
            ULONG pairfails = 0;

            if (si == di)
                continue;
            for (w = 1; w <= CHECK_MAXW; w++)
            {
                for (i = 0; i < sizeof(src); i++)
                {
                    seed = seed * 1103515245 + 12345;
                    src[i] = seed >> 16;
                }
                memset(dst, 0x5A, sizeof(dst));
                ConvertPixels(src, CHECK_SRCMOD, sf->fmt, dst, CHECK_DSTMOD, df->fmt, w, CHECK_H, bm);

                for (y = 0; y < CHECK_H; y++)
                {
                    for (x = 0; x < w; x++)
                    {
                        UBYTE *s = &src[y * CHECK_SRCMOD + x * sf->bpp];
                        UBYTE *d = &dst[y * CHECK_DSTMOD + x * df->bpp];
                        BOOL ok = TRUE;

                        if ((sf->r >= 0) &&
                            ((d[df->r] != s[sf->r]) || (d[df->g] != s[sf->g]) || (d[df->b] != s[sf->b])))
                            ok = FALSE;
                        if ((df->a >= 0) && (d[df->a] != ((sf->a >= 0) ? s[sf->a] : 0xFF)))
                            ok = FALSE;
                        checks++;
                        if (!ok)
                        {
                            if (pairfails++ < 3)
                                printf("  FAIL: %s -> %s width %u pixel %u,%u: %02x%02x%02x%02x\n",
                                       sf->name, df->name, w, x, y, d[0], d[1], d[2], d[3]);
                            fails++;
                        }
                    }
                }
                /* nothing past the converted width may be touched */
                for (y = 0; y < CHECK_H; y++)
                {
                    for (i = w * df->bpp; i < CHECK_DSTMOD; i++)
                    {
                        if (dst[y * CHECK_DSTMOD + i] != 0x5A)
                        {
                            if (pairfails++ < 3)
                                printf("  FAIL: %s -> %s width %u wrote past the row (byte %lu)\n",
                                       sf->name, df->name, w, (unsigned long)i);
                            fails++;
                            break;
                        }
                    }
                }
            }
            if (pairfails > 3)
                printf("  ... %s -> %s: %lu failures\n", sf->name, df->name, (unsigned long)pairfails);
        }
    }
    printf("convertpixels CHECK: %lu pixels checked, %lu failed: %s\n",
           (unsigned long)checks, (unsigned long)fails, fails ? "FAIL" : "PASS");
    return fails;
}

int main(void)
{
    IPTR checkarg = 0;
    struct RDArgs *rda;

    struct BitMap *bitmap;
    struct timeval tv_start, tv_end;
    LONG t, i;

    ULONG *argb;
    UWORD *rgb15;
    ULONG *argb_inv;

    rda = ReadArgs("CHECK/S", &checkarg, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "convertpixels");
        return RETURN_FAIL;
    }
    FreeArgs(rda);

    if (!checkarg)
        printf("Convertpixels: Testing bitmap format conversion routines.\n");


    argb = AllocMem(GFXBUFSIZE, MEMF_ANY);
    if (!argb) {
        printf("Failed to allocate buffer for conversions\n");
        return RETURN_FAIL;
    }
    rgb15 = AllocMem(GFXBUFSIZE, MEMF_ANY);
    if (!rgb15) {
        FreeMem(argb, GFXBUFSIZE);
        printf("Failed to allocate buffer for conversions\n");
        return RETURN_FAIL;
    }
    argb_inv = AllocMem(GFXBUFSIZE, MEMF_ANY);
    if (!argb_inv) {
        FreeMem(rgb15, GFXBUFSIZE);
        FreeMem(argb, GFXBUFSIZE);
        printf("Failed to allocate buffer for conversions\n");
        return RETURN_FAIL;
    }

    argb[0] = 0x00112233;
    argb[1] = 0x00FFFFFF;
    argb[2] = 0xFF888888;
    argb[3] = 0x00FF0000;
    argb[4] = 0x0000FF00;
    argb[5] = 0x000000FF;
    argb[6] = 0x00FFFF00;
    argb[7] = 0x8899AABB;

    HiddBitMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    if (!HiddBitMapAttrBase) {
        FreeMem(argb_inv, GFXBUFSIZE);
        FreeMem(rgb15, GFXBUFSIZE);
        FreeMem(argb, GFXBUFSIZE);
        printf("Failed to obtain IID_Hidd_BitMap\n");
        return RETURN_FAIL;
    }
    
    bitmap = AllocBitMap(1, 1, 16, 0, NULL);
    if (!bitmap) {
        FreeMem(argb_inv, GFXBUFSIZE);
        FreeMem(rgb15, GFXBUFSIZE);
        FreeMem(argb, GFXBUFSIZE);
        printf("Failed to allocate a placeholder bitmap!\n");
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
        return RETURN_FAIL;
    }

    HiddDisplayAttrBase = OOP_ObtainAttrBase(IID_Hidd_Display);
    if (!HiddDisplayAttrBase) {
        FreeMem(argb_inv, GFXBUFSIZE);
        FreeMem(rgb15, GFXBUFSIZE);
        FreeMem(argb, GFXBUFSIZE);
        printf("Failed to obtain IID_Hidd_Display\n");
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
        return RETURN_FAIL;
    }

    if (checkarg)
    {
        ULONG fails = check_conversions(HIDD_BM_OBJ(bitmap));

        FreeBitMap(bitmap);
        FreeMem(argb_inv, GFXBUFSIZE);
        FreeMem(rgb15, GFXBUFSIZE);
        FreeMem(argb, GFXBUFSIZE);
        OOP_ReleaseAttrBase(IID_Hidd_Display);
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
        return fails ? RETURN_FAIL : RETURN_OK;
    }

    printf("\nConverting 32bit to 15bit...\n");

    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 4 * 1000000) break;
        ConvertPixels(argb, 0, SRC_PIXFMT, rgb15, 0, DST_PIXFMT, GFXBUFPIXCNT, 1, HIDD_BM_OBJ(bitmap));
    }
    printf("\n Elapsed time         : %d us (%f s)\n", (int)t, (double)t / 1000000);
    printf(" Conversions          : %d\n", (int)i);
    printf(" Conversions/sec      : %f\n", i * 1000000.0 / t);

    printf("\nConverting 15bit to 32bit...\n");

    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 4 * 1000000) break;
        ConvertPixels(rgb15, 0, DST_PIXFMT, argb_inv, 0, SRC_PIXFMT, GFXBUFPIXCNT, 1, HIDD_BM_OBJ(bitmap));
    }
    printf("\n Elapsed time         : %d us (%f s)\n", (int)t, (double)t / 1000000);
    printf(" Conversions          : %d\n", (int)i);
    printf(" Conversions/sec      : %f\n", i * 1000000.0 / t);

    {
        int i;
        
        for(i = 0; i < 8; i++)
        {
            printf("    ARGB32 %08x = RGB15 %04x (%02x %02x %02x) (%3d%% %3d%% %3d%%) [%08x]\n",
                    (unsigned int)argb[i], rgb15[i],
                    (rgb15[i] & 0x7C00) >> 10,
                    (rgb15[i] & 0x03E0) >> 5,
                    (rgb15[i] & 0x001F),
                    ((rgb15[i] & 0x7C00) >> 10) * 100 / 31,
                    ((rgb15[i] & 0x03E0) >> 5) * 100 / 31,
                    (rgb15[i] & 0x001F) * 100 / 31,
                    (unsigned int)argb_inv[i]
                    );
        }
    }

    printf("\nConverting 32bit to 24bit...\n");

    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 4 * 1000000) break;
        ConvertPixels(argb, 0, SRC_PIXFMT, rgb15, 0, DST_PIXFMT24, GFXBUFPIXCNT, 1, HIDD_BM_OBJ(bitmap));
    }
    printf("\n Elapsed time         : %d us (%f s)\n", (int)t, (double)t / 1000000);
    printf(" Conversions          : %d\n", (int)i);
    printf(" Conversions/sec      : %f\n", i * 1000000.0 / t);

    printf("\nConverting 24bit to 32bit...\n");

    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 4 * 1000000) break;
        ConvertPixels(rgb15, 0, DST_PIXFMT24, argb_inv, 0, SRC_PIXFMT, GFXBUFPIXCNT, 1, HIDD_BM_OBJ(bitmap));
    }
    printf("\n Elapsed time         : %d us (%f s)\n", (int)t, (double)t / 1000000);
    printf(" Conversions          : %d\n", (int)i);
    printf(" Conversions/sec      : %f\n", i * 1000000.0 / t);

    printf("\nPerforming 32bit format conversions...\n");

    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 4 * 1000000) break;
        ConvertPixels(argb, 0, SRC_PIXFMT, rgb15, 0, DST_PIXFMT2, GFXBUFPIXCNT, 1, HIDD_BM_OBJ(bitmap));
    }
    printf("\n Elapsed time         : %d us (%f s)\n", (int)t, (double)t / 1000000);
    printf(" Conversions          : %d\n", (int)i);
    printf(" Conversions/sec      : %f\n", i * 1000000.0 / t);

    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 4 * 1000000) break;
        ConvertPixels(rgb15, 0, DST_PIXFMT2, argb_inv, 0, SRC_PIXFMT, GFXBUFPIXCNT, 1, HIDD_BM_OBJ(bitmap));
    }
    printf("\n Elapsed time         : %d us (%f s)\n", (int)t, (double)t / 1000000);
    printf(" Conversions          : %d\n", (int)i);
    printf(" Conversions/sec      : %f\n", i * 1000000.0 / t);

    printf("\nTesting complete.\n");

    FreeMem(argb_inv, GFXBUFSIZE);
    FreeMem(rgb15, GFXBUFSIZE);
    FreeMem(argb, GFXBUFSIZE);

    OOP_ReleaseAttrBase(IID_Hidd_Display);
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);

    return 0;
}

