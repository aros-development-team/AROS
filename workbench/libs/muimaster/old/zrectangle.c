/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef __AROS__
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#endif

#include <zunepriv.h>
#include <builtin.h>
#include <renderinfo.h>
#include <pen.h>
#include <macros.h>
#include <prefs.h>
#include <Notify.h>
#include <Rectangle.h>
#include <rectangledata.h>
#include <stdlib.h>
#include <string.h>

static const int __version = 1;
static const int __revision = 1;

#ifdef __AROS__
#define gdk_draw_line(w,rp,xa,ya,xb,yb) \
    ({ Move((rp),(xa),(ya)); Draw((rp),(xb),(yb)); })
#endif

/******************************************************************************/
/* NEW                                                                        */
/******************************************************************************/

static ULONG
mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_RectangleData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Font, MUIV_Font_Title,
	TAG_MORE, _U(msg->ops_AttrList));

    if (!obj)
	return NULL;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    data->Type = RECTANGLE_TYPE_NORMAL;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Rectangle_BarTitle:
	    data->BarTitle = g_strdup((STRPTR)tag->ti_Data);
kprintf("*** Rectangle->BarTitle = \"%s\"\n", data->BarTitle);
	    break;
	case MUIA_Rectangle_HBar:
	    data->Type = RECTANGLE_TYPE_HBAR;
	    break;
	case MUIA_Rectangle_VBar:
	    data->Type = RECTANGLE_TYPE_VBAR;
	    break;
	}
    }

    return (ULONG)obj;
}


static ULONG
mDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_RectangleData *data = INST_DATA(cl, obj);

    if (data->BarTitle)
	g_free(data->BarTitle);

    return DoSuperMethodA(cl, obj, msg);
}


/******************************************************************************/
/* GET                                                                        */
/******************************************************************************/

static ULONG
mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
/*----------------------------------------------------------------------------*/
/* small macro to simplify return value storage */
/*----------------------------------------------------------------------------*/
#define STORE *(msg->opg_Storage)

    struct MUI_RectangleData *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
    case MUIA_Version:
	STORE = __version;
	return(TRUE);
    case MUIA_Revision:
	STORE = __revision;
	return(TRUE);

    case MUIA_Rectangle_BarTitle:
	STORE = (ULONG)data->BarTitle;
	return(TRUE);
    case MUIA_Rectangle_HBar:
	STORE = (data->Type == RECTANGLE_TYPE_HBAR);
	return(TRUE);
    case MUIA_Rectangle_VBar:
	STORE = (data->Type == RECTANGLE_TYPE_VBAR);
	return(TRUE);
    }

/* our handler didn't understand the attribute, we simply pass
** it to our superclass now
*/
    return(DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


static ULONG
mSetup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_RectangleData *data = INST_DATA(cl, obj);

kprintf("*** Rectangle->Setup calling DoSuperMethodA()\n");
    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    if (data->BarTitle)
    {
kprintf("*** Rectangle->Setup calling zune_text_new()\n");
	data->ztext = zune_text_new(NULL, data->BarTitle,
				    ZTEXT_ARG_NONE, 0);
    }

    return TRUE;
}


static ULONG
mCleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_RectangleData *data = INST_DATA(cl, obj);

    if (data->ztext)
	zune_text_destroy(data->ztext);

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}


/******************************************************************************/
/*
** AskMinMax method will be called before the window is opened
** and before layout takes place. We need to tell MUI the
** minimum, maximum and default size of our object.
*/
/******************************************************************************/

static ULONG
mAskMinMax(struct IClass *cl, Object *obj,
	   struct MUIP_AskMinMax *msg)
{
    struct MUI_RectangleData *data = INST_DATA(cl, obj);

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
kprintf("*** Rectangle->AskMinMax calling zune_text_get_bounds()\n");
	zune_text_get_bounds(data->ztext, obj);
	msg->MinMaxInfo->MinWidth  += data->ztext->width;
	msg->MinMaxInfo->MinHeight += data->ztext->height;
	if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
	{
	    msg->MinMaxInfo->MinWidth  += 1;
	    msg->MinMaxInfo->MinHeight += 1;
	}
    }

    msg->MinMaxInfo->DefWidth  = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;

    msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    g_print("Rectangle_AskMinMax %p: Min=%dx%d Max=%dx%d Def=%dx%d\n", obj,
	    msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight,
	    msg->MinMaxInfo->MaxWidth, msg->MinMaxInfo->MaxHeight,
	    msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->DefHeight);

    return TRUE;
}

/******************************************************************************/
/*
** Draw method is called whenever MUI feels we should render
** our object. This usually happens after layout is finished
** or when we need to refresh in a simplerefresh window.
** Note: You may only render within the rectangle
**       _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj).
*/
/******************************************************************************/

