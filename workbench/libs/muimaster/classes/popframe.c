/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_PopframeData
{
    Object *wnd;
    Object *frameadjust;
    CONST_STRPTR wintitle;
};



/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popframe_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopframeData   *data;
    struct TagItem  	    *tag, *tags;
    //Object *frame;

    obj = (Object *)DoSuperNew(cl, obj,
			       ButtonFrame,
			       InnerSpacing(4,4),
			       MUIA_Background, MUII_ButtonBack,
			       MUIA_InputMode, MUIV_InputMode_RelVerify,
			       TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->wintitle = NULL;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Window_Title:
		data->wintitle = (CONST_STRPTR)tag->ti_Data;
		break;
	}
    }

    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1,
	     MUIM_Popframe_OpenWindow);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Popframe_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PopframeData *data = INST_DATA(cl, obj);

    if (data->wnd)
    {
	D(bug("Popframe_Dispose(%p) : MUI_DisposeObject(%p)\n", obj, data->wnd));
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);   
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Popframe_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
#if 0
    struct MUI_PopframeData *data = INST_DATA(cl, obj);
    if (data->wnd)
    {
	D(bug("Popframe_Hide(%p) : closing window %p\n", obj, data->wnd));
    	set(data->wnd,MUIA_Window_Open,FALSE);
	D(bug("Popframe_Hide(%p) : app REMMEMBER win (%p)\n", obj, data->wnd));
    	DoMethod(_app(obj),OM_REMMEMBER,(IPTR)data->wnd);
	D(bug("Popframe_Hide(%p) : MUI_DisposeObject(%p)\n", obj, data->wnd));
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
#endif
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/**************************************************************************
 MUIM_Popframe_OpenWindow
**************************************************************************/
STATIC IPTR Popframe_OpenWindow(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PopframeData *data = INST_DATA(cl, obj);

    if (!data->wnd)
    {
    	Object *ok_button, *cancel_button;
    	struct MUI_Frame_Spec *frame_spec;
	ULONG x = 0, y = 0;

	get(_win(obj), MUIA_Window_LeftEdge, &x);
	get(_win(obj), MUIA_Window_TopEdge, &y);

	get(obj, MUIA_Framedisplay_Spec, &frame_spec);

    	data->wnd = WindowObject,
	  MUIA_Window_Title, (IPTR)data->wintitle,
          MUIA_Window_Activate, TRUE,
	    MUIA_Window_IsSubWindow, TRUE,
	    MUIA_Window_LeftEdge, _left(obj) + x,
	    MUIA_Window_TopEdge, _bottom(obj) + y + 1,
    	    WindowContents, VGroup,
		Child, data->frameadjust = MUI_NewObject(
		    MUIC_Frameadjust,
		    MUIA_CycleChain, 1,
		    MUIA_Frameadjust_Spec, frame_spec,
		    TAG_DONE),
		Child, HGroup,
	            MUIA_Group_SameWidth, TRUE,
		    Child, ok_button = MUI_MakeObject(MUIO_Button, (IPTR)"_Ok"),
		    Child, HVSpace,
		    Child, HVSpace,
		    Child, cancel_button = MUI_MakeObject(MUIO_Button, (IPTR)"_Cancel"),
		    End,
		End,
	    End;

	if (data->wnd)
	{
	    set(ok_button, MUIA_CycleChain, 1);
	    set(cancel_button, MUIA_CycleChain, 1);

	    DoMethod(_app(obj),OM_ADDMEMBER,(IPTR)data->wnd);

	    DoMethod(ok_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popframe_CloseWindow, TRUE);
	    DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popframe_CloseWindow, FALSE);
	    DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popframe_CloseWindow, FALSE);
	}
    }

    if (data->wnd)
    {
	ULONG opened;

	set(data->wnd, MUIA_Window_Open,TRUE);
	get(data->wnd, MUIA_Window_Open, &opened);
	if (!opened)
	{
	    DoMethod(obj, MUIM_Popframe_CloseWindow, FALSE);
	}
    }

    return 1;
}


/**************************************************************************
 MUIM_Popframe_CloseWindow
**************************************************************************/
STATIC IPTR Popframe_CloseWindow(struct IClass *cl, Object *obj,
				 struct MUIP_Popframe_CloseWindow *msg)
{
    struct MUI_PopframeData *data = INST_DATA(cl, obj);
    int ok = msg->ok;

    set(data->wnd, MUIA_Window_Open, FALSE);

    if (ok)
    {
	STRPTR spec;
	get(data->frameadjust, MUIA_Frameadjust_Spec, &spec);
/*  	D(bug("popframe: got %s\n", spec)); */
	set(obj, MUIA_Framedisplay_Spec, (IPTR)spec);
    }

    DoMethod(_app(obj), OM_REMMEMBER, (IPTR)data->wnd);
    MUI_DisposeObject(data->wnd);
    data->wnd = NULL;
    data->frameadjust = NULL;
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Popframe_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Popframe_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Popframe_Dispose(cl, obj, msg);
	case MUIM_Hide: return Popframe_Hide(cl, obj, (APTR)msg);
	case MUIM_Popframe_OpenWindow: return Popframe_OpenWindow(cl, obj, (APTR)msg);
	case MUIM_Popframe_CloseWindow: return Popframe_CloseWindow(cl, obj, (APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popframe_desc = { 
    MUIC_Popframe, 
    MUIC_Framedisplay, 
    sizeof(struct MUI_PopframeData), 
    (void*)Popframe_Dispatcher 
};

