/*
    Copyright (C) 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "arosdrmmode.h"

#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddBitMapNouveauAttrBase

#define HiddBitMapAttrBase          (SD(cl)->bitMapAttrBase)
#define HiddPixFmtAttrBase          (SD(cl)->pixFmtAttrBase)
#define HiddBitMapNouveauAttrBase   (SD(cl)->bitMapNouveauAttrBase)

#define GART_TRANSFER_ALLOWED(width, height)    ((((width) * (height)) >= (32 * 32)) && (carddata->GART))

VOID HIDDNouveauSetOffsets(OOP_Object * bm, LONG newxoffset, LONG newyoffset)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    bmdata->xoffset = newxoffset;
    bmdata->yoffset = newyoffset;
}

/* PUBLIC METHODS */
OOP_Object * METHOD(NouveauBitMap, Root, New)
{
    IPTR width, height, depth, displayable, bytesperpixel;
    OOP_Object * pf;
    struct HIDDNouveauBitMapData * bmdata = NULL;
    HIDDT_StdPixFmt stdfmt = vHidd_StdPixFmt_Unknown;
    struct CardData * carddata = &(SD(cl)->carddata);

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (!o)
        goto exit_fail;

    bmdata = OOP_INST_DATA(cl, o);

    /* Initialize default values */
    bmdata->fbid = 0;
    bmdata->xoffset = 0;
    bmdata->yoffset = 0;
    bmdata->bo = NULL;
    
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
    OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdfmt);
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
    
    D(bug("[Nouveau] BitMap New: %d x %d x %d\n", width, height, depth));

    /* Check if requested format is one of the supported ones */
    if ((stdfmt != vHidd_StdPixFmt_BGR032) && (stdfmt != vHidd_StdPixFmt_RGB16_LE))
        goto exit_fail;

    /* Check if requested depth is a supported one */
    if (depth < 16)
        goto exit_fail;
    
    /* Check if requested byted per pixel is a supported one */
    if ((bytesperpixel != 2) && (bytesperpixel != 4))
        goto exit_fail;

    /* Initialize properties */
    bmdata->width = width;
    bmdata->height = height;
    bmdata->depth = depth;
    bmdata->bytesperpixel = bytesperpixel;
    bmdata->pitch = bmdata->width * bmdata->bytesperpixel;
    if (carddata->architecture >= NV_ARCH_50)
        bmdata->pitch = (bmdata->pitch + 255) & ~255; 
    else
        bmdata->pitch = (bmdata->pitch + 63) & ~63;

    if (displayable) bmdata->displayable = TRUE; else bmdata->displayable = FALSE;
    InitSemaphore(&bmdata->semaphore);

    /* Creation of buffer object */
    nouveau_bo_new(SD(cl)->carddata.dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 
            bmdata->pitch * bmdata->height,
            &bmdata->bo);
    if (bmdata->bo == NULL)
        goto exit_fail;

    bmdata->compositing = (OOP_Object *)GetTagData(aHidd_BitMap_Nouveau_CompositingHidd, 0, msg->attrList);
    if (bmdata->compositing == NULL)
        goto exit_fail;

    return o;

exit_fail:

    bug("[Nouveau]: Failed to create bitmap %dx%d %d %d\n", width, height, depth, stdfmt);

    if (o)
    {
        OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
    }

    return NULL;
}

VOID NouveauBitMap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    D(bug("[Nouveau] Dispose %x\n", o));

    /* Unregister from framebuffer if needed */
    if (bmdata->fbid != 0)
    {
        struct nouveau_device_priv *nvdev = nouveau_device(SD(cl)->carddata.dev);
        drmModeRmFB(nvdev->fd, bmdata->fbid);   
        bmdata->fbid = 0;
    }

    if (bmdata->bo)
    {
        UNMAP_BUFFER
        nouveau_bo_ref(NULL, &bmdata->bo); /* Release reference */
    }

    OOP_DoSuperMethod(cl, o, msg);
}

