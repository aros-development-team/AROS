/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <hidd/gfx.h>
#include "gfx_intern.h"

/****************************************************************************************
 *
 * FIXME: Does not yet handle SwapPixelBytes flag of HIDDT_PixelFormat structure!
 *
 ****************************************************************************************/

#define SHIFT_PIX(pix, shift)   \
    (( (shift) < 0) ? (pix) >> (-shift) : (pix) << (shift) )


#define GETPIX32(s, pix) \
    do { pix = *(ULONG *)s; s = (UBYTE *)s + 4; } while (0)

#if AROS_BIG_ENDIAN
#define GETPIX24(s, pix)                \
    do                                  \
    {                                   \
        pix = (((UBYTE *)s)[0] << 16) | \
              (((UBYTE *)s)[1] << 8)  | \
              ((UBYTE *)s)[2];          \
        s = (UBYTE *)s + 3;             \
    } while (0);                         

#else
#define GETPIX24(s, pix)                \
    do                                  \
    {                                   \
        pix = (((UBYTE *)s)[2] << 16) | \
              (((UBYTE *)s)[1] << 8)  | \
              ((UBYTE *)s)[0];          \
        s = (UBYTE *)s + 3;             \
    } while (0);                         

#endif

#define GETPIX16(s, pix) \
    do { pix = *(UWORD *)s; s = (UBYTE *)s + 2; } while (0)
#define GETPIX16OE(s, pix) \
    do { pix = AROS_SWAP_BYTES_WORD(*(UWORD *)s); s = (UBYTE *)s + 2; } while (0)


#define GETPIX8(s, pix)    \
    do { pix = *(UBYTE *)s; s = (UBYTE *)s + 1; } while (0)

#define GET_TRUE_PIX(s, pix, pf)                                        \
    switch ((pf)->bytes_per_pixel) {                                    \
        case 4: GETPIX32(s, pix); break;                                \
        case 3: GETPIX24(s, pix); break;                                \
        case 2: if ((pf)->flags & vHidd_PixFmt_SwapPixelBytes_Flag)     \
                GETPIX16OE(s, pix);                                     \
            else                                                        \
                GETPIX16(s, pix);                                       \
            break;                                                      \
        default: D(bug("RUBBISH BYTES PER PIXEL IN GET_TRUE_PIX()\n")); break;  \
    }

#define GET_PAL_PIX(s, pix, pf, lut)        \
    switch (pf->bytes_per_pixel) {          \
        case 4: GETPIX32(s, pix); break;    \
        case 3: GETPIX24(s, pix); break;    \
        case 2: GETPIX16(s, pix); break;    \
        case 1: GETPIX8 (s, pix); break;    \
        default: D(bug("RUBBISH BYTES PER PIXEL IN GET_PAL_PIX()\n")); break;   \
    }                                       \
    pix = lut[pix];

#define PUT_PAL_PIX(d, pix, pf)             \
    switch (pf->bytes_per_pixel) {          \
        case 4: PUTPIX32(d, pix); break;    \
        case 3: PUTPIX24(d, pix); break;    \
        case 2: PUTPIX16(d, pix); break;    \
        case 1: PUTPIX8 (d, pix); break;    \
        default: D(bug("RUBBISH BYTES PER PIXEL IN PUT_TRUE_PIX_CM()\n")); break;   \
    }

#define PUTPIX32(d, pix) \
    do { *(ULONG *)d = pix; d = (UBYTE *)d + 4; } while (0)

#if AROS_BIG_ENDIAN
#define PUTPIX24(d, pix)                                        \
    do                                                          \
    {                                                           \
        ((UBYTE *)d)[0] = (UBYTE)((pix >> 16) & 0x000000FF);    \
        ((UBYTE *)d)[1] = (UBYTE)((pix >> 8 ) & 0x000000FF);    \
        ((UBYTE *)d)[2] = (UBYTE)( pix & 0x000000FF);           \
        d = (UBYTE *)d + 3;                                     \
    } while (0)
