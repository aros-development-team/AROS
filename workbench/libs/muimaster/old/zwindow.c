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
#include <proto/utility.h>
#endif

#include <zunepriv.h>
#include <builtin.h>
#include <renderinfo.h>
#include <prefs.h>
#include <Notify.h>
#include <Window.h>
#include <windowdata.h>
#include <window_impl.h>

static const int __version = 1;
static const int __revision = 1;


/******************************************************************************/
/******************************************************************************/
static ULONG window_Open(struct IClass *cl, Object *obj);
static ULONG window_Close(struct IClass *cl, Object *obj);

/* code for setting MUIA_Window_RootObject */
static void
window_change_root_object (struct MUI_WindowData *data, Object *obj,
			   Object *newRoot)
{
    static ULONG disconnect = MUIM_DisconnectParent;
    Object *oldRoot = data->wd_RootObject;

    if (!(data->wd_Flags & MUIWF_OPENED))
    {
	if (oldRoot)
	{
	    if (data->wd_ActiveObject
		&& (Object *)data->wd_ActiveObject->data == oldRoot)
		set(obj, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
	    if (_flags(oldRoot) & MADF_CYCLECHAIN)
		data->wd_CycleChain = g_list_remove(data->wd_CycleChain,
						    oldRoot);
	    DoMethodA(oldRoot, (Msg)&disconnect);
	}

	data->wd_RootObject = newRoot;
	if (newRoot)
	{
	    _parent(newRoot) = NULL;
	    /* if window is in App tree, inform child */
	    if (muiNotifyData(obj)->mnd_GlobalInfo)
		DoMethod(newRoot, MUIM_ConnectParentWindow,
			 _U(obj), _U(&data->wd_RenderInfo));
	}
    }
}


/* code for setting MUIA_Window_ActiveObject */
static void
window_set_active_object (struct MUI_WindowData *data, Object *obj,
			  ULONG newval)
{
    static const STACKULONG goActive[]   = { MUIM_GoActive };
    static const STACKULONG goInactive[] = { MUIM_GoInactive };

    if (data->wd_ActiveObject
	&& (ULONG)data->wd_ActiveObject->data == newval)
	return;

/*  g_print("A setting active obj to %lx\n", newval); */
kprintf("A setting active obj to $%lx\n", newval);

    switch (newval)
    {
	case MUIV_Window_ActiveObject_None:
	    if (data->wd_ActiveObject)
	    {
		DoMethodA((Object *)data->wd_ActiveObject->data,
			  (Msg)goInactive);
		data->wd_ActiveObject = NULL;
	    }
	    break;

	case MUIV_Window_ActiveObject_Next:
/*  g_print("list has %d elem\n", g_list_length(data->wd_CycleChain)); */
	    if (data->wd_ActiveObject)
	    {
		DoMethodA((Object *)data->wd_ActiveObject->data,
			  (Msg)goInactive);
		data->wd_ActiveObject = g_list_next(data->wd_ActiveObject);
	    }
	    else if (data->wd_CycleChain)
	    {
		data->wd_ActiveObject = g_list_first(data->wd_CycleChain);
		/* anyway wd_CycleChain is the first node */
		ASSERT(data->wd_ActiveObject == data->wd_CycleChain);
	    }

	    if (data->wd_ActiveObject)
	    {
		DoMethodA((Object *)data->wd_ActiveObject->data,
			  (Msg)goActive);
	    }
	    break;

	case MUIV_Window_ActiveObject_Prev:
	    if (data->wd_ActiveObject)
	    {
		DoMethodA((Object *)data->wd_ActiveObject->data, (Msg)goInactive);
		data->wd_ActiveObject = g_list_previous(data->wd_ActiveObject);
	    }
	    else if (data->wd_CycleChain)
	    {
		data->wd_ActiveObject = g_list_last(data->wd_CycleChain);
	    }

	    if (data->wd_ActiveObject)
		DoMethodA((Object *)data->wd_ActiveObject->data,
			  (Msg)goActive);
	    break;

	default:
	    if (data->wd_ActiveObject)
	    {
		if ((gpointer)newval == data->wd_ActiveObject)
		    return;
		DoMethodA((Object *)data->wd_ActiveObject->data,
			  (Msg)goInactive);
	    }
	    else if ((gpointer)newval == NULL)
		return;

	    data->wd_ActiveObject =
		g_list_find(data->wd_CycleChain,
			    (gpointer)newval);
	    if (data->wd_ActiveObject)
	    {
		DoMethodA((Object *)data->wd_ActiveObject->data,
			  (Msg)goActive);	    
	    }
	    break;
    }
/*  g_print("active obj is now %lx\n", data->wd_ActiveObject ? data->wd_ActiveObject->data : 0L); */
}


/*
 * calculate real dimensions from programmer requirements.
 * may be overridden by user settings if MUIA_Window_ID is set.
 */
/* MUIV_Window_Height_Screen and MUIV_Window_Height_Visible
 * are not handled yet, as their Width couterparts.
 */
static void
window_select_dimensions (struct MUI_WindowData *data)
{
    if (!data->wd_Width)
    {
	if (data->wd_ReqWidth > 0)
	    data->wd_Width = data->wd_ReqWidth;
	else if (data->wd_ReqWidth == MUIV_Window_Width_Default)
	    data->wd_Width = data->wd_MinMax.DefWidth;
	else if (_between(MUIV_Window_Width_MinMax(100),
			  data->wd_ReqWidth,
			  MUIV_Window_Width_MinMax(0)))
	{
	    data->wd_Width = data->wd_MinMax.MinWidth
		- data->wd_ReqWidth
		* (data->wd_MinMax.MaxWidth - data->wd_MinMax.MinWidth);
	}
	else if (_between(MUIV_Window_Width_Screen(100),
			  data->wd_ReqWidth,
			  MUIV_Window_Width_Screen(0)))
	{
	    data->wd_Width = data->wd_RenderInfo.mri_ScreenWidth
		* (- (data->wd_ReqWidth + 200)) / 100;
	}
	else if (_between(MUIV_Window_Width_Visible(100),
			  data->wd_ReqWidth,
			  MUIV_Window_Width_Visible(0)))
	{
	    data->wd_Width = data->wd_RenderInfo.mri_ScreenWidth
		* (- (data->wd_ReqWidth + 100)) / 100;
	}

	if (data->wd_ReqHeight > 0)
	    data->wd_Height = data->wd_ReqHeight;
	else if (data->wd_ReqHeight == MUIV_Window_Height_Default)
	    data->wd_Height = data->wd_MinMax.DefHeight;
	else if (_between(MUIV_Window_Height_MinMax(100),
			  data->wd_ReqHeight,
			  MUIV_Window_Height_MinMax(0)))
	{
	    data->wd_Height = data->wd_MinMax.MinHeight
		- data->wd_ReqHeight
		* (data->wd_MinMax.MaxHeight - data->wd_MinMax.MinHeight);
	}
	else if (_between(MUIV_Window_Height_Screen(100),
			  data->wd_ReqHeight,
			  MUIV_Window_Height_Screen(0)))
	{
	    data->wd_Height = data->wd_RenderInfo.mri_ScreenHeight
		* (- (data->wd_ReqHeight + 200)) / 100;
	}
	else if (_between(MUIV_Window_Height_Visible(100),
			  data->wd_ReqHeight,
			  MUIV_Window_Height_Visible(0)))
	{
	    data->wd_Height = data->wd_RenderInfo.mri_ScreenHeight
		* (- (data->wd_ReqHeight + 100)) / 100;
	}

	/* scaled */
	if (data->wd_ReqWidth == MUIV_Window_Width_Scaled)
	    data->wd_Width = data->wd_Height * data->wd_MinMax.MinWidth
		/ data->wd_MinMax.MinHeight;
	else if (data->wd_ReqHeight == MUIV_Window_Width_Scaled)
	    data->wd_Height = data->wd_Width * data->wd_MinMax.MinHeight
		/ data->wd_MinMax.MinWidth;
    }
    data->wd_Width = CLAMP(data->wd_Width, data->wd_MinMax.MinWidth,
			   data->wd_MinMax.MaxWidth);
    data->wd_Height = CLAMP(data->wd_Height, data->wd_MinMax.MinHeight,
			    data->wd_MinMax.MaxHeight);
}

/******************************************************************************/
/* NEW                                                                        */
/******************************************************************************/

static ULONG
mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);
    data->wd_RenderInfo.mri_WindowObject = obj;

    InitMinList(&(data->wd_EHList));
    InitMinList(&(data->wd_CCList));

