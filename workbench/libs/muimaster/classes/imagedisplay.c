/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"

extern struct Library *MUIMasterBase;

struct MUI_ImagedisplayData
{
    char *spec;
    struct MUI_ImageSpec_intern *img;
    ULONG flags;
    WORD defwidth;
    WORD defheight;
};

#define MIF_FREEVERT         (1<<0)
#define MIF_FREEHORIZ        (1<<1)


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Imagedisplay_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImagedisplayData   *data;
    struct TagItem  	    *tag, *tags;
    
    D(bug("Imagedisplay_New starts\n"));

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->flags = MIF_FREEHORIZ | MIF_FREEVERT;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Imagedisplay_Spec:
		data->spec = zune_image_spec_duplicate(tag->ti_Data);
		break;
	    case MUIA_Imagedisplay_FreeHoriz:
		/* MUI implements some tag for optionnally prevent rescaling
		 * of displayed image - without affecting imagedisplay resize -
		 * see MUIPrefs/Buttons/Checkmarks/Look for nonrescaled image,
		 * and try a popimage for yourself to see that by default they
		 * get rescaled. It's not the same effect as MUI_Image_FreeHoriz.
		 * -dlc 20030323
		 */
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEHORIZ);
		break;
	    case MUIA_Imagedisplay_FreeVert:
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEVERT);
		break;
    	}
    }

    if (!data->spec)
    {
	data->spec = StrDup("0:128");
    }

    if (!data->spec)
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
    	return NULL;
    }
    
    D(bug("Imagedisplay_New(%lx) spec=%lx\n", obj, data->img));
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Imagedisplay_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);

    zune_image_spec_free(data->spec);
    DoSuperMethodA(cl,obj,(Msg)msg);
    return 0;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Imagedisplay_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Imagedisplay_Spec:
		if (data->spec)
		    zune_image_spec_free(data->spec);
		data->spec = zune_image_spec_duplicate(tag->ti_Data);

		if (_flags(obj) & MADF_CANDRAW)
		    zune_imspec_hide(data->img);

		if (_flags(obj) & MADF_SETUP)
		{
		    zune_imspec_cleanup(data->img);
		    data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
		}

		if (_flags(obj)&MADF_CANDRAW)
		    zune_imspec_show(data->img, obj);

		MUI_Redraw(obj,MADF_DRAWOBJECT);
		break;
	}
    }
    
    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Imagedisplay_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
	case    MUIA_Imagedisplay_Spec:
		*msg->opg_Storage = (ULONG)data->spec;
	        break;
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Imagedisplay_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg))
	return NULL;

    if (data->spec)
	data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Imagedisplay_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);

    if (data->spec)
	zune_imspec_cleanup(data->img);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Imagedisplay_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (data->img)
    {
	struct MUI_MinMax minmax;

	zune_imspec_askminmax(data->img, &minmax);

	msg->MinMaxInfo->MinWidth += minmax.MinWidth;
	msg->MinMaxInfo->MinHeight += minmax.MinHeight;
   
	msg->MinMaxInfo->DefWidth += minmax.DefWidth;
	msg->MinMaxInfo->DefHeight += minmax.DefHeight;

	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    return 1;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR Imagedisplay_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (data->img)
	zune_imspec_show(data->img, obj);
    return 1;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Imagedisplay_Hide(struct IClass *cl, Object *obj,struct MUIP_Hide *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);

    if (data->img)
	zune_imspec_hide(data->img);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Imagedisplay_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct MUI_ImagedisplayData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;

    if (data->img)
    {
	WORD left, top, width, height;

	left = _mleft(obj);
	top = _mtop(obj);
	width = _mwidth(obj);
	height = _mheight(obj);

	if (!(data->flags & MIF_FREEVERT) || !(data->flags & MIF_FREEHORIZ))
	{
	    struct MUI_MinMax minmax;

	    zune_imspec_askminmax(data->img, &minmax);
	    data->defwidth = minmax.DefWidth;
	    data->defheight = minmax.DefHeight;

	    if (!(data->flags & MIF_FREEVERT) && (height > data->defheight))
	    {
		WORD freespace = height - data->defheight;

		top += freespace / 2;
		height = data->defheight;
	    }

	    if (!(data->flags & MIF_FREEHORIZ) && (width > data->defwidth))
	    {
		WORD freespace = width - data->defwidth;

		left += freespace / 2;
		width = data->defwidth;
	    }
	}

	zune_imspec_draw(data->img, muiRenderInfo(obj),
			 left, top, width, height,
			 0, 0, IDS_NORMAL);
    }

    return 1;
}


BOOPSI_DISPATCHER(IPTR, Imagedisplay_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Imagedisplay_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Imagedisplay_Dispose(cl, obj, msg);
	case OM_SET: return Imagedisplay_Set(cl, obj, (APTR)msg);
	case OM_GET: return Imagedisplay_Get(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Imagedisplay_AskMinMax(cl,obj,(APTR)msg);
	case MUIM_Setup: return Imagedisplay_Setup(cl,obj,(APTR)msg);
	case MUIM_Cleanup: return Imagedisplay_Cleanup(cl,obj,(APTR)msg);
	case MUIM_Show: return Imagedisplay_Show(cl,obj,(APTR)msg);
	case MUIM_Hide: return Imagedisplay_Hide(cl,obj,(APTR)msg);
	case MUIM_Draw: return Imagedisplay_Draw(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Imagedisplay_desc = { 
    MUIC_Imagedisplay, 
    MUIC_Area, 
    sizeof(struct MUI_ImagedisplayData), 
    (void*)Imagedisplay_Dispatcher 
};

