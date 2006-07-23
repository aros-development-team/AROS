/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2006, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "textengine.h"
#include "prefs.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

static const int __version  = 1;
static const int __revision = 1;

struct Rectangle_DATA
{
    STRPTR         BarTitle;
    ULONG          Type;
    ZText         *ztext;
};

#define RECTANGLE_TYPE_NORMAL 0
#define RECTANGLE_TYPE_HBAR 1
#define RECTANGLE_TYPE_VBAR 2

static void draw_line(struct RastPort *rp, int xa, int ya, int xb, int yb)
{
    Move(rp,xa,ya);
    Draw(rp,xb,yb);
}


IPTR Rectangle__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Rectangle_DATA *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
	MUIA_Font, MUIV_Font_Title,
	TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj)
	return 0;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    data->Type = RECTANGLE_TYPE_NORMAL;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem**)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Rectangle_BarTitle:
	    data->BarTitle = StrDup((STRPTR)tag->ti_Data);
	    break;
	case MUIA_Rectangle_HBar:
	    data->Type = RECTANGLE_TYPE_HBAR;
	    break;
	case MUIA_Rectangle_VBar:
	    data->Type = RECTANGLE_TYPE_VBAR;
	    break;
	}
    }

    D(bug("muimaster.library/rectangle.c: New Rectangle Object at 0x%lx\n",obj));

    return (IPTR)obj;
}

IPTR Rectangle__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Rectangle_DATA *data = INST_DATA(cl, obj);

    if (data->BarTitle) mui_free(data->BarTitle);

    return DoSuperMethodA(cl, obj, msg);
}


IPTR Rectangle__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
/*----------------------------------------------------------------------------*/
/* small macro to simplify return value storage */
/*----------------------------------------------------------------------------*/
#define STORE *(msg->opg_Storage)

    struct Rectangle_DATA *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
    case MUIA_Version:
	STORE = __version;
	return TRUE;

    case MUIA_Revision:
	STORE = __revision;
	return TRUE;

    case MUIA_Rectangle_BarTitle:
	STORE = (IPTR)data->BarTitle;
	return TRUE;

    case MUIA_Rectangle_HBar:
	STORE = (data->Type == RECTANGLE_TYPE_HBAR);
	return TRUE;

    case MUIA_Rectangle_VBar:
	STORE = (data->Type == RECTANGLE_TYPE_VBAR);
	return TRUE;
    }

/* our handler didn't understand the attribute, we simply pass
** it to our superclass now
*/
    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

IPTR Rectangle__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Rectangle_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    if (data->BarTitle)
    {
	data->ztext = zune_text_new(NULL, data->BarTitle,
				    ZTEXT_ARG_NONE, 0);
    }

    return TRUE;
}

IPTR Rectangle__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Rectangle_DATA *data = INST_DATA(cl, obj);

    if (data->ztext)
	zune_text_destroy(data->ztext);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax

 AskMinMax method will be called before the window is opened
 and before layout takes place. We need to tell MUI the
 minimum, maximum and default size of our object.
**************************************************************************/
IPTR Rectangle__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Rectangle_DATA *data = INST_DATA(cl, obj);

    /*
    ** let our superclass first fill in what it thinks about sizes.
    ** this will e.g. add the size of frame and inner spacing.
    */
    DoSuperMethodA(cl, obj, (Msg)msg);

    /*
    ** now add the values specific to our object. note that we
    ** indeed need to *add* these values, not just set them!
    */

    if (!data->BarTitle)
    {
/*  	msg->MinMaxInfo->MinWidth += 1; */
/*  	msg->MinMaxInfo->MinHeight += 1;	     */
	if (data->Type == RECTANGLE_TYPE_HBAR)
	{
	    msg->MinMaxInfo->MinHeight += 2;
	}
	else if (data->Type == RECTANGLE_TYPE_VBAR)
	{
	    msg->MinMaxInfo->MinWidth += 2;
	}
    }
    else
    {
	zune_text_get_bounds(data->ztext, obj);
	msg->MinMaxInfo->MinWidth  += data->ztext->width;
	msg->MinMaxInfo->MinHeight += data->ztext->height;
	D(bug("rect: minheight %ld\n",data->ztext->height));
	if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color == GROUP_TITLE_COLOR_3D)
	{
	    msg->MinMaxInfo->MinWidth  += 1;
	    msg->MinMaxInfo->MinHeight += 1;
	}
    }

    msg->MinMaxInfo->DefWidth  = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;

    msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return TRUE;
}


/**************************************************************************
 MUIM_Draw

 Draw method is called whenever MUI feels we should render
 our object. This usually happens after layout is finished
 or when we need to refresh in a simplerefresh window.
 Note: You may only render within the rectangle
       _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj).
**************************************************************************/
IPTR Rectangle__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Rectangle_DATA *data = INST_DATA(cl, obj);
    struct MUI_RenderInfo *mri;

    /*
    ** let our superclass draw itself first, area class would
    ** e.g. draw the frame and clear the whole region. What
    ** it does exactly depends on msg->flags.
    */

    DoSuperMethodA(cl, obj, (Msg)msg);

    /*
    ** if MADF_DRAWOBJECT isn't set, we shouldn't draw anything.
    ** MUI just wanted to update the frame or something like that.
    */

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;


