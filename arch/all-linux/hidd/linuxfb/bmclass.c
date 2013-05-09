/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Onscreen bitmap class for linux fb device
    Lang: English.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/graphics.h>
#include <hidd/unixio_inline.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "linuxfb_intern.h"
#include "bitmap.h"

/*********** BitMap::New() *************************************/

OOP_Object *LinuxBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[LinuxBM] Created base bitmap %p\n", o));

    if (NULL != o)
    {
        struct BitmapData *data = OOP_INST_DATA(cl, o);
        OOP_Object *pf;

        data->fbdev = GetTagData(aHidd_LinuxFBBitmap_FBDevInfo, -1, msg->attrList);

        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_BitsPerPixel, &data->bpp);
    }

    return o;
}

BOOL LinuxBM__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

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

