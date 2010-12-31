/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class.
    Lang: English.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "agfx.h"
#include "agfx_bitmap.h"

/****************************************************************************************/

OOP_Object *ABitmap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    ULONG   	 width, height;
    HIDDT_ModeID modeid;
    IPTR	 win_width  = 0;
    IPTR	 win_height = 0;
    struct bitmap_data *data;

    EnterFunc(bug("ABitmap::New()\n"));
	
    width  = GetTagData(aHidd_BitMap_Width , 0                   , msg->attrList);
    height = GetTagData(aHidd_BitMap_Height, 0                   , msg->attrList);
    modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);

    D(bug("[ABitmap] Creating Android bitmap: %ldx%ld, mode 0x%08x\n", width, height, modeid));

    /*
     * This relies on the fact that bitmaps with aHidd_BitMap_Displayable set to TRUE always
     * also get aHidd_BitMap_ModeID with valid value. Currently this seems to be true and i
     * beleive it should stay so
     */
    if (modeid != vHidd_ModeID_Invalid)
    {
	OOP_Object *gfx = (OOP_Object *)GetTagData(aHidd_BitMap_GfxHidd, 0, msg->attrList);
	OOP_Object *sync, *pixfmt;

	D(bug("[ABitmap] Display driver object: 0x%p\n", gfx));

	HIDD_Gfx_GetMode(gfx, modeid, &sync, &pixfmt);
	OOP_GetAttr(sync, aHidd_Sync_HDisp, &win_width);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &win_height);

	D(bug("[ABitmap] Display window size: %dx%d\n", win_width, win_height));
    }

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[ABitmap] Object created by superclass: 0x%p\n", o));
    if (o)
    {
        data = OOP_INST_DATA(cl, o);

	/* Get some info passed to us by the gdigfxhidd class */
	data->win_width  = win_width;
	data->win_height = win_height;
	data->bm_width	 = width;
	data->bm_height	 = height;
	data->bm_left	 = 0;
	data->bm_top	 = 0;

    	ReturnPtr("ABitmapGfx.BitMap::New()", OOP_Object *, o);
    } /* if (object allocated by superclass) */

dispose_bitmap:
    
    ReturnPtr("ABitmapGfx.BitMap::New()", OOP_Object *, NULL);
    
}

/****************************************************************************************/

VOID ABitmap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("ABitmapGfx.BitMap::Dispose()\n"));
    
    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("ABitmapGfx.BitMap::Dispose");
}


/****************************************************************************************/

VOID ABitmap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG   	    	idx;
    
    if (IS_BM_ATTR(msg->attrID, idx))
    {
        switch (idx)
	{
	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = data->bm_left;
	    return;

	case aoHidd_BitMap_TopEdge:
	    *msg->storage = data->bm_top;
	    return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID ABitmap__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, obj);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    BOOL	    change_position = FALSE;

    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if (IS_BM_ATTR(tag->ti_Tag, idx))
	{
	    switch(idx)
	    {
	    case aoHidd_BitMap_LeftEdge:
	        data->bm_left = tag->ti_Data;
		change_position = TRUE;
		break;

	    case aoHidd_BitMap_TopEdge:
	        data->bm_top = tag->ti_Data;
		change_position = TRUE;
		break;
	    }
	}
    }

    if (change_position)
    {
	/* Fix up position. We can completely scroll out
	   of our window into all 4 sides, but not more */
	if (data->bm_left > data->win_width)
	    data->bm_left = data->win_width;
	else if (data->bm_left < -data->bm_width)
	    data->bm_left = -data->bm_width;
	if (data->bm_top > data->win_height)
	    data->bm_top = data->win_height;
	else if (data->bm_top < -data->bm_height)
	    data->bm_top = -data->bm_height;

	/* TODO */
    }

    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID ABitmap__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    /* TODO */
}

/****************************************************************************************/

HIDDT_Pixel ABitmap__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	pixel;

    /* TODO */
    pixel = 0;

    return pixel;
}
