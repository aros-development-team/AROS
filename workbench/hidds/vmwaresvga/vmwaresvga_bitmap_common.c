/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Methods shared by the on- and offscreen bitmap classes. The
          onscreen bitmap draws into the scanout buffer and marks what it
          touched for the deferred flush; the offscreen one is plain RAM.
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <exec/alerts.h>
#include <string.h>    // memset() prototype

#include "vmwaresvga_intern.h"

#ifdef OnBitmap
#define DAMAGE(x1, y1, x2, y2)                                          \
    {                                                                   \
        struct Box box = { (x1), (y1), (x2), (y2) };                    \
        VMWareSVGA_KMS_DamageAdd(&XSD(cl)->kms, &box);                  \
    }
#else
#define DAMAGE(x1, y1, x2, y2)
#endif

#ifdef OnBitmap
/*********  BitMap::Clear()  *************************************/
VOID MNAME_BM(Clear)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    IPTR                width, height;

    D(bug(DEBUGNAME " %s()\n", __func__);)

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    /* Get width & height from bitmap */
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);

    switch (data->bytesperpix)
    {
        case 2:
            HIDD_BM_FillMemRect16(o, data->VideoData, 0, 0, width - 1, height - 1, data->pitch, GC_FG(msg->gc));
            break;
        case 4:
            HIDD_BM_FillMemRect32(o, data->VideoData, 0, 0, width - 1, height - 1, data->pitch, GC_FG(msg->gc));
            break;
        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
    }
    DAMAGE(0, 0, width - 1, height - 1);

    UNLOCK_BITMAP
}
#endif

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *pf;
    HIDDT_Pixel red;
    HIDDT_Pixel green;
    HIDDT_Pixel blue;
    ULONG xc_i;
    ULONG col_i;

    D(bug(DEBUGNAME " %s()\n", __func__);)

    pf = BM_PIXFMT(o);
    if (
            (vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf)) ||
            (vHidd_ColorModel_TrueColor == HIDD_PF_COLMODEL(pf))
        )
        return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
        return FALSE;
    if ((msg->firstColor + msg->numColors) > (1<<data->bpp))
        return FALSE;
    for (xc_i = msg->firstColor, col_i = 0; col_i < msg->numColors; xc_i++, col_i++)
    {
        red = msg->colors[col_i].red >> 8;
        green = msg->colors[col_i].green >> 8;
        blue = msg->colors[col_i].blue >> 8;
        data->cmap[xc_i] = 0x01000000 | red | (green << 8) | (blue << 16);
        msg->colors[col_i].pixval = xc_i;
    }
    return TRUE;
}

/*********  BitMap::PutPixel()  ***************************/

STATIC VOID putpixel(struct BitmapData *data, LONG x, LONG y, HIDDT_Pixel pixel)
{
    ULONG offset = (x*data->bytesperpix)+(y*data->pitch);

    if (data->bytesperpix == 1)
        *((UBYTE*)(data->VideoData + offset)) = pixel;
    else if (data->bytesperpix == 2)
        *((UWORD*)(data->VideoData + offset)) = pixel;
    else if (data->bytesperpix == 4)
        *((ULONG*)(data->VideoData + offset)) = pixel;
}

VOID MNAME_BM(PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    putpixel(data, msg->x, msg->y, msg->pixel);
    DAMAGE(msg->x, msg->y, msg->x, msg->y);

    UNLOCK_BITMAP
}

/*********  BitMap::GetPixel()  *********************************/
HIDDT_Pixel MNAME_BM(GetPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel = 0;
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG offset = (msg->x*data->bytesperpix)+(msg->y*data->pitch);

    if (!data->VideoData)
        return 0;
    if (data->bytesperpix == 1)
        pixel = *((UBYTE*)(data->VideoData + offset));
    else if (data->bytesperpix == 2)
        pixel = *((UWORD*)(data->VideoData + offset));
    else if (data->bytesperpix == 4)
        pixel = *((ULONG*)(data->VideoData + offset));
    return pixel;
}

static VOID CopyMemBox32(struct BitmapData *data,
    UBYTE *pixels, WORD x, WORD y, WORD width, WORD height, ULONG modulo,
    BOOL togpu)
{
    UBYTE *buffer = data->VideoData + (x*data->bytesperpix)+(y*data->pitch);
    UBYTE *src = (UBYTE *)pixels;
    ULONG ycnt = height;

    while (ycnt>0)
    {
        togpu ?
            CopyMem(src, buffer, width * data->bytesperpix) :
            CopyMem(buffer, src, width * data->bytesperpix);

        buffer += data->pitch;
        src += modulo;
        ycnt--;
    }
}

/*********  BitMap::PutImage()  ***************************/