static ULONG 
mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_RectangleData *data = INST_DATA(cl, obj);
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

/*      g_print("rect draw %p : %dx%d\n", obj, _mwidth(obj), _mheight(obj)); */

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

/*  	g_print("rect bartitle: textdraw %d %d %d\n", x1, x2, yt); */
	
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
	{
	    tw = data->ztext->width + 1;
	    th = data->ztext->height + 1;
	    x1 = _mleft(obj) + (_mwidth(obj) - tw) / 2;
	    x1 -= 1;
	    x2 = x1 + tw;
	    yt = _mtop(obj) + (_mheight(obj) - th) / 2;
kprintf("*** Rectangle->Draw calling zune_text_draw()\n");
	    zune_text_draw(data->ztext, obj, x1 + 1, x2, yt + 1);
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
kprintf("*** Rectangle->Draw calling zune_text_draw()\n");
	    zune_text_draw(data->ztext, obj, x1, x2 - 1, yt);

	    if (data->Type == RECTANGLE_TYPE_HBAR)
	    {
		int y = yt + data->ztext->height / 2;
		
		if ((x1 - 2) > _mleft(obj)
		    && (x2 + 2) < _mright(obj))
		{
		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  _mleft(obj), y, x1 - 2, y);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  x2 + 3, y, _mright(obj), y);

		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  _mleft(obj), y+1, x1 - 2, y+1);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  x2 + 3, y+1, _mright(obj), y+1);
		}
	    }
	}
	else /* black or white */
	{
	    if (__zprefs.group_title_color == GROUP_TITLE_COLOR_WHITE)
		SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    
	    tw = data->ztext->width;
	    th = data->ztext->height;
	    x1 = _mleft(obj) + (_mwidth(obj) - tw) / 2;
	    x2 = x1 + tw;
	    yt = _mtop(obj) + (_mheight(obj) - th) / 2;

kprintf("*** Rectangle->Draw calling zune_text_draw()\n");
	    zune_text_draw(data->ztext, obj, x1, x2 - 1, yt);

	    if (data->Type == RECTANGLE_TYPE_HBAR)
	    {
		int y = yt + data->ztext->height / 2;
	    
		if ((x1 - 2) > _mleft(obj)
		    && (x2 + 2) < _mright(obj))
		{
		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  _mleft(obj), y, x1 - 3, y);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  x2 + 2, y, _mright(obj), y);

		    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  _mleft(obj), y+1, x1 - 3, y+1);
		    gdk_draw_line(mri->mri_Window, _rp(obj),
				  x2 + 2, y+1, _mright(obj), y+1);
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
	    gdk_draw_line(mri->mri_Window, _rp(obj),
			  _mleft(obj), y, _mright(obj), y);

	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    gdk_draw_line(mri->mri_Window, _rp(obj),
			  _mleft(obj), y+1, _mright(obj), y+1);
	}
	else if (data->Type == RECTANGLE_TYPE_VBAR)
	{
	    int x = _mleft(obj) + _mwidth(obj) / 2 - 1;

	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	    gdk_draw_line(mri->mri_Window, _rp(obj),
			  x, _mtop(obj), x, _mbottom(obj));

	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	    gdk_draw_line(mri->mri_Window, _rp(obj),
			  x, _mtop(obj), x, _mbottom(obj));
	}
    }
    return TRUE;
}

/******************************************************************************/
/******************************************************************************/

AROS_UFH3S(IPTR, MyDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
/*----------------------------------------------------------------------------*/
/* watch out for methods we do understand                                     */
/*----------------------------------------------------------------------------*/
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW:
	    return(mNew(cl, obj, (struct opSet *) msg));
	case OM_DISPOSE:
	    return(mDispose(cl, obj, msg));
	case OM_GET:
	    return(mGet(cl, obj, (struct opGet *)msg));
	case MUIM_Setup :
	    return(mSetup(cl, obj, (APTR)msg));
	case MUIM_Cleanup :
	    return(mCleanup(cl, obj, (APTR)msg));
	case MUIM_AskMinMax :
	    return(mAskMinMax(cl, obj, (APTR)msg));
	case MUIM_Draw :
	    return(mDraw(cl, obj, (APTR)msg));
    }

/*----------------------------------------------------------------------------*/
/* we didn't understand the last method, so call our superclass               */
/*----------------------------------------------------------------------------*/
    return(DoSuperMethodA(cl, obj, msg));
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Rectangle_desc = { 
    MUIC_Rectangle, 
    MUIC_Area, 
    sizeof(struct MUI_RectangleData), 
    MyDispatcher 
};
