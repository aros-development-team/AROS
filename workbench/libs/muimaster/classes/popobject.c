#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_PopobjectData
{
    int light;
    int vol;
    int follow;
    struct Hook *objstr_hook;
    struct Hook *strobj_hook;
    struct Hook *window_hook;
    struct Hook open_hook;
    struct Hook close_hook;
    Object *object;
    Object *wnd;
};

#ifndef _AROS
__asm ULONG Open_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 void **msg)
#else
AROS_UFH3(ULONG,Open_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    struct MUI_PopobjectData *data = (struct MUI_PopobjectData *)hook->h_Data;
    Object *string = (Object*)msg[0];

    if (!data->wnd)
    {
	data->wnd = WindowObject,
	    MUIA_Window_Borderless, TRUE,
	    MUIA_Window_CloseGadget, FALSE,
	    MUIA_Window_SizeGadget, FALSE,
	    MUIA_Window_DepthGadget, FALSE,
	    MUIA_Window_DragBar, FALSE,
	    WindowContents,data->object,
	    End;
	if (!data->wnd) return 0;
	DoMethod(_app(obj),OM_ADDMEMBER,data->wnd);
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
    return 1;
}


#ifndef _AROS
__asm ULONG Close_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 void **msg)
#else
AROS_UFH3(ULONG,Close_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    struct MUI_PopobjectData *data= (struct MUI_PopobjectData *)hook->h_Data;
    Object *string = (Object*)msg[0];
    LONG suc = (LONG)msg[1];

    if (data->wnd)
    {
	set(data->wnd,MUIA_Window_Open,FALSE);
	if (data->objstr_hook && suc)
	    CallHookPkt(data->objstr_hook,data->object,string);
    }
    return 0;
}


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popobject_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopobjectData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->follow = 1;
    data->vol = 1;
    data->light = 1;

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

    data->open_hook.h_Entry = (HOOKFUNC)Open_Function;
    data->open_hook.h_Data = data;
    data->close_hook.h_Entry = (HOOKFUNC)Close_Function;
    data->close_hook.h_Data = data;

    SetAttrs(obj,
	MUIA_Popstring_OpenHook, &data->open_hook,
	MUIA_Popstring_CloseHook, &data->close_hook,
	MUIA_Popstring_Toggle, TRUE,
	TAG_DONE);

    return (IPTR)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Popobject_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopobjectData *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

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

/**************************************************************************
 MUIM_Show
**************************************************************************/
static IPTR Popobject_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    ULONG rc = DoSuperMethodA(cl,obj,(Msg)msg);
    if (!rc) return 0;
    return rc;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Popobject_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

#ifndef _AROS
__asm IPTR Popobject_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Popobject_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Popobject_New(cl, obj, (struct opSet *)msg);
	case OM_SET: return Popobject_Set(cl, obj, (struct opSet *)msg);
	case MUIM_Show: return Popobject_Show(cl, obj, (struct MUIP_Show*)msg);
	case MUIM_Hide: return Popobject_Hide(cl, obj, (struct MUIP_Hide*)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popobject_desc = { 
    MUIC_Popobject, 
    MUIC_Popstring, 
    sizeof(struct MUI_PopobjectData), 
    (void*)Popobject_Dispatcher 
};