#else
#define PUTPIX24(d, pix)                                     \
    do                                                       \
    {                                                        \
        ((UBYTE *)d)[2] = (UBYTE)((pix >> 16) & 0x000000FF); \
        ((UBYTE *)d)[1] = (UBYTE)((pix >> 8 ) & 0x000000FF); \
        ((UBYTE *)d)[0] = (UBYTE)( pix & 0x000000FF);        \
        d = (UBYTE *)d + 3;                                  \
    } while (0)

#endif

#define PUTPIX16(d, pix)    \
    do { *(UWORD *)d = pix; d = (UBYTE *)d + 2; } while (0)

#define PUTPIX16OE(d, pix)    \
    do { *(UWORD *)d = AROS_SWAP_BYTES_WORD(pix); d = (UBYTE *)d + 2; } while (0)

#define PUTPIX8(s, pix)    \
    do { *(UBYTE *)s = pix; s = (UBYTE *)s + 1; } while (0)

#define PUT_TRUE_PIX(d, pix, pf)                                    \
    switch (pf->bytes_per_pixel) {                                  \
        case 4: PUTPIX32(d, pix); break;                            \
        case 3: PUTPIX24(d, pix); break;                            \
        case 2: if ((pf)->flags & vHidd_PixFmt_SwapPixelBytes_Flag) \
                PUTPIX16OE(d, pix);                                 \
            else                                                    \
                PUTPIX16(d, pix);                                   \
            break;                                                  \
        default: D(bug("RUBBISH BYTES PER PIXEL IN PUT_TRUE_PIX()\n")); break;  \
    }

#define GET_PAL_PIX_CM(s, pix, pf)          \
    switch (pf->bytes_per_pixel) {          \
        case 4: GETPIX32(s, pix); break;    \
        case 3: GETPIX24(s, pix); break;    \
        case 2: GETPIX16(s, pix); break;    \
        case 1: GETPIX8 (s, pix); break;    \
        default: D(bug("RUBBISH BYTES PER PIXEL IN GET_PAL_PIX_CM()\n")); break;    \
    }

#define PUT_TRUE_PIX_CM(d, pix, pf)         \
    switch (pf->bytes_per_pixel) {          \
        case 4: PUTPIX32(d, pix); break;    \
        case 3: PUTPIX24(d, pix); break;    \
        case 2: PUTPIX16(d, pix); break;    \
        default: D(bug("RUBBISH BYTES PER PIXEL IN PUT_TRUE_PIX_CM()\n")); break;   \
    }

#define INIT_VARS()                 \
    UBYTE *src = *msg->srcPixels;   \
    UBYTE *dst = *msg->dstBuf;      \

#define INIT_FMTVARS()                          \
    HIDDT_PixelFormat *srcfmt = msg->srcPixFmt; \
    HIDDT_PixelFormat *dstfmt = msg->dstPixFmt;

/****************************************************************************************/

static VOID true_to_true(OOP_Class *cl, OOP_Object *o,
                         struct pHidd_BitMap_ConvertPixels *msg)
{
    LONG alpha_diff, red_diff, green_diff, blue_diff;

    INIT_VARS()
    INIT_FMTVARS()
    
