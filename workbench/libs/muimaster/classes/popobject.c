/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "debug.h"

extern struct Library *MUIMasterBase;

struct Popobject_DATA
{
    struct MUI_EventHandlerNode  ehn;
    int     	    	    	 light;
    int     	    	    	 vol;
    int     	    	    	 follow;
    struct Hook     	    	*strobj_hook;
    struct Hook     	    	*objstr_hook;
    struct Hook     	    	*window_hook;
    struct Hook     	    	 open_hook;
    struct Hook     	    	 close_hook;
    Object  	    	    	*object;
    Object  	    	    	*wnd;
};

AROS_UFH3(ULONG,Popobject_Open_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
{
    struct Popobject_DATA *data = (struct Popobject_DATA *)hook->h_Data;
    Object  	    	  *string = (Object*)msg[0];

    if (!data->wnd)
    {
    	static struct TagItem light_tags[] =
	{
	    {MUIA_Window_Borderless , TRUE  },
	    {MUIA_Window_CloseGadget, FALSE },
	    {MUIA_Window_SizeGadget , FALSE },
	    {MUIA_Window_DepthGadget, FALSE },
	    {MUIA_Window_DragBar    , FALSE },
	    {TAG_DONE	    	    	    }
	};
	
	data->wnd = WindowObject,
	    WindowContents,data->object,
	    data->light ? TAG_MORE : TAG_IGNORE, (IPTR)light_tags,
	    End;
	    
	if (!data->wnd) return 0;
	
	DoMethod(_app(obj),OM_ADDMEMBER,(IPTR)data->wnd);
	
	DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	    	 (IPTR)obj, 2, MUIM_Popstring_Close, FALSE);		 
    }

    if (data->strobj_hook)
    {
	if (!(CallHookPkt(data->strobj_hook,data->object,string)))
	    return 0;
    }

    if (data->window_hook)
    {
	CallHookPkt(data->strobj_hook,data->object,data->wnd);
    }

    SetAttrs(data->wnd,
	MUIA_Window_LeftEdge, _left(obj) + _window(obj)->LeftEdge,
	MUIA_Window_TopEdge, _bottom(obj)+1  + _window(obj)->TopEdge,
	MUIA_Window_Width, _width(obj),
    	MUIA_Window_Open, TRUE,
    	TAG_DONE);

    if (!(data->ehn.ehn_Events & IDCMP_CHANGEWINDOW))
    {
	data->ehn.ehn_Events |= IDCMP_CHANGEWINDOW;
        DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);	    
    }

    return 1;
}


AROS_UFH3(ULONG,Popobject_Close_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
{
    struct Popobject_DATA *data= (struct Popobject_DATA *)hook->h_Data;
    Object  	    	  *string = (Object*)msg[0];
    LONG    	    	   suc = (LONG)msg[1];

    if (data->wnd)
    {
	set(data->wnd,MUIA_Window_Open,FALSE);
	
	if (data->objstr_hook && suc)
	    CallHookPkt(data->objstr_hook,data->object,string);

	if (data->ehn.ehn_Events & IDCMP_CHANGEWINDOW)
	{
            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);	    
	    data->ehn.ehn_Events &= ~IDCMP_CHANGEWINDOW;
	}

    }
    return 0;
}

IPTR Popobject__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popobject_DATA   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->follow = 1;
    data->vol = 1;
    data->light = 1;

    data->ehn.ehn_Events   = 0;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    data->open_hook.h_Entry  = (HOOKFUNC)Popobject_Open_Function;
    data->open_hook.h_Data   = data;
    data->close_hook.h_Entry = (HOOKFUNC)Popobject_Close_Function;
    data->close_hook.h_Data  = data;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Popobject_Follow:
		    data->follow = tag->ti_Data;
		    break;

	    case    MUIA_Popobject_Light:
		    data->light = tag->ti_Data;
		    break;

	    case    MUIA_Popobject_Object:
		    data->object = (Object*)tag->ti_Data;
		    break;

	    case    MUIA_Popobject_ObjStrHook:
		    data->objstr_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_Popobject_StrObjHook:
		    data->strobj_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_Popobject_Volatile:
		    data->vol = tag->ti_Data;
		    break;

	    case    MUIA_Popobject_WindowHook:
		    data->window_hook = (struct Hook*)tag->ti_Data;
		    break;
    	}
    }

    SetAttrs(obj,
	MUIA_Popstring_OpenHook, (IPTR)&data->open_hook,
	MUIA_Popstring_CloseHook, (IPTR)&data->close_hook,
	MUIA_Popstring_Toggle, TRUE,
	TAG_DONE);

    return (IPTR)obj;
}

IPTR Popobject__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    if (!data->wnd && data->object)
	MUI_DisposeObject(data->object);
	
    return DoSuperMethodA(cl, obj, msg);
}

IPTR Popobject__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);
    struct TagItem  	  *tag, *tags;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Popobject_Follow:
		    data->follow = tag->ti_Data;
		    break;

	    case    MUIA_Popobject_ObjStrHook:
		    data->objstr_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_Popobject_StrObjHook:
		    data->strobj_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_Popobject_Volatile:
		    data->vol = tag->ti_Data;
		    break;

	    case    MUIA_Popobject_WindowHook:
		    data->window_hook = (struct Hook*)tag->ti_Data;
		    break;
    	}
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Popobject__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);
    
    if (data->ehn.ehn_Events & IDCMP_CHANGEWINDOW)
    {
	DoMethod(_win(obj),MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
	data->ehn.ehn_Events &= ~IDCMP_CHANGEWINDOW;    	
    }
    
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Popobject__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    IPTR rc = DoSuperMethodA(cl,obj,(Msg)msg);
    if (!rc) return 0;
    return rc;
}

IPTR Popobject__MUIM_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Popobject__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    if (data->follow && msg->imsg && data->wnd)
    {
    	if (msg->imsg->Class == IDCMP_CHANGEWINDOW)
	{
	    struct Window *popwin = NULL;
	    struct Window *parentwin = _window(obj);

	    get(data->wnd, MUIA_Window_Window, &popwin);
	    
	    if (popwin && parentwin)
	    {
	    	ChangeWindowBox(popwin, _left(obj) + parentwin->LeftEdge,
		    	    	    	_bottom(obj) + parentwin->TopEdge + 1,
					popwin->Width,
					popwin->Height);
	    }
	}
    }
    
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

BOOPSI_DISPATCHER(IPTR, Popobject_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:        	return Popobject__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE:    	return Popobject__OM_DISPOSE(cl, obj, msg);
	case OM_SET:        	return Popobject__OM_SET(cl, obj, (struct opSet *)msg);
	case MUIM_Cleanup:  	return Popobject__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	case MUIM_Show:     	return Popobject__MUIM_Show(cl, obj, (struct MUIP_Show*)msg);
	case MUIM_Hide:     	return Popobject__MUIM_Hide(cl, obj, (struct MUIP_Hide*)msg);
	case MUIM_HandleEvent:  return Popobject__MUIM_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

const struct __MUIBuiltinClass _MUI_Popobject_desc =
{ 
    MUIC_Popobject, 
    MUIC_Popstring, 
    sizeof(struct Popobject_DATA), 
    (void*)Popobject_Dispatcher 
};
