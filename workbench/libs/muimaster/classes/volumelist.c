/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "volumelist_private.h"

extern struct Library *MUIMasterBase;

static APTR construct_func(struct Hook *hook, APTR pool, struct Volumelist_Entry *entry)
{
    struct Volumelist_Entry *new;
    
    if ((new = AllocPooled(pool, sizeof(*new))))
    {
    	*new = *entry;
    }
    return new;
}

static void destruct_func(struct Hook *hook, APTR pool, struct Volumelist_Entry *entry)
{
    FreePooled(pool, entry, sizeof(struct Volumelist_Entry));
}

static LONG display_func(struct Hook *hook, char **array, struct Volumelist_Entry *entry)
{
    /* MUI: logo | devicename | %-used | bytes free | bytes used */
    
    if (entry->type == DLT_DEVICE)
    {
    	*array++ = "\33I[6:24]";
    }
    else if (entry->type == DLT_VOLUME)
    {
    	*array++ = "\33I[6:26]";
    }
    else
    {
    	*array++ = "\33I[6:29]";
    }
    
    *array++ = entry->name;
    *array++ = "";
    *array++ = "";
    *array   = "";
    
    return 0;
}


IPTR Volumelist__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct DosList *dl, *actdl;
    
    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
	MUIA_List_Format, (IPTR)",,,,",
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
    if (obj)
    {
    	struct Volumelist_DATA *data = INST_DATA(cl, obj);
	
	data->construct_hook.h_Entry 	= HookEntry;
	data->construct_hook.h_SubEntry = (HOOKFUNC)construct_func;
	data->destruct_hook.h_Entry 	= HookEntry;
	data->destruct_hook.h_SubEntry  = (HOOKFUNC)destruct_func;
	data->display_hook.h_Entry  	= HookEntry;
	data->display_hook.h_SubEntry 	= (HOOKFUNC)display_func;
	
	SetAttrs(obj, MUIA_List_ConstructHook, (IPTR)&data->construct_hook,
	    	      MUIA_List_DestructHook,  (IPTR)&data->destruct_hook,
		      MUIA_List_DisplayHook, (IPTR)&data->display_hook,
		      TAG_DONE);
		      
	dl = LockDosList(LDF_READ | LDF_VOLUMES | LDF_ASSIGNS | LDF_DEVICES);
	
	actdl = dl;
	while((actdl = NextDosEntry(actdl, LDF_DEVICES)))
	{
	    struct Volumelist_Entry entry;
	    
	    entry.type = DLT_DEVICE;
	#ifdef __AROS__
	    strncpy(entry.name, actdl->dol_DevName, sizeof(entry.name));
	#else
	    #warning "FIXME: AmigaOS: get device name"
	    strncpy(entry.name, "???", sizeof(entry.name));
	#endif
	    entry.name[sizeof(entry.name) - 2] = '\0';
	    strcat(entry.name, ":");
	    
	    DoMethod(obj, MUIM_List_InsertSingle, (IPTR)&entry, MUIV_List_Insert_Bottom);
	}

	actdl = dl;
	while((actdl = NextDosEntry(actdl, LDF_VOLUMES)))
	{
	    struct Volumelist_Entry entry;
	    
	    entry.type = DLT_VOLUME;
	#ifdef __AROS__
	    strncpy(entry.name, actdl->dol_DevName, sizeof(entry.name));
	#else
	    #warning "FIXME: AmigaOS: get device name"
	    strncpy(entry.name, "???", sizeof(entry.name));
	#endif
	    entry.name[sizeof(entry.name) - 2] = '\0';
	    strcat(entry.name, ":");
	    
	    DoMethod(obj, MUIM_List_InsertSingle, (IPTR)&entry, MUIV_List_Insert_Bottom);
	}

	actdl = dl;
	while((actdl = NextDosEntry(actdl, LDF_ASSIGNS)))
	{
	    struct Volumelist_Entry entry;
	    
	    entry.type = DLT_DIRECTORY;
	#ifdef __AROS__
	    strncpy(entry.name, actdl->dol_DevName, sizeof(entry.name));
	#else
	    #warning "FIXME: AmigaOS: get assign name"
	    strncpy(entry.name, "???", sizeof(entry.name));
	#endif
	    entry.name[sizeof(entry.name) - 2] = '\0';
	    strcat(entry.name, ":");
	    
	    DoMethod(obj, MUIM_List_InsertSingle, (IPTR)&entry, MUIV_List_Insert_Bottom);
	}
	
	UnLockDosList(LDF_READ | LDF_VOLUMES | LDF_ASSIGNS | LDF_DEVICES);
    }
    
    return (IPTR)obj;
}


#if ZUNE_BUILTIN_VOLUMELIST
BOOPSI_DISPATCHER(IPTR, Volumelist_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Volumelist__OM_NEW(cl, obj, (struct opSet *)msg);
	
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Volumelist_desc =
{ 
    MUIC_Volumelist, 
    MUIC_List, 
    sizeof(struct Volumelist_DATA), 
    (void*)Volumelist_Dispatcher 
};
#endif /* ZUNE_BUILTIN_VOLUMELIST */
