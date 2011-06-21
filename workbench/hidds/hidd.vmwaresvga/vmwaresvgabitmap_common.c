/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/alerts.h>
#include <string.h>    // memset() prototype
#include "svga_reg.h"
#include "vmwaresvgahardware.h"

#ifdef OnBitmap
/*********  BitMap::Clear()  *************************************/
VOID MNAME_BM(Clear)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg) 
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    IPTR    	    	width, height;

    D(bug("[VMWareSVGA] Clear()\n"));

    /* Get width & height from bitmap */

    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);

    writeVMWareSVGAFIFO(data->data, SVGA_CMD_RECT_FILL);
    writeVMWareSVGAFIFO(data->data, GC_FG(msg->gc));
    writeVMWareSVGAFIFO(data->data, 0);
    writeVMWareSVGAFIFO(data->data, 0);
    writeVMWareSVGAFIFO(data->data, width);
    writeVMWareSVGAFIFO(data->data, height);
    syncVMWareSVGAFIFO(data->data);
}
#endif

#if 0
/* this function does not really make sense for LUT bitmaps */

HIDDT_Pixel MNAME_BM(MapColor)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_MapColor *msg)
{
    D(bug("[VMWareSVGA] MapColor()\n"));
    return i;
}

/* this function does not really make sense for LUT bitmaps */

VOID MNAME_BM(UnMapPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UnmapPixel *msg)
{
    D(bug("[VMWareSVGA] UnMapPixel()\n"));
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
#ifdef OnBitmap
        vmwareWriteReg(data->data, SVGA_PALETTE_BASE + xc_i*3+0, msg->colors[col_i].red);
        vmwareWriteReg(data->data, SVGA_PALETTE_BASE + xc_i*3+1, msg->colors[col_i].green);
        vmwareWriteReg(data->data, SVGA_PALETTE_BASE + xc_i*3+2, msg->colors[col_i].blue);
#endif
        msg->colors[col_i].pixval = xc_i;
    }
    return TRUE;
}

/*********  BitMap::PutPixel()  ***************************/

STATIC VOID putpixel(struct BitmapData *data, LONG x, LONG y, HIDDT_Pixel pixel)
{
    ULONG offset;

#ifdef OnBitmap
    offset = (x*data->bytesperpix)+(y*data->data->bytesperline);
    if (
            (x>=data->mouse->x) && (x<(data->mouse->x+data->mouse->width)) &&
            (y>=data->mouse->y) && (y<(data->mouse->y+data->mouse->height))
        )
    {
        displayCursorVMWareSVGA(data->data, 0);
        data->mouse->visible = 0;
    }
#else
    offset = (x + (y*data->width))*data->bytesperpix;
#endif
    if (data->bytesperpix == 1)
        *((UBYTE*)(data->VideoData + offset)) = pixel;
    else if (data->bytesperpix == 2)
        *((UWORD*)(data->VideoData + offset)) = pixel;
    else if (data->bytesperpix == 4)
        *((ULONG*)(data->VideoData + offset)) = pixel;
#ifdef OnBitmap
    if (data->mouse->visible == 0)
    {
        displayCursorVMWareSVGA(data->data, 1);
        data->mouse->visible = 1;
    }
#endif
}

VOID MNAME_BM(PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
#ifdef OnBitmap
    struct Box box;
#endif

    putpixel(data, msg->x, msg->y, msg->pixel);
#ifdef OnBitmap
    box.x1 = box.x2 = msg->x;
    box.y1 = box.y2 = msg->y;
    refreshAreaVMWareSVGA(data->data, &box);
#endif
    return;
}

/*********  BitMap::GetPixel()  *********************************/
HIDDT_Pixel MNAME_BM(GetPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel = 0;
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG offset;

#ifdef OnBitmap
    offset = (msg->x*data->bytesperpix)+(msg->y*data->data->bytesperline);
#else
    offset = (msg->x + (msg->y*data->width))*data->bytesperpix;
#endif
    if (data->bytesperpix == 1)
        pixel = *((UBYTE*)(data->VideoData + offset));
    else if (data->bytesperpix == 2)
        pixel = *((UWORD*)(data->VideoData + offset));
    else if (data->bytesperpix == 4)
        pixel = *((ULONG*)(data->VideoData + offset));
    return pixel;
}

#if 0

/*********  BitMap::DrawPixel()  ***************************/

VOID MNAME_BM(DrawPixel)(OOP_Class *cl,OOP_ Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    return;
}

#endif

/*********  BitMap::PutImage()  ***************************/

