/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for Vesa hidd.
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
#include "vesagfxclass.h"

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) PCVesaBM__Root__ ## x
#define MNAME_BM(x) PCVesaBM__Hidd_BitMap__ ## x

/*********** BitMap::New() *************************************/
OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VesaGfx.BitMap::New()\n"));
    
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

	D(bug("[VesaBitMap] Bitmap %ld x % ld, %u bytes per pixel, %u bytes per line\n",
	      data->width, data->height, data->bytesperpix, data->bytesperline));
	D(bug("[VesaBitMap] Video data at 0x%p (%u bytes)\n", data->VideoData, data->bytesperline * data->height));

	if (OOP_GET(data->pixfmtobj, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_Palette)
	    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);

	data->DAC = AllocMem(768, MEMF_ANY);
	D(bug("[VesaBitMap] Palette data at 0x%p\n", data->DAC));
	if (data->DAC)
	    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);

	disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

	OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	o = NULL;
    } /* if created object */

    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/
VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug("[VesaBitMap] Dispose(0x%p)\n", o));

    if (data->DAC)
	FreeMem(data->DAC, 768);

    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("VesaGfx.BitMap::Dispose");
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
    IPTR xoffset = data->xoffset;
    IPTR yoffset = data->yoffset;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
		D(bug("[VesaBitMap] Setting Visible to %d\n", tag->ti_Data));
		data->disp = tag->ti_Data;
		if (data->disp) {
		    if (data->DAC)
			DACLoad(XSD(cl), data->DAC, 0, 256);
		}
		break;
	    }
	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    /* Bring local X/Y offsets back into sync with validated values */
    OOP_GetAttr(o, aHidd_BitMap_LeftEdge, &xoffset);
    OOP_GetAttr(o, aHidd_BitMap_TopEdge, &yoffset);

    if ((xoffset != data->xoffset) || (yoffset != data->yoffset))
    {
	D(bug("[VesaBitMap] Scroll to (%d, %d)\n", xoffset, yoffset));
	data->xoffset = xoffset;
	data->yoffset = yoffset;

	if (data->disp)
	{
	    LOCK_FRAMEBUFFER(XSD(cl));
	    vesaDoRefreshArea(&XSD(cl)->data, data, 0, 0, data->width, data->height);
	    UNLOCK_FRAMEBUFFER(XSD(cl));
	}
    }
}

/*** BitMap::SetColors() *************************************/

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct HWData *hwdata = &XSD(cl)->data;
    ULONG xc_i, col_i;
    UBYTE p_shift;
    UWORD red, green, blue;

    D(bug("[VesaBitMap] SetColors(%u, %u)\n", msg->firstColor, msg->numColors));

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) {
	D(bug("[VesaBitMap] DoSuperMethod() failed\n"));
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

    D(bug("[VesaBitMap] UpdateRect(%d, %d, %d, %d), bitmap 0x%p\n", msg->x, msg->y, msg->width, msg->height, o));
    if (data->disp) {
	LOCK_FRAMEBUFFER(XSD(cl));
        vesaDoRefreshArea(&XSD(cl)->data, data, msg->x, msg->y, msg->x + msg->width, msg->y + msg->height);
	UNLOCK_FRAMEBUFFER(XSD(cl));
    }
}
