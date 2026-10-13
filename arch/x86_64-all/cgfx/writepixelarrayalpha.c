/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: x86-64 version of cybergraphics.library/WritePixelArrayAlpha().
          Blends with SSE2, which every x86-64 CPU has, and hands the row
          work to the AVX2 kernels in writepixelarrayalpha_avx.c when the
          CPU and the kernel support them.
*/

#include <hidd/gfx.h>
#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

#include <emmintrin.h>

#include "writepixelarrayalpha_simd.h"

struct render_data
{
    UBYTE              *array;
    ULONG               modulo;
    UBYTE               alpha;      /* global alpha, 0xFF = source alpha only */
    struct IntCGFXBase *CyberGfxBase;
    OOP_MethodID        mid_PutAlphaImage;
    OOP_Class          *chunkybm;
    wpaa_blendrow_t     blendrow;
    wpaa_scalerow_t     scalerow;
};

/* Rows go through a bounded buffer so large blits need no large allocation */
#define WPAA_BUFSIZE 65536

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/****************************************************************************/

#define cpuid(num, subnum) \
    do { asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(num),"c"(subnum)); } while(0)

/*
 * AVX2 is usable when the CPU has it and the kernel saves the YMM state
 * (OSXSAVE set, XCR0 enabling both SSE and AVX state). The answer cannot
 * change, so it is worked out once.
 */
static int has_avx2(void)
{
    static int state = -1;

    if (state < 0)
    {
        ULONG eax, ebx, ecx, edx;
        int ret = 0;

        cpuid(0x00000000, 0x0);
        if (eax >= 7)
        {
            cpuid(0x00000001, 0x0);
            /* Bit 27 of ECX = OSXSAVE, bit 28 = AVX */
            if ((ecx & ((1 << 27) | (1 << 28))) == ((1 << 27) | (1 << 28)))
            {
                asm volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
                if ((eax & 0x6) == 0x6)
                {
                    cpuid(0x00000007, 0x0);
                    ret = (ebx & (1 << 5)) != 0;    /* Bit 5 of EBX = AVX2 */
                }
            }
        }
        state = ret;
    }

    return state;
}

/*
 * Blend four ARGB32 pixels at a time.
 *
 * The bytes of two pixels are widened to eight 16-bit lanes. For each
 * pixel the source alpha is copied to all four of its lanes and scaled by
 * the global alpha, a' = round(a * galpha / 255). Each channel is then
 *
 *     out = round((src * a' + dst * (255 - a')) / 255)
 *
 * which never exceeds 16 bits, the two weights adding up to 255. The
 * division is the usual (t + 128 + ((t + 128) >> 8)) >> 8. The alpha lane
 * comes out meaningless and is replaced: this path only runs for an opaque
 * destination, which stays opaque.
 */
static void blend_row_sse2(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha, BOOL dstalpha)
{
    const __m128i zero  = _mm_setzero_si128();
    const __m128i vga   = _mm_set1_epi16(galpha);
    const __m128i c127  = _mm_set1_epi16(127);
    const __m128i c128  = _mm_set1_epi16(128);
    const __m128i c255  = _mm_set1_epi16(255);
    const __m128i amask = _mm_set1_epi32(0x000000FF);
    const BOOL    noscale = (galpha == 0xFF);
    ULONG x = 0;

    for (; x + 4 <= width; x += 4)
    {
        __m128i s = _mm_loadu_si128((const __m128i *)(src + x * 4));
        __m128i sa = _mm_and_si128(s, amask);
        __m128i d, s16, d16, a, ia, t, lo, hi;

        /* Nothing to do where every source pixel is fully transparent */
        if (_mm_movemask_epi8(_mm_cmpeq_epi32(sa, zero)) == 0xFFFF)
            continue;

        /* Fully opaque source pixels simply replace the destination */
        if (noscale && (_mm_movemask_epi8(_mm_cmpeq_epi32(sa, amask)) == 0xFFFF))
        {
            _mm_storeu_si128((__m128i *)(dst + x * 4), s);
            continue;
        }

        d = _mm_loadu_si128((const __m128i *)(dst + x * 4));

        /* A translucent destination needs a division per pixel: leave it to the scalar code */
        if (dstalpha &&
            (_mm_movemask_epi8(_mm_cmpeq_epi32(_mm_and_si128(d, amask), amask)) != 0xFFFF))
        {
            ULONG i;

            for (i = 0; i < 4; i++)
                wpaa_blend_pixel(dst + (x + i) * 4, src + (x + i) * 4, galpha, dstalpha);
            continue;
        }

        /* Pixels 0 and 1 */
        s16 = _mm_unpacklo_epi8(s, zero);
        d16 = _mm_unpacklo_epi8(d, zero);
        a   = _mm_shufflehi_epi16(_mm_shufflelo_epi16(s16, 0), 0);
        t   = _mm_add_epi16(_mm_mullo_epi16(a, vga), c127);
        a   = _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);
        ia  = _mm_sub_epi16(c255, a);
        t   = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(s16, a), _mm_mullo_epi16(d16, ia)), c128);
        lo  = _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);

        /* Pixels 2 and 3 */
        s16 = _mm_unpackhi_epi8(s, zero);
        d16 = _mm_unpackhi_epi8(d, zero);
        a   = _mm_shufflehi_epi16(_mm_shufflelo_epi16(s16, 0), 0);
        t   = _mm_add_epi16(_mm_mullo_epi16(a, vga), c127);
        a   = _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);
        ia  = _mm_sub_epi16(c255, a);
        t   = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(s16, a), _mm_mullo_epi16(d16, ia)), c128);
        hi  = _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);

        _mm_storeu_si128((__m128i *)(dst + x * 4),
                         _mm_or_si128(_mm_packus_epi16(lo, hi), amask));
    }

    for (; x < width; x++)
        wpaa_blend_pixel(dst + x * 4, src + x * 4, galpha, dstalpha);
}

