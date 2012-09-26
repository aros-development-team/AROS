/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Onscreen bitmap class for linux fb device
    Lang: English.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/graphics.h>
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
        OOP_Object * pf;
        struct BitmapData *data = OOP_INST_DATA(cl, o);
        HIDDT_ModeID modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        IPTR width  = 0;
        IPTR height = 0;
        IPTR mod    = 0;
        IPTR bytesperpixel = 0;

        OOP_GetAttr(o, aHidd_BitMap_Width , &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &mod);
        OOP_GetAttr(o, aHidd_ChunkyBM_Buffer, (IPTR *)&data->VideoData);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);

        data->width         = width;
        data->height        = height;
        data->bytesperline  = mod;
        data->bytesperpix   = bytesperpixel;

        D(bug("[LinuxBM] Created bitmap %ldx%ld\n", width, height));
        D(bug("[LinuxBM] Buffer at 0x%p, %ld bytes per row\n", data->VideoData, mod));

        /*
         * We rely on the fact that bitmaps with aHidd_BitMap_Displayable set to TRUE always
         * also get aHidd_BitMap_ModeID with valid value. Currently this seems to be true and
         * i beleive it should stay so.
         */
        if (modeid != vHidd_ModeID_Invalid)
        {
            IPTR dwidth  = 0;
            IPTR dheight = 0;
            OOP_Object *gfx = (OOP_Object *)GetTagData(aHidd_BitMap_GfxHidd, 0, msg->attrList);
            OOP_Object *sync;
            struct FBDevInfo * fbdevinfo = NULL;

            D(bug("[LinuxBM] Display driver object: 0x%p\n", gfx));

            HIDD_Gfx_GetMode(gfx, modeid, &sync, (OOP_Object **)&data->pixfmt);
            OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
            OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);

            data->display_width  = dwidth;
            data->display_height = dheight;
            D(bug("[LinuxBM] Display size: %dx%d\n", dwidth, dheight));

            fbdevinfo = (struct FBDevInfo *)GetTagData(aHidd_LinuxFBBitmap_FBDevInfo,
                    (IPTR)NULL, msg->attrList);

            if (fbdevinfo != NULL)
            {
                data->RealVideoData = fbdevinfo->baseaddr;
                data->realbytesperline = fbdevinfo->pitch;
            }

            D(if (data->RealVideoData) bug("[LinuxBM] RealVideoData: 0x%p\n",data->RealVideoData));
        }
    }

    return o;
}

/**********  Bitmap::UpdateRect()  ***********************************/
VOID LinuxBM__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    /* NOTE: There is a strong assumption here that bitmap dimensions equal framebuffer dimensions */

    if (data->RealVideoData)
    {
        ULONG x = msg->x * data->bytesperpix;
        APTR src = data->VideoData     + msg->y * data->bytesperline     + x;
        APTR dst = data->RealVideoData + msg->y * data->realbytesperline + x;
        LONG y;
        OOP_Object * gfx = NULL;

        D(bug("[LinuxBM] 0x%p -> UpdateRect(%d, %d, %d, %d)\n", o, msg->x, msg->y, msg->width, msg->height));

        for (y = 0; y < msg->height; y++)
        {
            CopyMem(src,dst, msg->width * data->bytesperpix);

            src += data->bytesperline;
            dst += data->realbytesperline;
        }

        /* Notify the gfx hidd */
        OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfx);

        if (gfx)
        {
            struct pHidd_LinuxFB_FBChanged fbcmsg =
            {
               mID      : OOP_GetMethodID(IID_Hidd_LinuxFB, moHidd_LinuxFB_FBChanged),
               x        : msg->x,
               y        : msg->y,
               width    : msg->width,
               height   : msg->height,
               src      : data->VideoData,
               srcpitch : data->bytesperline
            };

            OOP_DoMethod(gfx, (OOP_Msg)&fbcmsg);
        }
    }
}

/* TODO: The following needs to be moved to ChunkyBM class */
#ifdef MOVE_THIS_TO_CHUNKYBM

/*********  BitMap::ObtainDirectAccess()  *************************************/
BOOL LinuxBM__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    ULONG width, height;
    
    /* Get width & height from bitmap object */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
    *msg->addressReturn    = LSD(cl)->baseaddr;
    *msg->widthReturn    = LSD(cl)->vsi.xres_virtual;
    *msg->heightReturn    = LSD(cl)->vsi.yres_virtual;
    *msg->bankSizeReturn = *msg->memSizeReturn = LSD(cl)->fsi.smem_len;
    
    return TRUE;
}

VOID LinuxBM__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
     /* Do nothing */
     return;
}

#endif
