/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for VideoCore hidd.
    Lang: English.
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/vcmbox.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

#include "videocore_bitmap.h"
#include "videocore_class.h"

#include LC_LIBDEFS_FILE

#ifdef VCMBoxBase
#undef VCMBoxBase
#endif

#define VCMBoxBase      (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxBase

#define MNAME_ROOT(x) VideoCoreOnBM__Root__ ## x
#define MNAME_BM(x) VideoCoreOnBM__Hidd_BitMap__ ## x

#define OnBitmap 1
#include "videocore_bitmap_common.c"

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VideoCore.BitMap::New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct BitmapData *data;
        OOP_Object *pf;
        IPTR width, height, depth;
        HIDDT_ModeID modeid;

        /* clear all data  */
        data = OOP_INST_DATA(cl, o);
        memset(data, 0, sizeof(struct BitmapData));

        /* Get attr values */
        OOP_GetAttr(o, aHidd_BitMap_Width, &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        
        ASSERT (width != 0 && height != 0 && depth != 0);
        /* 
            We must only create depths that are supported by the friend drawable
            Currently we only support the default depth
        */

        width=(width+15) & ~15;
        data->width = width;
        data->height = height;
        data->bpp = depth;
        data->disp = -1;

        data->bytesperpix = 1;
        if (depth>16)
            data->bytesperpix = 4;
        else if (depth>8)
            data->bytesperpix = 2;

        data->data = &XSD(cl)->data;
        data->mouse = &XSD(cl)->mouse;

        RawPutChar(0x03);

        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[1] = VCTAG_REQ;

        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[2] = VCTAG_SETRES;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[3] = 8;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[4] = 8;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[5] = data->width;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[6] = data->height;

        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[7] = VCTAG_SETVRES;          // duplicate physical size...
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[8] = 8;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[9] = 8;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[10] = data->width;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[11] = data->height;

        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[12] = VCTAG_SETDEPTH;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[13] = 4;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[14] = 4;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[15] = data->bpp;

        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[16] = VCTAG_FBALLOC;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[17] = 8;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[18] = 4;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[19] = 16;
        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[20] = 0;

        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[21] = 0;                      // terminate tags

        (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[0] = (22 << 2);                 // fill in request size

        VCMBoxWrite(VCMB_BASE, VCMB_FBCHAN, (unsigned int)(&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage);
        if ((VCMBoxRead(VCMB_BASE, VCMB_FBCHAN) == (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage)
            && ((&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[1] == VCTAG_RESP)
            && ((&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[18] == (VCTAG_RESP + 8)))
        {
            data->VideoData = (&((struct VideoCoreBase *)cl->UserData)->vsd)->vcsd_VCMBoxMessage[19];
       
            /* We should be able to get modeID from the bitmap */
            OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
            if (modeid != vHidd_ModeID_Invalid)
            {
                /*
                    Because of not defined BitMap_Show method show 
                    bitmap immediately
                */
    //            setModeVideoCore(&XSD(cl)->data, width, height);
                XSD(cl)->visible = data;	/* Set created object as visible */
                ReturnPtr("VideoCore.BitMap::New()", OOP_Object *, o);
            }
        }

        {
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
        }
        o = NULL;
    } /* if created object */
    ReturnPtr("VideoCore.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("VideoCore.BitMap::Dispose()\n")); 
    OOP_DoSuperMethod(cl, o, msg);
    ReturnVoid("VideoCore.BitMap::Dispose");
}
