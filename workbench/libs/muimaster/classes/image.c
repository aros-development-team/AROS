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
#include <proto/muimaster.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"

#warning quick hack to not draw the background for gradients. It should really be generalized
#include "imspec_intern.h"

extern struct Library *MUIMasterBase;

#define MIF_FREEVERT         (1<<0)
#define MIF_FREEHORIZ        (1<<1)
#define MIF_FONTMATCHWIDTH   (1<<3)
#define MIF_FONTMATCHHEIGHT  (1<<4)

struct MUI_ImageData
{
    char *spec;
    ULONG flags;
    struct MUI_ImageSpec_intern *img;
    struct Image *old_image;
    LONG   state; /* see IDS_* in intuition/imageclass.h */
};


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Image_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData   *data;
    struct TagItem  	    *tag, *tags;
    
/*      D(bug("Image_New starts\n")); */

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->state = IDS_NORMAL;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Selected:
		if (tag->ti_Data)
		    data->state = IDS_SELECTED;
		break;
	    case MUIA_Image_FontMatch:
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHWIDTH);
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHHEIGHT);
		break;
	    case MUIA_Image_FontMatchWidth:
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHWIDTH);
		break;
	    case MUIA_Image_FontMatchHeight:
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FONTMATCHHEIGHT);
		break;
	    case MUIA_Image_FreeHoriz:
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEHORIZ);
		break;
	    case MUIA_Image_FreeVert:
		_handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEVERT);
		break;
	    case MUIA_Image_Spec:
	    {
		char *spec;
		char spec_buf[20];

		if (tag->ti_Data >= MUII_WindowBack && tag->ti_Data < MUII_BACKGROUND)
		{
		    sprintf(spec_buf,"6:%ld",tag->ti_Data);
		    spec = spec_buf;
		}
		else if (tag->ti_Data >= MUII_BACKGROUND && tag->ti_Data < MUII_LASTPAT)
		{
		    sprintf(spec_buf,"0:%ld",tag->ti_Data);
		    spec = spec_buf;
		}
		else
		{
		    spec = (char*)tag->ti_Data;
		}
		data->spec = StrDup(spec);
	    }
	    break;
	    case MUIA_Image_OldImage:
		data->old_image = (struct Image *)tag->ti_Data;
		break;
	    case MUIA_Image_State:
		data->state = (LONG)tag->ti_Data;
		break;
    	}
    }

/*      if (!data->spec && !data->old_image) */
/*      { */
/*  	data->spec = StrDup("0:128"); */
/*      } */

/*      if (!data->spec && !data->old_image) */
/*      { */
/*  	CoerceMethod(cl,obj,OM_DISPOSE); */
/*      	return NULL; */
/*      } */
    