    LONG x, y;
    
    
    alpha_diff    = srcfmt->alpha_shift    - dstfmt->alpha_shift;
    red_diff     = srcfmt->red_shift     - dstfmt->red_shift;
    green_diff  = srcfmt->green_shift    - dstfmt->green_shift;
    blue_diff    = srcfmt->blue_shift    - dstfmt->blue_shift;


#if 0
bug("true_to_true()\n: src = %x  dest = %x srcfmt = %d %d %d %d [%d] destfmt = %d %d %d %d [%d]\n",
    src, dst, srcfmt->alpha_shift, srcfmt->red_shift, srcfmt->green_shift, srcfmt->blue_shift, srcfmt->bytes_per_pixel,
          dstfmt->alpha_shift, dstfmt->red_shift, dstfmt->green_shift, dstfmt->blue_shift, dstfmt->bytes_per_pixel);

bug("srcmasks = %p %p %p %p\n",
    srcfmt->alpha_mask,
    srcfmt->red_mask,
    srcfmt->green_mask,
    srcfmt->blue_mask);
bug("destmasks = %p %p %p %p  diffs = %d %d %d %d\n",
    dstfmt->alpha_mask,
    dstfmt->red_mask,
    dstfmt->green_mask,
    dstfmt->blue_mask,
    alpha_diff,
    red_diff,
    green_diff,
    blue_diff);
#endif
    for (y = 0; y < msg->height; y ++)
    {
        UBYTE * s = src;
        UBYTE * d = dst;

        for (x = 0; x < msg->width; x ++)
        {
            /* Get the source pixel */
            HIDDT_Pixel srcpix = 0, dstpix;

            GET_TRUE_PIX(s, srcpix, srcfmt);

            dstpix =      (SHIFT_PIX(srcpix & srcfmt->alpha_mask, alpha_diff) & dstfmt->alpha_mask)
                    | (SHIFT_PIX(srcpix & srcfmt->red_mask  , red_diff)   & dstfmt->red_mask)
                    | (SHIFT_PIX(srcpix & srcfmt->green_mask, green_diff) & dstfmt->green_mask)
                    | (SHIFT_PIX(srcpix & srcfmt->blue_mask , blue_diff)  & dstfmt->blue_mask);

#if 0
        bug("[ %p, %p, %p, %p ] "
            , srcpix
            , srcpix & srcfmt->blue_mask
            , SHIFT_PIX(srcpix & srcfmt->blue_mask, blue_diff)
            , SHIFT_PIX(srcpix & srcfmt->blue_mask, blue_diff) & dstfmt->blue_mask);

#endif

            /* Write the pixel to the destination buffer */
            PUT_TRUE_PIX(d, dstpix, dstfmt);

        } /* for (x) */

        src += msg->srcMod;
        dst += msg->dstMod;
    
    } /* for (y) */

    *msg->srcPixels = src;
    *msg->dstBuf    = dst;

}
/****************************************************************************************/

static VOID true_to_pal(OOP_Class *cl, OOP_Object *o,
                        struct pHidd_BitMap_ConvertPixels *msg)
{
    HIDDT_PixelLUT *lut = msg->pixlut;
    struct HIDDBitMapData *data = OOP_INST_DATA(cl, o);
    HIDDT_ColorLUT *cmap = (HIDDT_ColorLUT *)data->colmap;
    INIT_VARS()
    INIT_FMTVARS()
    ULONG x, y, c;
    ULONG cols;

    D(bug("[ConvertPixels] true_to_pal(): pixlut is 0x%p, colormap is 0x%p\n", lut, cmap));

    if (lut)
        cols = lut->entries;
    else if (cmap)
        cols = cmap->entries;
    else
        cols = 0;

