/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "dirlist_private.h"

extern struct Library *MUIMasterBase;

static APTR construct_func(struct Hook *hook, APTR pool, struct Dirlist_Entry *entry)
{
    struct Dirlist_Entry *new;
    
    if ((new = AllocPooled(pool, sizeof(*new))))
    {
    	*new = *entry;
    }
    return new;
}

static void destruct_func(struct Hook *hook, APTR pool, struct Dirlist_Entry *entry)
{
    FreePooled(pool, entry, sizeof(struct Dirlist_Entry));
}

static LONG display_func(struct Hook *hook, char **array, struct Dirlist_Entry *entry)
{
    struct Dirlist_DATA *data = hook->h_Data;
    struct DateTime	 dt;
    
    /* MUI: name | size | Date  | Time  | Protection | Comment */
    
    *array++ = entry->fib.fib_FileName;
    
    if (entry->fib.fib_DirEntryType > 0)
    {
    	*array++ = "\33r\33I[6:22]";
    }
    else
    {
    	snprintf(data->size_string, sizeof(data->size_string), "\33r%ld", entry->fib.fib_Size);
    	*array++ = data->size_string;
    }
    
    dt.dat_Stamp   = entry->fib.fib_Date;
    dt.dat_Format  = FORMAT_DOS;
    dt.dat_Flags   = 0;
    dt.dat_StrDay  = NULL;
    dt.dat_StrDate = data->date_string;
    dt.dat_StrTime = data->time_string;
    
    DateToStr(&dt);
    
    *array++ = data->date_string;
    *array++ = data->time_string;
    
    data->prot_string[0] = (entry->fib.fib_Protection & FIBF_SCRIPT)  ? 's' : '-';
    data->prot_string[1] = (entry->fib.fib_Protection & FIBF_PURE)    ? 'p' : '-';
    data->prot_string[2] = (entry->fib.fib_Protection & FIBF_ARCHIVE) ? 'a' : '-';
    data->prot_string[3] = (entry->fib.fib_Protection & FIBF_READ)    ? '-' : 'r';
    data->prot_string[4] = (entry->fib.fib_Protection & FIBF_WRITE)   ? '-' : 'w';
    data->prot_string[5] = (entry->fib.fib_Protection & FIBF_EXECUTE) ? '-' : 'e';
    data->prot_string[6] = (entry->fib.fib_Protection & FIBF_DELETE ) ? '-' : 'd';
    data->prot_string[7] = '\0';
    
    *array++ = data->prot_string;
    *array   = entry->fib.fib_Comment;
    
    return 0;
}

IPTR Dirlist__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg);

IPTR Dirlist__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
	MUIA_List_Format, (IPTR)",,,,,",
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
    if (obj)
    {
    	struct Dirlist_DATA *data = INST_DATA(cl, obj);
	
	data->status = MUIV_Dirlist_Status_Invalid;
	
	data->construct_hook.h_Entry 	= HookEntry;
	data->construct_hook.h_SubEntry = (HOOKFUNC)construct_func;
	data->destruct_hook.h_Entry 	= HookEntry;
	data->destruct_hook.h_SubEntry  = (HOOKFUNC)destruct_func;
	data->display_hook.h_Entry  	= HookEntry;
	data->display_hook.h_SubEntry 	= (HOOKFUNC)display_func;
	data->display_hook.h_Data   	= data;
	
	SetAttrs(obj, MUIA_List_ConstructHook, (IPTR)&data->construct_hook,
	    	      MUIA_List_DestructHook,  (IPTR)&data->destruct_hook,
		      MUIA_List_DisplayHook, (IPTR)&data->display_hook,
		      TAG_DONE);
		      
    	Dirlist__OM_SET(cl, obj, msg);
    }
    
    return (IPTR)obj;
}