VOID METHOD(NouveauBitMap, Root, Get)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_BITMAP_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_BitMap_LeftEdge:
            *msg->storage = bmdata->xoffset;
            return;
        case aoHidd_BitMap_TopEdge:
            *msg->storage = bmdata->yoffset;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(NouveauBitMap, Root, Set)
{
    struct TagItem  *tag, *tstate;
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    ULONG idx;
    LONG newxoffset = bmdata->xoffset;
    LONG newyoffset = bmdata->yoffset;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_LeftEdge:
                newxoffset = tag->ti_Data;
                break;
            case aoHidd_BitMap_TopEdge:
                newyoffset = tag->ti_Data;
                break;
            }
        }
    }

    if ((newxoffset != bmdata->xoffset) || (newyoffset != bmdata->yoffset))
    {
        /* If there was a change requested, validate it */
        struct pHidd_Compositing_ValidateBitMapPositionChange vbpcmsg =
        {
            mID : SD(cl)->mid_ValidateBitMapPositionChange,
            bm : o,
            newxoffset : &newxoffset,
            newyoffset : &newyoffset
        };
        
        OOP_DoMethod(bmdata->compositing, (OOP_Msg)&vbpcmsg);
        
        if ((newxoffset != bmdata->xoffset) || (newyoffset != bmdata->yoffset))
        {
            /* If change passed validation, execute it */
            struct pHidd_Compositing_BitMapPositionChanged bpcmsg =
            {
                mID : SD(cl)->mid_BitMapPositionChanged,
                bm : o
            };

            HIDDNouveauSetOffsets(o, newxoffset, newyoffset);
        
            OOP_DoMethod(bmdata->compositing, (OOP_Msg)&bpcmsg);
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);

    /* FIXME "Optimistics" synchronization (yes, I know it's wrong) */
    IPTR map = (IPTR)bmdata->bo->map;
    
    /* If the current map was NULL, wait until bitmap lock is released. 
       When it happens, map the buffer */
    if (map == (IPTR)NULL)
    {
        LOCK_BITMAP
        MAP_BUFFER
        addr += (IPTR)bmdata->bo->map;
    }
    else
        addr += map;
    
    switch(bmdata->bytesperpixel)
    {
    case(1):
        /* Not supported */
        break;
    case(2):
        writew(msg->pixel, (APTR)addr);
        break;
    case(4):
        writel(msg->pixel, (APTR)addr);
        break;
    }

    if (map == (IPTR)NULL)    
        UNLOCK_BITMAP
}

HIDDT_Pixel METHOD(NouveauBitMap, Hidd_BitMap, GetPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);
    HIDDT_Pixel pixel = 0;

    /* FIXME "Optimistics" synchronization (yes, I know it's wrong) */
    IPTR map = (IPTR)bmdata->bo->map;

    /* If the current map was NULL, wait until bitmap lock is released. 
       When it happens, map the buffer */
    if (map == (IPTR)NULL)
    {
        LOCK_BITMAP
        MAP_BUFFER
        addr += (IPTR)bmdata->bo->map;
    }
    else
        addr += map;
    
    switch(bmdata->bytesperpixel)
    {
    case(1):
        /* Not supported */
        break;
    case(2):
        pixel = readw((APTR)addr);
        break;
    case(4):
        pixel = readl((APTR)addr);
        break;
    }
    
    if (map == (IPTR)NULL)    
        UNLOCK_BITMAP
    
    return pixel;
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, Clear)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    BOOL ret = FALSE;
    
    LOCK_BITMAP
    UNMAP_BUFFER

    switch(carddata->architecture)
    {
    case(NV_ARCH_03):
    case(NV_ARCH_04):
    case(NV_ARCH_10):
    case(NV_ARCH_20):
    case(NV_ARCH_30):
    case(NV_ARCH_40):
        ret = HIDDNouveauNV04FillSolidRect(carddata, bmdata, 
                    0, 0, bmdata->width - 1, bmdata->height - 1, GC_DRMD(msg->gc), GC_BG(msg->gc));
        break;
    case(NV_ARCH_50):
        ret = HIDDNouveauNV50FillSolidRect(carddata, bmdata, 
                    0, 0, bmdata->width - 1, bmdata->height - 1, GC_DRMD(msg->gc), GC_BG(msg->gc));
        break;
    case(NV_ARCH_C0):
        ret = HIDDNouveauNVC0FillSolidRect(carddata, bmdata, 
                    0, 0, bmdata->width - 1, bmdata->height - 1, GC_DRMD(msg->gc), GC_BG(msg->gc));
        break;
    }    

    UNLOCK_BITMAP

    if (ret)
        return;    
    
    /* Fallback to default method */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* There is no pHidd_BitMap_FillRect structure - it's the same as pHidd_BitMap_DrawRect */
