/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
#include "compositing.h"

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
	OOP_MethodID	     disp_mid;
	struct BitmapData   *data;
	IPTR 	    	     width, height, depth, multi;
	IPTR		     displayable;
	HIDDT_ModeID 	     modeid;

	data = OOP_INST_DATA(cl, o);

	/* clear all data  */
	memset(data, 0, sizeof(struct BitmapData));

	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width, &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&data->gfxhidd);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&data->pixfmtobj);
	OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
	OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_Depth, &depth);
	OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_BytesPerPixel, &multi);
	
        data->compositing = (OOP_Object *)
            GetTagData(aHidd_VesaGfxBitMap_CompositingHidd, 0, msg->attrList);

	ASSERT (width != 0 && height != 0 && depth != 0);
	/* 
	   We must only create depths that are supported by the friend drawable
	   Currently we only support the default depth
	   */

	width=(width+15) & ~15;
	data->width = width;
	data->height = height;
	data->bpp = depth;

	data->bytesperpix = multi;
	data->bytesperline = width * multi;
	D(bug("[VesaBitMap] Size %dx%d, %u bytes per pixel, %u bytes per line, displayable: %u\n", width, height, multi, data->bytesperline, displayable));

	OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
	if (modeid != vHidd_ModeID_Invalid) {
	    OOP_Object *sync, *pf;
	    IPTR dwidth, dheight;

	    HIDD_Gfx_GetMode(data->gfxhidd, modeid, &sync, &pf);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
	    data->disp_width  = dwidth;
	    data->disp_height = dheight;
	}

    	data->VideoData = AllocVec(data->bytesperline * height, MEMF_PUBLIC | MEMF_CLEAR);
	D(bug("[VesaBitMap] Video data at 0x%p (%u bytes)\n", data->VideoData, width * height * multi));

	if (data->VideoData) {
	    HIDDT_ColorModel cmod;
	    struct TagItem tags[2];

            tags[0].ti_Tag = aHidd_ChunkyBM_Buffer;
            tags[0].ti_Data = (IPTR)data->VideoData;
            tags[1].ti_Tag = TAG_END;
	    OOP_SetAttrs(o, tags);

	    if (!displayable)
	        ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);

	    OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_ColorModel, &cmod);
	    if (cmod != vHidd_ColorModel_Palette)
		ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);

	    data->DAC = AllocMem(768, MEMF_ANY);
	    D(bug("[VesaBitMap] Palette data at 0x%p\n", data->DAC));
	    if (data->DAC)
		ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
	}

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
    FreeVec(data->VideoData);

    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("VesaGfx.BitMap::Dispose");
}

/*********  BitMap::PutPixel()  ***************************/
// FIXME: in theory we shouldn't need this method since the superclass implements it

VOID MNAME_BM(PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return;
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG   	       idx;

    if (IS_VesaGfxBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_VesaGfxBitMap_Drawable:
		*msg->storage = (IPTR)data->VideoData;
		return;
	}
    }
    else if (IS_BM_ATTR(msg->attrID, idx))
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

    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
		D(bug("[VesaBitMap] Setting Visible to %d\n", tag->ti_Data));
		data->disp = tag->ti_Data;
		if (data->disp)
                {
		    if (data->DAC)
			DACLoad(XSD(cl), data->DAC, 0, 256);
		}
		break;
	    case aoHidd_BitMap_LeftEdge:
	        xoffset = tag->ti_Data;
		break;
	    case aoHidd_BitMap_TopEdge:
	        yoffset = tag->ti_Data;
		break;
	    }
	}
    }

    if ((xoffset != data->xoffset) || (yoffset != data->yoffset))
    {
        /* If there was a change requested, validate it */
        struct pHidd_Compositing_ValidateBitMapPositionChange vbpcmsg =
        {
            mID : XSD(cl)->mid_ValidateBitMapPositionChange,
            bm : o,
            newxoffset : &xoffset,
            newyoffset : &yoffset
        };

        OOP_DoMethod(data->compositing, (OOP_Msg)&vbpcmsg);

        if ((xoffset != data->xoffset) || (yoffset != data->yoffset))
        {
            /* If change passed validation, execute it */
            struct pHidd_Compositing_BitMapPositionChanged bpcmsg =
            {
                mID : XSD(cl)->mid_BitMapPositionChanged,
                bm : o
            };

            data->xoffset = xoffset;
            data->yoffset = yoffset;

            OOP_DoMethod(data->compositing, (OOP_Msg)&bpcmsg);
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
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

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
    {
	D(bug("[VesaBitMap] DoSuperMethod() failed\n"));
	return FALSE;
    }

    if ((msg->firstColor + msg->numColors) > (1 << data->bpp))
	return FALSE;

    if (data->DAC)
    {
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

    D(bug("[VesaBitMap] UpdateRect(%d, %d, %d, %d), bitmap 0x%p\n",
        msg->x, msg->y, msg->width, msg->height, o));
    struct pHidd_Compositing_BitMapRectChanged compmsg =
    {
        mID : XSD(cl)->mid_BitMapRectChanged,
        bm : o,
        x : msg->x,
        y : msg->y,
        width : msg->width,
        height : msg->height,
    };

    OOP_DoMethod(data->compositing, (OOP_Msg)&compmsg);
}
