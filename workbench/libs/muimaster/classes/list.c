#include <string.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "textengine.h"

extern struct Library *MUIMasterBase;

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define ENTRY_TITLE (-1)

struct ListEntry
{
    APTR data;
    LONG *widths; /* The widths of the column */
    LONG height; /* line height */
};

struct ColumnInfo
{
    int colno; /* Column number */
    int user_width; /* user setted width -1 if entry width */
    int min_width; /* min width percentage */
    int max_width; /* min width percentage */
    int weight;
    int delta; /* ignored for the first and last column, defaults to 4 */
    int bar;
    char *prepare;

    int entries_width; /* width of the entries (the maximum of the widths of all entries) */
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
    LONG entries_visible; /* number of visible entries, determined at MUIM_Layout */
    LONG entries_active;
    LONG insert_position; /* pos of the last insertion */

    LONG entry_maxheight; /* Maximum height of an entry */

    LONG vertprop_entries;
    LONG vertprop_visible;
    LONG vertprop_first;

    LONG confirm_entries_num; /* These are the correct entries num, used so you cannot set MUIA_List_Entries to wrong values */

    LONG entries_top_pixel; /* Where the entries start */

    /* Column managment, is allocated by ParseListFormat() and freed by CleanListFormat() */
    LONG columns; /* Number of columns the list has */
    struct ColumnInfo *ci;
    char **preparses;
    char **strings; /* the strings for the display function, one more as needed (for the entry position) */

    /* Titlestuff */
    int title_height; /* The complete height of the title */
    char *title; /* On single comlums this is the title, otherwise 1 */

    struct MUI_EventHandlerNode ehn;
    int mouse_click; /* see below if mouse is hold down */
};

#define MOUSE_CLICK_ENTRY 1 /* on entry clicked */ 
#define MOUSE_CLICK_TITLE 2 /* on title clicked */

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
 the list. Returns 0 if not. It also allocates the space for the title.
 It can be accesses with data->entries[ENTRY_TITLE]
**************************************************************************/
static int SetListSize(struct MUI_ListData *data, LONG size)
{
    struct ListEntry **new_entries;
    int new_entries_allocated;

    if (size + 1 <= data->entries_allocated) return 1;

    new_entries_allocated = data->entries_allocated*2+4;
    if (new_entries_allocated < size + 1) new_entries_allocated = size + 1 + 10; /* 10 is just random */

    new_entries = (struct ListEntry**)AllocVec(new_entries_allocated * sizeof(struct ListEntry*),0);
    if (!new_entries) return 0;
    if (data->entries)
    {
	CopyMem(data->entries - 1, new_entries, (data->entries_num+1)*sizeof(struct ListEntry*));
	FreeVec(data->entries - 1);
    }
    data->entries = new_entries + 1;
    data->entries_allocated = new_entries_allocated;
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
    memmove(&data->entries[pos+count],&data->entries[pos],(data->entries_num - pos)*sizeof(struct ListEntry*));
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
    memmove(&data->entries[pos],&data->entries[pos+count],(data->entries_num - (pos + count))*sizeof(struct ListEntry *));
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
    	FreeVec(data->strings-1);
    	data->strings = NULL;
    }
    data->columns = 0;
}

/**************************************************************************
 Parses the given format string (also frees a previouly parsed format).
 Return 0 on failure.
**************************************************************************/
static int ParseListFormat(struct MUI_ListData *data, char *format)
{
    int new_columns,i;
    char *ptr;
    char c;

    if (!format) format = "";

    ptr = format;

    FreeListFormat(data);

    new_columns = 1;

    /* Count the number of columns first */
    while ((c = *ptr++))
	if (c == ',') new_columns++;

    if (!(data->preparses = (char**)AllocVec((new_columns+10)*sizeof(char*),0)))
	return 0;

    if (!(data->strings = (char**)AllocVec((new_columns+1+10)*sizeof(char*),0))) /* hold enough space also for the entry pos, used by orginal MUI and also some security space */
	return 0;

    if (!(data->ci = (struct ColumnInfo *)AllocVec(new_columns*sizeof(struct ColumnInfo),MEMF_CLEAR)))
	return 0;

    for (i=0;i<new_columns;i++)
    {
	data->ci[i].colno = i;
	data->ci[i].weight = 100;
	data->ci[i].delta = 4;
	data->ci[i].user_width = -1;
    }

    data->columns = new_columns;
    data->strings++; /* Skip entry pos */

    return 1;
}