#define pHidd_BitMap_FillRect pHidd_BitMap_DrawRect

VOID METHOD(NouveauBitMap, Hidd_BitMap, FillRect)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    BOOL ret = FALSE;
    
    LOCK_BITMAP
    UNMAP_BUFFER

    switch(carddata->architecture)
    {
    case(NV_ARCH_03):
    case(NV_ARCH_04):
    case(NV_ARCH_10):
    case(NV_ARCH_20):
    case(NV_ARCH_30):
    case(NV_ARCH_40):
        ret = HIDDNouveauNV04FillSolidRect(carddata, bmdata, 
                    msg->minX, msg->minY, msg->maxX, msg->maxY, GC_DRMD(msg->gc), GC_FG(msg->gc));
        break;
    case(NV_ARCH_50):
        ret = HIDDNouveauNV50FillSolidRect(carddata, bmdata, 
                    msg->minX, msg->minY, msg->maxX, msg->maxY, GC_DRMD(msg->gc), GC_FG(msg->gc));
        break;
    case(NV_ARCH_C0):
        ret = HIDDNouveauNVC0FillSolidRect(carddata, bmdata, 
                    msg->minX, msg->minY, msg->maxX, msg->maxY, GC_DRMD(msg->gc), GC_FG(msg->gc));
        break;
    }

    UNLOCK_BITMAP

    if (ret)
        return;    
    
    /* Fallback to default method */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutImage)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    LOCK_BITMAP

    /* For larger transfers use GART */
    if (GART_TRANSFER_ALLOWED(msg->width, msg->height))
    {
        BOOL result = FALSE;
        
        /* RAM->CPU->GART GART->GPU->VRAM */
        UNMAP_BUFFER

        ObtainSemaphore(&carddata->gartsemaphore);
        
        result = HiddNouveauNVAccelUploadM2MF(
                    msg->pixels, msg->modulo, msg->pixFmt,
                    msg->x, msg->y, msg->width, msg->height, 
                    cl, o);
        
        ReleaseSemaphore(&carddata->gartsemaphore);

        if (result)
        {
            UNLOCK_BITMAP;
            return;
        }
    }

    /* Fallback */

    /* RAM->CPU->VRAM */
    {
    APTR dstBuff = NULL;
    
    MAP_BUFFER

    /* Calculate destination buffer pointer */
    dstBuff = (APTR)((IPTR)bmdata->bo->map + (msg->y * bmdata->pitch) + (msg->x * bmdata->bytesperpixel));
    
    HiddNouveauWriteFromRAM(
        msg->pixels, msg->modulo, msg->pixFmt,
        dstBuff, bmdata->pitch,
        msg->width, msg->height,
        cl, o);
    }

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, GetImage)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    LOCK_BITMAP

    /* For larger transfers use GART */
    if (GART_TRANSFER_ALLOWED(msg->width, msg->height))
    {
        BOOL result = FALSE;
        
        /* VRAM->CPU->GART GART->GPU->RAM */
        UNMAP_BUFFER

        ObtainSemaphore(&carddata->gartsemaphore);
        
        result = HiddNouveauNVAccelDownloadM2MF(
                    msg->pixels, msg->modulo, msg->pixFmt,
                    msg->x, msg->y, msg->width, msg->height, 
                    cl, o);

        ReleaseSemaphore(&carddata->gartsemaphore);

        if (result)
        {
            UNLOCK_BITMAP;
            return;
        }
    }

    /* Fallback */

    /* VRAM->CPU->RAM */
    {
    APTR srcBuff = NULL;
    
    MAP_BUFFER

    /* Calculate source buffer pointer */
    srcBuff = (APTR)((IPTR)bmdata->bo->map + (msg->y * bmdata->pitch) + (msg->x * bmdata->bytesperpixel));
    
    HiddNouveauReadIntoRAM(
        srcBuff, bmdata->pitch,
        msg->pixels, msg->modulo, msg->pixFmt,
        msg->width, msg->height,
        cl, o);
    }

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutAlphaImage)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    LOCK_BITMAP

    /* Try hardware method NV10-NV40*/
    if (
        ((carddata->architecture >= NV_ARCH_10) && (carddata->architecture <= NV_ARCH_40))
        
        && (bmdata->bytesperpixel > 1)
        && (GART_TRANSFER_ALLOWED(msg->width, msg->height)))
    {
        BOOL result = FALSE;
        
        /* RAM->CPU->GART GART->GPU->VRAM */
        UNMAP_BUFFER

        ObtainSemaphore(&carddata->gartsemaphore);
        
        result = HiddNouveauAccelARGBUpload3D(
                    msg->pixels, msg->modulo,
                    msg->x, msg->y, msg->width, msg->height, 
                    cl, o);
        
        ReleaseSemaphore(&carddata->gartsemaphore);

        if (result)
        {
            MAP_BUFFER; /* FIXME: This is needed to flush execution buffer, atrifact otherwise */
            UNLOCK_BITMAP;
            return;
        }
    }
    
    /* Try optimization for NV50 */
    if (
        (carddata->architecture >= NV_ARCH_50)
        
        && (bmdata->bytesperpixel > 1)
        && (GART_TRANSFER_ALLOWED(msg->width, msg->height)))
    {
        /* Hardware method is not currently possible for NV50 as the implementation
           relies on tiled bitmaps. AROS uses linear bitmaps for all card families.
           The optimization in this case is to use base class implementation,
           which does GetImage->Process->PutImage. Since all NV50 cards are
           PCI-E based, the greatest limiting factor - VRAM->RAM download speed in
           not a problem (1,1 Gbps on my GF8300). This approach is actually faster
           than "per-pixel" functions below by order of 10. */
           
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        UNLOCK_BITMAP;
        return;
    }

    /* Fallback to software method */
    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            MAP_BUFFER

            HIDDNouveauBitMapPutAlphaImage16(bmdata, msg->pixels, msg->modulo, msg->x, 
                msg->y, msg->width, msg->height);
        }
        break;

    case 4:
        {
            MAP_BUFFER

            HIDDNouveauBitMapPutAlphaImage32(bmdata, msg->pixels, msg->modulo, msg->x, 
                msg->y, msg->width, msg->height);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
}

