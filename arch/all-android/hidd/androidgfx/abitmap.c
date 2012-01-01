/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class.
    Lang: English.
*/

#define DEBUG 0
#define DNEW(x)
#define DUPD(x)

#include <sys/types.h>
#include <android/configuration.h>

#include <aros/debug.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "agfx.h"
#include "agfx_bitmap.h"
#include "server.h"

/****************************************************************************************/

OOP_Object *ABitmap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    DNEW(bug("ABitmap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	struct bitmap_data *data = OOP_INST_DATA(cl, o);

	OOP_Object *gfx	    = NULL;
	OOP_Object *sync    = NULL;
	OOP_Object *pixfmt  = NULL;
	HIDDT_ModeID modeid = vHidd_ModeID_Invalid;
    	IPTR width 	    = 0;
    	IPTR height	    = 0;
    	IPTR mod	    = 0;

	/* For faster operation we cache some data about ourselves */
	OOP_GetAttr(o, aHidd_BitMap_Width , &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &mod);
	OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
	OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfx);
	OOP_GetAttr(o, aHidd_ChunkyBM_Buffer, &data->pixels);

	data->bm_width	  = width;
	data->bm_height	  = height;
	data->mod         = mod;
	/*
	 * Orientation currently depend on width:height ratio.
	 * For all existing devices this seems to be true. However it's not perfect. Perhaps we need to
	 * add Orientation attribute to sync class. Or introduce ability to subclass syncs.
	 * In fact would be good to have own support for screen rotation in AROS. This is not designed yet.
	 */
	data->orientation = (width > height) ? ACONFIGURATION_ORIENTATION_LAND : ACONFIGURATION_ORIENTATION_PORT;

	DNEW(bug("[ABitmap] Created bitmap %ldx%ld\n", width, height));
	DNEW(bug("[ABitmap] Buffer at 0x%p, %ld bytes per row\n", data->pixels, mod));
	DNEW(bug("[ABitmap] Display driver object: 0x%p\n", gfx));

	HIDD_Gfx_GetMode(gfx, modeid, &sync, &pixfmt);
	OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

	data->win_width  = width;
	data->win_height = height;
	DNEW(bug("[ABitmap] Display window size: %dx%d\n", win_width, win_height));
    }

    return o;
}

/****************************************************************************************/

VOID ABitmap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;
    
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
    ULONG idx;
#ifdef ENABLE_SCROLL
    BOOL change_position = FALSE;
#endif
    BOOL show            = FALSE;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if (IS_BM_ATTR(tag->ti_Tag, idx))
	{
	    switch(idx)
	    {
#ifdef ENABLE_SCROLL
	    case aoHidd_BitMap_LeftEdge:
	        data->bm_left = tag->ti_Data;
		change_position = TRUE;
		break;

	    case aoHidd_BitMap_TopEdge:
	        data->bm_top = tag->ti_Data;
		change_position = TRUE;
		break;
#endif

	    case aoHidd_BitMap_Visible:
	    	data->visible = tag->ti_Data;
	    	show = tag->ti_Data;
	    	break;
	    }
	}
    }

#ifdef ENABLE_SCROLL
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
#endif

    if (show)
    {
	struct ShowRequest show;

	show.req.cmd	 = cmd_Show;
	show.req.len	 = 8;
	show.displayid	 = 0;
	show.left	 = data->bm_left;
	show.top	 = data->bm_top;
	show.width	 = data->bm_width;
	show.height	 = data->bm_height;
	show.mod	 = data->mod;
	show.orientation = data->orientation;
	show.addr	 = data->pixels;

	DoRequest(&show.req, XSD(cl));
    }

    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID ABitmap__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    if (data->visible)
    {
    	struct UpdateRequest update;

	DUPD(bug("[ABitmap 0x%p] UpdateRect(%d, %d, %d, %d)\n", o, msg->x, msg->y, msg->width, msg->height));

    	update.req.cmd = cmd_Update;
    	update.req.len = 5;
    	update.id      = 0;
    	update.left    = msg->x;
    	update.top     = msg->y;
    	update.width   = msg->width;
    	update.height  = msg->height;

    	SendRequest(&update.req, XSD(cl));
    }
}