/**************************************************************************
 Call the MUIM_List_Display for the given entry. It fills out
 data->string and data->preparses
**************************************************************************/
static void DisplayEntry(struct IClass *cl, Object *obj, int entry_pos)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    APTR entry_data;
    int col;

    /* Preparses are not required to be set, so we clear them first */
    for (col=0;col<data->columns;col++) data->preparses[col] = NULL;

    if (entry_pos == ENTRY_TITLE)
    {
    	if (data->columns == 1 && (data->title != (char*)1))
    	{
	    *data->strings = data->title;
	    return;
    	}
    	entry_data = NULL; /* it's a title request */
    }
    else entry_data = data->entries[entry_pos]->data;

    /* Get the display formation */
    DoMethod(obj, MUIM_List_Display, entry_data, entry_pos, data->strings, data->preparses);
}

/**************************************************************************
 Determine the widths of the entries
**************************************************************************/
static void CalcWidths(struct IClass *cl, Object *obj)
{
    int i,j;
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (!(_flags(obj) & MADF_SETUP)) return;

    for (j=0;j<data->columns;j++)
	data->ci[j].entries_width = 0;

    data->entry_maxheight = 0;

    for (i=(data->title?ENTRY_TITLE:0);i<data->entries_num;i++)
    {
    	struct ListEntry *entry = data->entries[i];
    	if (!entry) break;

	DisplayEntry(cl,obj,i);

	/* Clear the height */
	data->entries[i]->height = 0;

	for (j=0;j<data->columns;j++)
	{
	    ZText *text = zune_text_new(data->strings[j],data->preparses[j], ZTEXT_ARG_NONE, NULL);
	    if (text)
	    {
		zune_text_get_bounds(text,obj);
		if (text->height > data->entries[i]->height) data->entries[i]->height = text->height;
		data->entries[i]->widths[j] = text->width;

		if (text->width > data->ci[j].entries_width)
		{
		    /* This columns width is bigger than the other in the same columns, so we store this value */
		    data->ci[j].entries_width = text->width;
		}

		zune_text_destroy(text);
	    }
	}
	if (data->entries[i]->height > data->entry_maxheight) data->entry_maxheight = data->entries[i]->height;
    }
    if (!data->entry_maxheight) data->entry_maxheight = 1;
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

	    case    MUIA_List_Format:
		    format = (char*)tag->ti_Data;
		    break;

	    case    MUIA_List_Title:
		    data->title = (char*)tag->ti_Data;
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

    /* This is neccessary for at least the title */
    if (!SetListSize(data,0))
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    if (data->title)
    {
	if (!(data->entries[ENTRY_TITLE] = AllocListEntry(data)))
	{
	    CoerceMethod(cl,obj,OM_DISPOSE);
	    return NULL;
	}
    } else data->entries[ENTRY_TITLE] = NULL;


    if (array)
    {
    	int i;
    	for (i=0;array[i];i++); /* Count the number of elements */
    	/* Insert them */
    	DoMethod(obj, MUIM_List_Insert, array, i, MUIV_List_Insert_Top);
    }

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

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
    if (data->entries) FreeVec(data->entries - 1); /* title is currently before all other elements */
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

	    case    MUIA_List_VertProp_First:
		    data->vertprop_first = tag->ti_Data;
		    if (data->entries_first != tag->ti_Data)
		    {
			set(obj,MUIA_List_First,tag->ti_Data);
		    }
		    break;

	    case    MUIA_List_VertProp_Entries:
		    data->vertprop_entries = tag->ti_Data;
		    break;

	    case    MUIA_List_VertProp_Visible:
		    data->vertprop_visible = tag->ti_Data;
		    data->entries_visible = tag->ti_Data;
		    break;

	    case    MUIA_List_Active:
		    {
			LONG new_entries_active = tag->ti_Data;

			if (data->entries_num)
			{
			    switch (new_entries_active)
			    {
			        case    MUIV_List_Active_Top:
				        new_entries_active = 0;
				        break;

				case    MUIV_List_Active_Bottom:
				        new_entries_active = data->entries_num - 1;
				        break;

				case    MUIV_List_Active_Up:
					new_entries_active = data->entries_active - 1;
					break;

				case    MUIV_List_Active_Down:
					new_entries_active = data->entries_active + 1;
					break;

				case    MUIV_List_Active_PageUp:
					new_entries_active = data->entries_active - data->entries_visible;
					break;

				case    MUIV_List_Active_PageDown:
					new_entries_active = data->entries_active + data->entries_visible;
					break;
			    }

			    if (new_entries_active < 0) new_entries_active = 0;
			    else if (new_entries_active >= data->entries_num) new_entries_active = data->entries_num - 1;
			} else new_entries_active = -1;
			data->entries_active = new_entries_active;
			MUI_Redraw(obj,MADF_DRAWUPDATE);
		    }
		    break;

	    case    MUIA_List_First:
		    data->entries_first = tag->ti_Data;
		    MUI_Redraw(obj,MADF_DRAWUPDATE);
		    if (data->vertprop_first != tag->ti_Data)
		    {
			set(obj,MUIA_List_VertProp_First,tag->ti_Data);
		    }
		    break;

	    case    MUIA_List_Entries:
		    if (data->confirm_entries_num == tag->ti_Data)
		    {
			data->entries_num = tag->ti_Data;
			set(obj, MUIA_List_VertProp_Entries, data->entries_num);
		    }
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
	case MUIA_List_Title: STORE = (unsigned long)data->title; return 1;
	case MUIA_List_VertProp_Entries: STORE = STORE = data->vertprop_entries; return 1;
	case MUIA_List_VertProp_Visible: STORE = data->vertprop_visible; return 1;
	case MUIA_List_VertProp_First: STORE = data->vertprop_first; return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG List_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    if (!DoSuperMethodA(cl, obj, (Msg) msg)) return 0;
    CalcWidths(cl,obj);
    if (data->title) data->title_height = data->entries[ENTRY_TITLE]->height + 2;
    else data->title_height = 0;

    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehn);
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG List_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehn);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_Layout
**************************************************************************/
static ULONG List_Layout(struct IClass *cl, Object *obj,struct MUIP_Layout *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl,obj,(Msg)msg);

    /* Calc the entries visible */
    data->entries_visible = (_mheight(obj) - data->title_height)/data->entry_maxheight;
    data->entries_top_pixel = _mtop(obj) + data->title_height + (_mheight(obj) - data->title_height - data->entries_visible * data->entry_maxheight)/2;

    /* So the notify takes happens */
    set(obj, MUIA_List_VertProp_Visible, data->entries_visible);

    return rc;
}

