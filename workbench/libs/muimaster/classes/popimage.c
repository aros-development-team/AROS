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

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_PopimageData
{
    struct Hook press_hook;
    struct Hook close_hook;

    Object *wnd;
    Object *bitmap_string;
};


#ifndef _AROS
static __asm VOID Close_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 void **msg)
#else
AROS_UFH3(VOID,Close_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    struct MUI_PopimageData *data = (struct MUI_PopimageData *)hook->h_Data;
    int ok = (int)msg[0];

    set(data->wnd,MUIA_Window_Open,FALSE);
}

#ifndef _AROS
static __asm VOID Press_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 void **msg)
#else
AROS_UFH3(VOID,Press_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    struct MUI_PopimageData *data = (struct MUI_PopimageData *)hook->h_Data;
    if (!data->wnd)
    {
    	Object *ok_button, *cancel_button;

    	data->wnd = WindowObject,
    	    WindowContents, VGroup,
		Child, MUI_NewObject(MUIC_Imageadjust, TAG_DONE),
		Child, HGroup,
		    Child, ok_button = MUI_MakeObject(MUIO_Button,"_Ok"),
		    Child, cancel_button = MUI_MakeObject(MUIO_Button,"_Cancel"),
		    End,
		End,
	    End;

	if (data->wnd)
	{
	    DoMethod(_app(obj),OM_ADDMEMBER,data->wnd);

	    DoMethod(ok_button,MUIM_Notify,MUIA_Pressed,FALSE,obj,3,MUIM_CallHook,&data->close_hook,1);
	    DoMethod(cancel_button,MUIM_Notify,MUIA_Pressed,FALSE,obj,3,MUIM_CallHook,&data->close_hook,0);
	    DoMethod(data->wnd,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,obj,3,MUIM_CallHook,&data->close_hook,0);
	}
    }
    if (data->wnd)
    {
	set(data->wnd,MUIA_Window_Open,TRUE);
    }
}


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popimage_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopimageData   *data;
    struct TagItem  	    *tag, *tags;
    Object *image;

    obj = (Object *)DoSuperNew(cl, obj,
			ButtonFrame,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->press_hook.h_Data = data;
    data->press_hook.h_Entry = (HOOKFUNC)Press_Function;

    data->close_hook.h_Data = data;
    data->close_hook.h_Entry = (HOOKFUNC)Close_Function;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    DoMethod(obj,MUIM_Notify,MUIA_Pressed,FALSE,obj,2,MUIM_CallHook,&data->press_hook);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
STATIC IPTR Popimage_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    DoSuperMethodA(cl,obj,msg);
    return 0;
}

/**************************************************************************
 OM_SET
**************************************************************************/
STATIC IPTR Popimage_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct MUI_PopimageData *data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
 	}
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Popimage_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
}


/**************************************************************************
 MUIM_Hide
**************************************************************************/
static IPTR Popimage_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PopimageData *data = INST_DATA(cl, obj);
    if (data->wnd)
    {
    	set(data->wnd,MUIA_Window_Open,FALSE);
    	DoMethod(_app(obj),OM_REMMEMBER,data->wnd);
    	MUI_DisposeObject(data->wnd);
    	data->wnd = NULL;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}



#ifndef _AROS
__asm IPTR Popimage_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Popimage_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Popimage_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Popimage_Dispose(cl,obj,(APTR)msg);
	case OM_SET: return Popimage_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Popimage_Get(cl,obj,(APTR)msg);
	case MUIM_Hide: return Popimage_Hide(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popimage_desc = { 
    MUIC_Popimage, 
    MUIC_Image, 
    sizeof(struct MUI_PopimageData), 
    (void*)Popimage_Dispatcher 
};