static void ReadDirectory(Object *obj, struct Dirlist_DATA *data)
{
    struct FileInfoBlock *fib;
    BPTR    	    	  lock;
    
    if ((fib = AllocDosObject(DOS_FIB, NULL)))
    {
	if ((lock = Lock(data->directory, SHARED_LOCK)))
	{
    	    BOOL success, isdir;
	    
	    success = Examine(lock, fib);
	    isdir = (fib->fib_DirEntryType > 0);
	    
	    if (success && (fib->fib_DirEntryType > 0))
	    {
	    	for(;;)
		{		    
		    success = ExNext(lock, fib);
		    
    	    	    if (!success)
		    {
		    	if (IoErr() == ERROR_NO_MORE_ENTRIES) break;
			success = FALSE;
			break;
		    }
		    
		    if (data->filterhook)
		    {
		    	struct ExAllData ead = {0};
			
			ead.ed_Name    = fib->fib_FileName;
			ead.ed_Type    = fib->fib_DirEntryType;
			ead.ed_Size    = fib->fib_Size;
			ead.ed_Prot    = fib->fib_Protection;
			ead.ed_Days    = fib->fib_Date.ds_Days;
			ead.ed_Mins    = fib->fib_Date.ds_Minute;
			ead.ed_Ticks   = fib->fib_Date.ds_Tick;
			ead.ed_Comment = fib->fib_Comment;
			
			if (!CallHookPkt(data->filterhook, obj, &ead)) continue;
		    }
    	    	    else
		    {
			if (isdir && data->filesonly) continue;
			if (!isdir && data->drawersonly) continue;

			if (data->rejecticons)
			{
		    	    WORD len = strlen(fib->fib_FileName);

		    	    if (len >= 5)
			    {
				if (stricmp(fib->fib_FileName + len - 5, ".info") == 0) continue;
			    }
			}

			if (!isdir || data->filterdrawers)
			{
			    if (data->acceptpattern)
			    {
		    		if (!MatchPatternNoCase(data->acceptpattern, fib->fib_FileName)) continue;
			    }

			    if (data->rejectpattern)
			    {
		    		if (MatchPatternNoCase(data->rejectpattern, fib->fib_FileName)) continue;
			    }
			}
			
			if (isdir)
			{
			    set(obj, MUIA_Dirlist_NumDrawers, ++data->numdrawers);
			}
			else
			{
			    set(obj, MUIA_Dirlist_NumFiles, ++data->numfiles);
			    set(obj, MUIA_Dirlist_NumBytes, data->numbytes + fib->fib_Size);
			}
			
			DoMethod(obj, MUIM_List_InsertSingle, (IPTR)fib, MUIV_List_Insert_Bottom);

		    } /* no filterhook */
		    
		} /* for(;;) */
		
		set(obj, MUIA_Dirlist_Status, MUIV_Dirlist_Status_Valid);
		
		
	    } /* if (success && (fib->fib_DirEntryType > 0)) */
	    
    	    UnLock(lock);
	    
	} /* if ((lock = Lock(data->directory, SHARED_LOCK))) */
	FreeDosObject(DOS_FIB, fib);
	
    } /* if ((fib = AllocDosObject(DOS_FIB, NULL))) */
}