VOID MNAME_BM(PutImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    LONG dstmod = data->pitch;

    D(bug(DEBUGNAME " %s()\n", __func__);)

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    if (msg->pixFmt == vHidd_StdPixFmt_Native)
    {
        switch(data->bytesperpix)
        {
            case 1:
                /* Not supported */
                break;

            case 2:
                HIDD_BM_CopyMemBox16(o, msg->pixels, 0, 0,
                                        data->VideoData, msg->x, msg->y, msg->width, msg->height,
                                        msg->modulo, dstmod);
                break;

            case 3:
                HIDD_BM_CopyMemBox24(o, msg->pixels, 0, 0,
                                        data->VideoData, msg->x, msg->y, msg->width, msg->height,
                                        msg->modulo, dstmod);
                break;

            case 4:
                CopyMemBox32(data, msg->pixels, msg->x, msg->y, msg->width, msg->height, msg->modulo, TRUE);
                break;

        }
    }
    else if (msg->pixFmt == vHidd_StdPixFmt_Native32)
    {
        switch(data->bytesperpix)
        {
            case 1:
                /* Not supported */
                break;

            case 2:
                HIDD_BM_PutMem32Image16(o, msg->pixels,
                                        data->VideoData, msg->x, msg->y, msg->width, msg->height,
                                        msg->modulo, dstmod);
                break;

            case 3:
                HIDD_BM_PutMem32Image24(o, msg->pixels,
                                        data->VideoData, msg->x, msg->y, msg->width, msg->height,
                                        msg->modulo, dstmod);
                break;

            case 4:
                CopyMemBox32(data, msg->pixels, msg->x, msg->y, msg->width, msg->height, msg->modulo, TRUE);
                break;

        }
    }
    else
    {
        APTR dst_pixels, src_pixels;
        OOP_Object *srcPF, *dstPF;

        src_pixels = msg->pixels;
        dst_pixels = data->VideoData + msg->y * dstmod
            + msg->x * data->bytesperpix;
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&dstPF);
        srcPF = HIDD_DMEnum_GetPixFmt(XSD(cl)->dmenum, msg->pixFmt);

        HIDD_BM_ConvertPixels(o, &src_pixels,
            (HIDDT_PixelFormat *)srcPF, msg->modulo, &dst_pixels,
            (HIDDT_PixelFormat *)dstPF, dstmod, msg->width, msg->height,
            NULL);
    }

    DAMAGE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);

    UNLOCK_BITMAP
}

/*********  BitMap::GetImage()  ***************************/

VOID MNAME_BM(GetImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    LONG srcmod = data->pitch;

    D(bug(DEBUGNAME " %s()\n", __func__);)

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    if (msg->pixFmt == vHidd_StdPixFmt_Native)
    {
        switch(data->bytesperpix)
        {
            case 1:
                /* Not supported */
                break;

            case 2:
                HIDD_BM_CopyMemBox16(o, data->VideoData, msg->x, msg->y,
                                        msg->pixels, 0, 0, msg->width, msg->height,
                                        srcmod, msg->modulo);
                break;

            case 3:
                HIDD_BM_CopyMemBox24(o, data->VideoData, msg->x, msg->y,
                                        msg->pixels, 0, 0, msg->width, msg->height,
                                        srcmod, msg->modulo);
                break;

            case 4:
                CopyMemBox32(data, msg->pixels, msg->x, msg->y, msg->width, msg->height, msg->modulo, FALSE);
                break;
        }
    }
    else if (msg->pixFmt == vHidd_StdPixFmt_Native32)
    {
        switch(data->bytesperpix)
        {
            case 1:
                /* Not supported */
                break;

            case 2:
                HIDD_BM_GetMem32Image16(o, data->VideoData, msg->x, msg->y,
                                        msg->pixels, msg->width, msg->height,
                                        srcmod, msg->modulo);
                break;

            case 3:
                HIDD_BM_GetMem32Image24(o, data->VideoData, msg->x, msg->y,
                                        msg->pixels, msg->width, msg->height,
                                        srcmod, msg->modulo);
                break;

            case 4:
                CopyMemBox32(data, msg->pixels, msg->x, msg->y, msg->width, msg->height, msg->modulo, FALSE);
                break;
        }
    }
    else
    {
        APTR dst_pixels, src_pixels;
        OOP_Object *srcPF, *dstPF;

        src_pixels = data->VideoData + msg->y * srcmod
            + msg->x * data->bytesperpix;
        dst_pixels = msg->pixels;
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&srcPF);
        dstPF = HIDD_DMEnum_GetPixFmt(XSD(cl)->dmenum, msg->pixFmt);

        HIDD_BM_ConvertPixels(o, &src_pixels, (HIDDT_PixelFormat *)srcPF,
            srcmod, &dst_pixels, (HIDDT_PixelFormat *)dstPF,
            msg->modulo, msg->width, msg->height, NULL);
    }

    UNLOCK_BITMAP
}

/*********  BitMap::PutImageLUT()  ***************************/