    for (y = 0; y < msg->height; y ++)
    {
        UBYTE * s = src;
        UBYTE * d = dst;

        for (x = 0; x < msg->width; x ++)
        {
            /* Get the source pixel */
            HIDDT_Pixel srcpix = 0;
            HIDDT_Pixel dstpix = 0;
            ULONG best_distance = (ULONG)-1;
            ULONG a, r, g, b;

            GET_TRUE_PIX(s, srcpix, srcfmt);
            a = ALPHA_COMP(srcpix, srcfmt);
            r =   RED_COMP(srcpix, srcfmt);
            g = GREEN_COMP(srcpix, srcfmt);
            b =  BLUE_COMP(srcpix, srcfmt);

            D(bug("[ConvertPixels] Find best match for 0x%08X\n", srcpix));
            for (c = 0; c < cols; c++)
            {
                ULONG ca, cr, cg, cb;
                ULONG distance;

                D(bug("[ConvertPixels] Checking against %u ", c));
                if (lut)
                {
                    D(bug("(0x%08lX)\n", lut->pixels[c]));
                    ca = ALPHA_COMP(lut->pixels[c], srcfmt);
                    cr =   RED_COMP(lut->pixels[c], srcfmt);
                    cg = GREEN_COMP(lut->pixels[c], srcfmt);
                    cb =  BLUE_COMP(lut->pixels[c], srcfmt);
                }
                else
                {
                    D(bug("(0x%08lX)\n", cmap->colors[c].pixval));
                    ca = cmap->colors[c].alpha;
                    cr = cmap->colors[c].red;
                    cg = cmap->colors[c].green;
                    cb = cmap->colors[c].blue;
                }
                distance = color_distance(a, r, g, b, ca, cr, cg, cb);
                D(bug("[ConvertPixels] Distance is %u\n", distance));
                if (distance < best_distance)
                {
                    D(bug("[ConvertPixels] Best distance was %u, new best match is %u\n", best_distance, c));
                    best_distance = distance;
                    dstpix = c;
                }
            }

            D(bug("[ConvertPixels] Found color %u\n", dstpix));
            PUT_PAL_PIX(d, dstpix, dstfmt);
        }
        src += msg->srcMod;
        dst += msg->dstMod;
    }

    *msg->srcPixels = src;
    *msg->dstBuf    = dst;
}

static VOID pal_to_true(OOP_Class *cl, OOP_Object *o,
                        struct pHidd_BitMap_ConvertPixels *msg)
{
    HIDDT_PixelLUT *lut;
    struct HIDDBitMapData *data = OOP_INST_DATA(cl, o);

    INIT_VARS()
    INIT_FMTVARS()
    
    LONG x, y;

    lut = msg->pixlut;
    D(bug("[ConvertPixels] pal_to_true(): pixlut is 0x%p, colormap is 0x%p\n", lut, data->colmap));

    DB2(bug("[ConvertPixels] Buffer contents:\n"));
    for (y = 0; y < msg->height; y ++)
    {
        APTR s = src;
        APTR d = dst;

        for (x = 0; x < msg->width; x ++)
        {
            ULONG srcpix = 0;

            if (lut)
            {
                GET_PAL_PIX(s, srcpix, srcfmt, lut->pixels);

                DB2(bug("0x%08lX ", srcpix));
                /* We now have a pixel in Native32 format. Put it back */
                PUT_TRUE_PIX(d, srcpix, dstfmt);
            }
            else
            {
                if (data->colmap)
                {
                    HIDDT_Color col = {0};
                    HIDDT_Pixel red, green, blue, alpha;

                    /* For optimization purposes we don't swap bytes after MAP_RGBA
                       in order not to swap them back when putting into destination
                       buffer. */
                    GET_PAL_PIX_CM(s, srcpix, srcfmt);
                    HIDD_CM_GetColor(data->colmap, srcpix, &col);
                    red   = col.red;
                    green = col.green;
                    blue  = col.blue;
                    alpha = col.alpha;
                    srcpix = MAP_RGBA(red, green, blue, alpha, dstfmt);
                }
                /* If there's neither pixlut nor colormap provided
                   we'll end up in all black. At least won't crash
                   and make the problem clearly visible */
                DB2(bug("0x%08lX ", srcpix));
                PUT_TRUE_PIX_CM(d, srcpix, dstfmt);
            }
        }

        src += msg->srcMod;
        dst += msg->dstMod;
        DB2(bug("\n"));
    } 
    
    *msg->srcPixels = src;
    *msg->dstBuf    = dst;
}

/****************************************************************************************/