#ifdef __AROS__
    data->wd_CrtFlags = WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET
                      | WFLG_SIMPLE_REFRESH | WFLG_REPORTMOUSE | WFLG_NEWLOOKMENUS;
#else
    data->wd_CrtFlags = WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET;
#endif

    data->wd_Events = _zune_window_get_default_events();
    data->wd_ActiveObject = NULL;
    data->wd_ID = 0;
    data->wd_Title = "";
    data->wd_ReqHeight = MUIV_Window_Height_Default;
    data->wd_ReqWidth = MUIV_Window_Width_Default;
    data->wd_RootObject = NULL;
    data->wd_DefaultObject = NULL;
    data->wd_Flags = 0;
/* alternate dimensions */
/* no change in coordinates */
    data->wd_AltDim.Top = MUIV_Window_AltTopEdge_NoChange;
    data->wd_AltDim.Left = MUIV_Window_AltLeftEdge_NoChange;
/* default to min size */
    data->wd_AltDim.Width = MUIV_Window_AltWidth_MinMax(0);
    data->wd_AltDim.Height = MUIV_Window_AltHeight_MinMax(0);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Window_CloseGadget:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_CLOSEGADGET);
	    break;
	case MUIA_Window_SizeGadget:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_SIZEGADGET);
	    break;
	case MUIA_Window_Backdrop:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_BACKDROP);
	    break;
	case MUIA_Window_Borderless:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_BORDERLESS);
	    break;
	case MUIA_Window_DepthGadget:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_DEPTHGADGET);
	    break;
	case MUIA_Window_DragBar:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_DRAGBAR);
	    break;
	case MUIA_Window_SizeRight:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_SIZEBRIGHT);
	    break;
	case MUIA_Window_Height:
	    data->wd_ReqHeight = (LONG)tag->ti_Data;
	    break;
	case MUIA_Window_Width:
	    data->wd_ReqWidth = (LONG)tag->ti_Data;
	    break;
	case MUIA_Window_ID:
	    set(obj, MUIA_Window_ID, tag->ti_Data);
	    break;
	case MUIA_Window_Title:
	    set(obj, MUIA_Window_Title, tag->ti_Data);
	    break;
	case MUIA_Window_Activate:
	    set(obj, MUIA_Window_Activate, tag->ti_Data);
	    break;
	case MUIA_Window_DefaultObject:
	    set(obj, MUIA_Window_DefaultObject, tag->ti_Data);
	    break;
	case MUIA_Window_RootObject:
	    if (!tag->ti_Data)
	    {
		CoerceMethod(cl, obj, OM_DISPOSE);
		return 0;
	    }
	    set(obj, MUIA_Window_RootObject, tag->ti_Data);
	    break;
	case MUIA_Window_AltHeight:
	    data->wd_AltDim.Height = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AltWidth:
	    data->wd_AltDim.Width = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AltLeftEdge:
	    data->wd_AltDim.Left = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AltTopEdge:
	    data->wd_AltDim.Top = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AppWindow:
	    break;
	}
    }

    return (ULONG)obj;
}