VOID MNAME_BM(PutImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG restadd;
    UBYTE *buffer;
    ULONG ycnt;
    LONG xcnt;
    UBYTE *src=msg->pixels;

    D(bug(DEBUGNAME " %s()\n", __func__);)

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    restadd = (data->pitch - (msg->width*data->bytesperpix));
    buffer = data->VideoData + (msg->x*data->bytesperpix)+(msg->y*data->pitch);
    ycnt = msg->height;
    while (ycnt>0)
    {
        xcnt = msg->width;
        while (xcnt)
        {
            if (data->bytesperpix == 1)
            {
                *((UBYTE *)buffer) = (UBYTE)msg->pixlut->pixels[*src++];
                buffer++;
            }
            else if (data->bytesperpix == 2)
            {
                *((UWORD *)buffer) = (UWORD)msg->pixlut->pixels[*src++];
                buffer += 2;
            }
            else if (data->bytesperpix == 4)
            {
                *((ULONG *)buffer) = (ULONG)msg->pixlut->pixels[*src++];
                buffer += 4;
            }
            xcnt--;
        }
        buffer += restadd;
        src += (msg->modulo - msg->width);
        ycnt--;
    }

    DAMAGE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);

    UNLOCK_BITMAP
}

/*********  BitMap::GetImageLUT()  ***************************/

VOID MNAME_BM(GetImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    struct BitmapData *data =OOP_INST_DATA(cl, o);

    D(bug(DEBUGNAME " %s()\n", __func__);)

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    UNLOCK_BITMAP
}

/*********  BitMap::FillRect()  ***************************/

VOID MNAME_BM(FillRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct BitmapData *data =OOP_INST_DATA(cl, o);
    BOOL done=FALSE;
    HIDDT_Pixel pixel;
    HIDDT_DrawMode mode;
    LONG mod = data->pitch;

    pixel = GC_FG(msg->gc);
    mode = GC_DRMD(msg->gc);

    D(bug(DEBUGNAME " %s()\n", __func__);)

#ifdef OnBitmap
    if (data->width <= msg->minX) return;
    if (data->height <= msg->minY) return;
#endif

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    switch(mode)
    {
        case vHidd_GC_DrawMode_Copy:
            done = TRUE;
            switch(data->bytesperpix)
            {
                case 1:
                    /* Not supported */
                    done = FALSE;
                    break;

                case 2:
                    HIDD_BM_FillMemRect16(o, data->VideoData, msg->minX, msg->minY, msg->maxX, msg->maxY, mod, pixel);
                    break;

                case 3:
                    HIDD_BM_FillMemRect24(o, data->VideoData, msg->minX, msg->minY, msg->maxX, msg->maxY, mod, pixel);
                    break;

                case 4:
                    HIDD_BM_FillMemRect32(o, data->VideoData, msg->minX, msg->minY, msg->maxX, msg->maxY, mod, pixel);
                    break;

            }
            break;

        case vHidd_GC_DrawMode_Invert:
            done = TRUE;
            HIDD_BM_InvertMemRect(o, data->VideoData,
                                msg->minX * data->bytesperpix, msg->minY,
                                msg->maxX * data->bytesperpix + data->bytesperpix - 1, msg->maxY,
                                mod);
            break;

    } /* switch(mode) */

    if (!done)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    DAMAGE(msg->minX, msg->minY, msg->maxX, msg->maxY);

    UNLOCK_BITMAP
}

/*** BitMap::BlitColorExpansion() **********************************************/
VOID MNAME_BM(BlitColorExpansion)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG cemd;
    HIDDT_Pixel fg;
    HIDDT_Pixel bg;
    LONG x;
    LONG y;

    D(bug(DEBUGNAME " %s()\n", __func__);)

    LOCK_BITMAP
    if (!data->VideoData)
    {
        UNLOCK_BITMAP
        return;
    }

    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);
    if (cemd & vHidd_GC_ColExp_Opaque)
    {
        for (y=0; y<msg->height; y++)
        {
            for (x=0;x<msg->width;x++)
            {
                ULONG is_set;
                is_set = HIDD_BM_GetPixel(msg->srcBitMap, x+msg->srcX, y+msg->srcY);
                putpixel(data, x+msg->destX, y+msg->destY, is_set ? fg : bg);
            }
        }
    }
    else
    {
        for (y=0; y<msg->height; y++)
        {
            for (x=0;x<msg->width; x++)
            {
                if (HIDD_BM_GetPixel(msg->srcBitMap, x+msg->srcX, y+msg->srcY))
                    putpixel(data, x+msg->destX, y+msg->destY, fg);
            }
        }
    }

    DAMAGE(msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1);

    UNLOCK_BITMAP
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_VMWareSVGABM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_VMWareSVGABitMap_Drawable:
            *msg->storage = (IPTR)data->VideoData;
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

VOID MNAME_BM(UpdateRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    D(bug(DEBUGNAME " %s()\n", __func__));

    DAMAGE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);
}
