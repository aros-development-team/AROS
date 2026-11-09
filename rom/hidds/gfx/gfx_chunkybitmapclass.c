/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Gfx chunky bitmap class implementation.
*/

/****************************************************************************************/

#include "gfx_debug.h"

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/gfx.h>

#include "gfx_intern.h"

#include <string.h>

/* The bytes per pixel dispatch below relies on these */
GFX_ASSERT_NEXT_ID(FillMemRect8, FillMemRect16);
GFX_ASSERT_NEXT_ID(FillMemRect16, FillMemRect24);
GFX_ASSERT_NEXT_ID(FillMemRect24, FillMemRect32);
_Static_assert(offsetof(struct pHidd_BitMap_FillMemRect8, fill) ==
               offsetof(struct pHidd_BitMap_FillMemRect32, fill) &&
               offsetof(struct pHidd_BitMap_FillMemRect16, fill) ==
               offsetof(struct pHidd_BitMap_FillMemRect32, fill),
               "FillMemRect messages differ before fill");
GFX_ASSERT_BPP_METHODS(CopyMemBox);
GFX_ASSERT_NEXT_METHOD(PutMem32Image8, PutMem32Image16);
GFX_ASSERT_NEXT_METHOD(PutMem32Image16, PutMem32Image24);
GFX_ASSERT_NEXT_METHOD(GetMem32Image8, GetMem32Image16);
GFX_ASSERT_NEXT_METHOD(GetMem32Image16, GetMem32Image24);
GFX_ASSERT_NEXT_METHOD(CopyLUTMemBox16, CopyLUTMemBox24);
GFX_ASSERT_NEXT_METHOD(CopyLUTMemBox24, CopyLUTMemBox32);
GFX_ASSERT_BPP_METHODS(PutMemTemplate);
GFX_ASSERT_BPP_METHODS(PutMemPattern);

/****************************************************************************************/

OOP_Object *CBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct chunkybm_data    *data;
    OOP_Object              *pf;
    IPTR                    bytesperrow, bytesperpixel;
    struct TagItem          *tag;
    OOP_MethodID            dispose_mid;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
        return NULL;

    /* Initialize the instance data to 0 */
    data = OOP_INST_DATA(cl, o);

    SetMem(data, 0, sizeof (*data));

    /*
     * Scan-out bitmaps live in display memory a plane can read: only a driver
     * bitmap class can make one. There is no software overlay.
     */
    {
        IPTR scanout = FALSE;

        OOP_GetAttr(o, aHidd_BitMap_Scanout, &scanout);
        if (scanout)
        {
            OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

            OOP_CoerceMethod(cl, o, &dispose_mid);
            return NULL;
        }
    }

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
    OOP_GetAttr(o, aHidd_BitMap_Display, (APTR)&data->display);
    OOP_GetAttr(data->display, aHidd_Display_DMEnumerator, (APTR)&data->dmenum);
    /* Get some dimensions of the bitmap */
    OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &bytesperrow);
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);

    data->bytesperpixel = bytesperpixel;
    data->bytesperrow   = bytesperrow;

    tag = FindTagItem(aHidd_ChunkyBM_Buffer, msg->attrList);
    if (tag)
    {
        /*
         * NULL user-supplied buffer is valid.
         * In this case we create a bitmap with no buffer. We can attach it later.
         */
        data->own_buffer = FALSE;
        data->buffer     = (APTR)tag->ti_Data;

        return o;
    }
    else
    {
        IPTR height;

        OOP_GetAttr(o, aHidd_BitMap_Height, &height);

        data->own_buffer = TRUE;
        data->bufsize    = height * bytesperrow;
        data->buffer = AllocVec(data->bufsize, MEMF_ANY | MEMF_CLEAR);

        if (data->buffer)
            return o;
    }

    /* free all on error */
    dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

    OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
    return NULL;
}

/****************************************************************************************/

void CBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct chunkybm_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    if (data->own_buffer)
        FreeVec(data->buffer);
        
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE *dest;
    
    struct chunkybm_data *data;
    
    data = OOP_INST_DATA(cl, o);

    /* bitmap in chunky-mode */
    dest = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;
    
    switch(data->bytesperpixel)
    {
        case 1:
            *((UBYTE *) dest) = (UBYTE) msg->pixel;
            break;
                
        case 2:
            *((UWORD *) dest) = (UWORD) msg->pixel;
            break;
            
        case 3:
        #if AROS_BIG_ENDIAN
            dest[0] = (UBYTE)(msg->pixel >> 16) & 0x000000FF;
            dest[1] = (UBYTE)(msg->pixel >> 8) & 0x000000FF;
            dest[2] = (UBYTE)msg->pixel & 0x000000FF;
        #else
            dest[0] = (UBYTE)msg->pixel & 0x000000FF;
            dest[1] = (UBYTE)(msg->pixel >> 8) & 0x000000FF;
            dest[2] = (UBYTE)(msg->pixel >> 16) & 0x000000FF;
        #endif
            break;
                
/*       if (1 == ( ((IPTR)dest) & 1) )
                {
                  *((UBYTE *) dest++) = (UBYTE) msg->pixel >> 16;
                  *((UWORD *) dest  ) = (UWORD) msg->pixel;
                }
                else
                {
                  *((UWORD *) dest++) = (UWORD) msg->pixel >> 8;
                  *((UBYTE *) dest  ) = (UBYTE) msg->pixel;
                }
                break;
*/
        case 4:
            *((ULONG *) dest) = (ULONG) msg->pixel;
            break;
    }
        
}

/****************************************************************************************/

ULONG CBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
                                 struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel             retval = 0;
    UBYTE                   *src;
    struct chunkybm_data    *data;
    
    data = OOP_INST_DATA(cl, o);
          
    src = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;

    switch(data->bytesperpixel)
    {
        case 1:
            retval = (HIDDT_Pixel) *((UBYTE *) src);
            break;
            
        case 2:
            retval = (HIDDT_Pixel) *((UWORD *) src);
            break;
            
        case 3:
        #if AROS_BIG_ENDIAN
            retval = (HIDDT_Pixel) (src[0] << 16) + (src[1] << 8) + src[2];
        #else
            retval = (HIDDT_Pixel) (src[2] << 16) + (src[1] << 8) + src[0];
        #endif
            break;
            
            //(*((UBYTE *) src++) << 16) | *((UWORD *) src));
            //break;
        
        case 4:
            retval = ((ULONG) *((ULONG *) src));
            break;
    }

    return retval;
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct chunkybm_data  *data =OOP_INST_DATA(cl, o);
    HIDDT_Pixel            fg = GC_FG(msg->gc);
    HIDDT_DrawMode         mode = GC_DRMD(msg->gc);
    ULONG                  mod;

    mod = data->bytesperrow;

    switch(mode)
    {
        case vHidd_GC_DrawMode_Copy:
            if (data->bytesperpixel >= 1 && data->bytesperpixel <= 4)
            {
                /*
                 * HIDD_BM_FillMemRect8/16/24/32() according to bytes per pixel.
                 * The messages only differ in the type of fill.
                 */
                union
                {
                    struct pHidd_BitMap_FillMemRect8  m8;
                    struct pHidd_BitMap_FillMemRect16 m16;
                    struct pHidd_BitMap_FillMemRect32 m32;
                } p;

                p.m32.mID    = HiddBitMapBase + moHidd_BitMap_FillMemRect8 + data->bytesperpixel - 1;
                p.m32.dstBuf = data->buffer;
                p.m32.minX   = msg->minX;
                p.m32.minY   = msg->minY;
                p.m32.maxX   = msg->maxX;
                p.m32.maxY   = msg->maxY;
                p.m32.dstMod = mod;
                if (data->bytesperpixel == 1)
                    p.m8.fill  = fg;
                else if (data->bytesperpixel == 2)
                    p.m16.fill = fg;
                else
                    p.m32.fill = fg;
                OOP_DoMethod(o, &p.m32.mID);
            }
            break;
    
        case vHidd_GC_DrawMode_Invert:
            HIDD_BM_InvertMemRect(o,
                                 data->buffer,
                                 msg->minX * data->bytesperpixel,
                                 msg->minY,
                                 msg->maxX * data->bytesperpixel + data->bytesperpixel - 1,
                                 msg->maxY,
                                 mod);
            break;
            
        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
            
    } /* switch(mode) */
    
}

/****************************************************************************************/

/*
 * Native and Native32 transfers between a caller buffer and the chunky buffer,
 * shared by PutImage (get = FALSE) and GetImage (get = TRUE). Returns FALSE if
 * pixFmt needs ConvertPixels instead.
 */
