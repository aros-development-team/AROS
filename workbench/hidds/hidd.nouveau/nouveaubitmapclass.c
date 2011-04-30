/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
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

/* HELPER FUNCTIONS */
static inline int do_alpha(int a, int v)
{
    int tmp = a*v;
    return ((tmp << 8) + tmp + 32768) >> 16;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is mapped */
static VOID HIDDNouveauBitMapPutAlphaImage32(struct HIDDNouveauBitMapData * bmdata,
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
static VOID HIDDNouveauBitMapPutAlphaImage16(struct HIDDNouveauBitMapData * bmdata,
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
static VOID HIDDNouveauBitMapPutAlphaImage15(struct HIDDNouveauBitMapData * bmdata,
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

                    dst_red   = (destpix & 0x00007c00) >> 7;
                    dst_green = (destpix & 0x000003e0) >> 2;
                    dst_blue  = (destpix & 0x0000001f) << 3;

                    dst_red   += do_alpha(src_alpha, src_red - dst_red);
                    dst_green += do_alpha(src_alpha, src_green - dst_green);
                    dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);
                }

                destpix = (ULONG)(((dst_red << 7) & 0x7c00) | ((dst_green << 2) & 0x03e0) | ((dst_blue >> 3) & 0x001f));

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
static VOID HIDDNouveauBitMapPutAlphaTemplate32(struct HIDDNouveauBitMapData * bmdata,
    APTR srcalphabuff, ULONG srcalphapitch, ULONG destX, ULONG destY, ULONG width, ULONG height,
    LONG fg_red, LONG fg_green, LONG fg_blue)
{
    ULONG   x,y;
    LONG    src_alpha;
    LONG    dst_red, dst_green, dst_blue;
    ULONG   destpix;
    
    /* SUPPORTED : JAM1 */

    for(y = 0; y < height; y++)
    {
        /* Calculate line start addresses */
        IPTR srcaddr = (srcalphapitch * y) + (IPTR)srcalphabuff;
        IPTR destaddr = (destX * 4) + (bmdata->pitch * (destY + y)) + (IPTR)bmdata->bo->map;
        
        for (x = 0; x < width; x++)
        {
            src_alpha = *(UBYTE *)srcaddr;
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
                    dst_red = fg_red;
                    dst_green = fg_green;
                    dst_blue = fg_blue;
                }
                else
                {
                    /*
                    * Alpha blending with fg color and destination pixels.
                    * Get destination.
                    */
                    destpix = readl(destaddr);

                    dst_red   = (destpix & 0x00FF0000) >> 16;
                    dst_green = (destpix & 0x0000FF00) >> 8;
                    dst_blue  = (destpix & 0x000000FF);

                    dst_red   += do_alpha(src_alpha, fg_red - dst_red);
                    dst_green += do_alpha(src_alpha, fg_green - dst_green);
                    dst_blue  += do_alpha(src_alpha, fg_blue - dst_blue);
                }

                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);

                /* Store the new pixel */
                writel(destpix, destaddr);
            }

            /* Advance pointers */
            srcaddr += 1;
            destaddr += 4;
        }
    }
}


/* TEMP - FIXME HACK FOR PATCHRGBCONV */
void HACK_PATCHRGBCONV(OOP_Object * bitmap);
/* TEMP - FIXME HACK FOR PATCHRGBCONV */

