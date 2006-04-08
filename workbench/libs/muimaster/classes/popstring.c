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

/*  #define MYDEBUG 0 */
#include "debug.h"

extern struct Library *MUIMasterBase;

struct Popstring_DATA
{
    struct Hook *close_hook;
    struct Hook *open_hook;
    Object *string;
    Object *button;
    int open;
    int toggle;
    struct MUI_EventHandlerNode ehn;
};


IPTR Popstring__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popstring_DATA   *data;
    struct TagItem  	    *tag, *tags;
    Object *string, *button;

    button = (Object*)GetTagData(MUIA_Popstring_Button,0,msg->ops_AttrList);
    string = (Object*)GetTagData(MUIA_Popstring_String,0,msg->ops_AttrList);
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, 0,
        
        MUIA_Group_Horiz,          TRUE,
        MUIA_Group_Spacing,        0,
        (string ? Child : TAG_IGNORE), (IPTR) string,
        Child,              (IPTR) button,
        
        TAG_MORE,           (IPTR) msg->ops_AttrList
    );
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->button = button;
    data->string = string;

    data->ehn.ehn_Events   = IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Popstring_Toggle: data->toggle = tag->ti_Data; break;
	    case MUIA_Popstring_CloseHook: data->close_hook = (struct Hook*)tag->ti_Data;break;
	    case MUIA_Popstring_OpenHook: data->open_hook = (struct Hook*)tag->ti_Data;break;
    	}
    }

    DoMethod(button,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,1,MUIM_Popstring_Open);

    return (IPTR)obj;
}

IPTR Popstring__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Popstring_Toggle: data->toggle = tag->ti_Data; break;
	    case MUIA_Popstring_CloseHook: data->close_hook = (struct Hook*)tag->ti_Data;break;
	    case MUIA_Popstring_OpenHook: data->open_hook = (struct Hook*)tag->ti_Data;break;
 	}
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

#define STORE *(msg->opg_Storage)
IPTR Popstring__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
    	case MUIA_Popstring_String: STORE = (IPTR)data->string; return 1;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}
#undef STORE

IPTR Popstring__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    return TRUE;
}

IPTR Popstring__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Popstring__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    if (msg->muikey == MUIKEY_POPUP)
    {
	D(bug("Popstring__MUIM_HandleEvent %p got MUIKEY_POPUP\n", obj));
	DoMethod(obj, MUIM_Popstring_Open);
	return MUI_EventHandlerRC_Eat;
    }
    return 0;
}

IPTR Popstring__MUIM_Popstring_Close(struct IClass *cl, Object *obj, struct MUIP_Popstring_Close *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);
    if (data->close_hook && data->open)
    {
    	DoMethod(_app(obj), MUIM_Application_PushMethod, (IPTR)obj, 4, MUIM_CallHook, (IPTR)data->close_hook, (IPTR)data->string, msg->result);
	data->open = 0;
    	set(data->button,MUIA_Disabled, FALSE);
    }
    return 0;
}

IPTR Popstring__MUIM_Popstring_Open(struct IClass *cl, Object *obj, struct MUIP_Popstring_Open *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);
    if (data->open_hook)
    {
	if (data->open && data->toggle)
	{
	    DoMethod(obj,MUIM_Popstring_Close,FALSE);
	    return 0;
	}

	if (!data->open)
	{
	    if (DoMethod(obj, MUIM_CallHook, (IPTR)data->open_hook, (IPTR)data->string))
	    {
		/* Opening the popup window was successful */
		data->open = 1;
		if (!data->toggle) set(data->button,MUIA_Disabled, TRUE);
	    }
	}
    }
    return 0;
}


BOOPSI_DISPATCHER(IPTR, Popstring_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Popstring__OM_NEW(cl, obj, (APTR)msg);
	case OM_SET:
	    return Popstring__OM_SET(cl, obj, (APTR)msg);
	case OM_GET:
	    return Popstring__OM_GET(cl, obj, (APTR)msg);
        case MUIM_Setup:
	    return Popstring__MUIM_Setup(cl, obj, (APTR)msg);
        case MUIM_Cleanup:
	    return Popstring__MUIM_Cleanup(cl, obj, (APTR)msg);
	case MUIM_HandleEvent:
	    return Popstring__MUIM_HandleEvent(cl, obj, (APTR)msg);
	case MUIM_Popstring_Close:
	    return Popstring__MUIM_Popstring_Close(cl, obj, (APTR)msg);
	case MUIM_Popstring_Open:
	    return Popstring__MUIM_Popstring_Open(cl, obj, (APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Popstring_desc =
{ 
    MUIC_Popstring, 
    MUIC_Group, 
    sizeof(struct Popstring_DATA), 
    (void*)Popstring_Dispatcher 
};
