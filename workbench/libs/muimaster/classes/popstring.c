/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
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

extern struct Library *MUIMasterBase;

struct MUI_PopstringData
{
    struct Hook *close_hook;
    struct Hook *open_hook;
    Object *string;
    Object *button;
    int open;
    int toggle;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popstring_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopstringData   *data;
    struct TagItem  	    *tag, *tags;
    Object *string, *button;

    button = (Object*)GetTagData(MUIA_Popstring_Button,NULL,msg->ops_AttrList);
    string = (Object*)GetTagData(MUIA_Popstring_String,NULL,msg->ops_AttrList);
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        MUIA_Group_Horiz,          TRUE,
        MUIA_Group_Spacing,        0,
        Child,              (IPTR) string,
        Child,              (IPTR) button,
        
        TAG_MORE,           (IPTR) msg->ops_AttrList
    );
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->button = button;
    data->string = string;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
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

/**************************************************************************
 OM_SET
**************************************************************************/
STATIC IPTR Popstring_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct MUI_PopstringData *data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
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


/**************************************************************************
 MUIM_Popstring_Close
**************************************************************************/
static IPTR Popstring_Close(struct IClass *cl, Object *obj, struct MUIP_Popstring_Close *msg)
{
    struct MUI_PopstringData *data = INST_DATA(cl, obj);
    if (data->close_hook && data->open)
    {
    	DoMethod(_app(obj), MUIM_Application_PushMethod, (IPTR)obj, 4, MUIM_CallHook, (IPTR)data->close_hook, (IPTR)data->string, msg->result);
	data->open = 0;
    	set(data->button,MUIA_Disabled, FALSE);
    }
    return 0;
}

/**************************************************************************
 MUIM_Popstring_Open
**************************************************************************/
static IPTR Popstring_Open(struct IClass *cl, Object *obj, struct MUIP_Popstring_Open *msg)
{
    struct MUI_PopstringData *data = INST_DATA(cl, obj);
    if (data->open_hook)
    {
	if (data->open && data->toggle)
	{
	    DoMethod(obj,MUIM_Popstring_Close);
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
	case OM_NEW: return Popstring_New(cl, obj, (struct opSet *)msg);
	case OM_SET: return Popstring_Set(cl, obj, (struct opSet *)msg);
	case MUIM_Popstring_Close: return Popstring_Close(cl, obj, (struct MUIP_Popstring_Close*)msg);
	case MUIM_Popstring_Open: return Popstring_Open(cl, obj, (struct MUIP_Popstring_Open*)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popstring_desc = { 
    MUIC_Popstring, 
    MUIC_Group, 
    sizeof(struct MUI_PopstringData), 
    (void*)Popstring_Dispatcher 
};

