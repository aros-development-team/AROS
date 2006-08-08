/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "poppen_private.h"

extern struct Library *MUIMasterBase;


IPTR Poppen__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Poppen_DATA   *data;
    struct TagItem  	    *tag, *tags;
    //Object *image;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
			       ButtonFrame,
			       InnerSpacing(4,4),
			       MUIA_Background, MUII_ButtonBack,
			       MUIA_InputMode, MUIV_InputMode_RelVerify,
			       MUIA_Draggable,  TRUE,
			       TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->wintitle = NULL;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem**)&tags)); )
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

IPTR Poppen__MUIM_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);
    
    if (data->wnd)
    {
    	set(data->wnd,MUIA_Window_Open,FALSE);
    	DoMethod(_app(obj),OM_REMMEMBER,(IPTR)data->wnd);
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Poppen__MUIM_Poppen_OpenWindow(struct IClass *cl, Object *obj, Msg msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);

    if (!data->wnd)
    {
    	Object *ok_button, *cancel_button;
    	char *penspec;

	get(obj,MUIA_Pendisplay_Spec, &penspec);

    	data->wnd = WindowObject,
	  MUIA_Window_Title, (IPTR)data->wintitle,
          MUIA_Window_Activate, TRUE,
    	    WindowContents, (IPTR)VGroup,
		Child, (IPTR)(data->penadjust = PenadjustObject,
		    MUIA_CycleChain, 1,
		    MUIA_Penadjust_Spec, (IPTR)penspec,
		    End),
		Child, (IPTR)HGroup,
		    Child, (IPTR)(ok_button = MUI_MakeObject(MUIO_Button,(IPTR)"_Ok")),
		    Child, (IPTR)(cancel_button = MUI_MakeObject(MUIO_Button,(IPTR)"_Cancel")),
		    End,
		End,
	    End;

	if (data->wnd)
	{
	    set(ok_button, MUIA_CycleChain, 1);
	    set(cancel_button, MUIA_CycleChain, 1);

	    DoMethod(_app(obj),OM_ADDMEMBER,(IPTR)data->wnd);

	    DoMethod(ok_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 5,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Poppen_CloseWindow, TRUE);
	    DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 5,
		     MUIM_Application_PushMethod, (IPTR)obj, 2,
		     MUIM_Poppen_CloseWindow, FALSE);
	    DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)_app(obj), 5,
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

IPTR Poppen__MUIM_Poppen_CloseWindow(struct IClass *cl, Object *obj,
				 struct MUIP_Poppen_CloseWindow *msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);
    int ok = msg->ok;

    set(data->wnd, MUIA_Window_Open, FALSE);

    if (ok)
    {
	char *spec;
	
	get(data->penadjust, MUIA_Penadjust_Spec, &spec);
	
	set(obj, MUIA_Pendisplay_Spec, (IPTR)spec);
    }

    DoMethod(_app(obj), OM_REMMEMBER, (IPTR)data->wnd);
    MUI_DisposeObject(data->wnd);
    data->wnd = NULL;
    data->penadjust = NULL;
    return 1;
}

IPTR Poppen__MUIM_DisconnectParent(struct IClass *cl, Object *obj,
				   struct MUIP_DisconnectParent *msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);

    if (data->wnd) DoMethod(obj, MUIM_Poppen_CloseWindow, FALSE);
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

#if ZUNE_BUILTIN_POPPEN
BOOPSI_DISPATCHER(IPTR, Poppen_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:                  return Poppen__OM_NEW(cl, obj, (struct opSet *)msg);
	case MUIM_Hide:               return Poppen__MUIM_Hide(cl, obj, (APTR)msg);
	case MUIM_Poppen_OpenWindow:  return Poppen__MUIM_Poppen_OpenWindow(cl, obj, (APTR)msg);
	case MUIM_Poppen_CloseWindow: return Poppen__MUIM_Poppen_CloseWindow(cl, obj, (APTR)msg);
	case MUIM_DisconnectParent:   return Poppen__MUIM_DisconnectParent(cl, obj, (APTR)msg);
        default:                      return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Poppen_desc =
{ 
    MUIC_Poppen, 
    MUIC_Pendisplay, 
    sizeof(struct Poppen_DATA), 
    (void*)Poppen_Dispatcher 
};
#endif /* ZUNE_BUILTIN_POPPEN */
