/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <zunepriv.h>
#include <builtin.h>
#include <imagespec.h>
#include <drawing.h>
#include <Image.h>
#include <imagedata.h>
#include <Notify.h>
#include <Area.h>

static const int __version = 1;
static const int __revision = 1;

static ULONG
mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);
    data->mid_State = IDS_NORMAL;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Image_FontMatch:
		break;
	    case MUIA_Image_FontMatchHeight:
		break;
	    case MUIA_Image_FontMatchWidth:
		break;
	    case MUIA_Image_FreeHoriz:
		_handle_bool_tag(data->mid_Flags, tag->ti_Data, MIDF_FREEHORIZ);
		break;
	    case MUIA_Image_FreeVert:
		_handle_bool_tag(data->mid_Flags, tag->ti_Data, MIDF_FREEVERT);
		break;
	    case MUIA_Image_OldImage:
		break;
	    case MUIA_Image_Spec:
		data->mid_OrigSpec = (ULONG)tag->ti_Data;
		break;
	    case MUIA_Image_State:
		data->mid_State = (ULONG)tag->ti_Data;
		break;
	}
    }

    DoMethod(obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	     GPOINTER_TO_UINT(obj), 1, MUIM_Image_ToggleState);

    return (ULONG)obj;
}


static ULONG
mDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
/*  g_print("before MUI_Image dispose\n"); */
    if (data->mid_ImageSpec)
	zune_imspec_free(data->mid_ImageSpec);
/*  g_print("after MUI_Image dispose\n"); */
    return DoSuperMethodA(cl, obj, msg);
}


static ULONG
mSet(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Image_State:
		data->mid_State = (ULONG)tag->ti_Data;
		if (__zune_imspec_set_state(data->mid_ImageSpec, data->mid_State))
		    MUI_Redraw(obj, MADF_DRAWUPDATE);
		break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


static ULONG
mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
/*----------------------------------------------------------------------------*/
/* small macro to simplify return value storage */
/*----------------------------------------------------------------------------*/
#define STORE *(msg->opg_Storage)
    switch(msg->opg_AttrID)
    {
	case MUIA_Version:
	    STORE = __version;
	    return(TRUE);
	case MUIA_Revision:
	    STORE = __revision;
	    return(TRUE);
    }
    return(DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


static ULONG
mAskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MaxWidth += zune_imspec_get_width(data->mid_ImageSpec);
    msg->MinMaxInfo->MaxHeight += zune_imspec_get_height(data->mid_ImageSpec);

    if (zune_imspec_get_width(data->mid_ImageSpec) == MUI_MAXMAX)
    {
	msg->MinMaxInfo->MinWidth += 1;
	msg->MinMaxInfo->DefWidth += 1;
    }
    else
    {
	msg->MinMaxInfo->MinWidth += zune_imspec_get_width(data->mid_ImageSpec);
	msg->MinMaxInfo->DefWidth += zune_imspec_get_width(data->mid_ImageSpec);
    }

    if (zune_imspec_get_height(data->mid_ImageSpec) == MUI_MAXMAX)
    {
	msg->MinMaxInfo->MinHeight += 1;
	msg->MinMaxInfo->DefHeight += 1;
    }
    else
    {
	msg->MinMaxInfo->MinHeight += zune_imspec_get_height(data->mid_ImageSpec);
	msg->MinMaxInfo->DefHeight += zune_imspec_get_height(data->mid_ImageSpec);
    }

    if (data->mid_Flags & MIDF_FREEHORIZ)
	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

    if (data->mid_Flags & MIDF_FREEVERT)
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return TRUE;
}


static ULONG
mSetup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    if (!data->mid_ImageSpec)
    {
	data->mid_ImageSpec =
	    zune_image_spec_to_structure((ULONG)data->mid_OrigSpec);
/*  	g_print("zimage got spec %p\n", data->mid_ImageSpec); */
    }

    zune_imspec_setup(&data->mid_ImageSpec, muiRenderInfo(obj));
    return TRUE;
}


static ULONG
mCleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    zune_imspec_cleanup(&data->mid_ImageSpec, muiRenderInfo(obj));
    return (DoSuperMethodA(cl, obj, (Msg) msg));
}


static ULONG
mShow(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;
    zune_imspec_set_scaled_size(data->mid_ImageSpec, _mwidth(obj), _mheight(obj));
    zune_imspec_show(data->mid_ImageSpec, obj);
    return TRUE;
}


static ULONG
mHide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    zune_imspec_hide(data->mid_ImageSpec);
    return (DoSuperMethodA(cl, obj, (Msg) msg));
}

static ULONG 
mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    LONG w, h;

    DoSuperMethodA(cl,obj,(Msg)msg);
    if (!(msg->flags & MADF_DRAWOBJECT))
	return(0);

    w = zune_imspec_get_width(data->mid_ImageSpec);
    h = zune_imspec_get_height(data->mid_ImageSpec);

    if (data->mid_Flags & MIDF_FREEHORIZ)
	zune_imspec_set_width(data->mid_ImageSpec, _mwidth(obj));
    if (data->mid_Flags & MIDF_FREEVERT)
	zune_imspec_set_height(data->mid_ImageSpec, _mheight(obj));

    zune_draw_image(muiRenderInfo(obj), data->mid_ImageSpec,
		    _mleft(obj),
		    _mtop(obj),
		    _mwidth(obj),
		    _mheight(obj),
		    _mleft(obj), _mtop(obj), data->mid_Flags);

    zune_imspec_set_width(data->mid_ImageSpec, w);
    zune_imspec_set_height(data->mid_ImageSpec, h);

    return TRUE;
}



/*
 * toggle between IDS_NORMAL and IDS_SELECTED
 */
static ULONG
mToggleState(struct IClass *cl, Object *obj, struct MUIP_Image_ToggleState *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->mid_State == IDS_NORMAL)
	set(obj, MUIA_Image_State, IDS_SELECTED);
    else
	set(obj, MUIA_Image_State, IDS_NORMAL);
    return TRUE;
}


static ULONG
MyDispatcher (struct IClass *cl, Object *obj, Msg msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return(mNew(cl, obj, (struct opSet *) msg));
	case OM_DISPOSE:
	    return(mDispose(cl, obj, msg));
	case OM_SET:
	    return(mSet(cl, obj, (struct opSet *)msg));
	case OM_GET:
	    return(mGet(cl, obj, (struct opGet *)msg));
	case MUIM_AskMinMax :
	    return(mAskMinMax(cl, obj, (APTR)msg));
	case MUIM_Draw :
	    return(mDraw(cl, obj, (APTR)msg));
	case MUIM_Setup :
	    return(mSetup(cl, obj, (APTR)msg));
	case MUIM_Cleanup :
	    return(mCleanup(cl, obj, (APTR)msg));
	case MUIM_Show :
	    return(mShow(cl, obj, (APTR)msg));
	case MUIM_Hide :
	    return(mHide(cl, obj, (APTR)msg));
	case MUIM_Image_ToggleState:
	    return(mToggleState(cl, obj, (APTR)msg));
    }

    return(DoSuperMethodA(cl, obj, msg));
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Image_desc = { 
    MUIC_Image,
    MUIC_Area, 
    sizeof(struct MUI_ImageData), 
    MyDispatcher
};
