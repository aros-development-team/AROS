/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#include <aros/debug.h>
#include <proto/oop.h>

#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)
#define HiddPixFmtAttrBase  (SD(cl)->pixFmtAttrBase)

/* HACK HACK HACK */
extern struct nouveau_bo * hackfbo;
extern struct nouveau_device * hackdev;

/* HACK HACK HACK */

/* PUBLIC METHODS */
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
	    bmdata->bo = hackfbo; /* assuming fbo is already mapped */
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
    }
    
    return o;
}

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

OOP_Object * METHOD(NouveauOffBitMap, Root, New)
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

	    /* Creation of buffer object */
	    /* FIXME: nouveau_device should not be global */
	    /* FIXME: check result of call */
	    /* FIXME: take pitch/bpp when calculating size */
	    nouveau_bo_new(hackdev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 
	            width * height * 4,
	            &bmdata->bo);

        /* FIXME: if (!bmdata->bo) */
	    nouveau_bo_map(bmdata->bo, NOUVEAU_BO_RDWR);
    }
    
    return o;
}

VOID NouveauOffBitMap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    if (bmdata->bo)
    {
        nouveau_bo_unmap(bmdata->bo);
        nouveau_bo_ref(NULL, &bmdata->bo); /* Release reference */
    }
}

