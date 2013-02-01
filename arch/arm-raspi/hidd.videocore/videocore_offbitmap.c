/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for VideoCore hidd.
    Lang: English.
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/alerts.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <aros/symbolsets.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

#include "videocore_bitmap.h"
#include "videocore_class.h"

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) VideoCoreOffBM__Root__ ## x
#define MNAME_BM(x) VideoCoreOffBM__Hidd_BitMap__ ## x

#include "videocore_bitmap_common.c"

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VideoCore.BitMap::New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct BitmapData *data = OOP_INST_DATA(cl, o);
        LONG multi=1;
        IPTR width, height, depth;
        OOP_Object *friend, *pf;
        
        /* clear all data  */
        memset(data, 0, sizeof(struct BitmapData));

        /* Get attr values */
        OOP_GetAttr(o,  aHidd_BitMap_Width,     &width);
        OOP_GetAttr(o,  aHidd_BitMap_Height, 	&height);
        OOP_GetAttr(o,  aHidd_BitMap_PixFmt,	(IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth,     &depth);

        /* Get the friend bitmap. This should be a displayable bitmap */
        OOP_GetAttr(o,  aHidd_BitMap_Friend,	(IPTR *)&friend);

        /* If you got a friend bitmap, copy its colormap */
        if (friend)
        {
            struct BitmapData *src = OOP_INST_DATA(cl, friend);
            CopyMem(&src->cmap, &data->cmap, 4*16);
        }

        ASSERT (width != 0 && height != 0 && depth != 0);

        width = (width + 15) & ~15;
        data->width = width;
        data->height = height;
        data->bpp = depth;
        data->disp = 0;
        if (depth > 16)
            multi = 4;
        else if (depth > 8)
            multi = 2;
        data->bytesperpix = multi;
#warning "TODO: Allocate using GPU memory.. "
        data->VideoData = AllocVec(width*height*multi, MEMF_PUBLIC | MEMF_CLEAR);

        if (data->VideoData)
        {
            data->data = &XSD(cl)->data;
            if (XSD(cl)->activecallback)
                XSD(cl)->activecallback(XSD(cl)->callbackdata, o, TRUE);
            ReturnPtr("VideoCore.BitMap::New()", OOP_Object *, o);
        } /* data->VideoData */
        {
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
        }
        o = NULL;
    } /* o */
    ReturnPtr("VideoCore.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("VideoCore.BitMap::Dispose()\n"));
    if (data->VideoData)
        FreeVec(data->VideoData);
    OOP_DoSuperMethod(cl, o, msg);
    ReturnVoid("VideoCore.BitMap::Dispose");
}
