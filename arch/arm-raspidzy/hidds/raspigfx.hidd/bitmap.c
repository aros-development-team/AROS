/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for RasPi hidd.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <aros/symbolsets.h>
#define DEBUG 0
#include <aros/debug.h>

#include <string.h>

#include "bitmap.h"
#include "raspigfxclass.h"

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) RasPiBM__Root__ ## x
#define MNAME_BM(x) RasPiBM__Hidd_BitMap__ ## x

/*********** BitMap::New() *************************************/
OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("RasPiGfx.BitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	OOP_MethodID	   disp_mid;
	struct BitmapData *data;
	HIDDT_ModeID 	   modeid;
	OOP_Object	  *sync, *pf;

	data = OOP_INST_DATA(cl, o);

	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_GfxHidd , (APTR)&data->gfxhidd);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt  , (APTR)&data->pixfmtobj);
	OOP_GetAttr(o, aHidd_BitMap_ModeID  , &modeid);
	OOP_GetAttr(o, aHidd_ChunkyBM_Buffer, (IPTR *)&data->VideoData);

	HIDD_Gfx_GetMode(data->gfxhidd, modeid, &sync, &pf);

	data->width        = OOP_GET(o, aHidd_BitMap_Width);
	data->height       = OOP_GET(o, aHidd_BitMap_Height);
	data->bytesperline = OOP_GET(o, aHidd_BitMap_BytesPerRow);
	data->bytesperpix  = OOP_GET(data->pixfmtobj, aHidd_PixFmt_BytesPerPixel);
	data->disp_width   = OOP_GET(sync, aHidd_Sync_HDisp);
	data->disp_height  = OOP_GET(sync, aHidd_Sync_VDisp);

	D(bug("[RasPiBitMap] Bitmap %ld x % ld, %u bytes per pixel, %u bytes per line\n",
	      data->width, data->height, data->bytesperpix, data->bytesperline));
	D(bug("[RasPiBitMap] Video data at 0x%p (%u bytes)\n", data->VideoData, data->bytesperline * data->height));

	if (OOP_GET(data->pixfmtobj, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_Palette)
	    ReturnPtr("RasPiGfx.BitMap::New()", OOP_Object *, o);

	data->DAC = AllocMem(768, MEMF_ANY);
	D(bug("[RasPiBitMap] Palette data at 0x%p\n", data->DAC));
	if (data->DAC)
	    ReturnPtr("RasPiGfx.BitMap::New()", OOP_Object *, o);

	disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

	OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	o = NULL;
    } /* if created object */

    ReturnPtr("RasPiGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/
VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug("[RasPiBitMap] Dispose(0x%p)\n", o));

    if (data->DAC)
	FreeMem(data->DAC, 768);

    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("RasPiGfx.BitMap::Dispose");
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG   	       idx;

    if (IS_BM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	case aoHidd_BitMap_Visible:
	    *msg->storage = data->disp;
	    return;

	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = data->xoffset;
	    return;

	case aoHidd_BitMap_TopEdge:
	    *msg->storage = data->yoffset;
	    return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::Set() *******************************************/

VOID MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    LONG xoffset = data->xoffset;
    LONG yoffset = data->yoffset;
    LONG limit;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
		D(bug("[RasPiBitMap] Setting Visible to %d\n", tag->ti_Data));
		data->disp = tag->ti_Data;
		if (data->disp) {
		    //if (data->DAC)
			//DACLoad(XSD(cl), data->DAC, 0, 256);
		}
		break;
	    case aoHidd_BitMap_LeftEdge:
	        xoffset = tag->ti_Data;
		/* Our bitmap can not be smaller than display size
		   because of fakegfx.hidd limitations (it can't place
		   cursor beyond bitmap edges). Otherwize Intuition
		   will provide strange user experience (mouse cursor will
		   disappear) */
    		limit = data->disp_width - data->width;
    		if (xoffset > 0)
		    xoffset = 0;
		else if (xoffset < limit)
		    xoffset = limit;
		break;
	    case aoHidd_BitMap_TopEdge:
	        yoffset = tag->ti_Data;
		limit = data->disp_height - data->height;
		if (yoffset > 0)
		    yoffset = 0;
		else if (yoffset < limit)
		    yoffset = limit;
		break;
	    }
	}
    }

    if ((xoffset != data->xoffset) || (yoffset != data->yoffset))
    {
	D(bug("[RasPiBitMap] Scroll to (%d, %d)\n", xoffset, yoffset));
	data->xoffset = xoffset;
	data->yoffset = yoffset;

	if (data->disp)
	{

	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::SetColors() *************************************/

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    return TRUE;
}

VOID MNAME_BM(UpdateRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
}
