#include <string.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "textengine.h"

extern struct Library *MUIMasterBase;

struct ListEntry
{
    APTR entry;
    LONG *widths; /* The widths of the column */
    LONG height; /* line height */
};

struct MUI_ListData
{
    APTR intern_pool; /* The internal pool which the class has allocated */
    LONG intern_puddle_size;
    LONG intern_tresh_size;
    APTR pool; /* the pool which is used to allocate list entries */

    struct Hook *construct_hook;
    struct Hook *compare_hook;
    struct Hook *destruct_hook;
    struct Hook *display_hook;

    /* List managment, currently we use a simple flat array, which is not good if many entries are inserted/deleted */
    LONG entries_num; /* Number of Entries in the list */
    LONG entries_allocated;
    struct ListEntry **entries;

    LONG entries_first; /* first visible entry */
    LONG entries_active;
    LONG columns; /* Number of columns the list has */
};

/**************************************************************************
 Alloccate a single list entry, does not initialize it (except the pointer)
**************************************************************************/
static struct ListEntry *AllocListEntry(struct MUI_ListData *data)
{
    ULONG *mem;
    struct ListEntry *le;
    int size = sizeof(struct ListEntry) + sizeof(LONG)*data->columns + 4; /* sizeinfo */

    mem = (ULONG*)AllocPooled(data->pool, size);
    if (!mem) return NULL;

    mem[0] = size; /* Save the size */
    le = (struct ListEntry*)(mem+1);
    le->widths = (LONG*)(le + 1);
    return le;
}

/**************************************************************************
 Dealloccate a single list entry, does not deinitialize it
**************************************************************************/
static void FreeListEntry(struct MUI_ListData *data, struct ListEntry *entry)
{
    ULONG *mem = ((ULONG*)entry)-1;
    FreePooled(data->pool,mem,mem[0]);
}

/**************************************************************************
 Ensures that we there can be at least the given amount of entries within
 the list. Returns 0 if not.
**************************************************************************/
static int SetListSize(struct MUI_ListData *data, LONG size)
{
    struct ListEntry **new_entries;
    int new_entries_allocated;

    if (size <= data->entries_allocated) return 1;

    new_entries_allocated = data->entries_allocated*2+4;
    if (new_entries_allocated < size) new_entries_allocated = size + 10; /* random value */

    new_entries = (struct ListEntry**)AllocVec(new_entries_allocated * sizeof(struct ListEntry*),0);
    if (!new_entries) return 0;
    if (data->entries)
    {
	CopyMem(data->entries,new_entries,data->entries_num);
	FreeVec(data->entries);
    }
    data->entries = new_entries;
    return 1;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR List_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListData   *data;
    struct TagItem  	    *tag, *tags;
    APTR *array = NULL;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    data->columns = 1;
    data->entries_active = MUIV_List_Active_Off;
    data->intern_puddle_size = 2008;
    data->intern_tresh_size = 1024;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_List_Pool:
		    data->pool = (APTR)tag->ti_Data;
		    break;

	    case    MUIA_List_PoolPuddleSize:
		    data->intern_puddle_size = tag->ti_Data;
		    break;

	    case    MUIA_List_PoolThreshSize:
		    data->intern_tresh_size = tag->ti_Data;
		    break;

	    case    MUIA_List_CompareHook:
		    data->compare_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_List_ConstructHook:
		    data->construct_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_List_DestructHook:
		    data->destruct_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_List_DisplayHook:
		    data->display_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_List_SourceArray:
		    array = (APTR*)tag->ti_Data;
		    break;
    	}
    }

    if (!data->pool)
    {
	data->pool = data->intern_pool = CreatePool(0,data->intern_puddle_size,data->intern_tresh_size);
	if (!data->pool)
	{
	    CoerceMethod(cl,obj,OM_DISPOSE);
	    return NULL;
	}
    }

    if (array)
    {
    	int i;
    	for (i=0;array[i];i++);
    	DoMethod(obj, MUIM_List_Insert, array, i, MUIV_List_Insert_Top);
    }
    
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR List_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    if (data->intern_pool) DeletePool(data->intern_pool);
    if (data->entries) FreeVec(data->entries);
    DoSuperMethodA(cl,obj,msg);
    return 0;
}