ULONG METHOD(NouveauBitMap, Hidd_BitMap, BytesPerLine)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    
    return (bmdata->pitch);
}

BOOL METHOD(NouveauBitMap, Hidd_BitMap, ObtainDirectAccess)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    MAP_BUFFER

    *msg->addressReturn = (UBYTE*)bmdata->bo->map;
    *msg->widthReturn = bmdata->pitch / bmdata->bytesperpixel;
    *msg->heightReturn = bmdata->height;
    *msg->bankSizeReturn = *msg->memSizeReturn = bmdata->pitch * bmdata->height;

    return TRUE;
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, ReleaseDirectAccess)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    UNLOCK_BITMAP
}

#define COMPLEMENT_JAM2_DECISION_BLOCK                                              \
    else if (GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Invert)                          \
    {                                                                               \
        /* COMPLEMENT - read & write. Base method uses GetImage/PutImage.           \
           It is better to use it, if GetImage is fast(==PCIE) */                   \
        if (GART_TRANSFER_ALLOWED(msg->width, msg->height) && (carddata->IsPCIE))   \
        {                                                                           \
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);                                 \
            return;                                                                 \
        }                                                                           \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        /* JAM2 - only write. Base method uses PutImage. It is                      \
           better to use it, if it is accelerated */                                \
        if (GART_TRANSFER_ALLOWED(msg->width, msg->height))                         \
        {                                                                           \
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);                                 \
            return;                                                                 \
        }                                                                           \
    }                                                                               \


