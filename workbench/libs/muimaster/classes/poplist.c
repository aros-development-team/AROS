/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "debug.h"
#include "poplist_private.h"

extern struct Library *MUIMasterBase;

LONG PoplistStrObjFunc(struct Hook *hook, Object *popup, Object *str)
{
    struct Poplist_DATA *data = (struct Poplist_DATA *)hook->h_Data;
    STRPTR  	    	 strtext, listentry;
    LONG    	    	 index;
    
    get(str, MUIA_String_Contents, &strtext);
    
    for(index = 0; ; index++)
    {
    	DoMethod(data->list, MUIM_List_GetEntry, index, (IPTR)&listentry);
	
	if (!listentry)
	{
	    set(data->list, MUIA_List_Active, MUIV_List_Active_Off);
	    break;
	}
	
	if (stricmp(strtext, listentry) == 0)
	{
	    set(data->list, MUIA_List_Active, index);
	    break;
	}
    }
    
    return TRUE;
}

void PoplistObjStrFunc(struct Hook *hook, Object *popup, Object *str)
{
    STRPTR listentry;
    
    DoMethod(popup, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&listentry);

    if (listentry)
    {
    	set(str, MUIA_String_Contents, listentry);
    }  
}

IPTR Poplist__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    STRPTR array = (STRPTR)GetTagData(MUIA_Poplist_Array, 0, msg->ops_AttrList);
    Object *lv, *list;
    
    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
	MUIA_Popobject_Object, (IPTR)lv = ListviewObject,
	    MUIA_Listview_List, (IPTR)list = ListObject,
        	InputListFrame,
	    	array ? MUIA_List_SourceArray : TAG_IGNORE, (IPTR)array,
	    	End,
	    End,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
    if (obj)
    {
    	struct Poplist_DATA *data = INST_DATA(cl, obj);
	
	data->list = list;
	
	data->strobj_hook.h_Entry = HookEntry;
	data->strobj_hook.h_SubEntry = (HOOKFUNC)PoplistStrObjFunc;
	data->strobj_hook.h_Data = data;
	
	data->objstr_hook.h_Entry = HookEntry;
	data->objstr_hook.h_SubEntry = (HOOKFUNC)PoplistObjStrFunc;
	data->objstr_hook.h_Data = data;
	
	SetAttrs(obj, MUIA_Popobject_StrObjHook, (IPTR)&data->strobj_hook,
	    	      MUIA_Popobject_ObjStrHook, (IPTR)&data->objstr_hook,
		      TAG_DONE);
		     
	DoMethod(lv, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
	    	 (IPTR)obj, 2, MUIM_Popstring_Close, TRUE); 
    }
    
    return (IPTR)obj;
}

#if ZUNE_BUILTIN_POPLIST
BOOPSI_DISPATCHER(IPTR, Poplist_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Poplist__OM_NEW(cl, obj, (struct opSet *)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Poplist_desc =
{ 
    MUIC_Poplist, 
    MUIC_Popobject, 
    sizeof(struct Poplist_DATA), 
    (void*)Poplist_Dispatcher 
};
#endif /* ZUNE_BUILTIN_POPLIST */
