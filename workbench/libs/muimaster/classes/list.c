#include <string.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>

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
    APTR data;
    LONG *widths; /* The widths of the column */
    LONG height; /* line height */
};

struct ColumnInfo
{
    int colno; /* Column number */
    int entry_width; /* width of the entries (the maximum of the widths of all entries) */
    int user_width; /* user setted width -1 if entry width */
    int min_width; /* min width percentage */
    int max_width; /* min width percentage */
    int weight;
    int delta; /* ignored for the first and last column, defaults to 4 */
    int bar;
    char *prepare;
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
    LONG insert_position; /* pos of the last insertion */

    /* Column managment, only setted by ParseListFormat() and freed by CleanListFormat() */
    LONG columns; /* Number of columns the list has */
    struct ColumnInfo *ci;
    char **preparses;
    char **strings; /* the strings for the display function, one more as needed (for the entry position) */
    
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
 Prepares the insertion of count entries at pos.
 This function doesn't care if there is enough space in the datastructure.
 SetListSize() must be used first.
 With current implementation, this call will never fail
**************************************************************************/
static int PrepareInsertListEntries(struct MUI_ListData *data, int pos, int count)
{
    memmove(&data->entries[pos+count],&data->entries[pos],data->entries_num - pos);
    return 1;
}

/**************************************************************************
 Inserts a already initialized array of Entries at the given position.
 This function doesn't care if there is enough space in the datastructure
 Returns 1 if something failed (never in current implementation)
**************************************************************************/
#if 0
static int InsertListEntries(struct MUI_ListData *data, int pos, struct ListEntry **array, int count)
{
    memmove(&data->entries[pos+count],&data->entries[pos],data->entries_num - pos);
    memcpy(&data->entries[pos],array,count);
    return 1;
}
#endif


/**************************************************************************
 Removes count (already deinitalized) list entries starting az pos.
**************************************************************************/
static void RemoveListEntries(struct MUI_ListData *data, int pos, int count)
{
    memmove(&data->entries[pos],&data->entries[pos+count],data->entries_num - (pos + count));
}

/**************************************************************************
 Frees all memory allocated by ParseListFormat()
**************************************************************************/
static void FreeListFormat(struct MUI_ListData *data)
{
    if (data->ci)
    {
    	FreeVec(data->ci);
    	data->ci = NULL;
    }
    if (data->preparses)
    {
    	FreeVec(data->preparses);
    	data->preparses = NULL;
    }
    if (data->strings)
    {
    	FreeVec(data->strings);
    	data->strings = 0;
    }
    data->columns = 0;
}

/**************************************************************************
 Parses the given format string (also frees a previouly parsed format).
 Return 0 on failure.
**************************************************************************/
static int ParseListFormat(struct MUI_ListData *data, char *format)
{
    int new_columns;

    FreeListFormat(data);

    new_columns = 1;

    if (!(data->preparses = (char**)AllocVec(new_columns*sizeof(char*),0)))
	return 0;

    if (!(data->strings = (char**)AllocVec((new_columns+1)*sizeof(char*),0))) /* the entry pos is stored in the first elment (MUI) */
	return 0;

    if (!(data->ci = (struct ColumnInfo *)AllocVec(new_columns*sizeof(struct ColumnInfo),0)))
	return 0;

    data->columns = new_columns;

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
    char *format = NULL;
    
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
    	/* No memory pool given, so we create out own */
	data->pool = data->intern_pool = CreatePool(0,data->intern_puddle_size,data->intern_tresh_size);
	if (!data->pool)
	{
	    CoerceMethod(cl,obj,OM_DISPOSE);
	    return NULL;
	}
    }

    /* parese the list format */
    if (!(ParseListFormat(data,format)))
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    if (array)
    {
    	int i;
    	for (i=0;array[i];i++); /* Count the number of elements */
    	/* Insert them */
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
    if (!data->intern_pool && data->pool)
    {
	/* Call destruct method for every entry and free the entries manual to avoid notification */
    }

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
	case MUIA_List_InsertPosition: STORE = data->insert_position; return 1;
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
    LONG pos,count;

    count = msg->count;

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

    if (!(SetListSize(data,data->entries_num + count)))
	return ~0;

    if (pos != -1)
    {
    	LONG until = pos + count;
    	APTR *toinsert = msg->entries;

	if (!(PrepareInsertListEntries(data, pos, count)))
	    return ~0;

	while (pos < until)
	{
	    struct ListEntry *lentry;

	    if (!(lentry = AllocListEntry(data)))
	    {
	    	/* Panic, but we must be in a consistent state, so remove
	    	** the space where the following list entries should have gone
	    	*/
	    	RemoveListEntries(data, pos, until - pos);
		return ~0;
	    }

	    /* now call the construct method which returns us a pointer which we need to store */
	    lentry->data = (APTR)DoMethod(obj, MUIM_List_Construct, *toinsert, data->pool);
	    if (!lentry->data)
	    {
	    	FreeListEntry(data,lentry);
	    	RemoveListEntries(data, pos, until - pos);
		return ~0;
	    }

	    data->entries[pos] = lentry;
	    data->entries_num++;

	    toinsert++;
	    pos++;
	}

    	data->insert_position = pos;
    	return (ULONG)pos;
    } else
    {
	/* Sort insertion must works differnt */
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
    struct MUI_ListData *data = INST_DATA(cl, obj);
    if (!data->construct_hook) return (ULONG)msg->entry;
    if ((ULONG)data->construct_hook == MUIV_List_ConstructHook_String)
    {
    	int len = msg->entry?strlen((char*)msg->entry):0;
    	ULONG *mem = AllocPooled(msg->pool,len+5);
    	if (!mem) return 0;
    	mem[0] = len+5;
    	if (msg->entry) strcpy((char*)(mem+1),(char*)msg->entry);
    	else *(char*)(mem+1) = 0;
    	return (ULONG)(mem+1);
    }
    return CallHookPkt(data->construct_hook,msg->pool,msg->entry);
}

/**************************************************************************
 MUIM_List_Destruct
**************************************************************************/
STATIC ULONG List_Destruct(struct IClass *cl, Object *obj, struct MUIP_List_Destruct *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    if (!data->destruct_hook) return 0;
    if ((ULONG)data->destruct_hook == MUIV_List_DestructHook_String)
    {
    	ULONG *mem = ((ULONG*)msg->entry)-1;
	FreePooled(msg->pool,mem,mem[0]);
    } else
    {
	CallHookPkt(data->destruct_hook,msg->pool,msg->entry);
    }
    return 0;
}

/**************************************************************************
 MUIM_List_Compare
**************************************************************************/
STATIC ULONG List_Compare(struct IClass *cl, Object *obj, struct MUIP_List_Compare *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    return NULL;
}

/**************************************************************************
 MUIM_List_Display
**************************************************************************/
STATIC ULONG List_Display(struct IClass *cl, Object *obj, struct MUIP_List_Display *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
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