/* PUBLIC METHODS */
OOP_Object * METHOD(NouveauBitMap, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
        IPTR width, height, depth, displayable;
        OOP_Object * pf;
        struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
        
        OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
        
        /* Initialize properties */
        bmdata->width = width;
        bmdata->height = height;
        bmdata->depth = depth;
        if (depth <= 8)
            bmdata->bytesperpixel = 1;
        else if (depth <= 16)
            bmdata->bytesperpixel = 2;
        else
            bmdata->bytesperpixel = 4;
        bmdata->pitch = (bmdata->width + 63) & ~63;
        bmdata->pitch *= bmdata->bytesperpixel;
        if (displayable) bmdata->displayable = TRUE; else bmdata->displayable = FALSE;
        InitSemaphore(&bmdata->semaphore);

        /* Display information */
        bmdata->fbid = 0; /* Default value */
        bmdata->xoffset = 0;
        bmdata->yoffset = 0;

        /* Creation of buffer object */
        /* FIXME: check result of call */
        nouveau_bo_new(SD(cl)->carddata.dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 
                bmdata->pitch * bmdata->height,
                &bmdata->bo);

        bmdata->compositing = (OOP_Object *)GetTagData(aHidd_BitMap_Nouveau_CompositingHidd, 0, msg->attrList);
        /* FIXME: check if compositing hidd was passed */
    }
    
    /* TEMP - FIXME HACK FOR PATCHRGBCONV */
    /* Yes, it can be called more than once */
    if (!SD(cl)->rgbpatched)
    {
        SD(cl)->rgbpatched = TRUE;
        HACK_PATCHRGBCONV(o);
    }
    /* TEMP - FIXME HACK FOR PATCHRGBCONV */
    
    
    return o;
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
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
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

            bmdata->xoffset = newxoffset;
            bmdata->yoffset = newyoffset;
        
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
        writeb(msg->pixel, (APTR)addr);
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
        pixel = readb((APTR)addr);
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
        ret = FALSE; /* TODO:NVC0: IMPLEMENT */
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
        ret = FALSE; /* TODO:NVCO: IMPLEMENT */
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
    if (((msg->width * msg->height) >= (64 * 64)) && (carddata->GART) && (carddata->architecture != NV_ARCH_C0))/* TODO:NVCO: IMPLEMENT */
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
    if (((msg->width * msg->height) >= (64 * 64)) && (carddata->GART) && (carddata->architecture != NV_ARCH_C0))/* TODO:NVCO: IMPLEMENT */
    {
        BOOL result = FALSE;
        
        /* RAM->CPU->GART GART->GPU->VRAM */
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

    /* RAM->CPU->VRAM */
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

    /* TODO: implement accelerated putalphaimage */

    /*
    * Treat each depth case separately
    */
    if (bmdata->bytesperpixel == 4)
    {
        LOCK_BITMAP
        MAP_BUFFER
        HIDDNouveauBitMapPutAlphaImage32(bmdata, msg->pixels, msg->modulo, msg->x, 
            msg->y, msg->width, msg->height);
        UNLOCK_BITMAP
        return;
    }
    else if (bmdata->bytesperpixel == 2)
    {
        if (bmdata->depth == 16)
        {
            LOCK_BITMAP
            MAP_BUFFER
            HIDDNouveauBitMapPutAlphaImage16(bmdata, msg->pixels, msg->modulo, msg->x, 
                msg->y, msg->width, msg->height);
            UNLOCK_BITMAP
            return;
        }
        else if (bmdata->depth == 15)
        {
            LOCK_BITMAP
            MAP_BUFFER
            HIDDNouveauBitMapPutAlphaImage15(bmdata, msg->pixels, msg->modulo, msg->x, 
                msg->y, msg->width, msg->height);
            UNLOCK_BITMAP
            return;
        }
    }
 
    /* Fallback to default method */    
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
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

/* TEMP IMPLEMENTATION */

struct pHidd_BitMap_PutMemAlphaTemplate32
{
    OOP_MethodID    mID;
    OOP_Object      *gc;
    UBYTE           *alpha;
    ULONG           modulo;
    APTR            dst;
    ULONG           dstMod;
    WORD            x, y;
    WORD            width, height;
    BOOL            invertalpha;
};


VOID BM__Hidd_BitMap__PutMemAlphaTemplate32(OOP_Class *cl, OOP_Object *o,
                       struct pHidd_BitMap_PutMemAlphaTemplate32 *msg)
{
    WORD        x, y;
    UBYTE       *pixarray = msg->alpha;
    ULONG       *xbuf;
    UBYTE       *buf;
    OOP_Object  *gc = msg->gc;
    HIDDT_Color color;
    LONG        a_red, a_green, a_blue;
    LONG        b_red = 0, b_green = 0, b_blue = 0;
    WORD        type = 0;

    EnterFunc(bug("BitMap::PutAlphaTemplate(x=%d, y=%d, width=%d, height=%d)\n"
        , msg->x, msg->y, msg->width, msg->height));

    if (msg->width <= 0 || msg->height <= 0)
        return;

    HIDD_BM_UnmapPixel(o, GC_FG(gc), &color);

    a_red   = color.red >> 8;
    a_green = color.green >> 8;
    a_blue  = color.blue >> 8;

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

        HIDD_BM_UnmapPixel(o, GC_BG(gc), &color);
        b_red   = color.red >> 8;
        b_green = color.green >> 8;
        b_blue  = color.blue >> 8;
    }

    if (msg->invertalpha) type++;
    
    buf = msg->dst + msg->y * msg->dstMod + msg->x * 4;

    for(y = msg->y; y < msg->y + msg->height; y++)
    {

        xbuf = (ULONG *)buf;

        switch(type)
        {
            case 0: /* JAM1 */
            for(x = 0; x < msg->width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = *pixarray++;

                destpix = xbuf[x];

                #if 1// AROS_BIG_ENDIAN        
                dst_red   = (destpix & 0x00FF0000) >> 16;
                dst_green = (destpix & 0x0000FF00) >> 8;
                dst_blue  = (destpix & 0x000000FF);
                #else
                dst_red   = (destpix & 0x0000FF00) >> 8;
                dst_green = (destpix & 0x00FF0000) >> 16;
                dst_blue  = (destpix & 0xFF000000) >> 24;
                #endif

                dst_red   += do_alpha(alpha, a_red - dst_red);
                dst_green += do_alpha(alpha, a_green - dst_green);
                dst_blue  += do_alpha(alpha, a_blue - dst_blue);

                #if 1//AROS_BIG_ENDIAN
                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                #else
                destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
                #endif

                xbuf[x] = destpix;

            } /* for(x = 0; x < msg->width; x++) */
            break;

            case 1: /* JAM1 | INVERSVID */
            for(x = 0; x < msg->width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = (*pixarray++) ^ 255;

                destpix = xbuf[x];

                #if AROS_BIG_ENDIAN        
                dst_red   = (destpix & 0x00FF0000) >> 16;
                dst_green = (destpix & 0x0000FF00) >> 8;
                dst_blue  = (destpix & 0x000000FF);
                #else
                dst_red   = (destpix & 0x0000FF00) >> 8;
                dst_green = (destpix & 0x00FF0000) >> 16;
                dst_blue  = (destpix & 0xFF000000) >> 24;
                #endif

                dst_red   += do_alpha(alpha, a_red - dst_red);
                dst_green += do_alpha(alpha, a_green - dst_green);
                dst_blue  += do_alpha(alpha, a_blue - dst_blue);

                #if AROS_BIG_ENDIAN
                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                #else
                destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
                #endif

                xbuf[x] = destpix;

            } /* for(x = 0; x < msg->width; x++) */
            break;

            case 2: /* COMPLEMENT */
            for(x = 0; x < msg->width; x++)
            {
                ULONG   destpix;
                UBYTE   alpha;

                alpha = *pixarray++;

                destpix = xbuf[x];
                if (alpha >= 0x80) destpix = ~destpix;
                xbuf[x] = destpix;

            } /* for(x = 0; x < msg->width; x++) */
            break;

            case 3: /* COMPLEMENT | INVERSVID*/
            for(x = 0; x < msg->width; x++)
            {
                ULONG   destpix;
                UBYTE   alpha;

                alpha = *pixarray++;

                destpix = xbuf[x];
                if (alpha < 0x80) destpix = ~destpix;
                xbuf[x] = destpix;

            } /* for(x = 0; x < msg->width; x++) */
            break;

            case 4: /* JAM2 */
            for(x = 0; x < msg->width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = *pixarray++;

                dst_red   = b_red   + ((a_red   - b_red)   * alpha) / 256;
                dst_green = b_green + ((a_green - b_green) * alpha) / 256;
                dst_blue  = b_blue  + ((a_blue  - b_blue)  * alpha) / 256;

                #if AROS_BIG_ENDIAN
                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                #else
                destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
                #endif

                xbuf[x] = destpix;

            } /* for(x = 0; x < msg->width; x++) */
            break;

            case 5: /* JAM2 | INVERSVID */
            for(x = 0; x < msg->width; x++)
            {
                ULONG   destpix;
                LONG    dst_red, dst_green, dst_blue, alpha;

                alpha = (*pixarray++) ^ 255;

                dst_red   = b_red   + ((a_red   - b_red)   * alpha) / 256;
                dst_green = b_green + ((a_green - b_green) * alpha) / 256;
                dst_blue  = b_blue  + ((a_blue  - b_blue)  * alpha) / 256;

                #if AROS_BIG_ENDIAN
                destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                #else
                destpix = (dst_blue << 24) + (dst_green << 16) + (dst_red << 8);
                #endif

                xbuf[x] = destpix;

            } /* for(x = 0; x < msg->width; x++) */
            break;

        } /* switch(type) */

  
        buf += msg->dstMod;
        pixarray += msg->modulo - msg->width;

    } /* for(y = msg->y; y < msg->y + msg->height; y++) */

    ReturnVoid("BitMap::PutMemAlphaTemplate32");
}