/**************************************************************************
 Draw an entry at entry_pos at the given y location. To draw the title,
 set pos to ENTRY_TITLE
**************************************************************************/
static VOID List_DrawEntry(struct IClass *cl, Object *obj, int entry_pos, int y)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int col,x1,x2;
    struct ListEntry *entry = data->entries[entry_pos];

    /* To be surem we don't draw anything if there is no title */
    if (entry_pos == ENTRY_TITLE && !data->title) return;

    DisplayEntry(cl,obj,entry_pos);
    x1 = _mleft(obj);

    for (col = 0; col < data->columns; col++)
    {
	ZText *text;
	x2 = x1 + data->ci[col].entries_width;

	if ((text = zune_text_new(data->strings[col],data->preparses[col], ZTEXT_ARG_NONE, NULL)))
	{
	    /* Could be made simpler, as we don't need really the bounds */
	    zune_text_get_bounds(text, obj);
	    zune_text_draw(text, obj, x1, x2, y); /* totally wrong! */
	    zune_text_destroy(text);
	}
	x1 = x2 + data->ci[col].delta;
    }
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG List_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int entry_pos,y;
    APTR clip;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (msg->flags & MADF_DRAWUPDATE)
	DoMethod(obj,MUIM_DrawBackground,_mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj),0,0);

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
			   _mwidth(obj), _mheight(obj));

    y = _mtop(obj);
    if (data->title_height && data->title)
    {
	List_DrawEntry(cl,obj,ENTRY_TITLE,y);
	y += data->entries[ENTRY_TITLE]->height;
	SetAPen(_rp(obj),_pens(obj)[MPEN_SHADOW]);
	Move(_rp(obj),_mleft(obj), y);
	Draw(_rp(obj),_mright(obj), y);
	SetAPen(_rp(obj),_pens(obj)[MPEN_SHINE]);
	y++;
	Move(_rp(obj),_mleft(obj), y);
	Draw(_rp(obj),_mright(obj), y);
    }

    y = data->entries_top_pixel;

    for (entry_pos = data->entries_first; entry_pos < data->entries_first + data->entries_visible && entry_pos < data->entries_num; entry_pos++)
    {
	struct ListEntry *entry = data->entries[entry_pos];
	if (entry_pos == data->entries_active)
	{
	    /* TODO: Correct background */
	    SetAPen(_rp(obj),_pens(obj)[MPEN_FILL]);
	    RectFill(_rp(obj),_mleft(obj),y,_mright(obj), y + data->entry_maxheight - 1);
	}
	List_DrawEntry(cl,obj,entry_pos,y);
    	y += data->entry_maxheight;
    }

    MUI_RemoveClipping(muiRenderInfo(obj),clip);

    return 0;
}

