/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"

extern struct Library *MUIMasterBase;

#define MIF_FREEVERT   (1<<0)
#define MIF_FREEHORIZ  (1<<1)

struct MUI_ImageData
{
    char *spec;
    ULONG flags;
    struct MUI_ImageSpec *img;
};


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Image_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Image_FreeHoriz:
		    _handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEHORIZ);
		    break;

	    case    MUIA_Image_FreeVert:
		    _handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEVERT);
		    break;

	    case    MUIA_Image_Spec:
		    {
		    	char *spec;
		    	char spec_buf[20];

		    	if (tag->ti_Data >= MUII_WindowBack && tag->ti_Data < MUII_BACKGROUND)
		    	{
			    sprintf(spec_buf,"6:%ld",tag->ti_Data);
			    spec = spec_buf;
		    	} else
		    	{
			    if (tag->ti_Data >= MUII_BACKGROUND && tag->ti_Data < MUII_LASTPAT)
			    {
				sprintf(spec_buf,"0:%ld",tag->ti_Data);
				spec = spec_buf;
			    } else spec = (char*)tag->ti_Data;
		    	}
			data->spec = StrDup(spec);
		    }
		    break;
    	}
    }

    if (!data->spec) data->spec = StrDup("0:128");
    if (!data->spec)
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
    	return NULL;
    }
    
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Image_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    if (data->spec) FreeVec(data->spec);
    DoSuperMethodA(cl,obj,(Msg)msg);
    return 0;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Image_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Imagedisplay_Spec:
	    case    MUIA_Image_Spec:
		    {
		    	char *spec;
		    	char spec_buf[20];

		    	if (tag->ti_Data >= MUII_WindowBack && tag->ti_Data < MUII_BACKGROUND)
		    	{
			    sprintf(spec_buf,"6:%ld",tag->ti_Data);
			    spec = spec_buf;
		    	} else
		    	{
			    if (tag->ti_Data >= MUII_BACKGROUND && tag->ti_Data < MUII_LASTPAT)
			    {
				sprintf(spec_buf,"0:%ld",tag->ti_Data);
				spec = spec_buf;
			    } else spec = (char*)tag->ti_Data;
		    	}

		        if (data->spec) FreeVec(data->spec);
		        data->spec = StrDup(spec);
		    }

		    if (_flags(obj)&MADF_CANDRAW)
			zune_imspec_hide(data->img);

		    if (_flags(obj)&MADF_SETUP)
		    {
			zune_imspec_cleanup(&data->img, muiRenderInfo(obj));
			zune_imspec_free(data->img);
			    
			data->img = zune_image_spec_to_structure((IPTR)data->spec,obj);
			zune_imspec_setup(&data->img, muiRenderInfo(obj));
		    }

		    if (_flags(obj)&MADF_CANDRAW)
			zune_imspec_show(data->img,obj);

		    MUI_Redraw(obj,MADF_DRAWUPDATE);
		    break;
    	}
    }
    
    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Image_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
	case    MUIA_Image_Spec:
		*msg->opg_Storage = (ULONG)data->spec;
	        break;

	case    MUIA_Imagedisplay_Spec:
		*msg->opg_Storage = (ULONG)data->spec;
	        break;
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Image_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    if (!DoSuperMethodA(cl,obj,(Msg)msg)) return NULL;

    data->img = zune_image_spec_to_structure((IPTR)data->spec,obj);
    zune_imspec_setup(&data->img, muiRenderInfo(obj));
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Image_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    zune_imspec_cleanup(&data->img, muiRenderInfo(obj));
    zune_imspec_free(data->img);
    data->img = NULL;
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Image_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth += zune_imspec_get_minwidth(data->img);
    msg->MinMaxInfo->MinHeight += zune_imspec_get_minheight(data->img);
   
    msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;

    if (data->flags & MIF_FREEHORIZ) msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    else  msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;

    if (data->flags & MIF_FREEVERT) msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    else msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;

    return 1;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR Image_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl,obj,(Msg)msg);

    zune_imspec_show(data->img,obj);
    return 1;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Image_Hide(struct IClass *cl, Object *obj,struct MUIP_Hide *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    zune_imspec_hide(data->img);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Image_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    LONG selected;
    DoSuperMethodA(cl,obj,(Msg)msg);

    get(obj,MUIA_Selected,&selected);

    zune_draw_image(muiRenderInfo(obj), data->img,
		    _mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj),
		    0, 0, selected << IMSPEC_SELECTED);
    return 1;
}


#ifndef _AROS
__asm IPTR Image_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Image_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Image_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Image_Dispose(cl, obj, msg);
	case OM_SET: return Image_Set(cl, obj, (APTR)msg);
	case OM_GET: return Image_Get(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Image_AskMinMax(cl,obj,(APTR)msg);
	case MUIM_Setup: return Image_Setup(cl,obj,(APTR)msg);
	case MUIM_Cleanup: return Image_Cleanup(cl,obj,(APTR)msg);
	case MUIM_Show: return Image_Show(cl,obj,(APTR)msg);
	case MUIM_Hide: return Image_Hide(cl,obj,(APTR)msg);
	case MUIM_Draw: return Image_Draw(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Image_desc = { 
    MUIC_Image, 
    MUIC_Area, 
    sizeof(struct MUI_ImageData), 
    (void*)Image_Dispatcher 
};

