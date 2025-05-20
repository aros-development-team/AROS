/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
*/

#define __OOP_NOATTRBASES__

#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/intuition.h>

#include <devices/timer.h>

#include <stdio.h>

#undef ConvertPixels

static OOP_AttrBase HiddBitMapAttrBase;

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
    OOP_Object *gfxhidd = NULL;
    OOP_Object *srcpf, *dstpf;
    APTR src = srcPixels;
    APTR dst = dstPixels;

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);

    if (!gfxhidd) {
        printf("ConvertPixels(): Failed to obtain graphics driver\n");
        return;
    }

    srcpf = HIDD_Gfx_GetPixFmt(gfxhidd, srcPixFmt);
    dstpf = HIDD_Gfx_GetPixFmt(gfxhidd, dstPixFmt);

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

int main(void)
{
    struct BitMap *bitmap;
    struct timeval tv_start, tv_end;
    LONG t, i;

    ULONG *argb;
    UWORD *rgb15;
    ULONG *argb_inv;

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

    OOP_ReleaseAttrBase(IID_Hidd_BitMap);

    return 0;
}