/* Copy four pixels at a time, scaling the alpha byte of each */
static void scale_row_sse2(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha)
{
    const __m128i vga   = _mm_set1_epi32(galpha);
    const __m128i c127  = _mm_set1_epi32(127);
    const __m128i amask = _mm_set1_epi32(0x000000FF);
    ULONG x = 0;

    for (; x + 4 <= width; x += 4)
    {
        __m128i s = _mm_loadu_si128((const __m128i *)(src + x * 4));
        __m128i a = _mm_and_si128(s, amask);
        /* The alpha sits in the low word of each lane and the product fits 16 bits */
        __m128i v = _mm_add_epi32(_mm_mullo_epi16(a, vga), c127);

        a = _mm_srli_epi32(_mm_add_epi32(v, _mm_srli_epi32(v, 8)), 8);

        _mm_storeu_si128((__m128i *)(dst + x * 4),
                         _mm_or_si128(_mm_andnot_si128(amask, s), a));
    }

    for (; x < width; x++)
        wpaa_scale_pixel(dst + x * 4, src + x * 4, galpha);
}

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

        AROS_LH10(ULONG, WritePixelArrayAlpha,

/*  SYNOPSIS */
        AROS_LHA(APTR             , src         , A0),
        AROS_LHA(UWORD            , srcx        , D0),
        AROS_LHA(UWORD            , srcy        , D1),
        AROS_LHA(UWORD            , srcmod      , D2),
        AROS_LHA(struct RastPort *, rp          , A1),
        AROS_LHA(UWORD            , destx       , D3),
        AROS_LHA(UWORD            , desty       , D4),
        AROS_LHA(UWORD            , width       , D5),
        AROS_LHA(UWORD            , height      , D6),
        AROS_LHA(ULONG            , globalalpha , D7),

/*  LOCATION */
        struct Library *, CyberGfxBase, 36, Cybergraphics)

/*  FUNCTION
        Alpha-blends all or part of a rectangular block of raw pixel values
        into a RastPort. The source data must be in 32-bit ARGB format: 1 byte
        per component, in the order alpha, red, green, blue.

    INPUTS
        srcRect - pointer to the pixel values.
        srcx, srcy - top-lefthand corner of portion of source rectangle to
            use (in pixels).
        srcmod - the number of bytes in each row of the source rectangle.
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the affected area (in pixels).
        globalalpha - an alpha value applied globally to every pixel taken
            from the source rectangle, on top of each pixel's own alpha.
            The full 32-bit range is used: 0xFFFFFFFF leaves the source
            alpha as it is, 0x80000000 halves it, 0 draws nothing. Only the
            most significant 8 bits take part.

    RESULT
        count - the number of pixels written to.

    NOTES
        Because of the X11 driver you have to set the drawmode
        to JAM1 with SetDrMd().

    EXAMPLE

    BUGS

    SEE ALSO
        WritePixelArray(), graphics.library/SetDrMd()

    INTERNALS
        This is the x86-64 version. Where the destination bitmap has no
        blending code of its own, i.e. moHidd_BitMap_PutAlphaImage would end
        up in the base bitmap class' read-blend-write loop, the blend is done
        here with SSE2 or AVX2 instead. Bitmaps whose driver implements the
        method are left to the driver.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG start_offset;
    LONG pixwritten = 0;
    struct render_data data;
    struct Rectangle rr;

    if (width == 0 || height == 0)
        return 0;

    /* Fully transparent: nothing to draw */
    if ((globalalpha >> 24) == 0)
        return 0;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
        D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap "
            "in WritePixelArrayAlpha() !!!\n"));
        return 0;
    }

    /* Compute the start of the array */

    start_offset = ((ULONG)srcy) * srcmod + srcx * 4;

    data.array  = ((UBYTE *)src) + start_offset;
    data.modulo = srcmod;
    data.alpha  = globalalpha >> 24;

    data.CyberGfxBase      = GetCGFXBase(CyberGfxBase);
    data.mid_PutAlphaImage = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutAlphaImage);
    data.chunkybm          = OOP_FindClass(CLID_Hidd_ChunkyBM);

    if (has_avx2())
    {
        data.blendrow = WritePixelArrayAlpha_BlendRow_AVX;
        data.scalerow = WritePixelArrayAlpha_ScaleRow_AVX;
    }
    else
    {
        data.blendrow = blend_row_sse2;
        data.scalerow = scale_row_sse2;
    }

    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;

    pixwritten = DoRenderFunc(rp, NULL, &rr, RenderHook, &data, TRUE);

    return pixwritten;

    AROS_LIBFUNC_EXIT
} /* WritePixelArrayAlpha */