/* TEMP IMPLEMENTATION */


VOID METHOD(NouveauBitMap, Hidd_BitMap, PutAlphaTemplate)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);



    switch(bmdata->bytesperpixel)
    {
    case 1:
        {
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
        break;

    case 2:
        {
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
        break;

    case 4:
        {
            LOCK_BITMAP
            MAP_BUFFER
            struct pHidd_BitMap_PutMemAlphaTemplate32 __m = 
            {
                /*SD(cl)->mid_PutMemTemplate32*/0, msg->gc, msg->alpha, msg->modulo,
                bmdata->bo->map, bmdata->pitch, msg->x, msg->y,
                msg->width, msg->height, msg->invertalpha
            }, *m = &__m;
//            OOP_DoMethod(o, (OOP_Msg)m);
            BM__Hidd_BitMap__PutMemAlphaTemplate32(cl, o, m);
            UNLOCK_BITMAP
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    return;    
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutTemplate)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        {
            struct pHidd_BitMap_PutMemTemplate8 __m = 
            {
                SD(cl)->mid_PutMemTemplate8, msg->gc, msg->template, msg->modulo,
                msg->srcx, bmdata->bo->map, bmdata->pitch, msg->x, msg->y,
                msg->width, msg->height, msg->inverttemplate
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;

    case 2:
        {
            struct pHidd_BitMap_PutMemTemplate16 __m = 
            {
                SD(cl)->mid_PutMemTemplate16, msg->gc, msg->template, msg->modulo,
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
                SD(cl)->mid_PutMemTemplate32, msg->gc, msg->template, msg->modulo,
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

    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        {
            struct pHidd_BitMap_PutMemPattern8 __m = 
            {
                SD(cl)->mid_PutMemPattern8, msg->gc, msg->pattern, msg->patternsrcx,
                msg->patternsrcy, msg->patternheight, msg->patterndepth, msg->patternlut,
                msg->invertpattern, msg->mask, msg->maskmodulo, msg->masksrcx,
                bmdata->bo->map, bmdata->pitch, msg->x, msg->y, msg->width,
                msg->height
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
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
