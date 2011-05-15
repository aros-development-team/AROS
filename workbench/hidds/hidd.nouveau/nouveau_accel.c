/*
 * Copyright 2009 Nouveau Project
 * Copyright (C) 2010-2011, The AROS Development Team. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nouveau_intern.h"
#include "nouveau_class.h"
#include <proto/oop.h>

#undef HiddBitMapAttrBase
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)

static inline int do_alpha(int a, int v)
{
    int tmp = a*v;
    return ((tmp << 8) + tmp + 32768) >> 16;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is mapped */
VOID HIDDNouveauBitMapPutAlphaImage32(struct HIDDNouveauBitMapData * bmdata,
    APTR srcbuff, ULONG srcpitch, ULONG destX, ULONG destY, ULONG width, ULONG height)
{
    ULONG x,y;
    
    for(y = 0; y < height; y++)
    {
        /* Calculate line start addresses */
        IPTR srcaddr = (srcpitch * y) + (IPTR)srcbuff;
        IPTR destaddr = (destX * 4) + (bmdata->pitch * (destY + y)) + (IPTR)bmdata->bo->map;
        
        for (x = 0; x < width; x++)
        {
            ULONG       destpix;
            ULONG       srcpix;
            LONG        src_red, src_green, src_blue, src_alpha;
            LONG        dst_red, dst_green, dst_blue;

            /* Read RGBA pixel from input array */
            srcpix = *(ULONG *)srcaddr;
#if AROS_BIG_ENDIAN
            src_red   = (srcpix & 0x00FF0000) >> 16;
            src_green = (srcpix & 0x0000FF00) >> 8;
            src_blue  = (srcpix & 0x000000FF);
            src_alpha = (srcpix & 0xFF000000) >> 24;
#else
            src_red   = (srcpix & 0x0000FF00) >> 8;
            src_green = (srcpix & 0x00FF0000) >> 16;
            src_blue  = (srcpix & 0xFF000000) >> 24;
            src_alpha = (srcpix & 0x000000FF);
#endif

            /*
            * If alpha=0, do not change the destination pixel at all.
            * This saves us unnecessary reads and writes to VRAM.
            */
            if (src_alpha != 0)
            {
                /*
                * Full opacity. Do not read the destination pixel, as
                * it's value does not matter anyway.
                */
                if (src_alpha == 0xff)
                {
                    dst_red = src_red;
                    dst_green = src_green;
                    dst_blue = src_blue;
                }
                else
                {
                    /*
                    * Alpha blending with source and destination pixels.
                    * Get destination.
                    */
                    destpix = readl(destaddr);

                    dst_red   = (destpix & 0x00FF0000) >> 16;
                    dst_green = (destpix & 0x0000FF00) >> 8;
                    dst_blue  = (destpix & 0x000000FF);

                    dst_red   += do_alpha(src_alpha, src_red - dst_red);
                    dst_green += do_alpha(src_alpha, src_green - dst_green);
                    dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);
                }

                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);

                /* Store the new pixel */
                writel(destpix, destaddr);
            }

            /* Advance pointers */
            srcaddr += 4;
            destaddr += 4;
        }
    }
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is mapped */
VOID HIDDNouveauBitMapPutAlphaImage16(struct HIDDNouveauBitMapData * bmdata,
    APTR srcbuff, ULONG srcpitch, ULONG destX, ULONG destY, ULONG width, ULONG height)
{
    ULONG x,y;
    
    for(y = 0; y < height; y++)
    {
        /* Calculate line start addresses */
        IPTR srcaddr = (srcpitch * y) + (IPTR)srcbuff;
        IPTR destaddr = (destX * 2) + (bmdata->pitch * (destY + y)) + (IPTR)bmdata->bo->map;
        
        for (x = 0; x < width; x++)
        {
            UWORD       destpix;
            ULONG       srcpix;
            LONG        src_red, src_green, src_blue, src_alpha;
            LONG        dst_red, dst_green, dst_blue;

            /* Read RGBA pixel from input array */
            srcpix = *(ULONG *)srcaddr;
#if AROS_BIG_ENDIAN
            src_red   = (srcpix & 0x00FF0000) >> 16;
            src_green = (srcpix & 0x0000FF00) >> 8;
            src_blue  = (srcpix & 0x000000FF);
            src_alpha = (srcpix & 0xFF000000) >> 24;
#else
            src_red   = (srcpix & 0x0000FF00) >> 8;
            src_green = (srcpix & 0x00FF0000) >> 16;
            src_blue  = (srcpix & 0xFF000000) >> 24;
            src_alpha = (srcpix & 0x000000FF);
#endif

            /*
            * If alpha=0, do not change the destination pixel at all.
            * This saves us unnecessary reads and writes to VRAM.
            */
            if (src_alpha != 0)
            {
                /*
                * Full opacity. Do not read the destination pixel, as
                * it's value does not matter anyway.
                */
                if (src_alpha == 0xff)
                {
                    dst_red = src_red;
                    dst_green = src_green;
                    dst_blue = src_blue;
                    }
                else
                {
                    /*
                    * Alpha blending with source and destination pixels.
                    * Get destination.
                    */

                    destpix = readw(destaddr);

                    dst_red   = (destpix & 0x0000F800) >> 8;
                    dst_green = (destpix & 0x000007e0) >> 3;
                    dst_blue  = (destpix & 0x0000001f) << 3;

                    dst_red   += do_alpha(src_alpha, src_red - dst_red);
                    dst_green += do_alpha(src_alpha, src_green - dst_green);
                    dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);
                }

                destpix = (((dst_red << 8) & 0xf800) | ((dst_green << 3) & 0x07e0) | ((dst_blue >> 3) & 0x001f));

                writew(destpix, destaddr);
            }

            /* Advance pointers */
            srcaddr += 4;
            destaddr += 2;
        }
    }
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is mapped */
VOID HIDDNouveauBitMapPutAlphaTemplate32(struct HIDDNouveauBitMapData * bmdata,
    OOP_Object * gc, OOP_Object * bm, BOOL invertalpha,
    UBYTE * srcalpha, ULONG srcpitch, ULONG destX, ULONG destY, ULONG width, ULONG height)
{
    WORD        x, y;
    UBYTE       *pixarray = srcalpha;
    HIDDT_Color color;
    LONG        fg_red, fg_green, fg_blue;
    LONG        bg_red = 0, bg_green = 0, bg_blue = 0;
    WORD        type = 0;

    if (width <= 0 || height <= 0)
        return;

    HIDD_BM_UnmapPixel(bm, GC_FG(gc), &color);

    fg_red   = color.red >> 8;
    fg_green = color.green >> 8;
    fg_blue  = color.blue >> 8;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
        type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
        type = 2;
    }
    else
    {
        type = 4;

        HIDD_BM_UnmapPixel(bm, GC_BG(gc), &color);
        bg_red   = color.red >> 8;
        bg_green = color.green >> 8;
        bg_blue  = color.blue >> 8;
    }

    if (invertalpha) type++;
    

    for(y = 0; y < height; y++)
    {
        IPTR destaddr = (destX * 4) + ((destY + y) * bmdata->pitch) + (IPTR)bmdata->bo->map;

        switch(type)
        {
            case 0: /* JAM1 */
            for(x = 0; x < width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = *pixarray++;


                if (alpha != 0) /* If alpha=0, do not change the destination pixel at all. */
                {
                    if (alpha == 0xff) /* Full opacity. Do not read the destination pixel. */
                    {
                        dst_red = fg_red;
                        dst_green = fg_green;
                        dst_blue = fg_blue;
                    }
                    else
                    {
                        destpix = readl(destaddr);

                        dst_red   = (destpix & 0x00FF0000) >> 16;
                        dst_green = (destpix & 0x0000FF00) >> 8;
                        dst_blue  = (destpix & 0x000000FF);

                        dst_red   += do_alpha(alpha, fg_red - dst_red);
                        dst_green += do_alpha(alpha, fg_green - dst_green);
                        dst_blue  += do_alpha(alpha, fg_blue - dst_blue);
                    }

                    destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                    writel(destpix, destaddr);
                }

                destaddr += 4;

            } /* for(x = 0; x < msg->width; x++) */
            break;

            case 1: /* JAM1 | INVERSVID */
            for(x = 0; x < width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = (*pixarray++) ^ 255;


                if (alpha != 0) /* If alpha=0, do not change the destination pixel at all. */
                {
                    if (alpha == 0xff) /* Full opacity. Do not read the destination pixel. */
                    {
                        dst_red = fg_red;
                        dst_green = fg_green;
                        dst_blue = fg_blue;
                    }
                    else
                    {
                        destpix = readl(destaddr);

                        dst_red   = (destpix & 0x00FF0000) >> 16;
                        dst_green = (destpix & 0x0000FF00) >> 8;
                        dst_blue  = (destpix & 0x000000FF);

                        dst_red   += do_alpha(alpha, fg_red - dst_red);
                        dst_green += do_alpha(alpha, fg_green - dst_green);
                        dst_blue  += do_alpha(alpha, fg_blue - dst_blue);
                    }

                    destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                    writel(destpix, destaddr);
                }

                destaddr += 4;

            } /* for(x = 0; x < width; x++) */
            break;

            case 2: /* COMPLEMENT */
            for(x = 0; x < width; x++)
            {
                ULONG   destpix;
                UBYTE   alpha;

                alpha = *pixarray++;


                if (alpha >= 0x80) 
                {
                    destpix = readl(destaddr);
                    destpix = ~destpix;
                    writel(destpix, destaddr);
                }

                destaddr += 4;

            } /* for(x = 0; x < width; x++) */
            break;

            case 3: /* COMPLEMENT | INVERSVID*/
            for(x = 0; x < width; x++)
            {
                ULONG   destpix;
                UBYTE   alpha;

                alpha = *pixarray++;


                if (alpha < 0x80)
                {
                    destpix = readl(destaddr);
                    destpix = ~destpix;
                    writel(destpix, destaddr);
                }

                destaddr += 4;

            } /* for(x = 0; x < width; x++) */
            break;

            case 4: /* JAM2 */
            for(x = 0; x < width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = *pixarray++;


                dst_red   = bg_red   + ((fg_red   - bg_red)   * alpha) / 256;
                dst_green = bg_green + ((fg_green - bg_green) * alpha) / 256;
                dst_blue  = bg_blue  + ((fg_blue  - bg_blue)  * alpha) / 256;

                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);

                writel(destpix, destaddr);
                destaddr += 4;

            } /* for(x = 0; x < width; x++) */
            break;

            case 5: /* JAM2 | INVERSVID */
            for(x = 0; x < width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = (*pixarray++) ^ 255;


                dst_red   = bg_red   + ((fg_red   - bg_red)   * alpha) / 256;
                dst_green = bg_green + ((fg_green - bg_green) * alpha) / 256;
                dst_blue  = bg_blue  + ((fg_blue  - bg_blue)  * alpha) / 256;

                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                writel(destpix, destaddr);

                destaddr += 4;

            } /* for(x = 0; x < width; x++) */
            break;

        } /* switch(type) */

        pixarray += srcpitch - width;

    } /* for(y = 0; y < height; y++) */
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is mapped */
VOID HIDDNouveauBitMapPutAlphaTemplate16(struct HIDDNouveauBitMapData * bmdata,
    OOP_Object * gc, OOP_Object * bm, BOOL invertalpha,
    UBYTE * srcalpha, ULONG srcpitch, ULONG destX, ULONG destY, ULONG width, ULONG height)
{
    WORD        x, y;
    UBYTE       *pixarray = srcalpha;
    HIDDT_Color color;
    LONG        fg_red, fg_green, fg_blue;
    LONG        bg_red = 0, bg_green = 0, bg_blue = 0;
    WORD        type = 0;

    if (width <= 0 || height <= 0)
        return;

    HIDD_BM_UnmapPixel(bm, GC_FG(gc), &color);

    fg_red   = color.red >> 8;
    fg_green = color.green >> 8;
    fg_blue  = color.blue >> 8;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
        type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
        type = 2;
    }
    else
    {
        type = 4;

        HIDD_BM_UnmapPixel(bm, GC_BG(gc), &color);
        bg_red   = color.red >> 8;
        bg_green = color.green >> 8;
        bg_blue  = color.blue >> 8;
    }

    if (invertalpha) type++;
    

    for(y = 0; y < height; y++)
    {
        IPTR destaddr = (destX * 2) + ((destY + y) * bmdata->pitch) + (IPTR)bmdata->bo->map;

        switch(type)
        {
            case 0: /* JAM1 */
            for(x = 0; x < width; x++)
            {
                UWORD   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = *pixarray++;


                if (alpha != 0) /* If alpha=0, do not change the destination pixel at all. */
                {
                    if (alpha == 0xff) /* Full opacity. Do not read the destination pixel. */
                    {
                        dst_red = fg_red;
                        dst_green = fg_green;
                        dst_blue = fg_blue;
                    }
                    else
                    {
                        destpix = readw(destaddr);

                        dst_red   = (destpix & 0x0000F800) >> 8;
                        dst_green = (destpix & 0x000007e0) >> 3;
                        dst_blue  = (destpix & 0x0000001f) << 3;

                        dst_red   += do_alpha(alpha, fg_red - dst_red);
                        dst_green += do_alpha(alpha, fg_green - dst_green);
                        dst_blue  += do_alpha(alpha, fg_blue - dst_blue);
                    }

                    destpix = (((dst_red << 8) & 0xf800) | ((dst_green << 3) & 0x07e0) | ((dst_blue >> 3) & 0x001f));
                    writew(destpix, destaddr);
                }

                destaddr += 2;

            } /* for(x = 0; x < msg->width; x++) */
            break;

            case 1: /* JAM1 | INVERSVID */
            for(x = 0; x < width; x++)
            {
                UWORD   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = (*pixarray++) ^ 255;


                if (alpha != 0) /* If alpha=0, do not change the destination pixel at all. */
                {
                    if (alpha == 0xff) /* Full opacity. Do not read the destination pixel. */
                    {
                        dst_red = fg_red;
                        dst_green = fg_green;
                        dst_blue = fg_blue;
                    }
                    else
                    {
                        destpix = readw(destaddr);

                        dst_red   = (destpix & 0x0000F800) >> 8;
                        dst_green = (destpix & 0x000007e0) >> 3;
                        dst_blue  = (destpix & 0x0000001f) << 3;

                        dst_red   += do_alpha(alpha, fg_red - dst_red);
                        dst_green += do_alpha(alpha, fg_green - dst_green);
                        dst_blue  += do_alpha(alpha, fg_blue - dst_blue);
                    }

                    destpix = (((dst_red << 8) & 0xf800) | ((dst_green << 3) & 0x07e0) | ((dst_blue >> 3) & 0x001f));
                    writew(destpix, destaddr);
                }

                destaddr += 2;

            } /* for(x = 0; x < width; x++) */
            break;

            case 2: /* COMPLEMENT */
            for(x = 0; x < width; x++)
            {
                UWORD   destpix;
                UBYTE   alpha;

                alpha = *pixarray++;


                if (alpha >= 0x80) 
                {
                    destpix = readw(destaddr);
                    destpix = ~destpix;
                    writew(destpix, destaddr);
                }

                destaddr += 2;

            } /* for(x = 0; x < width; x++) */
            break;

            case 3: /* COMPLEMENT | INVERSVID*/
            for(x = 0; x < width; x++)
            {
                UWORD   destpix;
                UBYTE   alpha;

                alpha = *pixarray++;


                if (alpha < 0x80)
                {
                    destpix = readw(destaddr);
                    destpix = ~destpix;
                    writew(destpix, destaddr);
                }

                destaddr += 2;

            } /* for(x = 0; x < width; x++) */
            break;

            case 4: /* JAM2 */
            for(x = 0; x < width; x++)
            {
                UWORD   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = *pixarray++;


                dst_red   = bg_red   + ((fg_red   - bg_red)   * alpha) / 256;
                dst_green = bg_green + ((fg_green - bg_green) * alpha) / 256;
                dst_blue  = bg_blue  + ((fg_blue  - bg_blue)  * alpha) / 256;

                destpix = (((dst_red << 8) & 0xf800) | ((dst_green << 3) & 0x07e0) | ((dst_blue >> 3) & 0x001f));
                writew(destpix, destaddr);

                destaddr += 2;

            } /* for(x = 0; x < width; x++) */
            break;

            case 5: /* JAM2 | INVERSVID */
            for(x = 0; x < width; x++)
            {
                UWORD   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = (*pixarray++) ^ 255;


                dst_red   = bg_red   + ((fg_red   - bg_red)   * alpha) / 256;
                dst_green = bg_green + ((fg_green - bg_green) * alpha) / 256;
                dst_blue  = bg_blue  + ((fg_blue  - bg_blue)  * alpha) / 256;

                destpix = (((dst_red << 8) & 0xf800) | ((dst_green << 3) & 0x07e0) | ((dst_blue >> 3) & 0x001f));
                writew(destpix, destaddr);

                destaddr += 2;

            } /* for(x = 0; x < width; x++) */
            break;

        } /* switch(type) */

        pixarray += srcpitch - width;

    } /* for(y = 0; y < height; y++) */
}

