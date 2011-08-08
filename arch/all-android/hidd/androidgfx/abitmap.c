/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class.
    Lang: English.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "agfx.h"
#include "agfx_bitmap.h"

/****************************************************************************************/

OOP_Object *ABitmap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("ABitmap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	struct bitmap_data *data = OOP_INST_DATA(cl, o);
    	BOOL onscreen = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
	HIDDT_ModeID modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    	OOP_MethodID disp_mid;
    	IPTR width  = 0;
    	IPTR height = 0;
    	IPTR mod    = 0;

	/*
     	 * We rely on the fact that bitmaps with aHidd_BitMap_Displayable set to TRUE always
     	 * also get aHidd_BitMap_ModeID with valid value. Currently this seems to be true and
     	 * i beleive it should stay so.
     	 */
    	if (modeid != vHidd_ModeID_Invalid)
    	{
	    IPTR win_width  = 0;
	    IPTR win_height = 0;
	    OOP_Object *gfx = (OOP_Object *)GetTagData(aHidd_BitMap_GfxHidd, 0, msg->attrList);
	    OOP_Object *sync, *pixfmt;

	    D(bug("[ABitmap] Display driver object: 0x%p\n", gfx));

	    HIDD_Gfx_GetMode(gfx, modeid, &sync, &pixfmt);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &win_width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &win_height);

	    D(bug("[ABitmap] Display window size: %dx%d\n", win_width, win_height));

	    data->win_width  = win_width;
	    data->win_height = win_height;
    	}

	/* If this is offscreen bitmap, we are done. */
    	if (!onscreen)
	    ReturnPtr("ABitmap::New()", OOP_Object *, o);

	OOP_GetAttr(o, aHidd_BitMap_Width , &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &mod);
	OOP_GetAttr(o, aHidd_ChunkyBM_Buffer, (IPTR *)&data->pixels);

	D(bug("[ABitmap] Displayable Android bitmap %ldx%ld\n", width, height));
	D(bug("[ABitmap] Buffer at 0x%p, %ld bytes per row\n", data->pixels, mod));

	/* Displayable pixel buffer is Java ByteBuffer object. */
	HostLib_Lock();
	data->buffer = JNI_NewDirectByteBuffer(data->pixels, mod * height);
	HostLib_Unlock();

	D(bug("[ABitmap] ByteBuffer 0x%p\n", data->buffer));

	if (data->buffer)
	{
	    data->bm_width  = width;
	    data->bm_height = height;
	    data->bm_mod    = mod;

	    ReturnPtr("ABitmap::New()", OOP_Object *, o);
	}

	disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, &disp_mid);

    }
    ReturnPtr("ABitmap::New()", OOP_Object *, NULL);
    
}

/****************************************************************************************/

VOID ABitmap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("ABitmapGfx.BitMap::Dispose()\n"));

    /* Free pixel buffer */
    if (data->buffer)
    {
	HostLib_Lock();
	JNI_DeleteLocalRef(data->buffer);
	HostLib_Unlock();
    }

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

VOID ABitmap__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    D(bug("[ABitmap 0x%p] UpdateRect(%d, %d, %d, %d)\n", o, msg->x, msg->y, msg->width, msg->height));

    /* TODO */
}
