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

struct MUI_PopimageData
{
    Object *wnd;
    Object *imageadjust;
    ULONG adjust_type;
    CONST_STRPTR wintitle;
};



/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popimage_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopimageData   *data;
    struct TagItem  	    *tag, *tags;
    //Object *image;

    obj = (Object *)DoSuperNew(cl, obj,
			       ButtonFrame,
			       InnerSpacing(4,4),
			       MUIA_Background, MUII_ButtonBack,
			       MUIA_InputMode, MUIV_InputMode_RelVerify,
			       TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->wintitle = NULL;
    data->adjust_type = MUIV_Imageadjust_Type_All;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Window_Title:
		data->wintitle = (CONST_STRPTR)tag->ti_Data;
		break;

	    case MUIA_Imageadjust_Type:
		data->adjust_type = tag->ti_Data;
		break;
	}
    }

    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1,
	     MUIM_Popimage_OpenWindow);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Popimage_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);

    if (data->wnd)
    {
	D(bug("Popimage_Dispose(%p) : MUI_DisposeObject(%p)\n", obj, data->wnd));
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);   
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Popimage_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
#if 0
    struct MUI_PopimageData *data = INST_DATA(cl, obj);
    if (data->wnd)
    {
	D(bug("Popimage_Hide(%p) : closing window %p\n", obj, data->wnd));
    	set(data->wnd,MUIA_Window_Open,FALSE);
	D(bug("Popimage_Hide(%p) : app REMMEMBER win (%p)\n", obj, data->wnd));
    	DoMethod(_app(obj),OM_REMMEMBER,(IPTR)data->wnd);
	D(bug("Popimage_Hide(%p) : MUI_DisposeObject(%p)\n", obj, data->wnd));
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
#endif
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/**************************************************************************
 MUIM_Popimage_OpenWindow
**************************************************************************/
STATIC IPTR Popimage_OpenWindow(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);

    if (!data->wnd)
    {
    	Object *ok_button, *cancel_button;
    	char *img_spec;
	ULONG x = 0, y = 0;

	get(_win(obj), MUIA_Window_LeftEdge, &x);
	get(_win(obj), MUIA_Window_TopEdge, &y);

	get(obj,MUIA_Imagedisplay_Spec, &img_spec);

    	data->wnd = WindowObject,
	  MUIA_Window_Title, (IPTR)data->wintitle,
          MUIA_Window_Activate, TRUE,
	    MUIA_Window_IsSubWindow, TRUE,
	    MUIA_Window_LeftEdge, _left(obj) + x,
	    MUIA_Window_TopEdge, _bottom(obj) + y + 1,
    	    WindowContents, VGroup,
		Child, data->imageadjust = MUI_NewObject(
		    MUIC_Imageadjust,
		    MUIA_CycleChain, 1,
		    MUIA_Imageadjust_Spec, img_spec,
		    MUIA_Imageadjust_Type, data->adjust_type,
		    TAG_DONE),
		Child, HGroup,
	            MUIA_Group_SameWidth, TRUE,
		    Child, ok_button = MUI_MakeObject(MUIO_Button,(IPTR)"_Ok"),
		    Child, HVSpace,
		    Child, HVSpace,
		    Child, cancel_button = MUI_MakeObject(MUIO_Button,(IPTR)"_Cancel"),
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
		     MUIM_Popimage_CloseWindow, TRUE);
	    DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popimage_CloseWindow, FALSE);
	    DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Popimage_CloseWindow, FALSE);
	}
    }

    if (data->wnd)
    {
	ULONG opened;

	set(data->wnd, MUIA_Window_Open,TRUE);
	get(data->wnd, MUIA_Window_Open, &opened);
	if (!opened)
	{
	    DoMethod(obj, MUIM_Popimage_CloseWindow, FALSE);
	}
    }

    return 1;
}


/**************************************************************************
 MUIM_Popimage_CloseWindow
**************************************************************************/
STATIC IPTR Popimage_CloseWindow(struct IClass *cl, Object *obj,
				 struct MUIP_Popimage_CloseWindow *msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);
    int ok = msg->ok;

    set(data->wnd, MUIA_Window_Open, FALSE);

    if (ok)
    {
	char *spec;
	get(data->imageadjust, MUIA_Imageadjust_Spec, &spec);
/*  	D(bug("popimage: got %s\n", spec)); */
	set(obj, MUIA_Imagedisplay_Spec, spec);
    }

    DoMethod(_app(obj), OM_REMMEMBER, (IPTR)data->wnd);
    MUI_DisposeObject(data->wnd);
    data->wnd = NULL;
    data->imageadjust = NULL;
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Popimage_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Popimage_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Popimage_Dispose(cl, obj, msg);
	case MUIM_Hide: return Popimage_Hide(cl, obj, (APTR)msg);
	case MUIM_Popimage_OpenWindow: return Popimage_OpenWindow(cl, obj, (APTR)msg);
	case MUIM_Popimage_CloseWindow: return Popimage_CloseWindow(cl, obj, (APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popimage_desc = { 
    MUIC_Popimage, 
    MUIC_Imagedisplay, 
    sizeof(struct MUI_PopimageData), 
    (void*)Popimage_Dispatcher 
};

