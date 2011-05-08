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

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes lock on GART object is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HiddNouveauNVAccelDownloadM2MF(
    UBYTE * dstpixels, ULONG dstpitch, HIDDT_StdPixFmt dstPixFmt,
    ULONG x, ULONG y, ULONG width, ULONG height, 
    OOP_Class *cl, OOP_Object *o)  
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_channel *chan = carddata->chan;
    struct nouveau_grobj *m2mf = carddata->NvMemFormat;
    struct nouveau_bo *bo = bmdata->bo;
    unsigned cpp = bmdata->bytesperpixel;
    unsigned line_len = width * cpp;
    unsigned src_offset = 0, src_pitch = 0, linear = 0;
    /* Maximum DMA transfer */
    unsigned line_count = carddata->GART->size / line_len;
    char * dst = (char *)dstpixels;

//    if (!nv50_style_tiled_pixmap(pspix)) {
        linear     = 1;
        src_pitch  = bmdata->pitch;
        src_offset += (y * src_pitch) + (x * cpp);
//    }

    /* HW limitations */
    if (line_count > 2047)
        line_count = 2047;

    while (height) {
        char *src;

        if (line_count > height)
            line_count = height;

        if (MARK_RING(chan, 32, 6))
            return FALSE;

        BEGIN_RING(chan, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN, 2);
        if (OUT_RELOCo(chan, bo, NOUVEAU_BO_GART | NOUVEAU_BO_VRAM |
                   NOUVEAU_BO_RD) ||
            OUT_RELOCo(chan, carddata->GART, NOUVEAU_BO_GART |
                   NOUVEAU_BO_WR)) {
            MARK_UNDO(chan);
            return FALSE;
        }

        if (carddata->architecture >= NV_ARCH_50) {
            if (!linear) {
                BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_IN, 7);
                OUT_RING  (chan, 0);
                OUT_RING  (chan, bo->tile_mode << 4);
                OUT_RING  (chan, bmdata->width * cpp);
                OUT_RING  (chan, bmdata->height);
                OUT_RING  (chan, 1);
                OUT_RING  (chan, 0);
                OUT_RING  (chan, (y << 16) | (x * cpp));
            } else {
                BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_IN, 1);
                OUT_RING  (chan, 1);
            }

            BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_OUT, 1);
            OUT_RING  (chan, 1);

            BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN_HIGH, 2);
            if (OUT_RELOCh(chan, bo, src_offset, NOUVEAU_BO_GART |
                       NOUVEAU_BO_VRAM | NOUVEAU_BO_RD) ||
                OUT_RELOCh(chan, carddata->GART, 0, NOUVEAU_BO_GART |
                       NOUVEAU_BO_WR)) {
                MARK_UNDO(chan);
                return FALSE;
            }
        }

        BEGIN_RING(chan, m2mf,
               NV04_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
        if (OUT_RELOCl(chan, bo, src_offset, NOUVEAU_BO_GART |
                   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD) ||
            OUT_RELOCl(chan, carddata->GART, 0, NOUVEAU_BO_GART |
                   NOUVEAU_BO_WR)) {
            MARK_UNDO(chan);
            return FALSE;
        }
        OUT_RING  (chan, src_pitch);
        OUT_RING  (chan, line_len);
        OUT_RING  (chan, line_len);
        OUT_RING  (chan, line_count);
        OUT_RING  (chan, (1<<8)|1);
        OUT_RING  (chan, 0);

        /* Download from GART */
        if (nouveau_bo_map(carddata->GART, NOUVEAU_BO_RD)) {
            MARK_UNDO(chan);
            return FALSE;
        }
        src = carddata->GART->map;

        HiddNouveauReadIntoRAM(
            src, line_len,
            dst, dstpitch, dstPixFmt,
            width, line_count,
            cl, o);
        dst += dstpitch * line_count;
        nouveau_bo_unmap(carddata->GART);

        if (linear)
            src_offset += line_count * src_pitch;
        height -= line_count;
        y += line_count;
    }

    return TRUE;
}