/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR List_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListData   *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_List_CompareHook:
		    data->compare_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_List_ConstructHook:
		    data->construct_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_List_DestructHook:
		    data->destruct_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_List_DisplayHook:
		    data->display_hook = (struct Hook*)tag->ti_Data;
		    break;
    	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG List_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_ListData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
	case MUIA_List_Entries: STORE = data->entries_num; return 1;
	case MUIA_List_First: STORE = data->entries_first; return 1;
	case MUIA_List_Active: STORE = data->entries_active; return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
#undef STORE
}


/**************************************************************************
 MUIM_List_Insert
**************************************************************************/
STATIC ULONG List_Insert(struct IClass *cl, Object *obj, struct MUIP_List_Insert *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos;
    switch (msg->pos)
    {
    	case    MUIV_List_Insert_Top:
    		pos = 0;
	        break;

    	case    MUIV_List_Insert_Active:
		if (data->entries_active != -1) pos = data->entries_active;
		else pos = data->entries_active;
	        break;

	case    MUIV_List_Insert_Sorted:
		pos = -1;
	        break;

	case    MUIV_List_Insert_Bottom:
		pos = data->entries_num;
	        break;

	default:
		if (msg->pos > data->entries_num) pos = data->entries_num;
		else if (msg->pos < 0) pos = 0;
		else pos = msg->pos;
	        break;		

    }

    if (!(SetListSize(data,data->entries_num + msg->count)))
	return ~0;

    if (pos != -1)
    {
    	return (ULONG)pos;
    }
    return ~0;
}

/**************************************************************************
 MUIM_List_InsertSingle
**************************************************************************/
STATIC ULONG List_InsertSingle(struct IClass *cl, Object *obj, struct MUIP_List_InsertSingle *msg)
{
    return DoMethod(obj,MUIM_List_Insert, &msg->entry, 1, msg->pos);
}

/**************************************************************************
 MUIM_List_Construct
**************************************************************************/
STATIC ULONG List_Construct(struct IClass *cl, Object *obj, struct MUIP_List_Construct *msg)
{
    return NULL;
}

/**************************************************************************
 MUIM_List_Destruct
**************************************************************************/
STATIC ULONG List_Destruct(struct IClass *cl, Object *obj, struct MUIP_List_Destruct *msg)
{
    return NULL;
}

/**************************************************************************
 MUIM_List_Compare
**************************************************************************/
STATIC ULONG List_Compare(struct IClass *cl, Object *obj, struct MUIP_List_Compare *msg)
{
    return NULL;
}

/**************************************************************************
 MUIM_List_Display
**************************************************************************/
STATIC ULONG List_Display(struct IClass *cl, Object *obj, struct MUIP_List_Display *msg)
{
    return NULL;
}


#ifndef _AROS
__asm IPTR List_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,List_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return List_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return List_Dispose(cl,obj, msg);
	case OM_SET: return List_Set(cl,obj,(struct opSet *)msg);
	case OM_GET: return List_Get(cl,obj,(struct opGet *)msg);
	case MUIM_List_Insert: return List_Insert(cl,obj,(APTR)msg);
	case MUIM_List_InsertSingle: return List_InsertSingle(cl,obj,(APTR)msg);
	case MUIM_List_Construct: return List_Construct(cl,obj,(APTR)msg);
	case MUIM_List_Destruct: return List_Destruct(cl,obj,(APTR)msg);
	case MUIM_List_Compare: return List_Compare(cl,obj,(APTR)msg);
	case MUIM_List_Display: return List_Display(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_List_desc = { 
    MUIC_List, 
    MUIC_Area, 
    sizeof(struct MUI_ListData), 
    (void*)List_Dispatcher 
};