/**************************************************************************
 Makes the entry at the given mouse position the active one.
 Relx and Rely are relative mouse coordinates to the upper left of
 the object
**************************************************************************/
static VOID List_MakeActive(struct IClass *cl, Object *obj, LONG relx, LONG rely)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG eclicky = rely + _top(obj) - data->entries_top_pixel; /* y coordinates transfromed to the entries */
    LONG new_act = eclicky / data->entry_maxheight + data->entries_first;

    if (new_act >= data->entries_num) new_act = data->entries_num - 1;
    else if (new_act < 0) new_act = 0;

    /* Only if changed and if there are really entries within the list */
    if (new_act != data->entries_active && data->entries_num)
    {
    	LONG new_entries_first = data->entries_first;
    	if (new_act < new_entries_first)
	    new_entries_first = new_act;
	if (new_act >= data->entries_first + data->entries_visible - 1)
	    new_entries_first = new_act - data->entries_visible + 1;

	SetAttrs(obj,MUIA_List_Active,new_act,
		     new_entries_first!=data->entries_first?MUIA_List_First:TAG_IGNORE,new_entries_first,
		     TAG_DONE);
    }
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG List_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
    	LONG mx = msg->imsg->MouseX - _left(obj);
    	LONG my = msg->imsg->MouseY - _top(obj);
	switch (msg->imsg->Class)
	{
	    case    IDCMP_MOUSEBUTTONS:
		    if (msg->imsg->Code == SELECTDOWN)
		    {
			if (mx >= 0 && mx < _width(obj) && my >= 0 && my < _height(obj))
			{
			    LONG eclicky = my + _top(obj) - data->entries_top_pixel; /* y coordinates transfromed to the entries */
			    data->mouse_click = MOUSE_CLICK_ENTRY;
			    /* Now check if it was clicked on a title or on the entries */
			    if (eclicky >= 0 && eclicky < data->entries_visible * data->entry_maxheight)
			    {
				List_MakeActive(cl, obj, mx, my);
			    }

			    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehn);

			    return MUI_EventHandlerRC_Eat;
			}
		    } else
		    {
			if (msg->imsg->Code == SELECTUP && data->mouse_click)
			{
			    DoMethod(_win(obj),MUIM_Window_RemEventHandler, &data->ehn);
			    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			    DoMethod(_win(obj),MUIM_Window_AddEventHandler, &data->ehn);
			    data->mouse_click = 0;
			    return 0;
			}
		    }
		    break;

	    case    IDCMP_MOUSEMOVE:
		    if (data->mouse_click)
		    {
			List_MakeActive(cl, obj, mx, my);
		    }
		    break;
	}
    }

    return 0;
}