/******************************************************************************/
/* DISPOSE                                                                    */
/******************************************************************************/

static ULONG
mDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_WindowData *data      = INST_DATA(cl, obj);

    if ((data->wd_Flags & MUIWF_OPENED))
    {
	g_warning("Window_DISPOSE: you must close me first !\n");
	set(obj, MUIA_Window_Open, FALSE);
    }
    if (data->wd_RootObject)
	MUI_DisposeObject(data->wd_RootObject);
    return DoSuperMethodA(cl, obj, msg);
}



/******************************************************************************/
/* SET                                                                        */
/******************************************************************************/
static ULONG
mSet(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Window_Activate:
		_handle_bool_tag(data->wd_Flags, tag->ti_Data, MUIWF_ACTIVE);
		break;
	    case MUIA_Window_ActiveObject:
		window_set_active_object(data, obj, tag->ti_Data);
		break;
	    case MUIA_Window_CloseRequest:
		_handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, MUIWF_CLOSEREQUESTED);
		break;
	    case MUIA_Window_DefaultObject:
		data->wd_DefaultObject = (APTR)tag->ti_Data;
		break;
	    case MUIA_Window_ID:
		data->wd_ID = tag->ti_Data;
		break;
	    case MUIA_Window_Open:
		if (tag->ti_Data && !(data->wd_Flags & MUIWF_OPENED))
		    window_Open(cl, obj);
		else if (!tag->ti_Data && (data->wd_Flags & MUIWF_OPENED))
		    window_Close(cl, obj);
		break;
	    case MUIA_Window_RootObject:
		window_change_root_object(data, obj, (Object *)tag->ti_Data);
		break;
	    case MUIA_Window_Title:
		data->wd_Title = (STRPTR)tag->ti_Data;
		break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/******************************************************************************/
