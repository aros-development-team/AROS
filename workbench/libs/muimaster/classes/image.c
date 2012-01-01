/*
    Copyright  2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include <intuition/imageclass.h>
#include <datatypes/pictureclass.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/cybergraphics.h>
#include <proto/datatypes.h>

#include <string.h>

/*  #define MYDEBUG 1 */

#include "../datatypescache.h"
#include "../imspec_intern.h"

#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"

// FIXME: quick hack to not draw the background for gradients. It should really be generalized
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
    struct  NewImage *propimage;
    struct Image *old_image;
    Object *prop; /* private hack to use prop images */
    LONG   state; /* see IDS_* in intuition/imageclass.h */
    ULONG  specnr;
};


/**************************************************************************
 OM_NEW
**************************************************************************/
IPTR Image__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageData   *data;
    struct TagItem    *tags;
    struct TagItem  	    *tag;
    Object *prop;

/*      D(bug("Image_New starts\n")); */

    prop = (Object *)GetTagData(MUIA_Image_Prop, 0, msg->ops_AttrList);


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

		    data->specnr = tag->ti_Data;

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

    data->prop = prop;

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
IPTR Image__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
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
IPTR Image__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
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
		data->specnr = tag->ti_Data;
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
		    // FIXME: quick hack to not draw the background for gradients. It should really be generalized
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
IPTR Image__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
	case    MUIA_Image_Spec:
	    *msg->opg_Storage = (IPTR)data->spec;
	    return TRUE;
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

struct NewImage *GetPropImage(struct MUI_ImageData *data, Object *prop)
{
    struct MUI_AreaData *adata = muiAreaData(prop);
    struct MUI_ImageSpec_intern *spec = adata->mad_Background;

