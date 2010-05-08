/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#include <aros/debug.h>
#include <proto/oop.h>

#include "arosdrmmode.h"

#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)

/* HACK HACK HACK */
extern struct nouveau_device * hackdev;

/* HACK HACK HACK */

/* PUBLIC METHODS */
VOID METHOD(NouveauBitMap, Hidd_BitMap, PutPixel)
{
    /* FIXME: take format (byte/word/long) into account */
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);
    addr += (IPTR)bmdata->bo->map;

    writel(msg->pixel, (APTR)addr);
}

HIDDT_Pixel METHOD(NouveauBitMap, Hidd_BitMap, GetPixel)
{
    /* FIXME: take format (byte/word/long) into account */
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);
    addr += (IPTR)bmdata->bo->map;

    return readl((APTR)addr);
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
        bmdata->pitch = (bmdata->width * bmdata->bytesperpixel + 63) & ~63;
        bmdata->fbid = 0; /* Default value */

	    /* Creation of buffer object */
	    /* FIXME: nouveau_device should not be global */
	    /* FIXME: check result of call */
	    /* FIXME: take pitch/bpp when calculating size */
	    nouveau_bo_new(hackdev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 
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

    /* Unregister from framebuffer if needed */
    if (bmdata->fbid != 0)
    {
        struct nouveau_device_priv *nvdev = nouveau_device(hackdev);
        drmModeRmFB(nvdev->fd, bmdata->fbid);   
//        bmdata->fbid = 0;
    }

    if (bmdata->bo)
    {
        nouveau_bo_unmap(bmdata->bo);
        nouveau_bo_ref(NULL, &bmdata->bo); /* Release reference */
    }

    if (bmdata->fbid == 0)
    {
        /* FIXME: There is some problem with calling this for bitmaps there
        were once assigned to framebuffer. INVASTIGATE. */
        OOP_DoSuperMethod(cl, o, msg);
    }
}