static VOID pal_to_pal(OOP_Class *cl, OOP_Object *o,
                       struct pHidd_BitMap_ConvertPixels *msg)
{
    HIDDT_PixelFormat *spf, *dpf;
     
    spf = msg->srcPixFmt;
    dpf = msg->dstPixFmt;
     
     
    if (spf->clut_shift == dpf->clut_shift
         && spf->clut_mask  == dpf->clut_mask)
    {
        /* This one is rather easy, just copy the data */
    }
    else
    {
         /* Convert pixel-by pixel */
    }
    
    return;
}

/****************************************************************************************/

static void native32_to_native(OOP_Class *cl, OOP_Object *o,
                               struct pHidd_BitMap_ConvertPixels *msg)
{
    INIT_VARS()
    HIDDT_PixelFormat   *dstfmt = msg->dstPixFmt;
    LONG                x, y;
    
    D(bug("SRC: Native32, DST: Native, height=%d, width=%d, bytes per pixel: %d, srcmod: %d, dstmod: %d, depth: %d\n"
    , msg->height, msg->width, dstfmt->bytes_per_pixel, msg->srcMod, msg->dstMod, dstfmt->depth));

    for ( y = 0; y < msg->height; y ++)
    {
        UBYTE *d = dst;
        UBYTE *s = src;

        for (x = 0; x < msg->width; x ++)
        {

            switch (dstfmt->bytes_per_pixel)
            {
            case 4:
                *(ULONG *)d  = (ULONG)*((HIDDT_Pixel *)s);
                d += 4; s += sizeof(HIDDT_Pixel);
                break;

            case 3:
            {
                HIDDT_Pixel dstpix;

                dstpix = *((HIDDT_Pixel *)s);

                d[0] = (UBYTE)((dstpix >> 16) & 0x000000FF);
                d[1] = (UBYTE)((dstpix >> 8)  & 0x000000FF);
                d[2] = (UBYTE)(dstpix  & 0x000000FF);

                    d += 3; s += sizeof(HIDDT_Pixel);
                break;
            }

            case 2:
                *((UWORD *)d) = (UWORD)(*((HIDDT_Pixel *)s));
                    d += 2; s += sizeof(HIDDT_Pixel);
                break;

            case 1:
                *d  = (UBYTE)*((HIDDT_Pixel *)s);
                    d += 1; s += sizeof(HIDDT_Pixel);
                break;

            #if 0
            case 0:
                if (dstfmt->depth == 1)
                {
                UBYTE mask;

                mask = XCOORD_TO_MASK(x);
                d = ((UBYTE *)dst) + XCOORD_TO_BYTEIDX(x);
                if (*((HIDDT_Pixel *)s) ++) {
                    *((UBYTE *)d) |= mask;
                } else {
                    *((UBYTE *)d) &= ~mask;
                }
                }
                break;
            #endif

            } /* switch() */

        } /* for (x) */

        src += msg->srcMod;
        dst += msg->dstMod;

    }
    
    *msg->srcPixels = src;
    *msg->dstBuf    = dst;
}

/****************************************************************************************/

static VOID quick_copy(OOP_Class *cl, OOP_Object *o,
                       struct pHidd_BitMap_ConvertPixels *msg)
{
        /* Just do a simple memcpy() of the pixels */
    INIT_VARS()
    HIDDT_PixelFormat   *srcfmt = msg->srcPixFmt;
    ULONG               bpl = msg->width * srcfmt->bytes_per_pixel;
    
    /* FIXME: This does not work well for formats with bytes_per_pixel < 1 */
    
    if (msg->srcMod == bpl && msg->dstMod == bpl)
    {
        memcpy(dst, src, bpl * msg->height);
    }
    else
    {
        ULONG i;
        ULONG copy_width;

        copy_width = msg->width * srcfmt->bytes_per_pixel;

        for (i = 0; i < msg->height; i ++)
        {
            memcpy(dst, src, copy_width);
            src += msg->srcMod;
            dst += msg->dstMod;
        }
    }

