/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "imspec.h"
#include "imagedisplay_private.h" 

extern struct Library *MUIMasterBase;

#define MIF_FREEVERT         (1<<0)
#define MIF_FREEHORIZ        (1<<1)


IPTR Imagedisplay__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Imagedisplay_DATA   *data;
    struct TagItem  	    *tag, *tags;
    
    D(bug("Imagedisplay_New starts\n"));

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->flags = MIF_FREEHORIZ | MIF_FREEVERT;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Imagedisplay_Spec:
		data->spec = zune_image_spec_duplicate(tag->ti_Data);
		break;
		
	    case MUIA_Imagedisplay_UseDefSize:
		_handle_bool_tag(data->flags, (!tag->ti_Data), (MIF_FREEHORIZ | MIF_FREEVERT));
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
    	return 0;
    }
    
    D(bug("Imagedisplay_New(%lx) spec=%lx\n", obj, data->img));
    return (IPTR)obj;
}

IPTR Imagedisplay__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    zune_image_spec_free(data->spec);
    DoSuperMethodA(cl,obj,(Msg)msg);
    return 0;
}

IPTR Imagedisplay__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
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

IPTR Imagedisplay__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
	case MUIA_Imagedisplay_Spec:
	    *msg->opg_Storage = (IPTR)data->spec;
	    return(TRUE);
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Imagedisplay__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg))
	return 0;

    if (data->spec)
	data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
    return 1;
}

IPTR Imagedisplay__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    if (data->spec)
	zune_imspec_cleanup(data->img);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Imagedisplay__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth += 3;
    msg->MinMaxInfo->MinHeight += 3;
   
    msg->MinMaxInfo->DefWidth += 16;
    msg->MinMaxInfo->DefHeight += 16;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return 1;
}

IPTR Imagedisplay__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (data->img)
	zune_imspec_show(data->img, obj);
    return 1;
}

IPTR Imagedisplay__MUIM_Hide(struct IClass *cl, Object *obj,struct MUIP_Hide *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    if (data->img)
	zune_imspec_hide(data->img);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Imagedisplay__MUIM_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;

    if (data->img)
    {
	WORD left, top, width, height;
	APTR clip;

	left = _mleft(obj);
	top = _mtop(obj);
	width = _mwidth(obj);
	height = _mheight(obj);

	/* if either horiz or vert is not rescalable, center it at def size */
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
	else
	{
	    struct MUI_MinMax minmax;

	    zune_imspec_askminmax(data->img, &minmax);

	    if (width > minmax.MaxWidth)
	    {
		width = minmax.MaxWidth;
		left = _mleft(obj) + (_mwidth(obj) - width) / 2;
	    }

	    if (height > minmax.MaxHeight)
	    {
		height = minmax.MaxHeight;
		top = _mtop(obj) + (_mheight(obj) - height) / 2;
	    }
	}

	clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
			       _mwidth(obj), _mheight(obj));
	zune_imspec_draw(data->img, muiRenderInfo(obj),
			 left, top, width, height,
			 0, 0, IDS_NORMAL);
	MUI_RemoveClipping(muiRenderInfo(obj), clip);
    }

    return 1;
}

IPTR Imagedisplay__MUIM_DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
    IPTR dummy;

    if (msg->obj == obj)
	return MUIV_DragQuery_Refuse;
    if (get(msg->obj, MUIA_Imagedisplay_Spec, &dummy)
	|| get(msg->obj, MUIA_Pendisplay_Spec, &dummy))
	return MUIV_DragQuery_Accept;
    return MUIV_DragQuery_Refuse;
}

IPTR Imagedisplay__MUIM_DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
    IPTR spec;

    if (get(msg->obj, MUIA_Imagedisplay_Spec, &spec))
    {
	set(obj, MUIA_Imagedisplay_Spec, spec);
    }
    else if (get(msg->obj, MUIA_Pendisplay_Spec, &spec))
    {
	char buf[67];

	strcpy(buf, "2:");
	strncpy(buf + 2, (const char *)spec, 64);
	buf[66] = 0;
	set(obj, MUIA_Imagedisplay_Spec, (IPTR)buf);
    }
    return 0;
}


#if ZUNE_BUILTIN_IMAGEDISPLAY
BOOPSI_DISPATCHER(IPTR, Imagedisplay_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:         return Imagedisplay__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE:     return Imagedisplay__OM_DISPOSE(cl, obj, msg);
	case OM_SET:         return Imagedisplay__OM_SET(cl, obj, (APTR)msg);
	case OM_GET:         return Imagedisplay__OM_GET(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Imagedisplay__MUIM_AskMinMax(cl,obj,(APTR)msg);
	case MUIM_Setup:     return Imagedisplay__MUIM_Setup(cl,obj,(APTR)msg);
	case MUIM_Cleanup:   return Imagedisplay__MUIM_Cleanup(cl,obj,(APTR)msg);
	case MUIM_Show:      return Imagedisplay__MUIM_Show(cl,obj,(APTR)msg);
	case MUIM_Hide:      return Imagedisplay__MUIM_Hide(cl,obj,(APTR)msg);
	case MUIM_Draw:      return Imagedisplay__MUIM_Draw(cl,obj,(APTR)msg);
	case MUIM_DragQuery: return Imagedisplay__MUIM_DragQuery(cl,obj,(APTR)msg);
	case MUIM_DragDrop:  return Imagedisplay__MUIM_DragDrop(cl,obj,(APTR)msg);
        default:             return DoSuperMethodA(cl, obj, msg);
    }    
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Imagedisplay_desc =
{ 
    MUIC_Imagedisplay, 
    MUIC_Area, 
    sizeof(struct Imagedisplay_DATA), 
    (void*)Imagedisplay_Dispatcher 
};
#endif /* ZUNE_BUILTIN_IMAGEDISPLAY */