/* GET                                                                        */
/******************************************************************************/

static ULONG mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_WindowData *data = INST_DATA(cl, obj);

    STORE = (ULONG)0;

    switch(msg->opg_AttrID)
    {
	case MUIA_Window_Activate:
	    STORE = (data->wd_Flags & MUIWF_ACTIVE) ? TRUE : FALSE;
	    return(TRUE);
	case MUIA_Window_ActiveObject:
	    if (data->wd_ActiveObject)
		STORE = (ULONG)data->wd_ActiveObject->data;
	    return(TRUE);
	case MUIA_Window_CloseRequest:
	    STORE = (data->wd_Flags & MUIWF_CLOSEREQUESTED) ? TRUE : FALSE;
	    return(TRUE);
	case MUIA_Window_DefaultObject:
	    STORE = (ULONG)data->wd_DefaultObject;
	    return(TRUE);
	case MUIA_Window_Height:
	    STORE = (ULONG)data->wd_Height;
	    return(TRUE);
	case MUIA_Window_ID:
	    STORE = data->wd_ID;
	    return(TRUE);
	case MUIA_Window_Open:
	    STORE = (data->wd_Flags & MUIWF_OPENED) ? TRUE : FALSE;
	    return(TRUE);
	case MUIA_Window_RootObject:
	    STORE = (ULONG)data->wd_RootObject;
	    return(TRUE);
	case MUIA_Window_Title:
	    STORE = (ULONG)data->wd_Title;
	    return(TRUE);
	case MUIA_Window_Width:
	    STORE = (ULONG)data->wd_Width;
	    return(TRUE);
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


/*
 * Called by Application (parent) object whenever this object is added.
 * init GlobalInfo
 */
static ULONG
mConnectParent(struct IClass *cl, Object *obj,
		     struct MUIP_ConnectParent *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
    Object                *parent = msg->parent;

    ASSERT(parent != NULL);
    ASSERT(muiNotifyData(parent)->mnd_GlobalInfo != NULL);

    muiNotifyData(obj)->mnd_GlobalInfo = muiNotifyData(parent)->mnd_GlobalInfo;
    if (data->wd_RootObject)
    {
#warning FIXME: mad_Background
#if 0
	if (muiAreaData(data->wd_RootObject)->mad_Background == NULL)
	    muiAreaData(data->wd_RootObject)->mad_Background =
		zune_imspec_copy(__zprefs.images[MUII_WindowBack]);
/*  	g_print("rootobj bg=%p\n", muiAreaData(data->wd_RootObject)->mad_Background); */
#endif
	DoMethod(data->wd_RootObject, MUIM_ConnectParentWindow,
		 _U(obj), _U(&data->wd_RenderInfo));
    }
    return TRUE;
}


/*
 * called by parent object
 */
static ULONG
mDisconnectParent(struct IClass *cl, Object *obj,
			struct MUIP_DisconnectParent *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    if (data->wd_RootObject)
	DoMethodA(data->wd_RootObject, (Msg)msg);

    muiNotifyData(obj)->mnd_GlobalInfo = NULL;

    return TRUE;
}

/*
 * Called before window is opened or resized. It determines its bounds,
 * so you can call window_select_dimensions() to find the final dims.
 */
static void
window_minmax (struct MUI_WindowData *data)
{
    /* inquire about sizes */
    DoMethod(data->wd_RootObject, MUIM_AskMinMax, (ULONG)&data->wd_MinMax);
    __area_finish_minmax(data->wd_RootObject, &data->wd_MinMax);

    data->wd_innerLeft   = __zprefs.window_inner_left;
    data->wd_innerRight  = __zprefs.window_inner_right;
    data->wd_innerTop    = __zprefs.window_inner_top;
    data->wd_innerBottom = __zprefs.window_inner_bottom;

    data->wd_MinMax.MinWidth += data->wd_innerLeft + data->wd_innerRight;
    data->wd_MinMax.MaxWidth += data->wd_innerLeft + data->wd_innerRight;
    data->wd_MinMax.DefWidth += data->wd_innerLeft + data->wd_innerRight;
    data->wd_MinMax.MinHeight += data->wd_innerTop + data->wd_innerBottom;
    data->wd_MinMax.MaxHeight += data->wd_innerTop + data->wd_innerBottom;
    data->wd_MinMax.DefHeight += data->wd_innerTop + data->wd_innerBottom;

/*      g_print("Window minmax: min=%dx%d, max=%dx%d, def=%dx%d\n (%dx%d)", */
/*  	    data->wd_MinMax.MinWidth, data->wd_MinMax.MinHeight, */
/*  	    data->wd_MinMax.MaxWidth, data->wd_MinMax.MaxHeight, */
/*  	    data->wd_MinMax.DefWidth, data->wd_MinMax.DefHeight, */
/*  	    data->wd_Width, data->wd_Height); */
}

/*
 * Called after window is opened or resized.
 * An expose event is already queued, it will trigger
 * MUIM_Draw for us when going back to main loop.
 */
static void
window_show (struct MUI_WindowData *data)
{
#ifdef __AROS__
    struct Window *win = data->wd_RenderInfo.mri_Window;
#endif

/*      int i; */

    _left(data->wd_RootObject) = data->wd_innerLeft + win->BorderLeft;
    _top(data->wd_RootObject)  = data->wd_innerTop  + win->BorderTop;
    _width(data->wd_RootObject) = data->wd_Width
	- (data->wd_innerLeft + data->wd_innerRight);
    _height(data->wd_RootObject) = data->wd_Height
	- (data->wd_innerBottom + data->wd_innerTop);

    DoMethod(data->wd_RootObject, MUIM_Layout);

kprintf("*** window_show() calling _zune_renderinfo_show()\n");
    _zune_renderinfo_show(&data->wd_RenderInfo);

/*  g_print("SHOW\n"); */
/*      for (i = 0; i < MUII_Count; i++) */
/*  	zune_imspec_show(__zprefs.images[i], NULL); */
    DoMethod(data->wd_RootObject, MUIM_Show);
}

static ULONG
window_Open(struct IClass *cl, Object *obj)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    if (!data->wd_RootObject)
	return FALSE;

/*  g_print("SETUP\n"); */
    if (!DoMethod(obj, MUIM_Window_Setup))
	return FALSE;
    /* I got display info, so calculate your display dependant data */
    if (!DoMethod(data->wd_RootObject, MUIM_Setup, _U(&data->wd_RenderInfo)))
    {
	DoMethod(obj, MUIM_Window_Cleanup);
	return FALSE;
    }
    /* no frame/inner space for root object ! */
    muiAreaData(data->wd_RootObject)->mad_Frame = 0;
    _addleft(data->wd_RootObject) = 0;
    _addtop(data->wd_RootObject) = 0;
    _subwidth(data->wd_RootObject) = 0;
    _subheight(data->wd_RootObject) = 0;

    /* inquire about sizes */
    window_minmax(data);
    window_select_dimensions(data);
#ifdef __AROS__
    data->wd_UserPort = muiGlobalInfo(obj)->mgi_UserPort;
#endif

    g_print("Window_Open: AskMinMax: min=%dx%d, max=%dx%d, def=%dx%d\n",
	    data->wd_MinMax.MinWidth, data->wd_MinMax.MinHeight,
	    data->wd_MinMax.MaxWidth, data->wd_MinMax.MaxHeight,
	    data->wd_MinMax.DefWidth, data->wd_MinMax.DefHeight);

    /* open window here ... */
    if (!_zune_window_open(data))
    {
	/* free display dependant data */
	DoMethod(data->wd_RootObject, MUIM_Cleanup);
	DoMethod(obj, MUIM_Window_Cleanup);
	return FALSE;
    }

    data->wd_Flags |= MUIWF_OPENED;

    window_show(data);

#ifdef __AROS__
    MUI_Redraw(data->wd_RootObject, MADF_DRAWALL);
#else
    /* expose event will trigger a MUIM_Draw */
#endif

    return TRUE;
}