VOID METHOD(NouveauBitMap, Hidd_BitMap, PutAlphaTemplate)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    /* Select acceleration method based on hardware and buffer size */
    if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
    {
        /* JAM1 - read & write. Base method uses GetImage/PutImage.
           Use 3D alpha blending where possible */
        if (GART_TRANSFER_ALLOWED(msg->width, msg->height))
        {
            /* These cards support 3D alpha blending */
            if ((carddata->architecture >= NV_ARCH_10) && (carddata->architecture <= NV_ARCH_40))
            {
                BOOL result = FALSE;
                HIDDT_Color color;
                LONG fg_red, fg_green, fg_blue;

                HIDD_BM_UnmapPixel(o, GC_FG(msg->gc), &color);

                fg_red   = color.red >> 8;
                fg_green = color.green >> 8;
                fg_blue  = color.blue >> 8;
                
                LOCK_BITMAP
                UNMAP_BUFFER
                
                ObtainSemaphore(&carddata->gartsemaphore);
                
                result = HiddNouveauAccelAPENUpload3D(msg->alpha, msg->invertalpha,
                    msg->modulo, (fg_red << 16) | (fg_green << 8) | fg_blue, 
                    msg->x, msg->y, msg->width, msg->height, cl, o);

                ReleaseSemaphore(&carddata->gartsemaphore);
                MAP_BUFFER; /* FIXME: This is needed to flush execution buffer, atrifact otherwise */
                UNLOCK_BITMAP

                if (result) return;
            }
            
            /* These cards don't support 3D alpha blending (yet), but they are all
               PCIE so GetImage is fast */
            if (carddata->architecture >= NV_ARCH_50)
            {
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
                return;
            }
        }
    }
    COMPLEMENT_JAM2_DECISION_BLOCK 

    /* This is software fallback */
    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            HIDDNouveauBitMapPutAlphaTemplate16(bmdata, msg->gc, o, msg->invertalpha,
                msg->alpha, msg->modulo, msg->x, msg->y, msg->width, msg->height);
        }
        break;

    case 4:
        {
            HIDDNouveauBitMapPutAlphaTemplate32(bmdata, msg->gc, o, msg->invertalpha,
                msg->alpha, msg->modulo, msg->x, msg->y, msg->width, msg->height);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutTemplate)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    /* Select execution method based on hardware and buffer size */
    if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
    {
        /* JAM1 - read & write. Base method uses GetImage/PutImage.
           Use software fallback. It performs only limited writes and thus it
           is faster than base method. */
    }
    COMPLEMENT_JAM2_DECISION_BLOCK

    /* This is software fallback */
    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            struct pHidd_BitMap_PutMemTemplate16 __m = 
            {
                SD(cl)->mid_PutMemTemplate16, msg->gc, msg->masktemplate, msg->modulo,
                msg->srcx, bmdata->bo->map, bmdata->pitch, msg->x, msg->y,
                msg->width, msg->height, msg->inverttemplate
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;

    case 4:
        {
            struct pHidd_BitMap_PutMemTemplate32 __m = 
            {
                SD(cl)->mid_PutMemTemplate32, msg->gc, msg->masktemplate, msg->modulo,
                msg->srcx, bmdata->bo->map, bmdata->pitch, msg->x, msg->y,
                msg->width, msg->height, msg->inverttemplate
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutPattern)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    /* Select execution method based on hardware and buffer size */
    if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
    {
        /* JAM1 - read & write. Base method uses GetImage/PutImage.
           Use software fallback. It performs only limited writes and thus it
           is faster than base method. */
    }
    COMPLEMENT_JAM2_DECISION_BLOCK

    /* This is software fallback */
    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            struct pHidd_BitMap_PutMemPattern16 __m = 
            {
                SD(cl)->mid_PutMemPattern16, msg->gc, msg->pattern, msg->patternsrcx,
                msg->patternsrcy, msg->patternheight, msg->patterndepth, msg->patternlut,
                msg->invertpattern, msg->mask, msg->maskmodulo, msg->masksrcx,
                bmdata->bo->map, bmdata->pitch, msg->x, msg->y, msg->width, msg->height
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;

    case 4:
        {
            struct pHidd_BitMap_PutMemPattern32 __m = 
            {
                SD(cl)->mid_PutMemPattern32, msg->gc, msg->pattern, msg->patternsrcx,
                msg->patternsrcy, msg->patternheight, msg->patterndepth, msg->patternlut,
                msg->invertpattern, msg->mask, msg->maskmodulo, msg->masksrcx,
                bmdata->bo->map, bmdata->pitch, msg->x,msg->y, msg->width,
                msg->height
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, DrawLine)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    LOCK_BITMAP

    if ((GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Copy) && (GC_COLMASK(msg->gc) == ~0))
    {
        MAP_BUFFER

        HIDDNouveauBitMapDrawSolidLine(bmdata, msg->gc, msg->x1, msg->y1, msg->x2, msg->y2);

        UNLOCK_BITMAP

        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    
    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, UpdateRect)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    
    if (bmdata->displayable)
    {
        struct pHidd_Compositing_BitMapRectChanged brcmsg =
        {
            mID : SD(cl)->mid_BitMapRectChanged,
            bm : o,
            x : msg->x,
            y : msg->y,
            width : msg->width,
            height : msg->height
        };
        
        OOP_DoMethod(bmdata->compositing, (OOP_Msg)&brcmsg);    
    }
}