IPTR Dirlist__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Dirlist_DATA *data = INST_DATA(cl, obj);
    struct TagItem  	*tag, *tags;
    BOOL    	    	 directory_changed = FALSE;
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        IPTR tidata = tag->ti_Data;

	switch (tag->ti_Tag)
	{
	    
    	    case MUIA_Dirlist_AcceptPattern:
		data->acceptpattern = (STRPTR)tidata;
		break;

	    case MUIA_Dirlist_Directory:
		data->directory = (STRPTR)tidata;
		directory_changed = TRUE;
		break;

	    case MUIA_Dirlist_DrawersOnly:
		data->drawersonly = tidata ? TRUE : FALSE;
		break;

	    case MUIA_Dirlist_FilesOnly:
		data->filesonly = tidata ? TRUE : FALSE;
		break;

	    case MUIA_Dirlist_FilterDrawers:
		data->filterdrawers = tidata ? TRUE : FALSE;
		break;

	    case MUIA_Dirlist_FilterHook:
		data->filterhook = (struct Hook *)tidata;
		break;

	    case MUIA_Dirlist_MultiSelDirs:
		data->multiseldirs = tidata ? TRUE : FALSE;
		break;

	    case MUIA_Dirlist_RejectIcons:
		data->rejecticons = tidata ? TRUE : FALSE;
		break;

	    case MUIA_Dirlist_RejectPattern:
	    	data->rejectpattern = (STRPTR)tidata;
		break;

	    case MUIA_Dirlist_SortDirs:
		data->sortdirs = tidata ? TRUE : FALSE;
		break;

	    case MUIA_Dirlist_SortHighLow:
		data->sorthighlow = tidata ? TRUE : FALSE;
		break;

	    case MUIA_Dirlist_SortType:
		data->sorttype = tidata;
		break;

    	    case MUIA_Dirlist_Status:
	    	data->status = tidata;
		break;
		
    	} /* switch (tag->ti_Tag) */
    
    } /* for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); ) */
 
    if (directory_changed)
    {
    	if (data->status == MUIV_Dirlist_Status_Valid)
	{
	    DoMethod(obj, MUIM_List_Clear);
	    
	    SetAttrs(obj, MUIA_Dirlist_Status	 , MUIV_Dirlist_Status_Invalid,
	    	    	  MUIA_Dirlist_NumBytes  , 0 	    	    	      ,
			  MUIA_Dirlist_NumFiles  , 0 	    	    	      ,
			  MUIA_Dirlist_NumDrawers, 0	    	    	      ,
			  TAG_DONE  	    	    	    	    	      );
	    	    	
	}
	
	if (data->directory)
	{
	    ReadDirectory(obj, data);
	}

    }
    
    return (msg->MethodID == OM_SET) ? DoSuperMethodA(cl, obj, (Msg)msg) : 0;
    
}
IPTR Dirlist__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Dirlist_DATA *data = INST_DATA(cl, obj);
    
    #define STORE *(msg->opg_Storage)
    
    switch(msg->opg_AttrID)
    {
    	case MUIA_Dirlist_AcceptPattern:
	    STORE = (IPTR)data->acceptpattern;
	    return 1;
	    
	case MUIA_Dirlist_Directory:
	    STORE = (IPTR)data->directory;
	    return 1;
	    
	case MUIA_Dirlist_DrawersOnly:
	    STORE = data->drawersonly;
	    return 1;
	    
	case MUIA_Dirlist_FilesOnly:
	    STORE = data->filesonly;
	    return 1;
	    
	case MUIA_Dirlist_FilterDrawers:
	    STORE = data->filterdrawers;
	    return 1;
	    
	case MUIA_Dirlist_FilterHook:
	    STORE = (IPTR)data->filterhook;
	    return 1;
	    
	case MUIA_Dirlist_MultiSelDirs:
	    STORE = data->multiseldirs;
	    return 1;
	    
	case MUIA_Dirlist_NumBytes:
	    STORE = data->numbytes;
	    return 1;
	    
	case MUIA_Dirlist_NumFiles:
	    STORE = data->numfiles;
	    return 1;
	    
	case MUIA_Dirlist_NumDrawers:
	    STORE = data->numdrawers;
	    return 1;
	    
	case MUIA_Dirlist_Path:
	    /* TODO */
	    return 1;
	    
	case MUIA_Dirlist_RejectIcons:
	    STORE = data->rejecticons;
	    return 1;
	    
	case MUIA_Dirlist_RejectPattern:
	    STORE = (IPTR)data->rejectpattern;
	    return 1;
	    
	case MUIA_Dirlist_SortDirs:
	    STORE = data->sortdirs;
	    return 1;
	    
	case MUIA_Dirlist_SortHighLow:
	    STORE = data->sorthighlow;
	    return 1;
	    
	case MUIA_Dirlist_SortType:
	    STORE = data->sorttype;
	    return 1;

	case MUIA_Dirlist_Status:
	    STORE = data->status;
	    return 1;
	    	    	    
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
    
}

#if ZUNE_BUILTIN_DIRLIST
BOOPSI_DISPATCHER(IPTR, Dirlist_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Dirlist__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_SET: return Dirlist__OM_SET(cl, obj, (struct opSet *)msg);
	case OM_GET: return Dirlist__OM_GET(cl, obj, (struct opGet *)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Dirlist_desc =
{ 
    MUIC_Dirlist, 
    MUIC_List, 
    sizeof(struct Dirlist_DATA), 
    (void*)Dirlist_Dispatcher 
};
#endif /* ZUNE_BUILTIN_DIRLIST */