/******************************************************************************/
/******************************************************************************/

static ULONG
window_Close(struct IClass *cl, Object *obj)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
/*      int i; */

/* remove from window */
/*  g_print("HIDE\n"); */
    DoMethod(data->wd_RootObject, MUIM_Hide);
/*      for (i = 0; i < MUII_Count; i++) */
/*  	zune_imspec_hide(__zprefs.images[i]); */
    _zune_renderinfo_hide(&data->wd_RenderInfo);

/* close here ... */
    _zune_window_close(data);
    data->wd_Flags &= ~MUIWF_OPENED;

/*  g_print("CLEANUP\n"); */
/* free display dependant data */
    DoMethod(data->wd_RootObject, MUIM_Cleanup);
    DoMethod(obj, MUIM_Window_Cleanup);

    return TRUE;
}

static ULONG
mRecalcDisplay(struct IClass *cl, Object *obj,
	             struct MUIP_Window_RecalcDisplay *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
/*      int i; */

/*  g_print("recalc\n"); */
/*  g_print("HIDE\n"); */
    DoMethod(data->wd_RootObject, MUIM_Hide);
/*      for (i = 0; i < MUII_Count; i++) */
/*  	zune_imspec_hide(__zprefs.images[i]); */
    _zune_renderinfo_hide(&data->wd_RenderInfo);

    /* inquire about sizes */
    window_minmax(data);
    /* resize window ? */
    window_select_dimensions(data);
    _zune_window_resize(data);
    window_show(data);
kprintf("Window->RecalcDisplay calling MUI_Redraw()...\n");
    MUI_Redraw(data->wd_RootObject, MADF_DRAWOBJECT);
    return TRUE;
}