/* Assumes input and output buffers are lock-protected */
/* Takes pixels from RAM buffer, converts them and puts them into destination
   buffer. The destination buffer can be in VRAM or GART or RAM */
BOOL HiddNouveauWriteFromRAM(
    APTR src, ULONG srcPitch, HIDDT_StdPixFmt srcPixFmt,
    APTR dst, ULONG dstPitch,
    ULONG width, ULONG height,
    OOP_Class *cl, OOP_Object *o)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    UBYTE dstBpp = bmdata->bytesperpixel;

    switch(srcPixFmt)
    {
    case vHidd_StdPixFmt_Native:
        switch(dstBpp)
        {
        case 1:
            /* Not supported */
            break;

        case 2:
            {
                struct pHidd_BitMap_CopyMemBox16 __m = 
                {
                    SD(cl)->mid_CopyMemBox16, src, 0, 0, dst,
                    0, 0, width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 4:
            {
                struct pHidd_BitMap_CopyMemBox32 __m = 
                {
                    SD(cl)->mid_CopyMemBox32, src, 0, 0, dst,
                    0, 0, width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        } /* switch(data->bytesperpixel) */
        break;

    case vHidd_StdPixFmt_Native32:
        switch(dstBpp)
        {
        case 1:
            /* Not supported */
            break;

        case 2:
            {
                struct pHidd_BitMap_PutMem32Image16 __m = 
                {
                    SD(cl)->mid_PutMem32Image16, src, dst,
                    0, 0, width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 4:
            {
                struct pHidd_BitMap_CopyMemBox32 __m = 
                {
                    SD(cl)->mid_CopyMemBox32, src, 0, 0, dst,
                    0, 0, width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        } /* switch(data->bytesperpixel) */
        break;
    default:
        {
            /* Use ConvertPixels to convert that data to destination format */
            APTR csrc = src;
            APTR * psrc = &csrc;
            APTR cdst = dst;
            APTR * pdst = &cdst;
            OOP_Object * dstPF = NULL;
            OOP_Object * srcPF = NULL;
            OOP_Object * gfxHidd = NULL;
            struct pHidd_Gfx_GetPixFmt __gpf =
            {
                SD(cl)->mid_GetPixFmt, srcPixFmt
            }, *gpf = &__gpf;
            
            OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&dstPF);
            OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&gfxHidd);
            srcPF = (OOP_Object *)OOP_DoMethod(gfxHidd, (OOP_Msg)gpf);

            {
                struct pHidd_BitMap_ConvertPixels __m =
                {
                    SD(cl)->mid_ConvertPixels, 
                    psrc, (HIDDT_PixelFormat *)srcPF, srcPitch,
                    pdst, (HIDDT_PixelFormat *)dstPF, dstPitch,
                    width, height, NULL
                }, *m = &__m;            
                OOP_DoMethod(o, (OOP_Msg)m);
            }
        }
        
        break;
    }

    return TRUE;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes lock on GART object is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HiddNouveauAccelARGBUpload3D(
    UBYTE * srcpixels, ULONG srcpitch,
    ULONG x, ULONG y, ULONG width, ULONG height, 
    OOP_Class *cl, OOP_Object *o)
{
    struct HIDDNouveauBitMapData srcdata;
    struct HIDDNouveauBitMapData * dstdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    unsigned cpp = 4; /* We are always getting ARGB buffer */
    unsigned line_len = width * cpp;
    /* Maximum DMA transfer */
    unsigned line_count = carddata->GART->size / line_len;
    char *src = (char *)srcpixels;

    /* HW limitations */
    if (line_count > 2047)
        line_count = 2047;

    while (height) {
        char *dst;

        if (line_count > height)
            line_count = height;

        /* Upload to GART */
        if (nouveau_bo_map(carddata->GART, NOUVEAU_BO_WR))
            return FALSE;
        dst = carddata->GART->map;

#if AROS_BIG_ENDIAN
        {
            /* Just use copy. Memory formats match */
            struct pHidd_BitMap_CopyMemBox32 __m = 
            {
                SD(cl)->mid_CopyMemBox32, src, 0, 0, dst,
                0, 0, width, height, srcpitch, line_len
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
#else
        {
            /* Use ConvertPixels to convert that data to destination format */
            APTR csrc = src;
            APTR * psrc = &csrc;
            APTR cdst = dst;
            APTR * pdst = &cdst;
            OOP_Object * dstPF = NULL;
            OOP_Object * srcPF = NULL;
            OOP_Object * gfxHidd = NULL;
            struct pHidd_Gfx_GetPixFmt __gpfsrc =
            {
                SD(cl)->mid_GetPixFmt, vHidd_StdPixFmt_BGRA32
            }, *gpfsrc = &__gpfsrc;
            struct pHidd_Gfx_GetPixFmt __gpfdst =
            {
                SD(cl)->mid_GetPixFmt, vHidd_StdPixFmt_ARGB32
            }, *gpfdst = &__gpfdst;

            OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&gfxHidd);
            srcPF = (OOP_Object *)OOP_DoMethod(gfxHidd, (OOP_Msg)gpfsrc);
            dstPF = (OOP_Object *)OOP_DoMethod(gfxHidd, (OOP_Msg)gpfdst);

            {
                struct pHidd_BitMap_ConvertPixels __m =
                {
                    SD(cl)->mid_ConvertPixels, 
                    psrc, (HIDDT_PixelFormat *)srcPF, srcpitch,
                    pdst, (HIDDT_PixelFormat *)dstPF, line_len,
                    width, height, NULL
                }, *m = &__m;            
                OOP_DoMethod(o, (OOP_Msg)m);
            }
        }
#endif

        src += srcpitch * line_count;
        nouveau_bo_unmap(carddata->GART);
        
        /* Wrap GART */
        srcdata.bo = carddata->GART;
        srcdata.width = width;
        srcdata.height = line_count;
        srcdata.depth = 32;
        srcdata.bytesperpixel = 4;
        srcdata.pitch = line_len;

        /* Render using 3D engine */
        switch(carddata->architecture)
        {
        case(NV_ARCH_40):
            HIDDNouveauNV403DCopyBox(carddata,
                &srcdata, dstdata,
                0, 0, x, y, width, height, BLENDOP_ALPHA);
            break;
        case(NV_ARCH_30):
            HIDDNouveauNV303DCopyBox(carddata,
                &srcdata, dstdata,
                0, 0, x, y, width, height, BLENDOP_ALPHA);
            break;
        case(NV_ARCH_20):
        case(NV_ARCH_10):
            HIDDNouveauNV103DCopyBox(carddata,
                &srcdata, dstdata,
                0, 0, x, y, width, height, BLENDOP_ALPHA);
            break;
        }

        height -= line_count;
        y += line_count;
    }

    return TRUE;
}

/* Assumes input and output buffers are lock-protected */
/* Takes pixels from source buffer, converts them and puts them into RAM
   buffer. The source buffer can be in VRAM or GART or RAM */
BOOL HiddNouveauReadIntoRAM(
    APTR src, ULONG srcPitch, 
    APTR dst, ULONG dstPitch, HIDDT_StdPixFmt dstPixFmt,
    ULONG width, ULONG height,
    OOP_Class *cl, OOP_Object *o)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    UBYTE srcBpp = bmdata->bytesperpixel;

    switch(dstPixFmt)
    {
    case vHidd_StdPixFmt_Native:
        switch(srcBpp)
        {
        case 1:
            /* Not supported */
            break;

        case 2:
            {
                struct pHidd_BitMap_CopyMemBox16 __m = 
                {
                    SD(cl)->mid_CopyMemBox16, src, 0, 0, dst,
                    0, 0, width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 4:
            {
                struct pHidd_BitMap_CopyMemBox32 __m = 
                {
                    SD(cl)->mid_CopyMemBox32, src, 0, 0, dst,
                    0, 0, width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        } /* switch(data->bytesperpixel) */
        break;

    case vHidd_StdPixFmt_Native32:
        switch(srcBpp)
        {
        case 1:
            /* Not supported */
            break;

        case 2:
            {
                struct pHidd_BitMap_GetMem32Image16 __m = 
                {
                    SD(cl)->mid_GetMem32Image16, src, 0, 0, dst, 
                    width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        case 4:
            {
                struct pHidd_BitMap_CopyMemBox32 __m = 
                {
                    SD(cl)->mid_CopyMemBox32, src, 0, 0, dst,
                    0, 0, width, height, srcPitch, dstPitch
                }, *m = &__m;
                OOP_DoMethod(o, (OOP_Msg)m);
            }
            break;

        } /* switch(data->bytesperpixel) */
        break;
    default:
        {
            /* Use ConvertPixels to convert that data to destination format */
            APTR csrc = src;
            APTR * psrc = &csrc;
            APTR cdst = dst;
            APTR * pdst = &cdst;
            OOP_Object * dstPF = NULL;
            OOP_Object * srcPF = NULL;
            OOP_Object * gfxHidd = NULL;
            struct pHidd_Gfx_GetPixFmt __gpf =
            {
                SD(cl)->mid_GetPixFmt, dstPixFmt
            }, *gpf = &__gpf;
            
            OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&srcPF);
            OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&gfxHidd);
            dstPF = (OOP_Object *)OOP_DoMethod(gfxHidd, (OOP_Msg)gpf);

            {
                struct pHidd_BitMap_ConvertPixels __m =
                {
                    SD(cl)->mid_ConvertPixels, 
                    psrc, (HIDDT_PixelFormat *)srcPF, srcPitch,
                    pdst, (HIDDT_PixelFormat *)dstPF, dstPitch,
                    width, height, NULL
                }, *m = &__m;            
                OOP_DoMethod(o, (OOP_Msg)m);
            }
        }
        
        break;
    }

    return TRUE;
}