    if (spec)
    {
	if (spec->type == IST_BITMAP)
	{
	    struct dt_node *node = spec->u.bitmap.dt;
	    if (node)
	    {
		if (node->mode == MODE_PROP)
		{
		    if (data->specnr == MUII_ArrowDown) return node->img_down;
		    if (data->specnr == MUII_ArrowUp) return node->img_up;
		    if (data->specnr == MUII_ArrowLeft) return node->img_left;
		    if (data->specnr == MUII_ArrowRight) return node->img_right;
		}
	    }
	}
    }
    return NULL;
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
IPTR Image__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);
    if (!DoSuperMethodA(cl, obj, (Msg)msg))
	return (IPTR)NULL;
    data->propimage = NULL;

    if (data->prop)
    {
	if (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_type == SCROLLBAR_TYPE_CUSTOM)
	{
	    struct NewImage *ni = GetPropImage(data, data->prop);
	    if (ni)
	    {
		set(obj, MUIA_Frame, MUIV_Frame_None);
		return(1);
	    }
	}
	else
	{
	    data->prop = NULL;
	}

    }

    if (data->spec)
    {
	data->img = zune_imspec_setup((IPTR)data->spec, muiRenderInfo(obj));
	// FIXME: quick hack to not draw the background for gradients. It should really be generalized
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
IPTR Image__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
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
IPTR Image__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);


    if (data->prop != NULL)
    {
	struct NewImage *ni = GetPropImage(data, data->prop);
	if (ni)
	{
	    msg->MinMaxInfo->MinWidth = ni->w >> 2;
	    msg->MinMaxInfo->MaxWidth = ni->w >> 2;
	    msg->MinMaxInfo->DefWidth = ni->w >> 2;

	    msg->MinMaxInfo->MinHeight = ni->h;
	    msg->MinMaxInfo->MaxHeight = ni->h;
	    msg->MinMaxInfo->DefHeight = ni->h;
	    data->propimage = ni;
	    return(1);
	}
    }

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
	else
	{
	    msg->MinMaxInfo->MinWidth += minmax.DefWidth;
	    msg->MinMaxInfo->MaxWidth += minmax.DefWidth;
	    msg->MinMaxInfo->DefWidth += minmax.DefWidth;
	}

	if (data->flags & MIF_FONTMATCHWIDTH)
	{
	    msg->MinMaxInfo->DefWidth *= _font(obj)->tf_XSize / 8;
	}


	if (data->flags & MIF_FREEVERT)
	{
	    msg->MinMaxInfo->MinHeight += minmax.MinHeight;
	    msg->MinMaxInfo->MaxHeight += minmax.MaxHeight;
	    msg->MinMaxInfo->DefHeight += minmax.DefHeight;
	}
	else
	{
	    msg->MinMaxInfo->MinHeight += minmax.DefHeight;
	    msg->MinMaxInfo->MaxHeight += minmax.DefHeight;
	    msg->MinMaxInfo->DefHeight += minmax.DefHeight;
	}

	if (data->flags & MIF_FONTMATCHHEIGHT)
	{
	    msg->MinMaxInfo->DefHeight *= _font(obj)->tf_YSize / 8;
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
IPTR Image__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
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
IPTR Image__MUIM_Hide(struct IClass *cl, Object *obj,struct MUIP_Hide *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    if (data->img)
	zune_imspec_hide(data->img);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/

void DrawAlphaStateImageToRP(struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp) {

/* Function:   draw an Image to a specific RastPort using AlphaValues
 * Input:      RastPortd rp:
    *                          RastPort to draw to
 *             NewImage ni:
    *                          Image to draw
 *             UWORD xp, yp:
    *                          location where to draw the Image
    * Bugs:	   Not known, throught the lack of AlphaHandling in cgfx/p96 there
    *             must all be done by hand so it's recommended to use this for small Images.
    * NOTES:      a temporary Buffer must be allocated to store the Background
*/

    UWORD 	ix, iy;
    UBYTE   *d;
    ULONG   depth;


    
    if (ni)
    {
	depth = (ULONG) GetBitMapAttr(rp->BitMap, BMA_DEPTH);

	d = (UBYTE *) ni->data;
	ix=ni->w;
	iy=ni->h;
	switch(state) {
	    case IDS_NORMAL:
		break;
	    case IDS_SELECTED:
		d += (ix >> 2) * 4;
		break;
	    case IDS_INACTIVENORMAL:
		d += (ix >> 2) * 8;
		break;
	}

	if (depth >= 15)
	{
	    WritePixelArrayAlpha(d, 0 , 0, ix*4, rp, xp, yp, ix >> 2, iy, 0xffffffff);
	}
	else if (ni->bitmap != NULL)
	{
	    if (ni->mask)
	    {
		BltMaskBitMapRastPort(ni->bitmap, 0, 0, rp, xp, yp, ix >> 2, iy, 0xe0, (PLANEPTR) ni->mask);  
	    }
	    else BltBitMapRastPort(ni->bitmap, 0, 0, rp, xp, yp, ix >> 2, iy, 0xc0);
	}
    }
}


IPTR Image__MUIM_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct MUI_ImageData *data = INST_DATA(cl, obj);

    D(bug("Image_Draw(%p): msg=0x%08lx state=%ld sss=%ld\n",
		obj, ((struct MUIP_Draw *)msg)->flags, data->state,
		!!(_flags(obj) & MADF_SHOWSELSTATE)));

    if (data->propimage == NULL) 
    {
	DoSuperMethodA(cl,obj,(Msg)msg);

	if (!(msg->flags & (MADF_DRAWOBJECT|MADF_DRAWUPDATE)))
	    return 0;
    }

    if (data->propimage)
    {
	//Object *p = NULL;
	//get(obj, MUIA_Parent, &p);
	//if (p) DoMethod(p, MUIM_DrawParentBackground, _left(obj), _top(obj), _width(obj), _height(obj), _left(obj), _top(obj), 0);
	//else 
	DoMethod(obj, MUIM_DrawParentBackground, _left(obj), _top(obj), _width(obj), _height(obj), _left(obj), _top(obj), 0);

	DrawAlphaStateImageToRP(_rp(obj), data->propimage, data->state, _left(obj), _top(obj));
    }
    else if (data->img)
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
	case OM_NEW:         return Image__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE:     return Image__OM_DISPOSE(cl, obj, msg);
	case OM_SET:         return Image__OM_SET(cl, obj, (APTR)msg);
	case OM_GET:         return Image__OM_GET(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Image__MUIM_AskMinMax(cl,obj,(APTR)msg);
	case MUIM_Setup:     return Image__MUIM_Setup(cl,obj,(APTR)msg);
	case MUIM_Cleanup:   return Image__MUIM_Cleanup(cl,obj,(APTR)msg);
	case MUIM_Show:      return Image__MUIM_Show(cl,obj,(APTR)msg);
	case MUIM_Hide:      return Image__MUIM_Hide(cl,obj,(APTR)msg);
	case MUIM_Draw:      return Image__MUIM_Draw(cl,obj,(APTR)msg);
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