/******************************************************************************/
/******************************************************************************/

static ULONG
mAddEventHandler(struct IClass *cl, Object *obj,
                 struct MUIP_Window_AddEventHandler *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

/*  g_print("add ehn\n"); */
    Enqueue((struct List *)&data->wd_EHList, (struct Node *)msg->ehnode);
    _zune_window_change_events(data);
    return TRUE;
}

/******************************************************************************/
/******************************************************************************/

static ULONG
mRemEventHandler(struct IClass *cl, Object *obj,
                 struct MUIP_Window_RemEventHandler *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    Remove((struct Node *)msg->ehnode);
    _zune_window_change_events(data);
    return TRUE;
}

/* Note that this is MUIM_Window_Setup, not MUIM_Setup */
static ULONG
mSetup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
/*      int i; */

    data->wd_ActiveObject = NULL;
    if (!_zune_renderinfo_setup(&data->wd_RenderInfo))
	return FALSE;

/*      for (i = 0; i < MUII_Count; i++) */
/*      { */
/*  	g_print("imspec_setup %d\n", i); */
/*  	zune_imspec_setup(&__zprefs.images[i], FALSE); */
/*      } */
    return TRUE;
}

static ULONG
mCleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
/*      int i; */

/*      for (i = 0; i < MUII_Count; i++) */
/*      { */
/*  	g_print("imspec_cleanup %d\n", i);	 */
/*  	zune_imspec_cleanup(&__zprefs.images[i], FALSE); */
/*      } */
    _zune_renderinfo_cleanup(&data->wd_RenderInfo);
    return TRUE;
}


static ULONG
mAddControlCharHandler(struct IClass *cl, Object *obj,
		 struct MUIP_Window_AddControlCharHandler *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    Enqueue((struct List *)&data->wd_CCList, (struct Node *)msg->ccnode);
    return TRUE;
}

static ULONG
mRemControlCharHandler(struct IClass *cl, Object *obj,
		 struct MUIP_Window_RemControlCharHandler *msg)
{
    Remove((struct Node *)msg->ccnode);
    return TRUE;
}

/******************************************************************************/
/******************************************************************************/

AROS_UFH3S(IPTR, Window_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW:
	    return(mNew(cl, obj, (struct opSet *) msg));
	case OM_DISPOSE:
	    return(mDispose(cl, obj, msg));
	case OM_SET:
	    return(mSet(cl, obj, (struct opSet *)msg));
	case OM_GET:
	    return(mGet(cl, obj, (struct opGet *)msg));
	case MUIM_Window_AddEventHandler :
	    return(mAddEventHandler(cl, obj, (APTR)msg));
	case MUIM_Window_RemEventHandler :
	    return(mRemEventHandler(cl, obj, (APTR)msg));
	case MUIM_ConnectParent :
	    return(mConnectParent(cl, obj, (APTR)msg));
	case MUIM_DisconnectParent :
	    return(mDisconnectParent(cl, obj, (APTR)msg));
	case MUIM_Window_RecalcDisplay :
	    return(mRecalcDisplay(cl, obj, (APTR)msg));
	case MUIM_Window_Setup :
	    return(mSetup(cl, obj, (APTR)msg));
	case MUIM_Window_Cleanup :
	    return(mCleanup(cl, obj, (APTR)msg));
	case MUIM_Window_AddControlCharHandler :
	    return(mAddControlCharHandler(cl, obj, (APTR)msg));
	case MUIM_Window_RemControlCharHandler :
	    return(mRemControlCharHandler(cl, obj, (APTR)msg));
    }

    return(DoSuperMethodA(cl, obj, msg));
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Window_desc = {
    MUIC_Window, 
    MUIC_Notify, 
    sizeof(struct MUI_WindowData), 
    Window_Dispatcher 
};

