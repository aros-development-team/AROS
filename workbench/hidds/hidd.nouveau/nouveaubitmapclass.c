/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>

#include "arosdrmmode.h"

#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)

/* PUBLIC METHODS */
VOID METHOD(NouveauBitMap, Hidd_BitMap, PutPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);
    
    /* FIXME "Optimistics" synchronization (yes, I know it's wrong) */
    IPTR map = (IPTR)bmdata->bo->map;
    
    /* If the current map was NULL, wait until bitmap lock is released. 
       When it happens, it is guaranteed that bo->map is not NULL */
    if (map == (IPTR)NULL)
    {
        LOCK_BITMAP;
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
        UNLOCK_BITMAP;
}

HIDDT_Pixel METHOD(NouveauBitMap, Hidd_BitMap, GetPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);
    HIDDT_Pixel pixel = 0;
    
    /* FIXME "Optimistics" synchronization (yes, I know it's wrong) */
    IPTR map = (IPTR)bmdata->bo->map;
    
    /* If the current map was NULL, wait until bitmap lock is released. 
       When it happens, it is guaranteed that bo->map is not NULL */
    if (map == (IPTR)NULL)
    {
        LOCK_BITMAP;
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
        UNLOCK_BITMAP;
    
    return pixel;
}

OOP_Object * METHOD(NouveauBitMap, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
        IPTR width, height, depth;
        OOP_Object * pf;
        struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
        
	    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
	    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        
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
        bmdata->fbid = 0; /* Default value */
        InitSemaphore(&bmdata->semaphore);

	    /* Creation of buffer object */
	    /* FIXME: check result of call */
        nouveau_bo_new(SD(cl)->carddata.dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 
	            bmdata->pitch * bmdata->height,
	            &bmdata->bo);

        /* FIXME: if (!bmdata->bo) */
        nouveau_bo_map(bmdata->bo, NOUVEAU_BO_RDWR);
    }
    
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
        nouveau_bo_unmap(bmdata->bo);
        nouveau_bo_ref(NULL, &bmdata->bo); /* Release reference */
    }

    OOP_DoSuperMethod(cl, o, msg);
}

