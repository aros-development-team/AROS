/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for SM502 hidd.
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
#include <aros/debug.h>

#include <string.h>

#include "bitmap.h"
#include "sm502gfxclass.h"

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) SM502BM__Root__ ## x
#define MNAME_BM(x) SM502BM__Hidd_BitMap__ ## x

/*********** BitMap::New() *************************************/
OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("SM502Gfx.BitMap::New()\n"));
    
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

	D(bug("[SM502BitMap] Bitmap %ld x % ld, %u bytes per pixel, %u bytes per line\n",
	      data->width, data->height, data->bytesperpix, data->bytesperline));
	D(bug("[SM502BitMap] Video data at 0x%p (%u bytes)\n", data->VideoData, data->bytesperline * data->height));

	if (OOP_GET(data->pixfmtobj, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_Palette)
	    ReturnPtr("SM502Gfx.BitMap::New()", OOP_Object *, o);

	data->DAC = AllocMem(768, MEMF_ANY);
	D(bug("[SM502BitMap] Palette data at 0x%p\n", data->DAC));
	if (data->DAC)
	    ReturnPtr("SM502Gfx.BitMap::New()", OOP_Object *, o);

	disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

	OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	o = NULL;
    } /* if created object */

    ReturnPtr("SM502Gfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/
VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug("[SM502BitMap] Dispose(0x%p)\n", o));

    if (data->DAC)
	FreeMem(data->DAC, 768);

    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("SM502Gfx.BitMap::Dispose");
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
		D(bug("[SM502BitMap] Setting Visible to %d\n", tag->ti_Data));
		data->disp = tag->ti_Data;
		if (data->disp) {
		    if (data->DAC)
			DACLoad(XSD(cl), data->DAC, 0, 256);
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
	D(bug("[SM502BitMap] Scroll to (%d, %d)\n", xoffset, yoffset));
	data->xoffset = xoffset;
	data->yoffset = yoffset;

	if (data->disp)
	{
	    LOCK_FRAMEBUFFER(XSD(cl));
	    sm502DoRefreshArea(&XSD(cl)->data, data, 0, 0, data->width, data->height);
	    UNLOCK_FRAMEBUFFER(XSD(cl));
	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::SetColors() *************************************/

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct SM502_HWData *hwdata = &XSD(cl)->data;
    ULONG xc_i, col_i;
    UBYTE p_shift;
    UWORD red, green, blue;

    D(bug("[SM502BitMap] SetColors(%u, %u)\n", msg->firstColor, msg->numColors));

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) {
	D(bug("[SM502BitMap] DoSuperMethod() failed\n"));
	return FALSE;
    }

    if ((msg->firstColor + msg->numColors) > (1 << data->bpp))
	return FALSE;

    if (data->DAC) {
	for ( xc_i = msg->firstColor, col_i = 0;
    	      col_i < msg->numColors; 
	      xc_i ++, col_i ++) {
	    red   = msg->colors[col_i].red   >> 8;
	    green = msg->colors[col_i].green >> 8;
	    blue  = msg->colors[col_i].blue  >> 8;

	    /* Update DAC registers */
	    p_shift = 8 - hwdata->palettewidth;
	    data->DAC[xc_i*3] = red >> p_shift;
	    data->DAC[xc_i*3+1] = green >> p_shift;
	    data->DAC[xc_i*3+2] = blue >> p_shift;
	}

	/* Upload palette to the DAC if the current bitmap is on display */
	if (data->disp)
	    DACLoad(XSD(cl), data->DAC, msg->firstColor, msg->numColors);
    }
    return TRUE;
}

VOID MNAME_BM(UpdateRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug("[SM502BitMap] UpdateRect(%d, %d, %d, %d), bitmap 0x%p\n", msg->x, msg->y, msg->width, msg->height, o));
    if (data->disp) {
	LOCK_FRAMEBUFFER(XSD(cl));
        sm502DoRefreshArea(&XSD(cl)->data, data, msg->x, msg->y, msg->x + msg->width, msg->y + msg->height);
	UNLOCK_FRAMEBUFFER(XSD(cl));
    }
}