/**************************************************************************
 MUIM_List_Clear
**************************************************************************/
static ULONG List_Clear(struct IClass *cl, Object *obj, struct MUIP_List_Clear *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    while (data->entries_num)
    {
    	struct ListEntry *lentry = data->entries[--data->entries_num];
	DoMethod(obj, MUIM_List_Destruct, lentry->data, data->pool);
	FreeListEntry(data,lentry);
    }
    /* Should never fail when shrinking */
    SetListSize(data,0);
    return 0;
}

/**************************************************************************
 MUIM_List_Remove
**************************************************************************/
static ULONG List_Remove(struct IClass *cl, Object *obj, struct MUIP_List_Remove *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos;
    LONG new_act;
    struct ListEntry *lentry;

    if (!data->entries_num) return 0;

    switch(msg->pos)
    {
	case    MUIV_List_Remove_First:
		pos = 0;
		break;

	case    MUIV_List_Remove_Active:
		pos = data->entries_active;
		break;

	case    MUIV_List_Remove_Last:
		pos = data->entries_num - 1;
		break;

        case    MUIV_List_Remove_Selected:
		/* TODO: needs special handling */
		pos = data->entries_active;
		break;

	default:
		pos = msg->pos;
		break;
    }

    new_act = data->entries_active;

    if (pos == new_act && new_act == data->entries_num - 1)
	new_act--; /* might become MUIV_List_Active_Off */

    lentry = data->entries[pos];
    DoMethod(obj, MUIM_List_Destruct, lentry->data, data->pool);
    RemoveListEntries(data,pos,1);
    data->confirm_entries_num--;

    SetAttrs(obj,
	MUIA_List_Entries, data->confirm_entries_num,
	pos == data->entries_active?MUIA_List_Active:TAG_DONE, new_act, /* Inform only if neccessary (for notify) */
	TAG_DONE);

    return 0;
}

/**************************************************************************
 MUIM_List_Insert
**************************************************************************/
static ULONG List_Insert(struct IClass *cl, Object *obj, struct MUIP_List_Insert *msg)
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

//    	data->confirm_entries_num = data->entries_num;

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
	    	if (data->entries_num != data->confirm_entries_num)
		    set(obj,MUIA_List_Entries,data->confirm_entries_num);
		return ~0;
	    }

	    data->entries[pos] = lentry;
	    data->confirm_entries_num++;

	    toinsert++;
	    pos++;
	}

	if (data->entries_num != data->confirm_entries_num)
	    set(obj,MUIA_List_Entries,data->confirm_entries_num);

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
 MUIM_List_GetEntry
**************************************************************************/
STATIC ULONG List_GetEntry(struct IClass *cl, Object *obj, struct MUIP_List_GetEntry *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int pos = msg->pos;

    if (pos == MUIV_List_GetEntry_Active) pos = data->entries_active;

    if (pos < 0 && pos >= data->entries_num)
    {
    	*msg->entry = NULL;
    	return NULL;
    }
    *msg->entry = data->entries[pos]->data;
    return (ULONG)*msg->entry;
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
    if (!data->display_hook)
    {
    	if (msg->entry)
    	{
	    *msg->strings = msg->entry;
	} else
	{
	    *msg->strings = 0;
	}
    	return 1;
    }

    *((ULONG*)(msg->strings-1)) = msg->entry_pos;
    return CallHookPkt(data->display_hook,msg->strings,msg->entry);
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
	case MUIM_Setup: return List_Setup(cl,obj,(struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return List_Cleanup(cl,obj,(struct MUIP_Cleanup *)msg);
	case MUIM_Draw: return List_Draw(cl,obj,(struct MUIP_Draw *)msg);
	case MUIM_Layout: return List_Layout(cl,obj,(struct MUIP_Layout *)msg);
	case MUIM_HandleEvent: return List_HandleEvent(cl,obj,(struct MUIP_HandleEvent *)msg);
	case MUIM_List_Clear: return List_Clear(cl,obj,(struct MUIP_List_Clear *)msg);
	case MUIM_List_Insert: return List_Insert(cl,obj,(APTR)msg);
	case MUIM_List_InsertSingle: return List_InsertSingle(cl,obj,(APTR)msg);
	case MUIM_List_GetEntry: return List_GetEntry(cl,obj,(APTR)msg);
	case MUIM_List_Remove: return List_Remove(cl,obj,(APTR)msg);
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