    *msg->srcPixels = src;
    *msg->dstBuf    = dst;

}

/****************************************************************************************/

/* TODO: Discuss this design decision: */

/* Should we pass HIDDT_PixelFormat * or HIDDT_StdPixFmt ?
  The first is more flexible for the user, as he will not only be restricted
  to standard pixel formats. However the user will have to convert
  from HIDDT_StdPixFmt to HIDDT_PixelFormat manually.
  
  In the latter case this conversion will be done inside the method below.
  This means that we can call an internal function directly
  to do the conversion and save two method calls.
*/  

/****************************************************************************************/

VOID BM__Hidd_BitMap__ConvertPixels(OOP_Class *cl, OOP_Object *o,
                    struct pHidd_BitMap_ConvertPixels *msg)
{
    /* For now we assume truecolor */
    HIDDT_PixelFormat *srcfmt, *dstfmt;
    
    srcfmt = msg->srcPixFmt;
    dstfmt = msg->dstPixFmt;
    

    D(bug("ConvertPixels: src=%d, dst=%d\n", srcfmt->stdpixfmt, dstfmt->stdpixfmt));

    /* Check if source and dest are the same format */
    if (srcfmt->stdpixfmt == dstfmt->stdpixfmt)
    {
        quick_copy(cl, o, msg);
        return;
    }
    
    
    if (srcfmt->stdpixfmt == vHidd_StdPixFmt_Native32
         && dstfmt->stdpixfmt == vHidd_StdPixFmt_Native)
    {
        native32_to_native(cl, o, msg);
        return;
    }
    
    switch (HIDD_PF_COLMODEL(srcfmt))
    {
    case vHidd_ColorModel_TrueColor:
        switch (HIDD_PF_COLMODEL(dstfmt))
        {
        case vHidd_ColorModel_TrueColor:
            if ((srcfmt->stdpixfmt >= FIRST_RGB_STDPIXFMT) &&
                    (dstfmt->stdpixfmt >= FIRST_RGB_STDPIXFMT) &&
                    (srcfmt->stdpixfmt <= LAST_RGB_STDPIXFMT) &&
                    (dstfmt->stdpixfmt <= LAST_RGB_STDPIXFMT))
            {
                HIDDT_RGBConversionFunction f;

                /* No semaphore protection here because:
                 * a) rgbconvertfuncs is never reallocated
                 * b) accesing a pointer is atomic
                 * c) semaphore does not protect from someone unloading the code
                 * without unregistering
                 */

                f = CSD(cl)->rgbconvertfuncs[srcfmt->stdpixfmt - FIRST_RGB_STDPIXFMT]
                                             [dstfmt->stdpixfmt - FIRST_RGB_STDPIXFMT];

                if (f)
                {
                    if ((*f)(*msg->srcPixels, msg->srcMod, srcfmt->stdpixfmt,
                             *msg->dstBuf, msg->dstMod, dstfmt->stdpixfmt,
                             msg->width, msg->height))
                    {
                        *msg->srcPixels += (msg->srcMod * msg->height);
                        *msg->dstBuf += (msg->dstMod * msg->height);

                        break;
                    }

                }
            }

            true_to_true(cl, o, msg);
            break;


        case vHidd_ColorModel_Palette:
        case vHidd_ColorModel_StaticPalette:
            true_to_pal(cl, o, msg);
            break;
        }
        break;

    case vHidd_ColorModel_Palette:
    case vHidd_ColorModel_StaticPalette:
        switch (HIDD_PF_COLMODEL(dstfmt))
        {
        case vHidd_ColorModel_TrueColor:
            pal_to_true(cl, o, msg);
            break;

        case vHidd_ColorModel_Palette:
        case vHidd_ColorModel_StaticPalette:
            pal_to_pal(cl,o, msg);
            break;

        }
        break;
    }
    
    return;
}

/****************************************************************************************/