/*
 * Would moHidd_BitMap_PutAlphaImage on this bitmap end up in the base
 * bitmap class? That is the case when the base class is the one that
 * implements it, and when the chunky bitmap class does but has no direct
 * loop for the bitmap's pixel format (it handles BGR032 and RGB16_LE
 * itself and passes everything else up).
 */
static BOOL blend_is_generic(struct render_data *data, OOP_Object *bm, OOP_Object *pf)
{
    struct IntCGFXBase *CyberGfxBase = data->CyberGfxBase;
    OOP_Class *implcl = NULL;

    if (!OOP_GetMethod(bm, data->mid_PutAlphaImage, &implcl) || !implcl)
        return FALSE;

    if (implcl == CyberGfxBase->basebm)
        return TRUE;

    if (data->chunkybm && (implcl == data->chunkybm))
    {
        IPTR stdpf = vHidd_StdPixFmt_Unknown;

        OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);

        return (stdpf != vHidd_StdPixFmt_BGR032) && (stdpf != vHidd_StdPixFmt_RGB16_LE);
    }

    return FALSE;
}

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase)
{
    struct IntCGFXBase *CyberGfxBase = data->CyberGfxBase;
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array = data->array + data->modulo * srcy + 4 * srcx;
    OOP_Object *pf = NULL;
    BOOL   generic = FALSE;
    BOOL   dstalpha = FALSE;
    ULONG  modulo, rows, bufsize, y;
    UBYTE *buf;

    OOP_GetAttr(dstbm_obj, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    if (pf)
    {
        IPTR amask = 0;

        generic = blend_is_generic(data, dstbm_obj, pf);

        /* Only a destination with an alpha channel of its own takes part in the blend */
        OOP_GetAttr(pf, aHidd_PixFmt_AlphaMask, &amask);
        dstalpha = (amask != 0);
    }

    if (!generic && (data->alpha == 0xFF))
    {
        /* The driver blends, and there is nothing to prepare for it */
        HIDD_BM_PutAlphaImage(dstbm_obj, dst_gc, array, data->modulo,
            rect->MinX, rect->MinY, width, height);

        return width * height;
    }

    modulo = width * 4;
    rows = WPAA_BUFSIZE / modulo;
    if (rows == 0)
        rows = 1;
    if (rows > height)
        rows = height;
    bufsize = rows * modulo;

    buf = AllocMem(bufsize, MEMF_ANY);
    if (!buf)
    {
        rows = 1;
        bufsize = modulo;
        buf = AllocMem(bufsize, MEMF_ANY);
        if (!buf)
            return 0;
    }

    for (y = 0; y < height; y += rows)
    {
        ULONG n = height - y;
        ULONG r;

        if (n > rows)
            n = rows;

        if (generic)
        {
            /*
             * Do what the base class would: fetch the destination as ARGB32,
             * blend, write it back. Only the middle step differs.
             */
            HIDD_BM_GetImage(dstbm_obj, buf, modulo, rect->MinX, rect->MinY + y,
                width, n, vHidd_StdPixFmt_ARGB32);

            for (r = 0; r < n; r++)
                data->blendrow(buf + r * modulo, array + (y + r) * data->modulo,
                    width, data->alpha, dstalpha);

            HIDD_BM_PutImage(dstbm_obj, dst_gc, buf, modulo, rect->MinX, rect->MinY + y,
                width, n, vHidd_StdPixFmt_ARGB32);
        }
        else
        {
            /*
             * The driver blends, from a copy with the global alpha applied:
             * the caller's array must not be modified.
             */
            for (r = 0; r < n; r++)
                data->scalerow(buf + r * modulo, array + (y + r) * data->modulo,
                    width, data->alpha);

            HIDD_BM_PutAlphaImage(dstbm_obj, dst_gc, buf, modulo,
                rect->MinX, rect->MinY + y, width, n);
        }
    }

    FreeMem(buf, bufsize);

    return width * height;
}
