/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.
*/

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/mbox.h>

#include <exec/alerts.h>
#include <aros/macros.h>
#include <string.h>

#include "vc4gfx_hardware.h"
#include "vc4gfx_hidd.h"

#include "vc4gfx_neon.h"

/* Shared by the onscreen (FB) and offscreen (GPU) bitmap classes: both have
 * uncached physical pixels in data->VideoData. RAM-backed offscreen bitmaps
 * leave VideoData NULL and fall through to ChunkyBM.
 */

/*********  BitMap::Clear()  *************************************/
VOID MNAME_BM(Clear)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (data->VideoData && data->bytesperpix == 4 && data->bytesperrow)
    {
        HIDDT_Pixel bg = GC_BG(msg->gc);
        ULONG width_bytes = data->width * 4;
        ULONG y;

        /* NEON, not DMA: fills only write, and NEON writes to the uncached
         * FB are already fast (the DMA win is for reads). DMA fill needs a
         * non-incrementing 2D source, which wedges the channel on BCM2835. */
        for (y = 0; y < data->height; y++)
            neon_fillline(data->VideoData + y * data->bytesperrow, bg, width_bytes);
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*********  BitMap::FillRect()  *************************************/
VOID MNAME_BM(FillRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (data->VideoData && data->bytesperpix == 4 && data->bytesperrow &&
        GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Copy)
    {
        HIDDT_Pixel fg = GC_FG(msg->gc);
        ULONG width = msg->maxX - msg->minX + 1;
        ULONG height = msg->maxY - msg->minY + 1;
        ULONG width_bytes = width * 4;
        UBYTE *dst = data->VideoData + msg->minY * data->bytesperrow + msg->minX * 4;
        ULONG y;

        /* NEON fill — see Clear(): fills are write-only and DMA fill
         * hangs on real hardware (non-incrementing 2D source). */
        for (y = 0; y < height; y++)
            neon_fillline(dst + y * data->bytesperrow, fg, width_bytes);
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*********  BitMap::PutImage()  *************************************/
VOID MNAME_BM(PutImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (data->VideoData && data->bytesperpix == 4 && data->bytesperrow &&
        (msg->pixFmt == vHidd_StdPixFmt_Native || msg->pixFmt == vHidd_StdPixFmt_Native32))
    {
        UBYTE *src = msg->pixels;
        UBYTE *dst = data->VideoData + msg->y * data->bytesperrow + msg->x * 4;
        ULONG copy_width = msg->width * 4;
        WORD y;

        /* No page flip here (see Gfx::CopyBox): a full-frame PutImage is
         * indistinguishable from mixed desktop rendering, and flipping
         * corrupts read-modify-write / cached-pointer rendering. Flipping
         * lives only on the gallium full-screen path. */

        if ((copy_width * msg->height) >= VC4_DMA_PUT_THRESHOLD &&
            vc4_dma_put(XSD(cl), src, msg->modulo, (ULONG)(IPTR)dst,
                        data->bytesperrow, copy_width, msg->height))
            return;

        for (y = 0; y < msg->height; y++)
        {
            neon_copyline(dst, src, copy_width);
            src += msg->modulo;
            dst += data->bytesperrow;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*********  BitMap::PutTemplate()  ***********************************/
VOID MNAME_BM(PutTemplate)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (data->VideoData && data->bytesperpix == 4 && data->bytesperrow &&
        GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Copy &&
        GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Opaque &&
        !msg->inverttemplate)
    {
        HIDDT_Pixel fg = GC_FG(msg->gc);
        HIDDT_Pixel bg = GC_BG(msg->gc);
        UBYTE *dst = data->VideoData + msg->y * data->bytesperrow + msg->x * 4;
        ULONG row_bytes = msg->width * sizeof(ULONG);
        ULONG *expanded = AllocVec(row_bytes, MEMF_PUBLIC);
        const UBYTE *mask_row = msg->masktemplate;
        WORD y;
        ULONG x;

        if (!expanded)
        {
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            return;
        }

        for (y = 0; y < msg->height; y++)
        {
            ULONG bit = msg->srcx;
            for (x = 0; x < msg->width; x++, bit++)
            {
                UBYTE byte = mask_row[bit >> 3];
                expanded[x] = (byte & (0x80 >> (bit & 7))) ? 0xFFFFFFFF : 0;
            }
            neon_blit_mask32_row_opaque(dst, expanded, msg->width, fg, bg);
            dst += data->bytesperrow;
            mask_row += msg->modulo;
        }

        FreeVec(expanded);
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*********  BitMap::BlitColorExpansion()  ****************************/
VOID MNAME_BM(BlitColorExpansion)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (data->VideoData && data->bytesperpix == 4 && data->bytesperrow &&
        GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Copy &&
        GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Opaque)
    {
        HIDDT_Pixel fg = GC_FG(msg->gc);
        HIDDT_Pixel bg = GC_BG(msg->gc);
        UBYTE *dst = data->VideoData + msg->destY * data->bytesperrow + msg->destX * 4;
        ULONG row_bytes = msg->width * sizeof(ULONG);
        ULONG *srcline = AllocVec(row_bytes, MEMF_PUBLIC);
        WORD y;

        if (!srcline)
        {
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            return;
        }

        for (y = 0; y < msg->height; y++)
        {
            HIDD_BM_GetImage(msg->srcBitMap, (UBYTE *)srcline, row_bytes,
                msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
            neon_blit_mask32_row_opaque(dst, srcline, msg->width, fg, bg);
            dst += data->bytesperrow;
        }

        FreeVec(srcline);
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*********  BitMap::GetImage()  *************************************/
VOID MNAME_BM(GetImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (data->VideoData && data->bytesperpix == 4 && data->bytesperrow &&
        (msg->pixFmt == vHidd_StdPixFmt_Native || msg->pixFmt == vHidd_StdPixFmt_Native32))
    {
        UBYTE *src = data->VideoData + msg->y * data->bytesperrow + msg->x * 4;
        UBYTE *dst = msg->pixels;
        ULONG copy_width = msg->width * 4;
        WORD y;

        if ((copy_width * msg->height) >= VC4_DMA_GET_THRESHOLD &&
            vc4_dma_get(XSD(cl), (ULONG)(IPTR)src, data->bytesperrow,
                        dst, msg->modulo, copy_width, msg->height))
            return;

        for (y = 0; y < msg->height; y++)
        {
            neon_copyline(dst, src, copy_width);
            src += data->bytesperrow;
            dst += msg->modulo;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *pf;
    HIDDT_Pixel red;
    HIDDT_Pixel green;
    HIDDT_Pixel blue;
    ULONG xc_i;
    ULONG col_i;

    pf = BM_PIXFMT(o);
    if (
            (vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf)) ||
            (vHidd_ColorModel_TrueColor == HIDD_PF_COLMODEL(pf))
        )
        return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
        return FALSE;
    if ((msg->firstColor + msg->numColors) > (1 << data->bpp))
        return FALSE;

#ifdef OnBitmap
    {
        struct VideoCoreGfx_staticdata *xsd =
            &((struct VideoCoreGfxBase *)cl->UserData)->vsd;
        unsigned int *m;
        ULONG nc = msg->numColors;

        VC4_MBOX_LOCK(xsd);
        m = xsd->vcsd_MBoxMessage;
        m[0] = AROS_LONG2LE((8 + nc) * 4);
        m[1] = AROS_LONG2LE(VCTAG_REQ);
        m[2] = AROS_LONG2LE(VCTAG_SETPALETTE);
        m[3] = AROS_LONG2LE((2 + nc) * 4);
        m[4] = 0;
        m[5] = AROS_LONG2LE(msg->firstColor);
        m[6] = AROS_LONG2LE(nc);
        for (xc_i = msg->firstColor, col_i = 0; col_i < nc; xc_i++, col_i++)
        {
            red   = msg->colors[col_i].red   >> 8;
            green = msg->colors[col_i].green >> 8;
            blue  = msg->colors[col_i].blue  >> 8;
            data->cmap[xc_i] =
                0x01000000 | red | (green << 8) | (blue << 16);
            m[7 + col_i] = AROS_LONG2LE((red << 24) | (green << 16) |
                                        (blue << 8) | 0xff);
            msg->colors[col_i].pixval = xc_i;
        }
        m[7 + nc] = 0;  /* end tag */

        if ((MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, m)
                == (volatile unsigned int *)-1)
            || (AROS_LE2LONG(m[4]) != (VCTAG_RESP + 4))
            || (AROS_LE2LONG(m[5]) != 0))
        {
            VC4_MBOX_UNLOCK(xsd);
            return FALSE;
        }
        VC4_MBOX_UNLOCK(xsd);
    }
#else
    for (xc_i = msg->firstColor, col_i = 0; col_i < msg->numColors; xc_i++, col_i++)
    {
        red   = msg->colors[col_i].red   >> 8;
        green = msg->colors[col_i].green >> 8;
        blue  = msg->colors[col_i].blue  >> 8;
        data->cmap[xc_i] = 0x01000000 | red | (green << 8) | (blue << 16);
        msg->colors[col_i].pixval = xc_i;
    }
#endif
    return TRUE;
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_VideoCoreGfxBM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_VideoCoreGfxBitMap_Drawable:
            *msg->storage = (IPTR)data->VideoData;
            break;
        case aoHidd_VideoCoreGfxBitMap_BackDrawable:
#ifdef OnBitmap
            *msg->storage = (IPTR)vc4_fb_backpage(XSD(cl));
#else
            *msg->storage = 0;
#endif
            break;
        case aoHidd_VideoCoreGfxBitMap_Flip:
            *msg->storage = 0;
            break;
        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}
