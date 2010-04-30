/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#include <aros/debug.h>
#include <proto/oop.h>

#undef HiddBitMapAttrBase
#define HiddBitMapAttrBase  (SD(cl)->bitMapAttrBase)

/* HACK HACK HACK */
extern APTR fbptr;

#define writel(val, addr)               (*(volatile ULONG*)(addr) = (val))
#define readl(addr)                     (*(volatile ULONG*)(addr))
/* HACK HACK HACK */

/* PUBLIC METHODS */
OOP_Object * METHOD(NouveauBitMap, Root, New)
{
    bug("NouveauBitMap::New\n");

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutPixel)
{
    IPTR addr = (msg->x * 4) + (1024 * 4 * msg->y);
    addr += (IPTR)fbptr;
    
    writel(msg->pixel, (APTR)addr);
}

HIDDT_Pixel METHOD(NouveauBitMap, Hidd_BitMap, GetPixel)
{
    HIDDT_Pixel pixel = 0;

    IPTR addr = (msg->x * 4) + (1024 * 4 * msg->y);
    addr += (IPTR)fbptr;

    pixel = readl((APTR)addr);

    return pixel;
}



OOP_Object * METHOD(NouveauOffBitMap, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
        IPTR width, height;
        struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
        
	    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
	    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	    bmdata->width = width;
	    bmdata->height = height;
	    bmdata->buffer = AllocVec(width * 4 * height, MEMF_ANY);
    }
    
    return o;
}

VOID METHOD(NouveauOffBitMap, Hidd_BitMap, PutPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * 4) + (bmdata->width * 4 * msg->y);
    addr += (IPTR)bmdata->buffer;

    *((ULONG*)(addr)) = msg->pixel;
}

HIDDT_Pixel METHOD(NouveauOffBitMap, Hidd_BitMap, GetPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    IPTR addr = (msg->x * 4) + (bmdata->width * 4 * msg->y);
    addr += (IPTR)bmdata->buffer;

    return *((ULONG*)(addr));
}

