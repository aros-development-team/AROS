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

#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_PoppenData
{
    Object *wnd;
    Object *penadjust;
    CONST_STRPTR wintitle;
};



/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Poppen_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PoppenData   *data;
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
	     MUIM_Poppen_OpenWindow);

    return (IPTR)obj;
}


/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Poppen_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PoppenData *data = INST_DATA(cl, obj);
    
    if (data->wnd)
    {
    	set(data->wnd,MUIA_Window_Open,FALSE);
    	DoMethod(_app(obj),OM_REMMEMBER,(IPTR)data->wnd);
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/**************************************************************************
 MUIM_Poppen_OpenWindow
**************************************************************************/
STATIC IPTR Poppen_OpenWindow(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PoppenData *data = INST_DATA(cl, obj);

    if (!data->wnd)
    {
    	Object *ok_button, *cancel_button;
    	char *penspec;

	get(obj,MUIA_Pendisplay_Spec, &penspec);

    	data->wnd = WindowObject,
	  MUIA_Window_Title, (IPTR)data->wintitle,
          MUIA_Window_Activate, TRUE,
    	    WindowContents, VGroup,
		Child, data->penadjust = PenadjustObject,
		    MUIA_CycleChain, 1,
		    MUIA_Penadjust_Spec, penspec,
		    TAG_DONE),
		Child, HGroup,
		    Child, ok_button = MUI_MakeObject(MUIO_Button,"_Ok"),
		    Child, cancel_button = MUI_MakeObject(MUIO_Button,"_Cancel"),
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
		     MUIM_Poppen_CloseWindow, TRUE);
	    DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Poppen_CloseWindow, FALSE);
	    DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)_app(obj), 6,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Poppen_CloseWindow, FALSE);
	}
    }

    if (data->wnd)
    {
	IPTR opened;

	set(data->wnd, MUIA_Window_Open,TRUE);
	get(data->wnd, MUIA_Window_Open, &opened);

	if (!opened)
	{
	    DoMethod(obj, MUIM_Poppen_CloseWindow, FALSE);
	}
    }

    return 1;
}


/**************************************************************************
 MUIM_Poppen_CloseWindow
**************************************************************************/
STATIC IPTR Poppen_CloseWindow(struct IClass *cl, Object *obj,
				 struct MUIP_Poppen_CloseWindow *msg)
{
    struct MUI_PoppenData *data = INST_DATA(cl, obj);
    int ok = msg->ok;

    set(data->wnd, MUIA_Window_Open, FALSE);

    if (ok)
    {
	char *spec;
	
	get(data->penadjust, MUIA_Penadjust_Spec, &spec);
	
	set(obj, MUIA_Pendisplay_Spec, spec);
    }

    DoMethod(_app(obj), OM_REMMEMBER, (IPTR)data->wnd);
    MUI_DisposeObject(data->wnd);
    data->wnd = NULL;
    data->penadjust = NULL;
    return 1;
}


BOOPSI_DISPATCHER(IPTR, Poppen_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Poppen_New(cl, obj, (struct opSet *)msg);
	case MUIM_Hide: return Poppen_Hide(cl, obj, (APTR)msg);
	case MUIM_Poppen_OpenWindow: return Poppen_OpenWindow(cl, obj, (APTR)msg);
	case MUIM_Poppen_CloseWindow: return Poppen_CloseWindow(cl, obj, (APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Poppen_desc = { 
    MUIC_Poppen, 
    MUIC_Pendisplay, 
    sizeof(struct MUI_PoppenData), 
    (void*)Poppen_Dispatcher 
};

