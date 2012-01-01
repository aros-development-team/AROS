/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: Quartz bitmap class.
    Lang: English.
*/

#define DEBUG 0
#define DNEW(x) x
#define DUPD(x)

#include <aros/debug.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "classbase.h"
#include "bitmapclass.h"

/****************************************************************************************/

OOP_Object *QBitmap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct UIKitBase *base = cl->UserData;

    DNEW(bug("QBitmap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	struct bitmap_data *data = OOP_INST_DATA(cl, o);
	OOP_Object *gfx	= NULL;
	HIDDT_ModeID modeid = vHidd_ModeID_Invalid;

	/* For faster operation we cache some data about ourselves */
	data->width  = OOP_GET(o, aHidd_BitMap_Width);
	data->height = OOP_GET(o, aHidd_BitMap_Height);
	data->mod    = OOP_GET(o, aHidd_BitMap_BytesPerRow);
	OOP_GetAttr(o, aHidd_BitMap_ModeID  , &modeid);
	OOP_GetAttr(o, aHidd_BitMap_GfxHidd , (IPTR *)&gfx);
	OOP_GetAttr(o, aHidd_ChunkyBM_Buffer, (IPTR *)&data->pixels);

	/*
	 * Orientation currently depend on width:height ratio.
	 * For all existing devices this seems to be true. However it's not perfect. Perhaps we need to
	 * add Orientation attribute to sync class. Or introduce ability to subclass syncs.
	 * In fact would be good to have own support for screen rotation in AROS. This is not designed yet.
	 */
	data->orientation = (data->width > data->height) ? O_LANDSCAPE : O_PORTRAIT;

	DNEW(bug("[QBitmap] Created bitmap %d x %d\n", data->width, data->height));
	DNEW(bug("[QBitmap] Buffer at 0x%p, %d bytes per row\n", data->pixels, data->mod));
	DNEW(bug("[QBitmap] Display driver object: 0x%p\n", gfx));

	HostLib_Lock();

	base->iface->NewContext(data);
	AROS_HOST_BARRIER

	HostLib_Unlock();

	DNEW(bug("[QBitmap] Bitmap context: 0x%p\n", data->context));
	
	if (!data->context)
	{
	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

	    OOP_CoerceMethod(cl, o, &disp_mid);
	    return NULL;
	}

	if (modeid != vHidd_ModeID_Invalid)
	{
	    OOP_Object *sync    = NULL;
	    OOP_Object *pixfmt  = NULL;

	    HIDD_Gfx_GetMode(gfx, modeid, &sync, &pixfmt);
	    
	    data->win_width  = OOP_GET(sync, aHidd_Sync_HDisp);
	    data->win_height = OOP_GET(sync, aHidd_Sync_VDisp);

	    DNEW(bug("[QBitmap] Display window size: %d x %d\n", data->win_width, data->win_height));
	}
    }

    return o;
}

/****************************************************************************************/

void QBitmap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct UIKitBase *base = cl->UserData;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
 
    HostLib_Lock();
    
    base->iface->DisposeContext(data->context);
    AROS_HOST_BARRIER

    HostLib_Unlock();
    
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID QBitmap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;
    
    if (IS_BM_ATTR(msg->attrID, idx))
    {
        switch (idx)
	{
	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = data->left;
	    return;

	case aoHidd_BitMap_TopEdge:
	    *msg->storage = data->top;
	    return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID QBitmap__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, obj);
    struct TagItem  *tag, *tstate;
    ULONG idx;
    BOOL change_position = FALSE;
    BOOL show            = FALSE;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if (IS_BM_ATTR(tag->ti_Tag, idx))
	{
	    switch(idx)
	    {
	    case aoHidd_BitMap_LeftEdge:
	        data->left = tag->ti_Data;
		change_position = TRUE;
		break;

	    case aoHidd_BitMap_TopEdge:
	        data->top = tag->ti_Data;
		change_position = TRUE;
		break;

	    case aoHidd_BitMap_Visible:
//	    	data->visible = tag->ti_Data;
	    	show = tag->ti_Data;
	    	break;
	    }
	}
    }

    if (change_position)
    {
	/* Fix up position. We can completely scroll out
	   of our window into all 4 sides, but not more */
	if (data->left > data->win_width)
	    data->left = data->win_width;
	else if (data->left < -data->width)
	    data->left = -data->width;
	if (data->top > data->win_height)
	    data->top = data->win_height;
	else if (data->top < -data->height)
	    data->top = -data->height;

	/* TODO */
    }

    if (show)
    {
	/* TODO */
    }

    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID QBitmap__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    /* TODO */
}