GFX_SHARED BOOL cbm_memimage(OOP_Class *cl, OOP_Object *o, struct chunkybm_data *data,
                             UBYTE *pixels, ULONG modulo, WORD x, WORD y,
                             WORD width, WORD height, HIDDT_StdPixFmt pixFmt, BOOL get)
{
    UWORD bpp = data->bytesperpixel;

    if (pixFmt == vHidd_StdPixFmt_Native ||
        (pixFmt == vHidd_StdPixFmt_Native32 && bpp == 4))
    {
        /* HIDD_BM_CopyMemBox8/16/24/32() according to bytes per pixel */
        struct pHidd_BitMap_CopyMemBox8 p;

        if (bpp < 1 || bpp > 4)
            return TRUE;

        p.mID    = HiddBitMapBase + moHidd_BitMap_CopyMemBox8 + bpp - 1;
        p.width  = width;
        p.height = height;
        if (get)
        {
            p.src    = data->buffer;
            p.srcX   = x;
            p.srcY   = y;
            p.srcMod = data->bytesperrow;
            p.dst    = pixels;
            p.dstX   = 0;
            p.dstY   = 0;
            p.dstMod = modulo;
        }
        else
        {
            p.src    = pixels;
            p.srcX   = 0;
            p.srcY   = 0;
            p.srcMod = modulo;
            p.dst    = data->buffer;
            p.dstX   = x;
            p.dstY   = y;
            p.dstMod = data->bytesperrow;
        }
        OOP_DoMethod(o, &p.mID);
        return TRUE;
    }

    if (pixFmt == vHidd_StdPixFmt_Native32)
    {
        /* 32 bit pixels in Native32 were handled above */
        if (bpp < 1 || bpp > 3)
            return TRUE;

        if (get)
        {
            /* HIDD_BM_GetMem32Image8/16/24() */
            struct pHidd_BitMap_GetMem32Image8 p;

            p.mID    = HiddBitMapBase + moHidd_BitMap_GetMem32Image8 + bpp - 1;
            p.src    = data->buffer;
            p.srcX   = x;
            p.srcY   = y;
            p.dst    = pixels;
            p.width  = width;
            p.height = height;
            p.srcMod = data->bytesperrow;
            p.dstMod = modulo;
            OOP_DoMethod(o, &p.mID);
        }
        else
        {
            /* HIDD_BM_PutMem32Image8/16/24() */
            struct pHidd_BitMap_PutMem32Image8 p;

            p.mID    = HiddBitMapBase + moHidd_BitMap_PutMem32Image8 + bpp - 1;
            p.src    = pixels;
            p.dst    = data->buffer;
            p.dstX   = x;
            p.dstY   = y;
            p.width  = width;
            p.height = height;
            p.srcMod = modulo;
            p.dstMod = data->bytesperrow;
            OOP_DoMethod(o, &p.mID);
        }
        return TRUE;
    }

    return FALSE;
}

VOID CBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    APTR dst_pixels, src_pixels;
    OOP_Object *srcpf;

    if (!cbm_memimage(cl, o, data, msg->pixels, msg->modulo, msg->x, msg->y,
                      msg->width, msg->height, msg->pixFmt, FALSE))
    {
        src_pixels = msg->pixels;
        dst_pixels = data->buffer + msg->y * data->bytesperrow
            + msg->x * data->bytesperpixel;
        srcpf = DMEnum__Internal__GetPixFmt(CSD(cl)->dmenumclass, msg->pixFmt);

        HIDD_BM_ConvertPixels(o, &src_pixels,
            (HIDDT_PixelFormat *)srcpf, msg->modulo, &dst_pixels,
            BM_PIXFMT(o), data->bytesperrow, msg->width, msg->height,
            NULL);
    }
}

/**************************************************************************/

int static inline
__attribute__((always_inline, const)) do_alpha(int a, int v)
{
    int tmp = a * v;
    return (tmp + (tmp >> 8) + 0x80) >> 8;
}