/*      D(bug("Image_New(%lx) spec=%lx\n", obj, data->img)); */
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Image_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->spec)
	zune_image_spec_free(data->spec);
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
	    case MUIA_Selected:
		if (tag->ti_Data)
		    data->state = IDS_SELECTED;
		else
		    data->state = IDS_NORMAL;
		D(bug("Image_Set(%p): state=%ld\n", obj, data->state));
		break;
	    case MUIA_Image_State:
		if (data->state != (LONG)tag->ti_Data)
		{
		    data->state = (LONG)tag->ti_Data;
		    MUI_Redraw(obj,MADF_DRAWOBJECT);
		}
		break;
	    case MUIA_Image_Spec:
		if (data->spec)
		    zune_image_spec_free(data->spec);
		data->spec = zune_image_spec_duplicate(tag->ti_Data);

		if ((_flags(obj) & MADF_CANDRAW) && data->img)
		    zune_imspec_hide(data->img);

		if (_flags(obj) & MADF_SETUP)
		{
		    if (data->img)
			zune_imspec_cleanup(data->img);
		    data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
#warning quick hack to not draw the background for gradients. It should really be generalized
                    if (data->img)
                    {
                        if (data->img->type == IST_SCALED_GRADIENT
			    || data->img->type == IST_TILED_GRADIENT)
                            set(obj, MUIA_FillArea, FALSE);
                        else
                            set(obj, MUIA_FillArea, TRUE);
		    }
                }

		if (_flags(obj) & MADF_CANDRAW)
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
static IPTR Image_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
	case    MUIA_Image_Spec:
		*msg->opg_Storage = (ULONG)data->spec;
	        return TRUE;
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Image_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
	return NULL;

    if (data->spec)
    {
	data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
#warning quick hack to not draw the background for gradients. It should really be generalized
        if (data->img)
        {
            if (data->img->type == IST_SCALED_GRADIENT
		|| data->img->type == IST_TILED_GRADIENT)
                set(obj, MUIA_FillArea, FALSE);
            else
                set(obj, MUIA_FillArea, TRUE);
        }

	if ((data->img != NULL) && (data->img->type == IST_BRUSH))
	{
	    set(obj, MUIA_Frame, MUIV_Frame_None);
	}
    }
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Image_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->img)
    {
	zune_imspec_cleanup(data->img);
	data->img = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Image_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->img)
    {
	struct MUI_MinMax minmax;

	zune_imspec_askminmax(data->img, &minmax);
   
	if (data->flags & MIF_FREEHORIZ)
	{
	    msg->MinMaxInfo->MinWidth += minmax.MinWidth;
	    msg->MinMaxInfo->MaxWidth += minmax.MaxWidth;
	    msg->MinMaxInfo->DefWidth += minmax.DefWidth;
	}
	else if ((data->flags & MIF_FONTMATCHWIDTH) &&
		 (_font(obj)->tf_XSize >= minmax.MinWidth) &&
		 (_font(obj)->tf_XSize <= minmax.MaxWidth))
	{
	    msg->MinMaxInfo->MinWidth += _font(obj)->tf_XSize;
	    msg->MinMaxInfo->MaxWidth += _font(obj)->tf_XSize;
	    msg->MinMaxInfo->DefWidth += _font(obj)->tf_XSize;
	}
	else
	{
	    msg->MinMaxInfo->MinWidth += minmax.MinWidth;
	    msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;
	    msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
	}

	if (data->flags & MIF_FREEVERT)
	{
	    msg->MinMaxInfo->MinHeight += minmax.MinHeight;
	    msg->MinMaxInfo->MaxHeight += minmax.MaxHeight;
	    msg->MinMaxInfo->DefHeight += minmax.DefHeight;
	}
	else if ((data->flags & MIF_FONTMATCHHEIGHT) &&
		 (_font(obj)->tf_YSize >= minmax.MinHeight) &&
		 (_font(obj)->tf_YSize <= minmax.MaxHeight))
	{
	    msg->MinMaxInfo->MinHeight += _font(obj)->tf_YSize;
	    msg->MinMaxInfo->MaxHeight += _font(obj)->tf_YSize;
	    msg->MinMaxInfo->DefHeight += _font(obj)->tf_YSize;
	}
	else
	{
	    msg->MinMaxInfo->MinHeight += minmax.MinHeight;
	    msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
	    msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
	}
    }
    else if (data->old_image)
    {
	msg->MinMaxInfo->MinWidth += data->old_image->Width;
	msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
	msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;

	msg->MinMaxInfo->MinHeight += data->old_image->Height;
	msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
	msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
    }
    else /* no imagespec specified */
    {
/*  	D(bug("*** Image_AskMinMax : no img, no old_img\n")); */
/*  	msg->MinMaxInfo->MinWidth += 8; */
/*  	msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth; */
/*  	msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth; */

/*  	msg->MinMaxInfo->MinHeight += 8; */
/*  	msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight; */
/*  	msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;	 */
    }
    return 1;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR Image_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl,obj,(Msg)msg);
    if (data->img)
	zune_imspec_show(data->img, obj);
    return 1;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Image_Hide(struct IClass *cl, Object *obj,struct MUIP_Hide *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->img)
	zune_imspec_hide(data->img);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Image_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    D(bug("Image_Draw(%p): msg=0x%08lx state=%ld sss=%ld\n",
       obj, ((struct MUIP_Draw *)msg)->flags, data->state,
	  !!(_flags(obj) & MADF_SHOWSELSTATE)));

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT|MADF_DRAWUPDATE)))
                return 0;

    if (data->img)
    {
	zune_imspec_draw(data->img, muiRenderInfo(obj),
			_mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj),
			0, 0, data->state);
    }
    else if (data->old_image)
    {
	DrawImage(_rp(obj), data->old_image, _mleft(obj),_mtop(obj));
    }
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Image_Dispatcher, cl, obj, msg)
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
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Image_desc = { 
    MUIC_Image, 
    MUIC_Area, 
    sizeof(struct MUI_ImageData), 
    (void*)Image_Dispatcher 
};

