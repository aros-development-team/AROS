/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Onscreen bitmap class for Linux FB Gfx Hidd
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <hidd/gfx.h>
#include <hidd/unixio.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "linuxfbgfx_intern.h"
#include "linuxfbgfx_bitmap.h"

/*********** BitMap::New() *************************************/

OOP_Object *LinuxFBBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[LinuxBM] Created base bitmap %p\n", o));

    if (NULL != o)
    {
        struct LinuxFBBitMapData *data = OOP_INST_DATA(cl, o);
        OOP_Object *pf;

        data->fbdev = GetTagData(aHidd_LinuxFBBitmap_FBDevInfo, -1, msg->attrList);

        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_BitsPerPixel, &data->bpp);
    }

    return o;
}

BOOL LinuxFBBM__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct LinuxFBBitMapData *data = OOP_INST_DATA(cl, o);

    if ((msg->firstColor + msg->numColors) > (1 << (data->bpp)))
    {
        return FALSE;
    }

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
    {
        return FALSE;
    }

    if (data->fbdev != -1)
    {
        struct LinuxFB_staticdata *fsd = LSD(cl);
        ULONG xc_i, col_i;

        for ( xc_i = msg->firstColor, col_i = 0;
              col_i < msg->numColors;
              xc_i ++, col_i ++)
        {
            struct fb_cmap col =
            {
                xc_i, 1,
                &msg->colors[col_i].red,
                &msg->colors[col_i].green,
                &msg->colors[col_i].blue,
                &msg->colors[col_i].alpha
            };

            Hidd_UnixIO_IOControlFile(fsd->unixio, data->fbdev, FBIOPUTCMAP, &col, NULL);
        }
    }
    return TRUE;
}