VOID CBM__Hidd_BitMap__PutAlphaImage(OOP_Class *cl, OOP_Object *o,
    struct pHidd_BitMap_PutAlphaImage *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    HIDDT_StdPixFmt pixFmt = BM_PIXFMT(o)->stdpixfmt;
    WORD x, y, src_step, dst_step;
    UBYTE *p, *q;
    UBYTE src_red, src_green, src_blue, src_alpha;
    UBYTE dst_red, dst_green, dst_blue;

    switch(pixFmt)
    {
        case vHidd_StdPixFmt_BGR032:

            p = msg->pixels;
            q = data->buffer + msg->y * data->bytesperrow
                + msg->x * data->bytesperpixel;
            src_step = msg->modulo - msg->width * 4;
            dst_step = data->bytesperrow - data->bytesperpixel * msg->width;

            for(y = 0; y < msg->height; y++)
            {
                for(x = 0; x < msg->width; x++)
                {
                    src_alpha = *p++;
                    src_red   = *p++;
                    src_green = *p++;
                    src_blue  = *p++;

                    switch(src_alpha)
                    {
                    case 0:
                        q += 4;
                        break;

                    case 0xff:
                        *q++ = src_blue;
                        *q++ = src_green;
                        *q++ = src_red;
                        *q++ = 0;
                        break;

                    default:
                        dst_blue = *q;
                        dst_blue += do_alpha(src_alpha, src_blue - dst_blue);
                        *q++ = dst_blue;

                        dst_green = *q;
                        dst_green += do_alpha(src_alpha, src_green - dst_green);
                        *q++ = dst_green;

                        dst_red = *q;
                        dst_red  += do_alpha(src_alpha, src_red - dst_red);
                        *q++ = dst_red;

                        *q++ = 0;
                    }
                }
                p += src_step;
                q += dst_step;
            }
            break;

        case vHidd_StdPixFmt_RGB16_LE:

            p = msg->pixels;
            q = data->buffer + msg->y * data->bytesperrow
                + msg->x * data->bytesperpixel;
            src_step = msg->modulo - msg->width * 4;
            dst_step = data->bytesperrow - data->bytesperpixel * msg->width;

            for(y = 0; y < msg->height; y++)
            {
                for(x = 0; x < msg->width; x++)
                {
                    src_alpha = *p++;
                    src_red   = *p++;
                    src_green = *p++;
                    src_blue  = *p++;

                    switch(src_alpha)
                    {
                    case 0:
                        q += 2;
                        break;

                    case 0xff:
                        *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
                        *q++ = src_red & 0xf8 | src_green >> 5;
                        break;

                    default:
                        dst_blue = *q;
                        dst_red = *(q + 1);
                        dst_green = dst_red << 5 | dst_blue >> 3 & 0x1c;
                        dst_blue <<= 3;
                        dst_red &= 0xf8;

                        dst_blue += do_alpha(src_alpha, src_blue - dst_blue);
                        dst_green += do_alpha(src_alpha, src_green - dst_green);
                        dst_red  += do_alpha(src_alpha, src_red - dst_red);

                        *q++ = (dst_green << 3) & 0xe0 | dst_blue >> 3;
                        *q++ = dst_red & 0xf8 | dst_green >> 5;
                    }
                }
                p += src_step;
                q += dst_step;
            }
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
    }
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    APTR src_pixels, dst_pixels;
    OOP_Object *dstpf;

    if (!cbm_memimage(cl, o, data, msg->pixels, msg->modulo, msg->x, msg->y,
                      msg->width, msg->height, msg->pixFmt, TRUE))
    {
        src_pixels = data->buffer + msg->y * data->bytesperrow
            + msg->x * data->bytesperpixel;
        dst_pixels = msg->pixels;
        dstpf = DMEnum__Internal__GetPixFmt(CSD(cl)->dmenumclass, msg->pixFmt);

        HIDD_BM_ConvertPixels(o, &src_pixels, BM_PIXFMT(o),
            data->bytesperrow, &dst_pixels, (HIDDT_PixelFormat *)dstpf,
            msg->modulo, msg->width, msg->height, NULL);
    }
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    if (data->bytesperpixel >= 2 && data->bytesperpixel <= 4)
    {
        /* HIDD_BM_CopyLUTMemBox16/24/32() according to bytes per pixel */
        struct pHidd_BitMap_CopyLUTMemBox16 p;

        p.mID    = HiddBitMapBase + moHidd_BitMap_CopyLUTMemBox16 + data->bytesperpixel - 2;
        p.src    = msg->pixels;
        p.srcX   = 0;
        p.srcY   = 0;
        p.dst    = data->buffer;
        p.dstX   = msg->x;
        p.dstY   = msg->y;
        p.width  = msg->width;
        p.height = msg->height;
        p.srcMod = msg->modulo;
        p.dstMod = data->bytesperrow;
        p.pixlut = msg->pixlut;
        OOP_DoMethod(o, &p.mID);
    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    if (data->bytesperpixel >= 1 && data->bytesperpixel <= 4)
    {
        /* HIDD_BM_PutMemTemplate8/16/24/32() according to bytes per pixel */
        struct pHidd_BitMap_PutMemTemplate8 p;

        p.mID            = HiddBitMapBase + moHidd_BitMap_PutMemTemplate8 + data->bytesperpixel - 1;
        p.gc             = msg->gc;
        p.masktemplate   = msg->masktemplate;
        p.modulo         = msg->modulo;
        p.srcx           = msg->srcx;
        p.dst            = data->buffer;
        p.dstMod         = data->bytesperrow;
        p.x              = msg->x;
        p.y              = msg->y;
        p.width          = msg->width;
        p.height         = msg->height;
        p.inverttemplate = msg->inverttemplate;
        OOP_DoMethod(o, &p.mID);
    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPattern *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    if (data->bytesperpixel >= 1 && data->bytesperpixel <= 4)
    {
        /* HIDD_BM_PutMemPattern8/16/24/32() according to bytes per pixel */
        struct pHidd_BitMap_PutMemPattern8 p;

        p.mID           = HiddBitMapBase + moHidd_BitMap_PutMemPattern8 + data->bytesperpixel - 1;
        p.gc            = msg->gc;
        p.pattern       = msg->pattern;
        p.patternsrcx   = msg->patternsrcx;
        p.patternsrcy   = msg->patternsrcy;
        p.patternheight = msg->patternheight;
        p.patterndepth  = msg->patterndepth;
        p.patternlut    = msg->patternlut;
        p.invertpattern = msg->invertpattern;
        p.mask          = msg->mask;
        p.maskmodulo    = msg->maskmodulo;
        p.masksrcx      = msg->masksrcx;
        p.dst           = data->buffer;
        p.dstMod        = data->bytesperrow;
        p.x             = msg->x;
        p.y             = msg->y;
        p.width         = msg->width;
        p.height        = msg->height;
        OOP_DoMethod(o, &p.mID);
    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL CBM__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    IPTR width, height;

    OOP_GetAttr(o, aHidd_BitMap_Width, &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);

    *msg->addressReturn = data->buffer;
    *msg->widthReturn = width;
    *msg->heightReturn = height;
    *msg->bankSizeReturn = *msg->memSizeReturn = data->bytesperrow * height;

    return TRUE;
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
}

/****************************************************************************************/

VOID CBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    EnterFunc(bug("BitMap::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if (IS_CHUNKYBM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_ChunkyBM_Buffer:
            *msg->storage = (IPTR)data->buffer;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/****************************************************************************************/

VOID CBM__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG idx;
    BOOL refresh_bpr = FALSE;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_CHUNKYBM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_ChunkyBM_Buffer:
                    if (data->own_buffer)
                    {
                        FreeVec(data->buffer);
                        data->own_buffer = FALSE;
                        data->bufsize    = 0;
                    }
                    data->buffer = (UBYTE *)tag->ti_Data;
                    D(bug("[CBM] New buffer now 0x%p\n", data->buffer));
                    break;
            }
        }
        else if (IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            /* BitMap's Set updates bytesPerRow either directly or via
             * GetBytesPerRow when a framebuffer's ModeID changes.
             */
            if (idx == aoHidd_BitMap_BytesPerRow || idx == aoHidd_BitMap_ModeID)
                refresh_bpr = TRUE;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (refresh_bpr)
    {
        IPTR bpr = 0, height = 0;
        OOP_Object *pf = NULL;

        OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &bpr);
        if (bpr)
        {
            /* A ModeID change brings a new pixelformat with it, so the
             * cached pixel size has to follow the row size - the rest of
             * this class addresses the buffer with both. */
            OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
            if (pf)
            {
                IPTR bpp = 0;

                OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bpp);
                if (bpp)
                    data->bytesperpixel = bpp;
            }

            /* Our buffer was sized for the mode we are leaving. Growing
             * into a deeper one needs more of it, and every access below
             * uses the new row size - keeping the old allocation would
             * write past its end. */
            OOP_GetAttr(o, aHidd_BitMap_Height, &height);
            if (data->own_buffer && height && (height * bpr > data->bufsize))
            {
                ULONG newsize = height * bpr;
                UBYTE *newbuf = AllocVec(newsize, MEMF_ANY | MEMF_CLEAR);

                if (!newbuf)
                {
                    /* Leave the bitmap describing what it can actually
                     * hold rather than a buffer we do not have. */
                    D(bug("[CBM] cannot grow buffer to %u bytes\n",
                          (unsigned)newsize));
                    return;
                }
                FreeVec(data->buffer);
                data->buffer  = newbuf;
                data->bufsize = newsize;
            }

            data->bytesperrow = bpr;
        }
    }
}