VOID MNAME_BM(PutImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
#ifdef OnBitmap
    struct Box box;
#endif
    ULONG offset;
    ULONG restadd;
    UBYTE *buffer;
    ULONG ycnt;
    LONG xcnt;
    UBYTE *src=(UBYTE *)msg->pixels;

    if (msg->pixFmt == vHidd_StdPixFmt_Native32)
    {
#ifdef OnBitmap
        offset = (msg->x*data->bytesperpix)+(msg->y*data->data->bytesperline);
        restadd = (data->data->bytesperline - (msg->width*data->bytesperpix));
#else
        offset = (msg->x + (msg->y*data->width))*data->bytesperpix;
        restadd = (data->width-msg->width)*data->bytesperpix;
#endif
        buffer = data->VideoData+offset;
        ycnt = msg->height;
        while (ycnt>0)
        {
            HIDDT_Pixel *p = (HIDDT_Pixel *)src;
            xcnt = msg->width;
            while (xcnt)
            {
                if (data->bytesperpix == 1)
                {
                    *((UBYTE *)buffer) = (UBYTE)*p++;
                    buffer++;
                }
                else if (data->bytesperpix == 2)
                {
                    *((UWORD *)buffer) = (UWORD)*p++;
                    buffer += 2;
                }
                else if (data->bytesperpix == 4)
                {
                    *((ULONG *)buffer) = (ULONG)*p++;
                        buffer += 4;
                }
                xcnt--;
            }
            buffer += restadd;
            src += msg->modulo;
            ycnt--;
        }
#ifdef OnBitmap
        box.x1 = msg->x;
        box.y1 = msg->y;
        box.x2 = box.x1+msg->width-1;
        box.y2 = box.y1+msg->height-1;
        refreshAreaVMWareSVGA(data->data, &box);
#endif
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*********  BitMap::GetImage()  ***************************/

VOID MNAME_BM(GetImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG offset;
    ULONG restadd;
    UBYTE *buffer;
    ULONG ycnt;
    LONG xcnt;
    UBYTE *src=msg->pixels;

    if (msg->pixFmt == vHidd_StdPixFmt_Native32)
    {
#ifdef OnBitmap
        offset = (msg->x*data->bytesperpix)+(msg->y*data->data->bytesperline);
        restadd = (data->data->bytesperline - (msg->width*data->bytesperpix));
#else
        offset = (msg->x + (msg->y*data->width))*data->bytesperpix;
        restadd = (data->width-msg->width)*data->bytesperpix;
#endif
        buffer = data->VideoData+offset;
        ycnt = msg->height;
        while (ycnt>0)
        {
            HIDDT_Pixel *p = (HIDDT_Pixel *)src;
            xcnt = msg->width;
            while (xcnt)
            {
                if (data->bytesperpix == 1)
                {
                    *p++ = (HIDDT_Pixel)*((UBYTE *)buffer);
                    buffer++;
                }
                else if (data->bytesperpix == 2)
                {
                    *p++ = (HIDDT_Pixel)*((UWORD *)buffer);
                    buffer += 2;
                }
                else if (data->bytesperpix == 4)
                {
                    *p++ = (HIDDT_Pixel)*((ULONG *)buffer);
                    buffer += 4;
                }
                xcnt--;
            }
            buffer += restadd;
            src += msg->modulo;
            ycnt--;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*********  BitMap::PutImageLUT()  ***************************/

VOID MNAME_BM(PutImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
#ifdef OnBitmap
    struct Box box;
#endif
    ULONG offset;
    ULONG restadd;
    UBYTE *buffer;
    ULONG ycnt;
    LONG xcnt;
    UBYTE *src=msg->pixels;

#ifdef OnBitmap
    offset = (msg->x*data->bytesperpix)+(msg->y*data->data->bytesperline);
    restadd = (data->data->bytesperline - (msg->width*data->bytesperpix));
#else
    offset = (msg->x + (msg->y*data->width))*data->bytesperpix;
    restadd = (data->width-msg->width)*data->bytesperpix;
#endif
    buffer = data->VideoData+offset;
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
#ifdef OnBitmap
    box.x1 = msg->x;
    box.y1 = msg->y;
    box.x2 = box.x1+msg->width-1;
    box.y2 = box.y1+msg->height-1;
    refreshAreaVMWareSVGA(data->data, &box);
#endif
}

/*********  BitMap::GetImageLUT()  ***************************/

VOID MNAME_BM(GetImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    D(bug("[VMWareSVGA] GetImageLUT()\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*********  BitMap::FillRect()  ***************************/

VOID MNAME_BM(FillRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
#ifdef OnBitmap
    struct BitmapData *data =OOP_INST_DATA(cl, o);
    struct HWData *hw;
    HIDDT_Pixel pixel;
    HIDDT_DrawMode mode;

    pixel = GC_FG(msg->gc);
    mode = GC_DRMD(msg->gc);
    hw = data->data;
    if (hw->capabilities & SVGA_CAP_RASTER_OP)
    {
        switch (mode)
        {
        case vHidd_GC_DrawMode_Clear:
            clearFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_And:
            andFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_AndReverse:
            andReverseFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Copy:
            copyFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_AndInverted:
            andInvertedFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_NoOp:
            noOpFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Xor:
            xorFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Or:
            orFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Nor:
            norFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Equiv:
            equivFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Invert:
            invertFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_OrReverse:
            orReverseFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_CopyInverted:
            copyInvertedFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_OrInverted:
            orInvertedFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Nand:
            nandFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        case vHidd_GC_DrawMode_Set:
            setFillVMWareSVGA(data->data, pixel, msg->minX, msg->minY, msg->maxX-msg->minX+1, msg->maxY-msg->minY+1);
            break;
        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
#else
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
#endif
}

/*** BitMap::BlitColorExpansion() **********************************************/
VOID MNAME_BM(BlitColorExpansion)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
#ifdef OnBitmap
    struct Box box;
#endif
    ULONG cemd;
    HIDDT_Pixel fg;
    HIDDT_Pixel bg;
    LONG x;
    LONG y;

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
#ifdef OnBitmap
    box.x1 = msg->destX;
    box.y1 = msg->destY;
    box.x2 = msg->destX+msg->width-1;
    box.y2 = msg->destY+msg->height-1;
    refreshAreaVMWareSVGA(data->data, &box);
#endif
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