D(bug("Draw Rectangle(0x%lx) %ldx%ldx%ldx%ld mw=%ld mh=%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj), _mwidth(obj), _mheight(obj)));

    if (_mwidth(obj) < 1 || _mheight(obj) < 1)
	return TRUE;

    mri = muiRenderInfo(obj);

    /*
     * ok, everything ready to render...
     */
    if (data->BarTitle)
    {
	/*
	 * Rewrite me, I'm ugly ! (but right :)
	 */
	int tw;
	int th;
	int x1;
	int x2;
	int yt;

        /*  D(bug("muimaster.library/rectangle.c: Draw Rectangle Object at 0x%lx %ldx%ldx%ldx%ld\n mw=%ld mh=%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj), _mwidth(obj), _mheight(obj))); */

	SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color == GROUP_TITLE_COLOR_3D)
	{
	    tw = data->ztext->width + 1;
	    th = data->ztext->height + 1;
	    x1 = _mleft(obj) + (_mwidth(obj) - tw) / 2;
	    x1 -= 1;
	    x2 = x1 + tw;
	    yt = _mtop(obj) + (_mheight(obj) - th) / 2;
	    zune_text_draw(data->ztext, obj, x1 + 1, x2, yt + 1);
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    zune_text_draw(data->ztext, obj, x1, x2 - 1, yt);

	    if (data->Type == RECTANGLE_TYPE_HBAR)
	    {
		int y = yt + data->ztext->height / 2;
		
		if ((x1 - 2) > _mleft(obj)
		    && (x2 + 2) < _mright(obj))
		{
		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
		    draw_line(_rp(obj), _mleft(obj), y, x1 - 2, y);
		    draw_line(_rp(obj), x2 + 3, y, _mright(obj), y);

		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
		    draw_line(_rp(obj), _mleft(obj), y+1, x1 - 2, y+1);
		    draw_line(_rp(obj), x2 + 3, y+1, _mright(obj), y+1);
		}
	    }
	}
	else /* black or white */
	{
	    if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color == GROUP_TITLE_COLOR_HILITE)
		SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    
	    tw = data->ztext->width;
	    th = data->ztext->height;
	    x1 = _mleft(obj) + (_mwidth(obj) - tw) / 2;
	    x2 = x1 + tw;
	    yt = _mtop(obj) + (_mheight(obj) - th) / 2;

	    zune_text_draw(data->ztext, obj, x1, x2 - 1, yt);

	    if (data->Type == RECTANGLE_TYPE_HBAR)
	    {
		int y = yt + data->ztext->height / 2;
	    
		if ((x1 - 2) > _mleft(obj)
		    && (x2 + 2) < _mright(obj))
		{
		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
		    draw_line(_rp(obj), _mleft(obj), y, x1 - 3, y);
		    draw_line(_rp(obj), x2 + 2, y, _mright(obj), y);

		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
		    draw_line(_rp(obj), _mleft(obj), y+1, x1 - 3, y+1);
		    draw_line(_rp(obj), x2 + 2, y+1, _mright(obj), y+1);
		}
	    }
	}
    }
    else /* no bar title */
    {
	if (data->Type == RECTANGLE_TYPE_HBAR)
	{
	    int y = _mtop(obj) + _mheight(obj) / 2 - 1;
	    
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	    draw_line(_rp(obj), _mleft(obj), y, _mright(obj), y);

	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    draw_line(_rp(obj), _mleft(obj), y+1, _mright(obj), y+1);
	}
	else if (data->Type == RECTANGLE_TYPE_VBAR)
	{
	    int x = _mleft(obj) + _mwidth(obj) / 2 - 1;

	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	    draw_line(_rp(obj), x, _mtop(obj), x, _mbottom(obj));

	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    draw_line(_rp(obj), x+1, _mtop(obj), x+1, _mbottom(obj));
	}
	else
	{
#ifdef MYDEBUG
	    WORD pnts[] = { _mleft(obj), _mtop(obj),  _mleft(obj), _mbottom(obj),
		_mright(obj), _mbottom(obj), _mright(obj),  _mtop(obj) };
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	    Move(_rp(obj), _mright(obj),  _mtop(obj));
	    PolyDraw(_rp(obj), 4, pnts);
#endif
	}
    }
    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, Rectangle_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:         return Rectangle__OM_NEW(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE:     return Rectangle__OM_DISPOSE(cl, obj, msg);
	case OM_GET:         return Rectangle__OM_GET(cl, obj, (struct opGet *)msg);
	case MUIM_Setup:     return Rectangle__MUIM_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup:   return Rectangle__MUIM_Cleanup(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Rectangle__MUIM_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw:      return Rectangle__MUIM_Draw(cl, obj, (APTR)msg);
        default:             return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Rectangle_desc =
{ 
    MUIC_Rectangle, 
    MUIC_Area, 
    sizeof(struct Rectangle_DATA), 
    (void*)Rectangle_Dispatcher 
};
