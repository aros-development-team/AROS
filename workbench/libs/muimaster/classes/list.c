/*
    Copyright © 2002-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdlib.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/view.h>
#include <devices/rawkeycodes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

/* #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "imspec.h"
#include "textengine.h"
#include "listimage.h"
#include "prefs.h"

extern struct Library *MUIMasterBase;

#define ENTRY_TITLE (-1)

#define FORMAT_TEMPLATE "DELTA=D/N,PREPARSE=P/K,WEIGHT=W/N,MINWIDTH=MIW/N," \
    "MAXWIDTH=MAW/N,COL=C/N,BAR/S"

#define BAR_WIDTH 2

#define UPDATEMODE_ALL                  (1 << 0)
#define UPDATEMODE_ENTRY                (1 << 1)
#define UPDATEMODE_NEEDED               (1 << 2)

enum
{
    ARG_DELTA,
    ARG_PREPARSE,
    ARG_WEIGHT,
    ARG_MINWIDTH,
    ARG_MAXWIDTH,
    ARG_COL,
    ARG_BAR,
    ARG_CNT
};


struct ListEntry
{
    APTR data;
    LONG width;    /* Line width */
    LONG height;   /* Line height */
    WORD flags;    /* see below */
    LONG widths[]; /* Widths of the columns */
};

#define ENTRY_SELECTED   (1<<0)
#define ENTRY_RENDER     (1<<1)


struct ColumnInfo
{
    int colno;      /* Column number */
    int user_width; /* user set width; -1 if entry width */
    int min_width;  /* min width percentage */
    int max_width;  /* min width percentage */
    int weight;
    int delta;      /* ignored for the first and last column, defaults to 4 */
    int bar;
    STRPTR preparse;
    int entries_width; /* width of the entries (maximum of all widths) */
};

struct MUI_ImageSpec_intern;

struct MUI_ListData
{
    /* bool attrs */
    ULONG flags;

    APTR intern_pool;    /* The internal pool which the class has allocated */
    LONG intern_puddle_size;
    LONG intern_thresh_size;
    APTR pool;           /* the pool which is used to allocate list entries */

    struct Hook *construct_hook;
    struct Hook *compare_hook;
    struct Hook *destruct_hook;
    struct Hook *display_hook;
    struct Hook *multi_test_hook;

    struct Hook default_compare_hook;

    /* List management, currently we use a simple flat array, which is not
     * good if many entries are inserted/deleted */
    LONG entries_num;           /* Number of Entries in the list */
    LONG entries_allocated;
    struct ListEntry **entries;

    LONG entries_first;         /* first visible entry */
    LONG entries_visible;       /* number of visible entries,
                                 * determined at MUIM_Layout */
    LONG entries_active;
    LONG insert_position;       /* pos of the last insertion */

    LONG entry_maxheight;       /* Maximum height of an entry */
    ULONG entry_minheight;      /* from MUIA_List_MinLineHeight */

    LONG entries_totalheight;
    LONG entries_maxwidth;

    LONG vertprop_entries;
    LONG vertprop_visible;
    LONG vertprop_first;

    LONG confirm_entries_num;   /* These are the correct entries num, used
                                 * so you cannot set MUIA_List_Entries to
                                 * wrong values */

    LONG entries_top_pixel;     /* Where the entries start */

    /* Column managment, is allocated by ParseListFormat() and freed
     * by CleanListFormat() */
    STRPTR format;
    LONG columns;               /* Number of columns the list has */
    LONG columns_allocated;     /* List has space for columns_allocated columns */
    struct ColumnInfo *ci;
    STRPTR *preparses;
    STRPTR *strings_mem;        /* safe pointer to allocated memory for strings[] */
    STRPTR *strings;            /* the strings for the display function, one
                                 * more than needed (for the entry position) */

    /* Titlestuff */
    int title_height;           /* The complete height of the title */
    STRPTR title;               /* On single column lists this is the title,
                                 * otherwise 1. NULL for no title(s) */

    /* Cursor images */
    struct MUI_ImageSpec_intern *list_cursor;
    struct MUI_ImageSpec_intern *list_select;
    struct MUI_ImageSpec_intern *list_selcur;

    /* Render optimization */
    int update; /* 1 - update everything, 2 - redraw entry at update_pos,
                 * 3 - scroll to current entries_first (old value is in
                 * update_pos) */
    int update_pos;

    LONG drop_mark;
    LONG drop_mark_y;

    /* list images */
    struct MinList images;

    /* user prefs */
    ListviewRefresh prefs_refresh;
    UWORD prefs_linespacing;
    BOOL prefs_smoothed;
    UWORD prefs_smoothval;

    /* render space handling */
    Object *area;
    BOOL area_replaced;
    BOOL area_connected;

    /***************************/
    /* Former Listview members */
    /***************************/

    Object *vert;
    BOOL vert_connected;
    IPTR scroller_pos;
    BOOL read_only;
    IPTR multiselect;
    LONG drag_type;

    /* clicked column */
    LONG click_column;
    LONG def_click_column;

    LONG mouse_click;            /* see below if mouse is held down */
    LONG mouse_x;
    LONG mouse_y;

    /* double click */
    ULONG last_secs;
    ULONG last_mics;
    ULONG last_active;

    struct MUI_EventHandlerNode ehn;

    /* user prefs */
    ListviewMulti prefs_multi;

    BOOL doubleclick;
    LONG seltype;

    struct Hook hook;
};

#define MOUSE_CLICK_ENTRY 1     /* on entry clicked */
#define MOUSE_CLICK_TITLE 2     /* on title clicked */

#define LIST_ADJUSTWIDTH   (1<<0)
#define LIST_ADJUSTHEIGHT  (1<<1)
#define LIST_AUTOVISIBLE   (1<<2)
#define LIST_DRAGSORTABLE  (1<<3)
#define LIST_SHOWDROPMARKS (1<<4)
#define LIST_QUIET         (1<<5)
#define LIST_CHANGED    (1<<6)

static BOOL IncreaseColumns(struct MUI_ListData *data, int new_columns);

/****** List.mui/MUIA_List_Active ********************************************
*
*   NAME
*       MUIA_List_Active -- (V4) [ISG], LONG
*
*   FUNCTION
*       The index of the active entry. There can be at most one active entry
*       in a list. The active entry is highlighted visibly, except for
*       read-only lists (those whose Listview has MUIA_Listview_Input set to
*       FALSE). Selecting an entry with the mouse, or moving through the list
*       with keyboard controls will also change the active entry (again
*       excepting read-only lists).
*
*       When set programmatically through this attribute, some special values
*       can be used:
*
*           MUIV_List_Active_Off
*           MUIV_List_Active_Top
*           MUIV_List_Active_Bottom
*           MUIV_List_Active_Up
*           MUIV_List_Active_Down
*           MUIV_List_Active_PageUp
*           MUIV_List_Active_PageDown
*
*       When this attribute is read, either the index of the active entry or
*       the special value MUIV_List_Active_Off will be returned.
*
*       Setting this attribute to a new value will additionally have the same
*       effect as calling the MUIM_List_Jump method with the specified or
*       implied index.
*
*   NOTES
*       The concept of an active entry must not be confused with that of a
*       selected entry.
*
*   SEE ALSO
*       MUIM_List_Jump, MUIM_List_Select, MUIA_Listview_Input
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_AutoVisible ***************************************
*
*   NAME
*       MUIA_List_Visible -- (V11) [ISG], BOOL
*
*   FUNCTION
*       When this attribute is set to true, the active entry will be in the
*       visible portion of the list whenever the list is unhidden.
*
*   SEE ALSO
*       MUIA_List_Active
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_CompareHook ***************************************
*
*   NAME
*       MUIA_List_CompareHook -- (V4) [IS.], struct Hook *
*
*   FUNCTION
*       The provided hook indicates the sort ordering of two list entries.
*       The hook receives list-entry data pointers as its second and third
*       arguments. The hook should return a negative value if the first entry
*       should be placed before the second entry, a positive value if the
*       first entry should be placed after the second entry, and zero if the
*       entries are equal.
*
*       In addition to being used internally for sorting operations, this hook
*       will be called when MUIM_List_Compare is externally invoked.
*
*       If this attribute is not specified or is set to NULL, all list entries
*       must be strings.
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_ConstructHook *************************************
*
*   NAME
*       MUIA_List_ConstructHook -- (V4) [IS.], struct Hook *
*
*   FUNCTION
*       The provided hook creates a list entry. Whenever a data item is added
*       to the list, it is the result of this hook that is really added.

*       The hook receives the list's memory pool as its second argument and
*       the data item for the entry as its third argument. The memory pool may
*       be used for allocating memory for the new entry if desired. The hook
*       should return the new data entry, or NULL if an error occurred.
*
*       If this attribute is not specified or is set to NULL, all list entries
*       must be strings (which must continue to exist for the lifetime of the
*       list).
*
*       If you want string entries to be duplicated within the list, so that
*       they do not have to be preserved externally, a built-in hook for this
*       purpose can be used by providing the special value
*       MUIV_List_ConstructHook_String.
*
*       Whenever this attribute is specified, a matching hook must be
*       specified for the MUIA_ListDestructHook attribute.
*
*   SEE ALSO
*       MUIA_List_DestructHook
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_DestructHook **************************************
*
*   NAME
*       MUIA_List_DestructHook -- (V4) [IS.], struct Hook *
*
*   FUNCTION
*       The provided hook destroys a list entry, and is called whenever a data
*       item is removed from the list. It can be used to deallocate any
*       resources for the entry that were allocated by the hook specified for
*       MUIA_List_ConstructHook.
*
*       The hook receives the list's memory pool as its second argument and
*       the list entry to be destroyed as its third argument. The hook should
*       have no return value.
*
*       If this attribute is not specified or is set to NULL, all list entries
*       must be strings (which must continue to exist for the lifetime of the
*       list).
*
*       If you want string entries to be duplicated within the list, so that
*       they do not have to be preserved externally, a built-in hook for this
*       purpose can be used by providing the special value
*       MUIV_List_ConstructHook_String.
*
*       This attribute must only be specified when a matching hook has been
*       specified for the MUIA_List_ConstructHook attribute. If you specified
*       MUIV_List_ConstructHook_String for MUIA_List_ConstructHook, you must
*       specify MUIV_List_DestructHook_String for this attribute.
*
*   SEE ALSO
*       MUIA_List_ConstructHook
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_DropMark ******************************************
*
*   NAME
*       MUIA_List_DropMark -- (V11) [..G], LONG
*
*   FUNCTION
*       Provides the index of the last position where a list entry was
*       successfully dropped. The initial value before any entry has been
*       dropped is undefined.
*
*   SEE ALSO
*       MUIA_List_DragSortable, MUIA_List_ShowDropMarks,
*       MUIA_Listview_Draggable
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_First *********************************************
*
*   NAME
*       MUIA_List_First -- (V4) [..G], LONG
*
*   FUNCTION
*       The index of the first entry that can be seen (assuming nothing
*       obscures the list) This value of this attribute is -1 when the
*       list's window is not open.
*
*   NOTES
*       Notification does not occur on this attribute in MUI.
*
*   SEE ALSO
*       MUIA_List_First, MUIA_List_Entries
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_MultiTestHook *************************************
*
*   NAME
*       MUIA_List_MultiTestHook -- (V4) [IS.], struct Hook *
*
*   FUNCTION
*       The provided hook indicates whether a particular list entry
*       may be multiselected. The hook receives the list-entry data pointer as
*       its third argument, and returns a Boolean value. If this attribute is
*       not specified or is set to NULL, all list entries are considered
*       multi-selectable.
*
*       Whenever an entry is about to be selected, this hook is called if
*       there are other entries already selected. If the hook returns TRUE,
*       the entry may be multi-selected; if the hook returns FALSE, the entry
*       remains unselected.
*
*       Additionally, if a non-multi-selectable entry has been selected (as
*       the only selected entry in the list), any attempt to select an
*       additional entry will fail.
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_ShowDropMarks *************************************
*
*   NAME
*       MUIA_List_ShowDropMarks -- (V11) [ISG], BOOL
*
*   FUNCTION
*       Specifies whether a visual indication will be shown of where a list
*       item will be inserted if dropped during a drag-and-drop operation.
*       Defaults to TRUE. Only has an affect when the list is a drop target
*       (e.g. when MUIA_List_DragSortable is TRUE).
*
*   SEE ALSO
*       MUIA_List_DragSortable, MUIA_List_DropMark, MUIA_Listview_Draggable
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_Title *********************************************
*
*   NAME
*       MUIA_List_Title -- (V6) [ISG], char *
*
*   FUNCTION
*       A heading for the list, placed above list entries. A value of NULL
*       means no title is used. A value of TRUE means that the custom
*       display hook provides a separate title for each column; the hook
*       must then provide column titles instead of normal column data when
*       the entry pointer provided is NULL.
*
*   NOTES
*       If a string is set for this attribute, it is not cached within the
*       object.
*
*   SEE ALSO
*       MUIA_List_DisplayHook
*
******************************************************************************
*
*/

/****** List.mui/MUIA_List_Visible *******************************************
*
*   NAME
*       MUIA_List_Visible -- (V4) [..G], LONG
*
*   FUNCTION
*       The number of entries that can be seen at once with the list's
*       current dimensions. This value of this attribute is -1 when the
*       list's window is not open.
*
*   NOTES
*       Notification does not occur on this attribute in MUI.
*
*   SEE ALSO
*       MUIA_List_First, MUIA_List_Entries
*
******************************************************************************
*
*/

/**************************************************************************
 Allocate a single list entry, does not initialize it (except the pointer)
**************************************************************************/
static struct ListEntry *AllocListEntry(struct MUI_ListData *data)
{
    struct ListEntry *le;
    /* use IncreaseColumns() to enlarge column entry array */
    IPTR size = sizeof(struct ListEntry) + sizeof(LONG) * (data->columns + 1);

    le = (struct ListEntry *) AllocVecPooled(data->pool, size);
    D(bug("List AllocListEntry %p, %ld bytes\n", le, size));
    if (le)
    {
        /* possible, that we have an external pool, which does not have 
           MEMF_CLEAR set.. */
        memset(le, 0, size);
    }
    return le;
}

/**************************************************************************
 Deallocate a single list entry, does not deinitialize it
**************************************************************************/
static void FreeListEntry(struct MUI_ListData *data,
    struct ListEntry *entry)
{
    D(bug("FreeListEntry %p\n", entry));
    FreeVecPooled(data->pool, entry);
}

/**************************************************************************
 Ensures that there can be at least the given amount of entries within
 the list. Returns 0 if not. It also allocates the space for the title.
 It can be accessed with data->entries[ENTRY_TITLE]
**************************************************************************/
static int SetListSize(struct MUI_ListData *data, LONG size)
{
    struct ListEntry **new_entries;
    int new_entries_allocated;

    if (size + 1 <= data->entries_allocated)
        return 1;

    new_entries_allocated = data->entries_allocated * 2 + 4;
    if (new_entries_allocated < size + 1)
        new_entries_allocated = size + 1 + 10;  /* 10 is just random */

    D(bug("List %p : SetListSize allocating %ld bytes\n", data,
            new_entries_allocated * sizeof(struct ListEntry *)));
    new_entries =
        AllocVec(new_entries_allocated * sizeof(struct ListEntry *), 0);
    if (NULL == new_entries)
        return 0;
    if (data->entries)
    {
        CopyMem(data->entries - 1, new_entries,
            (data->entries_num + 1) * sizeof(struct ListEntry *));
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
static int PrepareInsertListEntries(struct MUI_ListData *data, int pos,
    int count)
{
    memmove(&data->entries[pos + count], &data->entries[pos],
        (data->entries_num - pos) * sizeof(struct ListEntry *));
    return 1;
}

/**************************************************************************
 Removes count (already deinitalized) list entries starting az pos.
**************************************************************************/
static void RemoveListEntries(struct MUI_ListData *data, int pos, int count)
{
    // FIXME: segfault if entries_num = pos = count = 1
    memmove(&data->entries[pos], &data->entries[pos + count],
        (data->entries_num - (pos + count)) * sizeof(struct ListEntry *));
}

/**************************************************************************
 Frees all memory allocated by ParseListFormat()
**************************************************************************/
static void FreeListFormat(struct MUI_ListData *data)
{
    int i;

    if (data->ci)
    {
        for (i = 0; i < data->columns; i++)
        {
            FreeVec(data->ci[i].preparse);
            data->ci[i].preparse = NULL;
        }
        FreeVec(data->ci);
        data->ci = NULL;
    }
    FreeVec(data->preparses);
    data->preparses = NULL;
    if (data->strings_mem)
    {
        FreeVec(data->strings_mem);
        data->strings_mem = NULL;
        data->strings = NULL;
    }
    data->columns = 0;
}

/**************************************************************************
 Parses the given format string (also frees a previously parsed format).
 Use initial=FALSE for format changes outside OM_NEW.
 Return FALSE on failure.
**************************************************************************/
static BOOL ParseListFormat(struct MUI_ListData *data, STRPTR format,
    BOOL initial)
{
    int new_columns, i;
    STRPTR ptr;
    STRPTR format_sep;
    char c;

    IPTR args[ARG_CNT];
    struct RDArgs *rdargs;

    if (!format)
        format = (STRPTR) "";

    ptr = format;

    FreeListFormat(data);

    new_columns = 1;

    /* Count the number of columns first */
    while ((c = *ptr++))
        if (c == ',')
            new_columns++;

    if (!(data->preparses =
            AllocVec((new_columns + 10) * sizeof(STRPTR), MEMF_CLEAR)))
        return FALSE;

    if (!(data->strings_mem = AllocVec((new_columns + 1 + 10)
        * sizeof(STRPTR), MEMF_CLEAR)))    
                                  /* hold enough space also for the entry pos,
                                   * used by orginal MUI and also some
                                   * security space */
        return FALSE;
    data->strings=data->strings_mem;

    if (!(data->ci = AllocVec(new_columns * sizeof(struct ColumnInfo), MEMF_CLEAR)))
        return FALSE;

    // set defaults
    for (i = 0; i < new_columns; i++)
    {
        data->ci[i].colno = -1; // -1 means: use unassigned column
        data->ci[i].weight = 100;
        data->ci[i].delta = 4;
        data->ci[i].min_width = -1;
        data->ci[i].max_width = -1;
        data->ci[i].user_width = -1;
        data->ci[i].bar = FALSE;
        data->ci[i].preparse = NULL;
    }

    if ((format_sep = StrDup(format)) != 0)
    {
        for (i = 0; format_sep[i] != '\0'; i++)
        {
            if (format_sep[i] == ',')
                format_sep[i] = '\0';
        }

        if ((rdargs = AllocDosObject(DOS_RDARGS, NULL)) != 0)
        {
            ptr = format_sep;
            i = 0;
            do
            {
                rdargs->RDA_Source.CS_Buffer = ptr;
                rdargs->RDA_Source.CS_Length = strlen(ptr);
                rdargs->RDA_Source.CS_CurChr = 0;
                rdargs->RDA_DAList = 0;
                rdargs->RDA_Buffer = NULL;
                rdargs->RDA_BufSiz = 0;
                rdargs->RDA_ExtHelp = NULL;
                rdargs->RDA_Flags = 0;

                memset(args, 0, sizeof args);
                if (ReadArgs(FORMAT_TEMPLATE, args, rdargs))
                {
                    if (args[ARG_COL])
                        data->ci[i].colno = *(LONG *) args[ARG_COL];
                    if (args[ARG_WEIGHT])
                        data->ci[i].weight = *(LONG *) args[ARG_WEIGHT];
                    if (args[ARG_DELTA])
                        data->ci[i].delta = *(LONG *) args[ARG_DELTA];
                    if (args[ARG_MINWIDTH])
                        data->ci[i].min_width =
                            *(LONG *) args[ARG_MINWIDTH];
                    if (args[ARG_MAXWIDTH])
                        data->ci[i].max_width =
                            *(LONG *) args[ARG_MAXWIDTH];
                    data->ci[i].bar = args[ARG_BAR];
                    if (args[ARG_PREPARSE])
                        data->ci[i].preparse =
                            StrDup((STRPTR) args[ARG_PREPARSE]);

                    FreeArgs(rdargs);
                }
                ptr += strlen(ptr) + 1;
                i++;
            }
            while (i < new_columns);
            FreeDosObject(DOS_RDARGS, rdargs);
        }
        FreeVec(format_sep);
    }

    for (i = 0; i < new_columns; i++)
    {
        D(bug("colno %d weight %d delta %d preparse %s\n",
                data->ci[i].colno, data->ci[i].weight, data->ci[i].delta,
                data->ci[i].preparse));
    }

    if (initial)
    {
        /* called from OM_NEW */
        data->columns_allocated = new_columns;
    }
    else if (data->columns_allocated < new_columns) 
    {
        /* called by MUIA_List_Format */
        if (!IncreaseColumns(data, new_columns))
        {
            bug("[Zune:List] not enough memory for new columns!!\n");
            /* FIXME: proper handling? */
            return FALSE;
        }
        data->columns_allocated = new_columns;
    }
    data->columns = new_columns;
    /* Skip entry pos (-1) and enough indexes to get (-9). Based on MPlayer source codes. */
    data->strings += 10;

    return TRUE;
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

    for (col = 0; col < data->columns; col++)
        data->preparses[col] = data->ci[col].preparse;

    if (entry_pos == ENTRY_TITLE)
    {
        if ((data->columns == 1) && (data->title != (STRPTR) 1))
        {
            *data->strings = data->title;
            return;
        }
        entry_data = NULL;      /* it's a title request */
    }
    else
    {
        data->strings[-1] = (STRPTR)(IPTR)entry_pos;
        entry_data = data->entries[entry_pos]->data;
    }

    /* Get the display formation */
    DoMethod(obj, MUIM_List_Display, (IPTR) entry_data,
        (IPTR) data->strings, entry_pos, (IPTR) data->preparses);
}

/**************************************************************************
 Determine the dims of a single entry and adapt the columninfo according
 to it. pos might be ENTRY_TITLE. Returns 0 if pos entry needs to
 be redrawn after this operation, 1 if all entries need to be redrawn.
**************************************************************************/
static int CalcDimsOfEntry(struct IClass *cl, Object *obj, int pos)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListEntry *entry = data->entries[pos];
    int j;
    int ret = 0;

    if (!entry)
        return ret;

    if (!(_flags(obj) & MADF_SETUP))
        return ret;

    DisplayEntry(cl, obj, pos);

    /* Set height to at least minheight */
    if (data->entries[pos]->height < data->entry_minheight)
        data->entries[pos]->height = data->entry_minheight;

    for (j = 0; j < data->columns; j++)
    {
        ZText *text =
            zune_text_new(data->preparses[j], data->strings[j],
            ZTEXT_ARG_NONE, 0);
        if (text != NULL)
        {
            zune_text_get_bounds(text, obj);

            if (text->height > data->entries[pos]->height)
            {
                data->entries[pos]->height = text->height;
                /* entry height changed, redraw all entries later */
                ret = 1;
            }
            data->entries[pos]->widths[j] = text->width;

            if (text->width > data->ci[j].entries_width)
            {
                /* This entry has a greater width for this column than any
                 * other entry, so we store this value
                 */
                data->ci[j].entries_width = text->width;
                /* column width changed, redraw all entries later */
                ret = 1;
            }

            zune_text_destroy(text);
        }
    }
    if (data->entries[pos]->height > data->entry_maxheight)
    {
        data->entry_maxheight = data->entries[pos]->height;
        /* maximum entry height changed, redraw all entries later */
        ret = 1;
    }

    return ret;
}

/**************************************************************************
 Determine the widths of the entries
**************************************************************************/
static void CalcWidths(struct IClass *cl, Object *obj)
{
    int i, j;
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (!(_flags(obj) & MADF_SETUP))
        return;

    for (j = 0; j < data->columns; j++)
        data->ci[j].entries_width = 0;

    data->entry_maxheight = 0;
    data->entries_totalheight = 0;
    data->entries_maxwidth = 0;

    for (i = (data->title ? ENTRY_TITLE : 0); i < data->entries_num; i++)
    {
        CalcDimsOfEntry(cl, obj, i);
        data->entries_totalheight += data->entries[i]->height;
    }

    for (j = 0; j < data->columns; j++)
        data->entries_maxwidth += data->ci[j].entries_width
            + data->ci[j].delta + (data->ci[j].bar ? BAR_WIDTH : 0);

    if (!data->entry_maxheight)
        data->entry_maxheight = 1;
}

/**************************************************************************
 Calculates the number of visible entry lines. Returns 1 if it has
 changed
**************************************************************************/
static int CalcVertVisible(struct IClass *cl, Object *obj)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int old_entries_visible = data->entries_visible;
    int old_entries_top_pixel = data->entries_top_pixel;

    data->vertprop_visible = data->entries_visible =
        (_mheight(data->area) - data->title_height)
        / (data->entry_maxheight /* + data->prefs_linespacing */ );

    /* Distribute extra vertical space evenly between top and bottom of
     * list */

    data->entries_top_pixel = _mtop(data->area) + data->title_height
        + (_mheight(data->area) - data->title_height
        -
        data->entries_visible *
        (data->entry_maxheight /* + data->prefs_linespacing */ )) / 2;

    if (data->entries_visible != old_entries_visible)
    {
        superset(cl, obj, MUIA_List_Visible, data->entries_visible);
        superset(cl, obj, MUIA_List_VertProp_Visible, data->entries_visible);
    }

    return (old_entries_visible != data->entries_visible)
        || (old_entries_top_pixel != data->entries_top_pixel);
}

/**************************************************************************
 Resize arrays, so that there is enough space for new columns.
 The widths[] array must grow with increasing columns.
 columns_allocated must be checked, before calling this function.
 Space can only grow, not shrink.
 Return FALSE on error (no memory).
**************************************************************************/
static BOOL IncreaseColumns(struct MUI_ListData *data, int new_columns) 
{
    int i = 0;
    IPTR newsize, oldsize;
    struct ListEntry *le;

    D(bug("IncreaseColumns: %d => %d columns\n", data->columns_allocated, new_columns));

    newsize = sizeof(struct ListEntry) + sizeof(LONG) * (new_columns             + 1);
    oldsize = sizeof(struct ListEntry) + sizeof(LONG) * (data->columns_allocated + 1);

    if (data->title)
    {
        i = -1;
    }

    for (; i < data->entries_num; i++)
    {
        D(bug("IncreaseColumns: i: %d, size: %d => %d\n", i, oldsize, newsize));
        le = (struct ListEntry *) AllocVecPooled(data->pool, newsize);
        if (!le)
        {
            return FALSE;
        }
        memset(le, 0, newsize);
        D(bug("IncreaseColumns: CopyMem(%p, %p, %d)\n", data->entries[i],
            le, oldsize));
        CopyMem(data->entries[i], le, oldsize);
        FreeVecPooled(data->pool, data->entries[i]);
        data->entries[i]=le;
    }

    return TRUE;
}

/**************************************************************************
 Default hook to compare two list entries. Works for strings only.
**************************************************************************/
AROS_UFH3S(int, default_compare_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char *, s2, A2),
    AROS_UFHA(char *, s1, A1))
{
    AROS_USERFUNC_INIT

    return Stricmp(s1, s2);

    AROS_USERFUNC_EXIT
}

#define PROP_VERT_FIRST   1

static ULONG List_Function(struct Hook *hook, Object * obj, void **msg)
{
    struct MUI_ListData *data = (struct MUI_ListData *)hook->h_Data;
    SIPTR type = (SIPTR) msg[0];
    SIPTR val = (SIPTR) msg[1];

    switch (type)
    {
    case PROP_VERT_FIRST:
        get(data->vert, MUIA_Prop_First, &val);
        nnset(obj, MUIA_List_VertProp_First, val);
        break;
    }
    return 0;
}

/* At entry to this function, data->area is always set, but data->vert may
 * or may not be set */
static void List_HandleScrollerPos(struct IClass *cl, Object *obj)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    BOOL vert_not_used = FALSE;

    /* Disallow any changes after setup. This function should basically be
     * creation-time only */
    if (_flags(obj) & MADF_SETUP)
        return;

    /* Remove both objects */
    if (data->area_connected)
        DoMethod(obj, OM_REMMEMBER, data->area);
    if (data->vert_connected)
        DoMethod(obj, OM_REMMEMBER, data->vert);

    /* Add list and/or scroller */
    switch (data->scroller_pos)
    {
    case MUIV_Listview_ScrollerPos_None:
        vert_not_used = TRUE;
        DoMethod(obj, OM_ADDMEMBER, data->area);
        break;
    case MUIV_Listview_ScrollerPos_Left:
        if (!data->vert)
            data->vert =ScrollbarObject, MUIA_Group_Horiz, FALSE, End;
        DoMethod(obj, OM_ADDMEMBER, data->vert);
        DoMethod(obj, OM_ADDMEMBER, data->area);
        break;
    default:
        if (!data->vert)
            data->vert = ScrollbarObject, MUIA_Group_Horiz, FALSE, End;
        DoMethod(obj, OM_ADDMEMBER, data->area);
        DoMethod(obj, OM_ADDMEMBER, data->vert);
        break;
    }

    data->area_connected = TRUE;

    /* Handle case where it was decided that vert will not be used */
    if (vert_not_used)
    {
        if (data->vert)
        {
            if (data->vert_connected)
            {
                DoMethod(obj, MUIM_KillNotifyObj, MUIA_List_VertProp_First,
                    (IPTR) data->vert);
                DoMethod(obj, MUIM_KillNotifyObj, MUIA_List_VertProp_Visible,
                    (IPTR) data->vert);
                DoMethod(obj, MUIM_KillNotifyObj, MUIA_List_VertProp_Entries,
                    (IPTR) data->vert);
                data->vert_connected = FALSE;
            }

            MUI_DisposeObject(data->vert);
            data->vert = NULL;
        }
    }

    /* If at this point data->vert is not null, it means vert is to be
     * connected */
    if (data->vert && !data->vert_connected)
    {
        LONG entries = 0, first = 0, visible = 0;

        get(obj, MUIA_List_VertProp_First, &first);
        get(obj, MUIA_List_VertProp_Visible, &visible);
        get(obj, MUIA_List_VertProp_Entries, &entries);

        SetAttrs(data->vert, MUIA_Prop_First, first,
            MUIA_Prop_Visible, visible, MUIA_Prop_Entries, entries, TAG_DONE);

        DoMethod(data->vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime,
            (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->hook, PROP_VERT_FIRST,
            MUIV_TriggerValue);

        /* Pass prop object as DestObj (based on code in NList) */
        DoMethod(obj, MUIM_Notify, MUIA_List_VertProp_First, MUIV_EveryTime,
            (IPTR) data->vert, 3, MUIM_NoNotifySet,
            MUIA_Prop_First, MUIV_TriggerValue);
        DoMethod(obj, MUIM_Notify, MUIA_List_VertProp_Visible, MUIV_EveryTime,
            (IPTR) data->vert, 3, MUIM_NoNotifySet,
            MUIA_Prop_Visible, MUIV_TriggerValue);
        DoMethod(obj, MUIM_Notify, MUIA_List_VertProp_Entries, MUIV_EveryTime,
            (IPTR) data->vert, 3, MUIM_NoNotifySet,
            MUIA_Prop_Entries, MUIV_TriggerValue);

        data->vert_connected = TRUE;
    }
}

/**************************************************************************
 OM_NEW
**************************************************************************/
IPTR List__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListData *data;
    struct TagItem *tag;
    struct TagItem *tags;
    APTR *array = NULL;
    LONG new_entries_active = MUIV_List_Active_Off;
    struct TagItem rectattrs[2] =
        {{TAG_IGNORE, TAG_IGNORE }, {TAG_DONE, TAG_DONE}};
    Object *area;

    /* search for MUIA_Frame as it has to be passed to rectangle object */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        if (tag->ti_Tag == MUIA_Frame)
        {
            rectattrs[0].ti_Tag = MUIA_Frame;
            rectattrs[0].ti_Data = tag->ti_Data;
            tag->ti_Tag = TAG_IGNORE;
            break;
        }
    }

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Horiz, TRUE,
        MUIA_InnerLeft, 0,
        MUIA_InnerRight, 0,
        MUIA_Group_Spacing, 0,
        MUIA_Font, MUIV_Font_List,
        MUIA_ShowSelState, FALSE,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Background, MUII_ListBack,
        TAG_MORE, (IPTR) msg->ops_AttrList,
        TAG_DONE);

    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->columns = 1;
    data->entries_active = MUIV_List_Active_Off;
    data->intern_puddle_size = 2008;
    data->intern_thresh_size = 1024;
    data->default_compare_hook.h_Entry = (HOOKFUNC) default_compare_func;
    data->default_compare_hook.h_SubEntry = 0;
    data->compare_hook = &(data->default_compare_hook);
    data->flags = LIST_SHOWDROPMARKS;
    data->area_replaced = FALSE;
    data->area_connected = FALSE;
    data->vert_connected = FALSE;

    data->entries_visible = data->vertprop_visible = -1;
    data->last_active = -1;
    data->drop_mark = 0;

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    data->mouse_click = 0;
    data->mouse_x = MUI_MAXMAX;
    data->mouse_y = MUI_MAXMAX;

    /* HACK:
     * List is a group where part of area is rendered and part is filled
     * with other objects (inside of List dimensions). One such object is
     * up/down arrow. This object depends on RelVerify mode to control
     * behaviour. List also has the RelVerify mode. Area super class in case
     * of both of those objects adds an event handler node with the same
     * priority. Depending on the sort order, the event handler node of
     * "list" can eat a click event and up/down arrows stop working. The
     * hack is to decrease the priority of Area event handler for list to
     * always favor up/down arrow. There are other hacky ways of solving
     * this, but this seems least evil approach, as this hack is
     * encapsulated in the List class itself.
     */
    muiAreaData(obj)->mad_ehn.ehn_Priority--;

    data->hook.h_Entry = HookEntry;
    data->hook.h_SubEntry = (HOOKFUNC) List_Function;
    data->hook.h_Data = data;

    area = (Object *)GetTagData(MUIA_List_ListArea, (IPTR) 0,
        msg->ops_AttrList);

    if (!area)
        area = RectangleObject, MUIA_FillArea, FALSE, TAG_MORE,
            (IPTR) rectattrs, End;
    else
        data->area_replaced = TRUE;
    data->area = area;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_List_Active:
            new_entries_active = tag->ti_Data;
            break;

        case MUIA_List_Pool:
            data->pool = (APTR) tag->ti_Data;
            break;

        case MUIA_List_PoolPuddleSize:
            data->intern_puddle_size = tag->ti_Data;
            break;

        case MUIA_List_PoolThreshSize:
            data->intern_thresh_size = tag->ti_Data;
            break;

        case MUIA_List_CompareHook:
            data->compare_hook = (struct Hook *)tag->ti_Data;
            if (data->compare_hook == NULL)
                data->compare_hook = &data->default_compare_hook;
            break;

        case MUIA_List_ConstructHook:
            data->construct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DestructHook:
            data->destruct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DisplayHook:
            data->display_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_MultiTestHook:
            data->multi_test_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_SourceArray:
            array = (APTR *) tag->ti_Data;
            break;

        case MUIA_List_Format:
            data->format = (STRPTR) tag->ti_Data;
            break;

        case MUIA_List_Title:
            data->title = (STRPTR) tag->ti_Data;
            break;

        case MUIA_List_MinLineHeight:
            data->entry_minheight = tag->ti_Data;
            break;

        case MUIA_List_AdjustHeight:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_ADJUSTHEIGHT);
            break;

        case MUIA_List_AdjustWidth:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_ADJUSTWIDTH);
            break;

        case MUIA_List_AutoVisible:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_AUTOVISIBLE);
            break;

        case MUIA_List_ShowDropMarks:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_SHOWDROPMARKS);
            break;

        case MUIA_List_DragSortable:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_DRAGSORTABLE);
            break;

        case MUIA_Listview_ScrollerPos:
            data->scroller_pos = tag->ti_Data;
            break;

        case MUIA_Listview_Input:
            data->read_only = !tag->ti_Data;
            break;

        case MUIA_Listview_MultiSelect:
            data->multiselect = tag->ti_Data;
            break;

        case MUIA_Listview_DefClickColumn:
            data->def_click_column = tag->ti_Data;
            break;

        case MUIA_Listview_DragType:
            data->drag_type = tag->ti_Data;
            if (data->drag_type != MUIV_Listview_DragType_None)
                set(obj, MUIA_Draggable, TRUE);
            break;
        }
    }

    List_HandleScrollerPos(cl, obj);

    if (!data->pool)
    {
        /* No memory pool given, so we create our own */
        data->pool = data->intern_pool =
            CreatePool(0, data->intern_puddle_size,
            data->intern_thresh_size);
        if (!data->pool)
        {
            CoerceMethod(cl, obj, OM_DISPOSE);
            return 0;
        }
    }

    /* parse the list format */
    if (!ParseListFormat(data, data->format, TRUE))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    /* This is necessary for at least the title */
    if (!SetListSize(data, 0))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (!(data->entries[ENTRY_TITLE] = AllocListEntry(data)))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (array)
    {
        int i;
        /* Count the number of elements */
        for (i = 0; array[i] != NULL; i++)
            ;
        /* Insert them */
        DoMethod(obj, MUIM_List_Insert, (IPTR) array, i,
            MUIV_List_Insert_Top);
    }

    if ((data->entries_num) && (new_entries_active != MUIV_List_Active_Off))
    {
        switch (new_entries_active)
        {
        case MUIV_List_Active_Top:
            new_entries_active = 0;
            break;

        case MUIV_List_Active_Bottom:
            new_entries_active = data->entries_num - 1;
            break;
        }

        if (new_entries_active < 0)
            new_entries_active = 0;
        else if (new_entries_active >= data->entries_num)
            new_entries_active = data->entries_num - 1;

        data->entries_active = new_entries_active;
        /* Selected entry will be moved into visible area */
    }

    NewList((struct List *)&data->images);

    D(bug("List_New(0x%p)\n", obj));

    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
IPTR List__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    D(bug("List Dispose\n"));

    /* Call destruct method for every entry and free the entries manually
     * to avoid notification */
    while (data->confirm_entries_num)
    {
        struct ListEntry *lentry =
            data->entries[--data->confirm_entries_num];
        DoMethod(obj, MUIM_List_Destruct, (IPTR) lentry->data,
            (IPTR) data->pool);
        FreeListEntry(data, lentry);
    }

    if (data->intern_pool)
        DeletePool(data->intern_pool);
    if (data->entries)
        FreeVec(data->entries - 1);
            /* title is currently before all other elements */

    FreeListFormat(data);

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 OM_SET
**************************************************************************/
IPTR List__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct TagItem *tag;
    struct TagItem *tags;

    /* parse taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_List_CompareHook:
            data->compare_hook = (struct Hook *)tag->ti_Data;
            if (data->compare_hook == NULL)
                data->compare_hook = &data->default_compare_hook;
            break;

        case MUIA_List_ConstructHook:
            data->construct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DestructHook:
            data->destruct_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_DisplayHook:
            data->display_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_List_MultiTestHook:
            data->multi_test_hook = (struct Hook *)tag->ti_Data;
            if (data->multi_test_hook != NULL)
            {
                /* Clearing current selections is the easiest way to keep
                 * selections consistent with the new hook */
                DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All,
                    MUIV_List_Select_Off, NULL);
            }
            break;

        case MUIA_List_Title:
            data->title = (STRPTR) tag->ti_Data;
            DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
            break;

        case MUIA_List_VertProp_First:
            data->vertprop_first = tag->ti_Data;
            if (data->entries_first != tag->ti_Data)
            {
                set(obj, MUIA_List_First, tag->ti_Data);
            }
            break;

        case MUIA_List_Format:
            data->format = (STRPTR) tag->ti_Data;
            ParseListFormat(data, data->format, FALSE);
            // FIXME: should we check for errors?
            DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
            break;

        case MUIA_List_VertProp_Entries:
            data->vertprop_entries = tag->ti_Data;
            break;

        case MUIA_List_VertProp_Visible:
            data->vertprop_visible = tag->ti_Data;
            data->entries_visible = tag->ti_Data;
            break;

        case MUIA_List_Active:
            {
                LONG new_entries_active = tag->ti_Data;

                if ((data->entries_num)
                    && (new_entries_active != MUIV_List_Active_Off))
                {
                    switch (new_entries_active)
                    {
                    case MUIV_List_Active_Top:
                        new_entries_active = 0;
                        break;

                    case MUIV_List_Active_Bottom:
                        new_entries_active = data->entries_num - 1;
                        break;

                    case MUIV_List_Active_Up:
                        new_entries_active = data->entries_active - 1;
                        break;

                    case MUIV_List_Active_Down:
                        new_entries_active = data->entries_active + 1;
                        break;

                    case MUIV_List_Active_PageUp:
                        new_entries_active =
                            data->entries_active - data->entries_visible;
                        break;

                    case MUIV_List_Active_PageDown:
                        new_entries_active =
                            data->entries_active + data->entries_visible;
                        break;
                    }

                    if (new_entries_active < 0)
                        new_entries_active = 0;
                    else if (new_entries_active >= data->entries_num)
                        new_entries_active = data->entries_num - 1;
                }
                else
                    new_entries_active = -1;

                if (data->entries_active != new_entries_active)
                {
                    LONG old = data->entries_active;
                    data->entries_active = new_entries_active;

                    /* SelectChange stuff */
                    if (new_entries_active != -1)
                    {
                        DoMethod(obj, MUIM_List_SelectChange,
                            new_entries_active, MUIV_List_Select_On, 0);
                        DoMethod(obj, MUIM_List_SelectChange,
                            new_entries_active, MUIV_List_Select_Active, 0);
                    }
                    else
                        DoMethod(obj, MUIM_List_SelectChange,
                            MUIV_List_Active_Off, MUIV_List_Select_Off, 0);

                    if (!data->read_only)
                    {
                        data->entries[old]->flags |= ENTRY_RENDER;
                        if (!(data->flags & LIST_QUIET))
                        {
                            data->update = UPDATEMODE_ENTRY;
                            data->update_pos = old;
                            MUI_Redraw(obj, MADF_DRAWUPDATE);
                        }
                        data->entries[data->entries_active]->flags |= ENTRY_RENDER;
                        if (!(data->flags & LIST_QUIET))
                        {
                            data->update = UPDATEMODE_ENTRY;
                            data->update_pos = data->entries_active;
                            MUI_Redraw(obj, MADF_DRAWUPDATE);
                        }
                        else
                        {
                            data->update = UPDATEMODE_NEEDED;
                            data->flags |= LIST_CHANGED;
                        }
                    }

                    /* Make new active entry visible (if there is one and
                       list is visible) */
                    if (new_entries_active != -1
                        && (_flags(obj) & MADF_SETUP))
                    {
                        DoMethod(obj, MUIM_List_Jump,
                            MUIV_List_Jump_Active);
                    }
                }
            }
            break;

        case MUIA_List_First:
            data->update_pos = data->entries_first;
            data->entries_first = tag->ti_Data;
            if (!(data->flags & LIST_QUIET))
            {
                data->update = (UPDATEMODE_ENTRY|UPDATEMODE_ALL);
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                if (data->vertprop_first != tag->ti_Data)
                {
                    set(obj, MUIA_List_VertProp_First, tag->ti_Data);
                }
            }
            else
            {
                data->update = UPDATEMODE_ALL;
                data->flags |= LIST_CHANGED;
            }
            break;

        case MUIA_List_Visible:    /* Shouldn't be settable? */
            if (data->vertprop_visible != tag->ti_Data)
                set(obj, MUIA_List_VertProp_Visible, tag->ti_Data);
            break;

        case MUIA_List_Entries:
            if (data->confirm_entries_num == tag->ti_Data)
            {
                data->entries_num = tag->ti_Data;
                if (!(data->flags & LIST_QUIET))
                {
                    set(obj, MUIA_List_VertProp_Entries, data->entries_num);
                }
            }
            else
            {
                D(bug("Bug: confirm_entries != MUIA_List_Entries!\n"));
            }
            break;

        case MUIA_List_Quiet:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_QUIET);
            if (!(data->flags & LIST_QUIET))
            {
                if (data->flags & LIST_CHANGED)
                {
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                    if (data->entries_num != XGET(obj, MUIA_List_VertProp_Entries))
                        set(obj, MUIA_List_VertProp_Entries, data->entries_num);
                    if (data->entries_first != XGET(obj, MUIA_List_VertProp_First))
                        set(obj, MUIA_List_VertProp_First, data->entries_first);
                }
                data->flags &= ~LIST_CHANGED;
            }
            break;

        case MUIA_List_AutoVisible:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_AUTOVISIBLE);
            break;

        case MUIA_List_ShowDropMarks:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_SHOWDROPMARKS);
            break;

        case MUIA_List_DragSortable:
            _handle_bool_tag(data->flags, tag->ti_Data, LIST_DRAGSORTABLE);
            break;

        case MUIA_Selected:
            /* Swallow this so the Area class doesn't redraw us */
            tag->ti_Tag = TAG_IGNORE;
            break;

        case MUIA_Disabled:
            if (_flags(obj) & MADF_SETUP)
            {
                /* Stop listening for events we only listen to when mouse
                   button is down: we will not be informed of the button
                   being released */
                DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                    (IPTR) &data->ehn);
                data->ehn.ehn_Events &= ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS
                    | IDCMP_INACTIVEWINDOW);
                DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                    (IPTR) &data->ehn);
            }
            break;

        case MUIA_Listview_DoubleClick: /* private set */
            data->doubleclick = tag->ti_Data != 0;
            break;

        case MUIA_Listview_ScrollerPos: /* private set */
            data->scroller_pos = tag->ti_Data;
            List_HandleScrollerPos(cl, obj);
            break;

        case MUIA_Listview_Input: /* private set */
            data->read_only = !tag->ti_Data;
            break;

        case MUIA_Listview_MultiSelect: /* private set */
            data->multiselect = tag->ti_Data;
            break;

        case MUIA_Listview_DefClickColumn:
            data->def_click_column = tag->ti_Data;
            break;

        case MUIA_Listview_DragType:
            data->drag_type = tag->ti_Data;
            set(obj, MUIA_Draggable,
                tag->ti_Data != MUIV_Listview_DragType_None);
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
IPTR List__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_ListData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_List_Entries:
        STORE = data->entries_num;
        return 1;
    case MUIA_List_First:
        STORE = data->entries_first;
        return 1;
    case MUIA_List_Active:
        STORE = data->entries_active;
        return 1;
    case MUIA_List_InsertPosition:
        STORE = data->insert_position;
        return 1;
    case MUIA_List_Title:
        STORE = (IPTR) data->title;
        return 1;
    case MUIA_List_VertProp_Entries:
        STORE = data->vertprop_entries;
        return 1;
    case MUIA_List_VertProp_Visible:
    case MUIA_List_Visible:
        STORE = data->vertprop_visible;
        return 1;
    case MUIA_List_VertProp_First:
        STORE = data->vertprop_first;
        return 1;
    case MUIA_List_Format:
        STORE = (IPTR) data->format;
        return 1;
    case MUIA_List_AutoVisible:
        STORE = data->flags & LIST_AUTOVISIBLE;
        return 1;
    case MUIA_List_ShowDropMarks:
        STORE = data->flags & LIST_SHOWDROPMARKS;
        return 1;
    case MUIA_List_DragSortable:
        STORE = data->flags & LIST_DRAGSORTABLE;
        return 1;
    case MUIA_List_DropMark:
        STORE = data->drop_mark;
        return 1;
    case MUIA_Listview_ClickColumn:
        STORE = data->click_column;
        return 1;
    case MUIA_Listview_DoubleClick:
        STORE = data->doubleclick;
        return 1;
    case MUIA_Listview_SelectChange:
        STORE = FALSE;
        return 1;
    case MUIA_Listview_List:
        STORE = (IPTR)obj;
        return 1;
    case MUIA_Listview_DefClickColumn:
        STORE = data->def_click_column;
        return 1;
    case MUIA_Listview_DragType:
        STORE = data->drag_type;
        return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg))
        return 1;
    return 0;
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
IPTR List__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return 0;

    data->prefs_refresh = muiGlobalInfo(obj)->mgi_Prefs->list_refresh;
    data->prefs_linespacing =
        muiGlobalInfo(obj)->mgi_Prefs->list_linespacing;
    data->prefs_smoothed = muiGlobalInfo(obj)->mgi_Prefs->list_smoothed;
    data->prefs_smoothval = muiGlobalInfo(obj)->mgi_Prefs->list_smoothval;

    data->list_cursor =
        zune_imspec_setup(MUII_ListCursor, muiRenderInfo(obj));
    data->list_select =
        zune_imspec_setup(MUII_ListSelect, muiRenderInfo(obj));
    data->list_selcur =
        zune_imspec_setup(MUII_ListSelCur, muiRenderInfo(obj));

    data->prefs_multi = muiGlobalInfo(obj)->mgi_Prefs->list_multi;
    if (data->multiselect == MUIV_Listview_MultiSelect_Default)
    {
        if (data->prefs_multi == LISTVIEW_MULTI_SHIFTED)
            data->multiselect = MUIV_Listview_MultiSelect_Shifted;
        else
            data->multiselect = MUIV_Listview_MultiSelect_Always;
    }

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->ehn);

    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
IPTR List__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    zune_imspec_cleanup(data->list_cursor);
    zune_imspec_cleanup(data->list_select);
    zune_imspec_cleanup(data->list_selcur);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->ehn);
    data->mouse_click = 0;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
IPTR List__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    CalcWidths(cl, obj);

    if ((data->flags & LIST_ADJUSTWIDTH) && (data->entries_num > 0))
    {
        msg->MinMaxInfo->MinWidth += data->entries_maxwidth;
        msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
        msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;
    }
    else
    {
        msg->MinMaxInfo->MinWidth += 40;
        msg->MinMaxInfo->DefWidth += 100;
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    }

    if (data->entries_num > 0)
    {
        if (data->flags & LIST_ADJUSTHEIGHT)
        {
            msg->MinMaxInfo->MinHeight += data->entries_totalheight;
            msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
            msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
        }
        else
        {
            ULONG h = data->entry_maxheight + data->prefs_linespacing;
            msg->MinMaxInfo->MinHeight += 2 * h + data->prefs_linespacing;
            msg->MinMaxInfo->DefHeight += 8 * h + data->prefs_linespacing;
            msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
        }
    }
    else
    {
        msg->MinMaxInfo->MinHeight += 36;
        msg->MinMaxInfo->DefHeight += 96;
        msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    D(bug("List %p minheight=%d, line maxh=%d\n",
            obj, msg->MinMaxInfo->MinHeight, data->entry_maxheight));

    return TRUE;
}

/****i* List.mui/MUIM_Layout *************************************************
*
*   NAME
*       MUIM_Layout
*
******************************************************************************
*
*/

IPTR List__MUIM_Layout(struct IClass *cl, Object *obj,
    struct MUIP_Layout *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);
    LONG new_entries_first = data->entries_first;

    /* Calc the numbers of entries visible */
    CalcVertVisible(cl, obj);

    /* Ensure active entry is visible if requested */
    if (data->entries_active + 1 >=
        (data->entries_first + data->entries_visible)
        && (data->flags & LIST_AUTOVISIBLE) != 0)
        new_entries_first =
            data->entries_active - data->entries_visible + 1;

    /* Ensure there are no unnecessary empty lines */
    if ((new_entries_first + data->entries_visible >=
            data->entries_num)
        && (data->entries_visible <= data->entries_num))
        new_entries_first = data->entries_num - data->entries_visible;

    /* Always show the start of the list if it isn't long enough to fill the
       view */
    if (data->entries_num <= data->entries_visible)
        new_entries_first = 0;

    if (new_entries_first < 0)
        new_entries_first = 0;

    set(obj, new_entries_first != data->entries_first ?
        MUIA_List_First : TAG_IGNORE, new_entries_first);

    /* So the notify happens */
    set(obj, MUIA_List_VertProp_Visible, data->entries_visible);

    return rc;
}


/**************************************************************************
 MUIM_Show
**************************************************************************/
IPTR List__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg) msg);

    zune_imspec_show(data->list_cursor, obj);
    zune_imspec_show(data->list_select, obj);
    zune_imspec_show(data->list_selcur, obj);
    return rc;
}


/**************************************************************************
 MUIM_Hide
**************************************************************************/
IPTR List__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    zune_imspec_hide(data->list_cursor);
    zune_imspec_hide(data->list_select);
    zune_imspec_hide(data->list_selcur);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/**************************************************************************
 Draw an entry at entry_pos at the given row. To draw the title, set pos to
 ENTRY_TITLE
**************************************************************************/
static VOID List_DrawEntry(struct IClass *cl, Object *obj, int entry_pos,
    int y)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int col, x1, x2;

    /* To be sure we don't draw anything if there is no title */
    if (entry_pos == ENTRY_TITLE && !data->title)
        return;

    DisplayEntry(cl, obj, entry_pos);
    x1 = _mleft(data->area);

    for (col = 0; col < data->columns; col++)
    {
        ZText *text;
        x2 = x1 + data->ci[col].entries_width;

        if ((text =
                zune_text_new(data->preparses[col], data->strings[col],
                    ZTEXT_ARG_NONE, 0)))
        {
            /* Could be made simpler, as we don't really need the bounds */
            zune_text_get_bounds(text, obj);
            /* Note, this was MPEN_SHADOW before */
            SetAPen(_rp(obj), muiRenderInfo(obj)->mri_Pens[MPEN_TEXT]);
            zune_text_draw(text, obj, x1, x2, y);       /* totally wrong! */
            zune_text_destroy(text);
        }
        x1 = x2 + data->ci[col].delta + (data->ci[col].bar ? BAR_WIDTH : 0);
    }
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
IPTR List__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int entry_pos, y;
    APTR clip;
    int start, end;
    BOOL scroll_caused_damage = FALSE;
    struct MUI_ImageSpec_intern *highlight;
    IPTR ret = (IPTR)0;

    D(bug("[Zune:List] %s()\n", __func__);)

    if (data->flags & LIST_QUIET)
        return ret;

    D(
        bug("[Zune:List] %s: Rendering...\n", __func__);
        bug("[Zune:List] %s: update = %d\n", __func__, data->update);
    )

    ret = DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->area_replaced)
        return ret;

    /* Calculate the title height */
    if (data->title)
    {
        data->title_height = data->entries[ENTRY_TITLE]->height + 2;
    }
    else
    {
        data->title_height = 0;
    }

    /* Calc the numbers of entries visible */
    CalcVertVisible(cl, obj);

    if ((msg->flags & MADF_DRAWUPDATE) == 0 || data->update == UPDATEMODE_ALL)
    {
        DoMethod(obj, MUIM_DrawBackground, _mleft(data->area),
            _mtop(data->area), _mwidth(data->area), _mheight(data->area),
            0, data->entries_first * data->entry_maxheight, 0);
    }

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(data->area),
        _mtop(data->area), _mwidth(data->area), _mheight(data->area));

    if ((msg->flags & MADF_DRAWUPDATE) == 0 || data->update == UPDATEMODE_ALL)
    {
        y = _mtop(data->area);
        /* Draw Title
         */
        if (data->title_height && data->title)
        {
            List_DrawEntry(cl, obj, ENTRY_TITLE, y);
            y += data->entries[ENTRY_TITLE]->height;
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
            Move(_rp(obj), _mleft(data->area), y);
            Draw(_rp(obj), _mright(data->area), y);
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
            y++;
            Move(_rp(obj), _mleft(data->area), y);
            Draw(_rp(obj), _mright(data->area), y);
        }
    }

    y = data->entries_top_pixel;

    start = data->entries_first;
    end = data->entries_first + data->entries_visible;

    if ((msg->flags & MADF_DRAWUPDATE) && data->update == (UPDATEMODE_ENTRY|UPDATEMODE_ALL))
    {
        int diffy = data->entries_first - data->update_pos;
        int top, bottom;
        if (abs(diffy) < data->entries_visible)
        {
            scroll_caused_damage =
                (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;

            ScrollRaster(_rp(obj), 0, diffy * data->entry_maxheight,
                _mleft(data->area), y,
                _mright(data->area),
                y + data->entry_maxheight * data->entries_visible);

            scroll_caused_damage =
                scroll_caused_damage
                && (_rp(obj)->Layer->Flags & LAYERREFRESH);

            if (diffy > 0)
            {
                start = end - diffy;
                y += data->entry_maxheight * (data->entries_visible -
                    diffy);
            }
            else
                end = start - diffy;
        }

        top = y;
        bottom = y + (end - start) * data->entry_maxheight;

        DoMethod(obj, MUIM_DrawBackground, _mleft(data->area), top,
            _mwidth(data->area), bottom - top + 1, 0,
            top - _mtop(data->area) + data->entries_first
            * data->entry_maxheight, 0);
    }

    for (entry_pos = start;
        entry_pos < end && entry_pos < data->entries_num; entry_pos++)
    {
        struct ListEntry *entry = data->entries[entry_pos];

        if (!(msg->flags & MADF_DRAWUPDATE) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_ALL) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == (UPDATEMODE_ENTRY|UPDATEMODE_ALL)) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_ENTRY
                && data->update_pos == entry_pos) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_NEEDED
                && (entry->flags & ENTRY_RENDER)))
        {
            /* Choose appropriate highlight image */

            if (entry_pos == data->entries_active
                && (entry->flags & ENTRY_SELECTED) && !data->read_only)
                highlight = data->list_selcur;
            else if (entry_pos == data->entries_active && !data->read_only)
                highlight = data->list_cursor;
            else if (entry->flags & ENTRY_SELECTED)
                highlight = data->list_select;
            else
                highlight = NULL;

            /* Draw highlight or background */

            if (highlight != NULL)
            {
                zune_imspec_draw(highlight, muiRenderInfo(obj),
                    _mleft(data->area), y, _mwidth(data->area),
                    data->entry_maxheight,
                    0, y - data->entries_top_pixel, 0);
            }
            else if (((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_ENTRY
                && data->update_pos == entry_pos) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == UPDATEMODE_NEEDED
                && (entry->flags & ENTRY_RENDER)))
            {
                DoMethod(obj, MUIM_DrawBackground, _mleft(data->area), y,
                    _mwidth(data->area), data->entry_maxheight, 0,
                    y - _mtop(data->area) +
                    data->entries_first * data->entry_maxheight, 0);
            }

            List_DrawEntry(cl, obj, entry_pos, y);
            entry->flags &= ~ENTRY_RENDER;
        }
        y += data->entry_maxheight;
    }

    MUI_RemoveClipping(muiRenderInfo(obj), clip);

    data->update = 0;

    if (scroll_caused_damage)
    {
        if (MUI_BeginRefresh(muiRenderInfo(obj), 0))
        {
            /* Theoretically it might happen that more damage is caused
               after ScrollRaster. By something else, like window movement
               in front of our window. Therefore refresh root object of
               window, not just this object */

            Object *o = NULL;

            get(_win(obj), MUIA_Window_RootObject, &o);
            MUI_Redraw(o, MADF_DRAWOBJECT);

            MUI_EndRefresh(muiRenderInfo(obj), 0);
        }
    }

    ULONG x1 = _mleft(data->area);
    ULONG col;
    y = _mtop(data->area);

    if (data->title_height && data->title)
    {
        for (col = 0; col < data->columns; col++)
        {
            ULONG halfdelta = data->ci[col].delta / 2;
            x1 += data->ci[col].entries_width + halfdelta;

            if (x1 + (data->ci[col].bar ? BAR_WIDTH : 0) > _mright(data->area))
                break;

            if (data->ci[col].bar)
            {
                SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
                Move(_rp(obj), x1, y);
                Draw(_rp(obj), x1,
                    y + data->entries[ENTRY_TITLE]->height - 1);
                SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
                Move(_rp(obj), x1 + 1, y);
                Draw(_rp(obj), x1 + 1,
                    y + data->entries[ENTRY_TITLE]->height - 1);

                x1 += BAR_WIDTH;
            }
            x1 += data->ci[col].delta - halfdelta;
        }
        y += data->entries[ENTRY_TITLE]->height + 1;
    }

    x1 = _mleft(data->area);

    for (col = 0; col < data->columns; col++)
    {
        ULONG halfdelta = data->ci[col].delta / 2;
        x1 += data->ci[col].entries_width + halfdelta;

        if (x1 + (data->ci[col].bar ? BAR_WIDTH : 0) > _mright(data->area))
            break;

        if (data->ci[col].bar)
        {
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
            Move(_rp(obj), x1, y);
            Draw(_rp(obj), x1, _mbottom(data->area));
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
            Move(_rp(obj), x1 + 1, y);
            Draw(_rp(obj), x1 + 1, _mbottom(data->area));

            x1 += BAR_WIDTH;
        }

        x1 += data->ci[col].delta - halfdelta;
    }

    data->flags &= ~LIST_CHANGED;

    return 0;
}

/****** List.mui/MUIM_List_Clear *********************************************
*
*   NAME
*       MUIM_List_Clear (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Clear);
*
*   FUNCTION
*       Removes all entries from the list.
*
******************************************************************************
*
*/

IPTR List__MUIM_Clear(struct IClass *cl, Object *obj,
    struct MUIP_List_Clear *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    while (data->confirm_entries_num)
    {
        struct ListEntry *lentry =
            data->entries[--data->confirm_entries_num];
        DoMethod(obj, MUIM_List_Destruct, (IPTR) lentry->data,
            (IPTR) data->pool);
        FreeListEntry(data, lentry);

        data->flags |= LIST_CHANGED;
    }
    /* Should never fail when shrinking */
    SetListSize(data, 0);

    if (data->confirm_entries_num != data->entries_num)
    {
        SetAttrs(obj, MUIA_List_Entries, 0, MUIA_List_First, 0,
            /* Notify only when no entry was active */
            data->entries_active !=
            MUIV_List_Active_Off ? MUIA_List_Active : TAG_DONE,
            MUIV_List_Active_Off, TAG_DONE);

        data->update = UPDATEMODE_ALL;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return 0;
}

/****** List.mui/MUIM_List_Exchange ******************************************
*
*   NAME
*       MUIM_List_Exchange (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Exchange, LONG pos1, LONG pos2);
*
*   FUNCTION
*       Exchange two entries' positions.
*
*   INPUTS
*       pos1 - the current index of the first entry that should be moved, or
*           one of these special values:
*               MUIV_List_Exchange_Active: the active entry.
*               MUIV_List_Exchange_Top: the first entry.
*               MUIV_List_Exchange_Bottom: the last entry.
*       pos2 - the index of the entry that the first entry should be exchanged
*           with, or one of these special values:
*               MUIV_List_Exchange_Active: the active entry.
*               MUIV_List_Exchange_Top: the first entry.
*               MUIV_List_Exchange_Bottom: the last entry.
*               MUIV_List_Exchange_Next: the next entry after pos1.
*               MUIV_List_Exchange_Previous: the previous entry before pos1.
*
*   NOTES
*       This method will do nothing if either index is greater than the last
*       index in the list, or if MUIV_List_Exchange_Next or
*       MUIV_List_Exchange_Previous imply an index outside the list.
*
*   SEE ALSO
*       MUIM_List_Move
*
******************************************************************************
*
*/

IPTR List__MUIM_Exchange(struct IClass *cl, Object *obj,
    struct MUIP_List_Exchange *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos1, pos2;

    switch (msg->pos1)
    {
    case MUIV_List_Exchange_Top:
        pos1 = 0;
        break;
    case MUIV_List_Exchange_Active:
        pos1 = data->entries_active;
        break;
    case MUIV_List_Exchange_Bottom:
        pos1 = data->entries_num - 1;
        break;
    default:
        pos1 = msg->pos1;
    }

    switch (msg->pos2)
    {
    case MUIV_List_Exchange_Top:
        pos2 = 0;
        break;
    case MUIV_List_Exchange_Active:
        pos2 = data->entries_active;
        break;
    case MUIV_List_Exchange_Bottom:
        pos2 = data->entries_num - 1;
        break;
    case MUIV_List_Exchange_Next:
        pos2 = pos1 + 1;
        break;
    case MUIV_List_Exchange_Previous:
        pos2 = pos1 - 1;
        break;
    default:
        pos2 = msg->pos2;
    }

    if (pos1 >= 0 && pos1 < data->entries_num && pos2 >= 0
        && pos2 < data->entries_num && pos1 != pos2)
    {
        struct ListEntry *save = data->entries[pos1];
        data->entries[pos1] = data->entries[pos2];
        data->entries[pos2] = save;

        data->update = UPDATEMODE_ENTRY;
        data->update_pos = pos1;
        MUI_Redraw(obj, MADF_DRAWUPDATE);

        data->update = UPDATEMODE_ENTRY;
        data->update_pos = pos2;
        MUI_Redraw(obj, MADF_DRAWUPDATE);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**************************************************************************
 MUIM_List_Redraw
**************************************************************************/
IPTR List__MUIM_Redraw(struct IClass *cl, Object *obj,
    struct MUIP_List_Redraw *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    D(bug("[Zune:List] %s()\n", __func__);)

    if (msg->pos == MUIV_List_Redraw_All)
    {
        CalcWidths(cl, obj);
        data->update = UPDATEMODE_ALL;
        if (!(data->flags & LIST_QUIET))
        {
            MUI_Redraw(obj, MADF_DRAWUPDATE);
        }
        else
            data->flags |= LIST_CHANGED;
    }
    else
    {
        LONG pos = -1;
        if (msg->pos == MUIV_List_Redraw_Active)
            pos = data->entries_active;
        else if (msg->pos == MUIV_List_Redraw_Entry)
        {
            LONG i;
            for (i = 0; i < data->entries_num; i++)
                if (data->entries[i]->data == msg->entry)
                {
                    pos = i;
                    break;
                }
        }
        else
            pos = msg->pos;

        if (pos != -1)
        {
            data->entries[pos]->flags |= ENTRY_RENDER;
            if (!(data->flags & LIST_QUIET))
            {
                if (CalcDimsOfEntry(cl, obj, pos))
                    data->update = UPDATEMODE_ALL;
                else
                {
                    data->update = UPDATEMODE_ENTRY;
                    data->update_pos = pos;
                }

                MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
            else
            {
                if (CalcDimsOfEntry(cl, obj, pos))
                    data->update = UPDATEMODE_ALL;
                else if (!(data->update & UPDATEMODE_ALL))
                    data->update = UPDATEMODE_NEEDED;
                data->flags |= LIST_CHANGED;
            }
        }
    }

    return 0;
}

/****** List.mui/MUIM_List_Remove ********************************************
*
*   NAME
*       MUIM_List_Remove (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Remove, LONG pos);
*
*   FUNCTION
*       Removes entries from the list. If a destruct hook has been
*       installed, it will be called for the removed entry.
*
*   INPUTS
*       pos - the index of the entry to be removed. The following
*           special values can also be used:
*           MUIV_List_Remove_First: remove the first entry.
*           MUIV_List_Remove_Last: remove the last entry.
*           MUIV_List_Remove_Active: remove the active entry.
*           MUIV_List_Remove_Selected: remove all selected entries
*               (or the active entry if there are no selected entries).
*
*   NOTES
*       When the active entry is removed, the next entry becomes active
*       (if there is no entry below the active entry, the previous entry
*       becomes active instead).
*
*   SEE ALSO
*       MUIM_List_Insertsingle, MUIM_List_Insert, MUIA_List_DestructHook.
*
******************************************************************************
*
* It was not possible to use MUIM_List_NextSelected here because that method
* may skip entries if entries are removed during an iteration.
*
*/

IPTR List__MUIM_Remove(struct IClass *cl, Object *obj,
    struct MUIP_List_Remove *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos;
    LONG new_act;
    UWORD i;
    BOOL found, done = FALSE;
    struct ListEntry *lentry;
    Tag active_tag = TAG_DONE;

    if (!data->entries_num)
        return 0;

    switch (msg->pos)
    {
    case MUIV_List_Remove_First:
        pos = 0;
        break;

    case MUIV_List_Remove_Active:
        pos = data->entries_active;
        break;

    case MUIV_List_Remove_Last:
        pos = data->entries_num - 1;
        break;

    case MUIV_List_Remove_Selected:
        pos = 0;
        break;

    default:
        pos = msg->pos;
        break;
    }

    if (pos < 0 || pos >= data->entries_num)
        return 0;

    new_act = data->entries_active;

    while (!done)
    {
        if (msg->pos == MUIV_List_Remove_Selected)
        {
            /* Find the next selected entry */
            for (found = FALSE, i = pos;
                i < data->confirm_entries_num && !found; i++)
            {
                if (data->entries[i]->flags & ENTRY_SELECTED)
                {
                    pos = i;
                    found = TRUE;
                }
            }

            if (!found)
            {
                done = TRUE;

                /* If there were no selected entries, remove the active one */
                if (data->confirm_entries_num == data->entries_num
                    && data->entries_active != MUIV_List_Active_Off)
                {
                    pos = data->entries_active;
                    found = TRUE;
                }
            }
        }
        else
        {
            done = TRUE;
            found = TRUE;
        }

        if (found)
        {
            lentry = data->entries[pos];
            DoMethod(obj, MUIM_List_Destruct, (IPTR) lentry->data,
                (IPTR) data->pool);
            RemoveListEntries(data, pos, 1);
            data->confirm_entries_num--;

            if (pos < new_act)
            {
                new_act--;
                active_tag = MUIA_List_Active;
            }
            else if (pos == new_act)
                active_tag = MUIA_List_Active;
        }
    }

    /* Update entries count prior to range check */
    SetAttrs(obj, MUIA_List_Entries, data->confirm_entries_num, TAG_DONE);

    /* Ensure that the active element is in a valid range (it might become
     * MUIV_List_Active_Off (-1), but that's OK) */
    if (new_act >= data->entries_num)
        new_act = data->entries_num - 1;

    SetAttrs(obj,
        active_tag, new_act,   /* Inform only if necessary (for notify) */
        TAG_DONE);

    data->flags |= LIST_CHANGED;
    data->update = UPDATEMODE_ALL;
    if (!(data->flags & LIST_QUIET))
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return 0;
}

/****** List.mui/MUIM_List_Select ********************************************
*
*   NAME
*       MUIM_List_Select (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Select, LONG pos, LONG seltype, LONG *state);
*
*   FUNCTION
*       Selects or deselects entries in the list and/or enquires about their
*       current selection state. If a multiselection test hook has been
*       installed via MUIA_List_MultiTestHook, it will be called to validate
*       the requested selection.
*
*       This method may also be used to count the number of selected entries
*       (see below).
*
*   INPUTS
*       pos - the index of the entry to be selected. The following
*           special values can also be used:
*           MUIV_List_Select_Active: Select the active entry.
*           MUIV_List_Select_All: Select all entries.
*       seltype - the new selection state; one of the following:
*           MUIV_List_Select_Off: Select the entry.
*           MUIV_List_Select_On: Deselect the entry.
*           MUIV_List_Select_Toggle: Switch to the alternate selection state.
*           MUIV_List_Select_Ask: Do not change the selection state, just
*               retrieve the current state. If 'pos' is MUIV_List_Select_All,
*               the number of selected entries will be retrieved instead (V9).
*       state - a pointer in which to fill in the previous selection state
*           (either MUIV_List_Select_On or MUIV_List_Select_Off) or the
*           number of selected entries. May be NULL.
*
*   NOTES
*       If pos is MUIV_List_Select_All and seltype is not
*       MUIV_List_Select_Ask, the value filled in 'state' is undefined.
*
*   SEE ALSO
*       MUIA_List_Active, MUIA_List_MultiTestHook.
*
******************************************************************************
*
*/

IPTR List__MUIM_Select(struct IClass *cl, Object *obj,
    struct MUIP_List_Select *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, i, count, selcount = 0, state = 0;
    BOOL multi_allowed = TRUE, new_multi_allowed, new_select_state = FALSE;

    /* Establish the range of entries affected */
    switch (msg->pos)
    {
    case MUIV_List_Select_Active:
        pos = data->entries_active;
        if (pos == MUIV_List_Active_Off)
            count = 0;
        else
            count = 1;
        break;

    case MUIV_List_Select_All:
        pos = 0;
        count = data->entries_num;
        break;

    default:
        pos = msg->pos;
        count = 1;
        if (pos < 0 || pos >= data->entries_num)
            return 0;
        break;
    }

    if (msg->seltype != MUIV_List_Select_Ask && data->multi_test_hook != NULL)
    {
        /* Count selected entries and disallow selection of additional
           entries if there is a currently selected entry that is not
           multi-selectable */
        for (i = 0; i < data->entries_num && multi_allowed; i++)
        {
            if (data->entries[i]->flags & ENTRY_SELECTED)
            {
                selcount++;
                if (data->multi_test_hook != NULL && selcount == 1)
                    multi_allowed = CallHookPkt(data->multi_test_hook, NULL,
                        data->entries[i]->data);
            }
        }
    }

    /* Change or check state of each entry in the range */
    for (i = pos; i < pos + count; i++)
    {
        state = data->entries[i]->flags & ENTRY_SELECTED;
        switch (msg->seltype)
        {
        case MUIV_List_Select_Off:
            new_select_state = FALSE;
            break;

        case MUIV_List_Select_On:
            new_select_state = TRUE;
            break;

        case MUIV_List_Select_Toggle:
            new_select_state = !state;
            break;

        default:
            if (data->entries[i]->flags & ENTRY_SELECTED)
                selcount++;
            break;
        }

        if (msg->seltype != MUIV_List_Select_Ask)
        {
            if (new_select_state && !state)
            {
                /* Check if there is potential to select an additional entry */
                if (multi_allowed || selcount == 0)
                {
                    /* Check if the entry to be selected is multi-selectable */
                    if (data->multi_test_hook != NULL)
                        new_multi_allowed = CallHookPkt(data->multi_test_hook,
                            NULL, data->entries[i]->data);
                    else
                        new_multi_allowed = TRUE;

                    /* Check if the entry to be selected can be selected at
                       the same time as the already selected entries */
                    if (new_multi_allowed || selcount == 0)
                    {
                        /* Select the entry and update the selection count
                           and flag */
                        data->entries[i]->flags |= ENTRY_SELECTED;
                        selcount++;

                        multi_allowed = new_multi_allowed;
                    }
                }
            }
            else if (!new_select_state && state)
            {
                data->entries[i]->flags &= ~ENTRY_SELECTED;
                    selcount--;
            }
        }
    }

    /* Report old state or number of selected entries */
    if (msg->info)
    {
        if (msg->pos == MUIV_List_Select_All
            && msg->seltype == MUIV_List_Select_Ask)
            *msg->info = selcount;
        else
            *msg->info = state;
    }

    /* Redraw unless it was just an enquiry */
    if (msg->seltype != MUIV_List_Select_Ask)
    {
        if (count > 1)
            data->update = UPDATEMODE_ALL;
        else
        {
            data->update = UPDATEMODE_ENTRY;
            data->update_pos = pos;
        }
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
   
    return 0;
}

/****** List.mui/MUIM_List_Insert ********************************************
*
*   NAME
*       MUIM_List_Insert (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Insert, APTR *entries, LONG count, LONG pos);
*
*   FUNCTION
*       Adds multiple entries to the list. If a construct hook has been
*       installed, the results of passing the entries to this hook will be
*       inserted.
*
*   INPUTS
*       entries - an array of entries to be inserted.
*       count - the number of entries to insert. A special value of -1 may be
*           used, indicating that the array of entries is NULL-terminated.
*       pos - the index at which to insert the new entries. The following
*           special values can also be used:
*           MUIV_List_Insert_Top: insert at index 0.
*           MUIV_List_Insert_Bottom: insert after all existing entries.
*           MUIV_List_Insert_Active: insert at the index of the active entry
*               (or at index 0 if there is no active entry).
*           MUIV_List_Insert_Sorted: keep the list sorted.
*
*   SEE ALSO
*       MUIM_List_InsertSingle, MUIM_List_Remove, MUIA_List_ConstructHook.
*
******************************************************************************
*
*/

IPTR List__MUIM_Insert(struct IClass *cl, Object *obj,
    struct MUIP_List_Insert *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, count, sort, active;
    BOOL adjusted = FALSE;

    count = msg->count;
    sort = 0;

    if (count == -1)
    {
        /* Count the number of entries */
        for (count = 0; msg->entries[count] != NULL; count++)
            ;
    }

    if (count <= 0)
        return ~0;

    switch (msg->pos)
    {
    case MUIV_List_Insert_Top:
        pos = 0;
        break;

    case MUIV_List_Insert_Active:
        if (data->entries_active != -1)
            pos = data->entries_active;
        else
            pos = 0;
        break;

    case MUIV_List_Insert_Sorted:
        pos = data->entries_num;
        sort = 1;               /* we sort'em later */
        break;

    case MUIV_List_Insert_Bottom:
        pos = data->entries_num;
        break;

    default:
        if (msg->pos > data->entries_num)
            pos = data->entries_num;
        else if (msg->pos < 0)
            pos = 0;
        else
            pos = msg->pos;
        break;
    }
    data->insert_position = pos;

    if (!(SetListSize(data, data->entries_num + count)))
        return ~0;

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
             * the space where the following list entries should have gone
             */
            RemoveListEntries(data, pos, until - pos);
            return ~0;
        }

        /* now call the construct method which returns us a pointer which
           we need to store */
        lentry->data = (APTR) DoMethod(obj, MUIM_List_Construct,
            (IPTR) * toinsert, (IPTR) data->pool);
        if (!lentry->data)
        {
            FreeListEntry(data, lentry);
            RemoveListEntries(data, pos, until - pos);

            /* TODO: Also check for visible stuff like below */
            if (data->entries_num != data->confirm_entries_num)
                set(obj, MUIA_List_Entries, data->confirm_entries_num);
            return ~0;
        }

        lentry->flags |= ENTRY_RENDER;
        data->entries[pos] = lentry;
        data->confirm_entries_num++;

        data->flags |= LIST_CHANGED;

        if (_flags(obj) & MADF_SETUP)
        {
            /* We have to calculate the width and height of the newly
             * inserted entry. This has to be done after inserting the
             * element into the list */
            if (CalcDimsOfEntry(cl, obj, pos))
                adjusted = TRUE;
        }

        toinsert++;
        pos++;
    }
    pos--;

    /* Recalculate the number of visible entries */
    if (_flags(obj) & MADF_SETUP)
        CalcVertVisible(cl, obj);

    if (data->entries_num != data->confirm_entries_num)
    {
        SetAttrs(obj,
            MUIA_List_Entries, data->confirm_entries_num,
            MUIA_List_Visible, data->entries_visible, TAG_DONE);
    }

    /* If the array is already sorted, we could do a simple insert
     * sort and would be much faster than with qsort.
     * If an array is not yet sorted, does a MUIV_List_Insert_Sorted
     * sort the whole array?
     *
     * I think, we better sort the whole array:
     */
    if (sort)
    {
        /* TODO: which pos to return here !?        */
        DoMethod(obj, MUIM_List_Sort);

        if ((adjusted) && (data->flags & LIST_QUIET))
            data->update = UPDATEMODE_ALL;
    }
    else
    {
        data->update = UPDATEMODE_ALL;
        if (!(data->flags & LIST_QUIET))
            MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    superset(cl, obj, MUIA_List_InsertPosition, data->insert_position);

    /* Update index of active entry */
    if (data->entries_active >= data->insert_position)
    {
        active = data->entries_active + count;
        SET(obj, MUIA_List_Active, active);
    }

    return (ULONG) pos;
}

/****** List.mui/MUIM_List_InsertSingle **************************************
*
*   NAME
*       MUIM_List_InsertSingle (V7)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_InsertSingle, APTR entry, LONG pos);
*
*   FUNCTION
*       Adds a single entry to the list. If a construct hook has been
*       installed, the result of passing the entry to this hook will be
*       inserted.
*
*   INPUTS
*       entry - the entry to be inserted.
*       pos - the index at which to insert the new entry. The following
*           special values can also be used:
*           MUIV_List_Insert_Top: insert at index 0.
*           MUIV_List_Insert_Bottom: insert after all existing entries.
*           MUIV_List_Insert_Active: insert at the index of the active entry
*               (or at index 0 if there is no active entry).
*           MUIV_List_Insert_Sorted: keep the list sorted.
*
*   SEE ALSO
*       MUIM_List_Insert, MUIM_List_Remove, MUIA_List_ConstructHook.
*
******************************************************************************
*
*/

IPTR List__MUIM_InsertSingle(struct IClass *cl, Object *obj,
    struct MUIP_List_InsertSingle *msg)
{
    return DoMethod(obj, MUIM_List_Insert, (IPTR) & msg->entry, 1,
        msg->pos);
}

/****** List.mui/MUIM_List_GetEntry ******************************************
*
*   NAME
*       MUIM_List_GetEntry (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_GetEntry, LONG pos, APTR *entry);
*
*   FUNCTION
*       Retrieves an entry from the list. If the requested entry position is
*       invalid, the entry will be NULL.
*
*   INPUTS
*       pos - the index of the entry to get, or the special value
*           MUIV_List_GetEntry_Active to get the active entry.
*       entry - a pointer to a variable in which to store a pointer to the
*           entry data.
*
*   SEE ALSO
*       MUIM_List_Insert, MUIM_List_InsertSingle, MUIM_List_Remove.
*
******************************************************************************
*
*/

IPTR List__MUIM_GetEntry(struct IClass *cl, Object *obj,
    struct MUIP_List_GetEntry *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    int pos = msg->pos;

    if (pos == MUIV_List_GetEntry_Active)
        pos = data->entries_active;

    if (pos < 0 || pos >= data->entries_num)
    {
        *msg->entry = NULL;
        return 0;
    }
    *msg->entry = data->entries[pos]->data;
    return (IPTR) *msg->entry;
}

/**************************************************************************
 MUIM_List_Construct
**************************************************************************/
IPTR List__MUIM_Construct(struct IClass *cl, Object *obj,
    struct MUIP_List_Construct *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (NULL == data->construct_hook)
        return (IPTR) msg->entry;
    if ((IPTR) data->construct_hook == MUIV_List_ConstructHook_String)
    {
        int len = msg->entry ? strlen((STRPTR) msg->entry) : 0;
        ULONG *mem = AllocPooled(msg->pool, len + 5);

        if (NULL == mem)
            return 0;
        mem[0] = len + 5;
        if (msg->entry != NULL)
            strcpy((STRPTR) (mem + 1), (STRPTR) msg->entry);
        else
            *(STRPTR) (mem + 1) = 0;
        return (IPTR) (mem + 1);
    }
    return CallHookPkt(data->construct_hook, msg->pool, msg->entry);
}

/**************************************************************************
 MUIM_List_Destruct
**************************************************************************/
IPTR List__MUIM_Destruct(struct IClass *cl, Object *obj,
    struct MUIP_List_Destruct *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (NULL == data->destruct_hook)
        return 0;

    if ((IPTR) data->destruct_hook == MUIV_List_DestructHook_String)
    {
        ULONG *mem = ((ULONG *) msg->entry) - 1;
        FreePooled(msg->pool, mem, mem[0]);
    }
    else
    {
        CallHookPkt(data->destruct_hook, msg->pool, msg->entry);
    }
    return 0;
}

/****** List.mui/MUIM_List_Compare *******************************************
*
*   NAME
*       MUIM_List_Compare (V20)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Compare, APTR entry1, APTR entry2,
*           LONG sort_type1, LONG sort_type2);
*
*   FUNCTION
*       Compare two list entries according to the current comparison hook
*       (MUIA_List_CompareHook).
*
*   INPUTS
*       entry1 - the first entry data.
*       entry2 - the second entry data.
*       sort_type1 - undocumented.
*       sort_type2 - undocumented.
*
*   SEE ALSO
*       MUIA_List_CompareHook, MUIM_List_Sort.
*
******************************************************************************
*
*/

IPTR List__MUIM_Compare(struct IClass *cl, Object *obj,
    struct MUIP_List_Compare *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    return CallHookPkt(data->compare_hook, msg->entry2, msg->entry1);
}

/**************************************************************************
 MUIM_List_Display
**************************************************************************/
IPTR List__MUIM_Display(struct IClass *cl, Object *obj,
    struct MUIP_List_Display *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (NULL == data->display_hook)
    {
        if (msg->entry)
            *msg->array = msg->entry;
        else
            *msg->array = 0;
        return 1;
    }

    *((ULONG *) (msg->array - 1)) = msg->entry_pos;
    return CallHookPkt(data->display_hook, msg->array, msg->entry);
}

/**************************************************************************
 MUIM_List_SelectChange
**************************************************************************/
IPTR List__MUIM_SelectChange(struct IClass *cl, Object *obj,
    struct MUIP_List_SelectChange *msg)
{
    return 1;
}

/****** List.mui/MUIM_List_CreateImage ***************************************
*
*   NAME
*       MUIM_List_CreateImage (V11)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_CreateImage, Object *area, ULONG flags);
*
*   FUNCTION
*       Creates an image to be inserted within list entries. An instance of
*       any Area subclass is passed in and a blackbox value is returned that 
*       can be displayed by embedding its hexadecimal representation within 
*       any of the list's display strings (provided either statically or by
*       the list's display hook. The format string to be used is
*       "\330[%08lx]".
*
*       It is recommended that lists that embed images are instances of a
*       custom subclass so that this method can be called in the list's
*       MUIM_Setup method, and MUIM_List_DeleteImage can be called in the
*       list's MUIM_Cleanup method. However, this is not necessary as long as
*       MUIM_List_CreateImage is called after the list has had its MUIM_Setup
*       called, and MUIM_List_DeleteImage is called after the list has had its 
*       MUIM_Cleanup method called.
*
*   INPUTS
*       area - the area object that is used to generate the image.
*       flags - must be zero.
*
*   RESULT
*       A blackbox reference to the list image, or NULL on an error.
*
*   NOTES
*       If this method returns NULL, no special action needs to be taken, as
*       embedding this value in a list string will simply cause no image to
*       appear.
*
*       The object passed in becomes a child of the list, so it cannot be used
*       elsewhere simultaneously.
*
*   SEE ALSO
*       MUIM_List_DeleteImage, MUIA_List_DisplayHook.
*
******************************************************************************
*
* Connects an Area subclass object to the list, much like an object gets
* connected to a window. List calls Setup and AskMinMax on that object,
* keeps a reference to it (that reference will be returned).
* Text engine will dereference that pointer and draw the object with its
* default size.
*
*/

IPTR List__MUIM_CreateImage(struct IClass *cl, Object *obj,
    struct MUIP_List_CreateImage *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListImage *li;

    if (!msg->obj)
        return 0;

    /* List must be already setup in Setup of your subclass */
    if (!(_flags(obj) & MADF_SETUP))
        return 0;
    li = AllocPooled(data->pool, sizeof(struct ListImage));
    if (!li)
        return 0;
    li->obj = msg->obj;

    AddTail((struct List *)&data->images, (struct Node *)li);
    DoMethod(li->obj, MUIM_ConnectParent, (IPTR) obj);
    DoSetupMethod(li->obj, muiRenderInfo(obj));


    return (IPTR) li;
}

/****** List.mui/MUIM_List_DeleteImage ***************************************
*
*   NAME
*       MUIM_List_DeleteImage (V11)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_DeleteImage, APTR image);
*
*   FUNCTION
*       Deletes an image created by MUIM_List_CreateImage.
*
*   INPUTS
*       image - a blackbox reference to the list image, or NULL.
*
*   SEE ALSO
*       MUIM_List_CreateImage, MUIA_List_DisplayHook.
*
******************************************************************************
*
*/

IPTR List__MUIM_DeleteImage(struct IClass *cl, Object *obj,
    struct MUIP_List_DeleteImage *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListImage *li = (struct ListImage *)msg->listimg;

    if (li)
    {
        DoMethod(li->obj, MUIM_Cleanup);
        DoMethod(li->obj, MUIM_DisconnectParent);
        Remove((struct Node *)li);
        FreePooled(data->pool, li, sizeof(struct ListImage));
    }

    return 0;
}

/****** List.mui/MUIM_List_Jump **********************************************
*
*   NAME
*       MUIM_List_Jump (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Jump, LONG pos);
*
*   FUNCTION
*       Scrolls the list so that a particular entry is visible.
*
*   INPUTS
*       pos - index of entry that should become visible, or one of these
*           special values:
*               MUIV_List_Jump_Active: show the active entry.
*               MUIV_List_Jump_Top: show the first entry.
*               MUIV_List_Jump_Bottom: show the last entry.
*               MUIV_List_Jump_Up: show the previous hidden entry.
*               MUIV_List_Jump_Down: show the next hidden entry.
*
******************************************************************************
*
*/

IPTR List__MUIM_Jump(struct IClass *cl, Object *obj,
    struct MUIP_List_Jump *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos = msg->pos;

    switch (pos)
    {
    case MUIV_List_Jump_Top:
        pos = 0;
        break;

    case MUIV_List_Jump_Active:
        pos = data->entries_active;
        break;

    case MUIV_List_Jump_Bottom:
        pos = data->entries_num - 1;
        break;

    case MUIV_List_Jump_Down:
        pos = data->entries_first + data->entries_visible;
        break;

    case MUIV_List_Jump_Up:
        pos = data->entries_first - 1;
        break;
    }

    if (pos >= data->entries_num)
    {
        pos = data->entries_num - 1;
    }
    if (pos < 0)
        pos = 0;

    if (pos < data->entries_first)
    {
        set(obj, MUIA_List_First, pos);
    }
    else if (pos >= data->entries_first + data->entries_visible)
    {
        pos -= (data->entries_visible - 1);
        if (pos < 0)
            pos = 0;
        if (pos != data->entries_first)
        {
            set(obj, MUIA_List_First, pos);
        }
    }

    return TRUE;
}

/****** List.mui/MUIM_List_Sort **********************************************
*
*   NAME
*       MUIM_List_Sort (V4)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Sort);
*
*   FUNCTION
*       Sort the list's entries according to the current comparison hook
*       (MUIA_List_CompareHook).
*
*   NOTES
*       The active index does not change, so the active entry may do so.
*
*   SEE ALSO
*       MUIA_List_CompareHook, MUIM_List_Compare.
*
******************************************************************************
*
*/

IPTR List__MUIM_Sort(struct IClass *cl, Object *obj,
    struct MUIP_List_Sort *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    int i, j, max;
    struct MUIP_List_Compare cmpmsg =
        { MUIM_List_Compare, NULL, NULL, 0, 0 };
    BOOL changed = FALSE;

    D(bug("[Zune:List] %s()\n", __func__);)

    if (data->entries_num > 1)
    {
        /*
           Simple sort algorithm. Feel free to improve it.
         */
        for (i = 0; i < data->entries_num - 1; i++)
        {
            max = i;
            for (j = i + 1; j < data->entries_num; j++)
            {
                cmpmsg.entry1 = data->entries[max]->data;
                cmpmsg.entry2 = data->entries[j]->data;
                if ((LONG) DoMethodA(obj, (Msg) & cmpmsg) > 0)
                {
                    max = j;
                }
            }
            if (i != max)
            {
                APTR tmp = data->entries[i];
                data->entries[i] = data->entries[max];
                data->entries[i]->flags |= ENTRY_RENDER;
                data->entries[max] = tmp;
                data->entries[max]->flags |= ENTRY_RENDER;
                if (data->entries_active == i)
                    data->entries_active = max;
                else if (data->entries_active == max)
                    data->entries_active = i;
                changed = TRUE;
            }
        }
    }

    if (changed)
    {
        data->flags |= LIST_CHANGED;
        if (!(data->update & UPDATEMODE_ALL))
            data->update = UPDATEMODE_NEEDED;
        if (!(data->flags & LIST_QUIET))
            MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return 0;
}

/****** List.mui/MUIM_List_Move **********************************************
*
*   NAME
*       MUIM_List_Move (V9)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_Move, LONG from, LONG to);
*
*   FUNCTION
*       Move a list entry to a new position.
*
*   INPUTS
*       from - the current index of the entry that should be moved, or one of
*           these special values:
*               MUIV_List_Move_Active: the active entry.
*               MUIV_List_Move_Top: the first entry.
*               MUIV_List_Move_Bottom: the last entry.
*       to - the index of the entry's new position, or one of
*           these special values:
*               MUIV_List_Move_Active: the active entry.
*               MUIV_List_Move_Top: the first entry.
*               MUIV_List_Move_Bottom: the last entry.
*
*   NOTES
*       The active index does not change, so the active entry may do so.
*
*   SEE ALSO
*       MUIM_List_Exchange
*
******************************************************************************
*
*/

IPTR List__MUIM_Move(struct IClass *cl, Object *obj,
    struct MUIP_List_Move *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    LONG from, to;
    int i;

    /* Normalise special 'from' values */
    switch (msg->from)
    {
    case MUIV_List_Move_Top:
        from = 0;
        break;
    case MUIV_List_Move_Active:
        from = data->entries_active;
        break;
    case MUIV_List_Move_Bottom:
        from = data->entries_num - 1;
        break;
    default:
        from = msg->from;
    }

    /* Normalise special 'to' values */
    switch (msg->to)
    {
    case MUIV_List_Move_Top:
        to = 0;
        break;
    case MUIV_List_Move_Active:
        to = data->entries_active;
        break;
    case MUIV_List_Move_Bottom:
        to = data->entries_num - 1;
        break;
    case MUIV_List_Move_Next:
        to = from + 1;
        break;
    case MUIV_List_Move_Previous:
        to = from - 1;
        break;
    default:
        to = msg->to;
    }

    /* Check that values are within valid bounds */
    if (from > data->entries_num - 1 || from < 0
        || to > data->entries_num - 1 || to < 0 || from == to)
        return (IPTR) FALSE;

    /* Shift all entries in the range between the 'from' and 'to' positions */
    if (from < to)
    {
        struct ListEntry *backup = data->entries[from];
        for (i = from; i < to; i++)
            data->entries[i] = data->entries[i + 1];
        data->entries[to] = backup;
    }
    else
    {
        struct ListEntry *backup = data->entries[from];
        for (i = from; i > to; i--)
            data->entries[i] = data->entries[i - 1];
        data->entries[to] = backup;
    }

#if 0    /* Not done in MUI 3 */
    /* Update index of active entry */
    if (from == data->entries_active)
        data->entries_active = to;
    else if (data->entries_active > from && data->entries_active < to)
        data->entries_active--;
    else if (data->entries_active < from && data->entries_active >= to)
        data->entries_active++;
#endif

    /* Reflect list changes visually */
    data->flags |= LIST_CHANGED;
    data->update = UPDATEMODE_ALL;
    if (!(data->flags & LIST_QUIET))
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return TRUE;
}

/****** List.mui/MUIM_List_NextSelected **************************************
*
*   NAME
*       MUIM_List_NextSelected (V6)
*
*   SYNOPSIS
*       DoMethod(obj, MUIM_List_NextSelected, LONG *pos);
*
*   FUNCTION
*       Allows iteration through a list's selected entries by providing the
*       index of the next selected entry after the specified index.
*
*   INPUTS
*       pos - the address of a variable containing the index of the previous
*           selected entry. The variable must be initialised to the special
*           value MUIV_List_NextSelected_Start to find the first selected
*           entry. When this method returns, the variable will contain the
*           index of the next selected entry, or MUIV_List_NextSelected_End if
*           there are no more.
*
*   NOTES
*       If there are no selected entries but there is an active entry, the
*       index of the active entry will be stored (when
*       MUIV_List_NextSelected_Start is specified).
*
*       Some selected entries may be skipped if any entries are removed
*       between calls to this method during an iteration of a list.
*
*       MUIV_List_NextSelected_Start and MUIV_List_NextSelected_End may have
*       the same numeric value.
*
*   SEE ALSO
*       MUIM_List_Select, MUIM_List_Remove.
*
******************************************************************************
*
*/

IPTR List__MUIM_NextSelected(struct IClass *cl, Object *obj,
    struct MUIP_List_NextSelected *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, i;
    BOOL found = FALSE;

    /* Get the first entry to check */
    pos = *msg->pos;
    if (pos == MUIV_List_NextSelected_Start)
        pos = 0;
    else
        pos++;

    /* Find the next selected entry */
    for (i = pos; i < data->entries_num && !found; i++)
    {
        if (data->entries[i]->flags & ENTRY_SELECTED)
        {
            pos = i;
            found = TRUE;
        }
    }

    /* Return index of selected or active entry, or indicate there are no
       more */
    if (!found)
    {
        if (*msg->pos == MUIV_List_NextSelected_Start
            && data->entries_active != MUIV_List_Active_Off)
            pos = data->entries_active;
        else
            pos = MUIV_List_NextSelected_End;
    }
    *msg->pos = pos;

    return TRUE;
}

/**************************************************************************
 MUIM_List_TestPos
**************************************************************************/
IPTR List__MUIM_TestPos(struct IClass *cl, Object *obj,
    struct MUIP_List_TestPos *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result *result = msg->res;
    LONG col = -1, row = -1;
    UWORD flags = 0, i;
    LONG mx;
    LONG ey;
    LONG entries_visible;

    if (data->entries_visible <= data->entries_num)
        entries_visible = data->entries_visible;
    else
        entries_visible = data->entries_num;

    if ((msg->x == MUI_MAXMAX) && (msg->y == MUI_MAXMAX))
    {
        mx = data->mouse_x;
        ey = data->mouse_y;
    }
    else
    {
        mx = msg->x;
        ey = msg->y;
    }

    if ((mx != MUI_MAXMAX) && (ey != MUI_MAXMAX))
    {
        mx -= _left(data->area);
        /* y coordinates transformed to the entries */
        ey -= data->entries_top_pixel;

        /* Now check if it was clicked on a title or on entries */
        if (ey < 0)
            flags |= MUI_LPR_ABOVE;
        else if (ey >= entries_visible * data->entry_maxheight)
            flags |= MUI_LPR_BELOW;
        else
        {
            /* Identify row */
            row = ey / data->entry_maxheight + data->entries_first;
            result->yoffset =
                ey % data->entry_maxheight - data->entry_maxheight / 2;
        }

        if (mx < 0)
            flags |= MUI_LPR_LEFT;
        else if (mx >= _width(data->area))
            flags |= MUI_LPR_RIGHT;
        else
        {
            /* Identify column */
            if (data->entries_num > 0 && data->columns > 0)
            {
                LONG width_sum = 0;
                col = data->columns - 1;
                for (i = 0; i < data->columns; i++)
                {
                    result->xoffset = mx - width_sum;
                    width_sum +=
                        data->ci[i].entries_width +
                        data->ci[i].delta +
                        (data->ci[i].bar ? BAR_WIDTH : 0);
                    D(bug("[List/MUIM_TestPos] i %d "
                        "width %d width_sum %d mx %d\n",
                        i, data->ci[i].entries_width, width_sum, mx));
                    if (mx < width_sum)
                    {
                        col = i;
                        D(bug("[List/MUIM_TestPos] Column hit %d\n", col));
                        break;
                    }
                }
            }
        }
    }

    result->entry = row;
    result->column = col;
    result->flags = flags;

    return TRUE;
}

/****i* List.mui/MUIM_DragQuery **********************************************
*
*   NAME
*       MUIM_DragQuery
*
******************************************************************************
*
*/

IPTR List__MUIM_DragQuery(struct IClass *cl, Object *obj,
    struct MUIP_DragQuery *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    if (msg->obj == obj && (data->flags & LIST_DRAGSORTABLE))
        return MUIV_DragQuery_Accept;
    else
        return MUIV_DragQuery_Refuse;
}


/****i* List.mui/MUIM_DragFinish *********************************************
*
*   NAME
*       MUIM_DragFinish
*
******************************************************************************
*
*/

IPTR List__MUIM_DragFinish(struct IClass *cl, Object *obj,
    struct MUIP_DragFinish *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);

    data->drop_mark_y = -1;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/****i* List.mui/MUIM_DragReport *********************************************
*
*   NAME
*       MUIM_DragReport
*
******************************************************************************
*
*/

IPTR List__MUIM_DragReport(struct IClass *cl, Object *obj,
    struct MUIP_DragReport *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result pos;
    struct RastPort *rp = _rp(obj);
    LONG n, y;
    UWORD old_pattern;

    /* Choose new drop mark position */

    DoMethod(obj, MUIM_List_TestPos, msg->x, msg->y, (IPTR) &pos);
    if (pos.entry != -1)
    {
        n = pos.entry;
        if (pos.yoffset > 0)
            n++;
    }
    else if ((pos.flags & MUI_LPR_ABOVE) != 0)
        n = data->entries_first;
    else
    {
        n = MIN(data->entries_visible, data->entries_num)
            - data->entries_first;
    }

    /* Clear old drop mark */

    if ((data->flags & LIST_SHOWDROPMARKS) != 0)
    {
        y = data->entries_top_pixel + (n - data->entries_first)
            * data->entry_maxheight;
        if (y != data->drop_mark_y)
        {
            DoMethod(obj, MUIM_DrawBackground, _mleft(data->area),
                data->drop_mark_y, _mwidth(data->area), 1, 0, 0, 0);

            /* Draw new drop mark and store its position */

            SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], _pens(obj)[MPEN_SHADOW],
                JAM2);
            old_pattern = rp->LinePtrn;
            SetDrPt(rp, 0xF0F0);
            Move(rp, _mleft(data->area), y);
            Draw(rp, _mright(data->area), y);
            SetDrPt(rp, old_pattern);
            data->drop_mark_y = y;
        }
    }

    return TRUE;
}


/****i* List.mui/MUIM_DragDrop ***********************************************
*
*   NAME
*       MUIM_DragDrop
*
******************************************************************************
*
*/

IPTR List__MUIM_DragDrop(struct IClass *cl, Object *obj,
    struct MUIP_DragDrop *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result pos;
    LONG n;

    /* Find drop position */

    DoMethod(obj, MUIM_List_TestPos, msg->x, msg->y, (IPTR) &pos);
    if (pos.entry != -1)
    {
        /* Change drop position when coords move past centre of entry, not
         * entry boundary */

        n = pos.entry;
        if (pos.yoffset > 0)
            n++;
    }
    else if ((pos.flags & MUI_LPR_ABOVE) != 0)
        n = 0;
    else
        n = data->entries_num;

    data->drop_mark = n;

    if (data->flags & LIST_DRAGSORTABLE)
    {
        /* Ensure that dropped entry will be positioned between the two
         * entries that are above and below the drop mark, rather than
         * strictly at the numeric index shown */

        if (n > data->entries_active)
            n--;

        DoMethod(obj, MUIM_List_Move, MUIV_List_Move_Active, n);
        SET(obj, MUIA_List_Active, n);
    }

    return TRUE;
}


/****i* List.mui/MUIM_CreateDragImage ****************************************
*
*   NAME
*       MUIM_CreateDragImage
*
******************************************************************************
*
*/

static IPTR List__MUIM_CreateDragImage(struct IClass *cl, Object *obj,
    struct MUIP_CreateDragImage *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    BOOL success = TRUE;
    struct MUI_List_TestPos_Result pos;
    WORD width, height, left, top;
    struct MUI_DragImage *img = NULL;
    const struct ZuneFrameGfx *zframe;
    LONG depth;

    /* If entries aren't draggable, allow the list as a whole to be */
    if (data->drag_type == MUIV_Listview_DragType_None)
        return DoSuperMethodA(cl, obj, msg);

    /* Get info on dragged entry */
    DoMethod(obj, MUIM_List_TestPos, _left(data->area) - msg->touchx,
        _top(data->area) - msg->touchy, (IPTR) &pos);
    if (pos.entry == -1)
        success = FALSE;

    if (success)
    {
        /* Get boundaries of entry */
        width = _mwidth(data->area);
        height = data->entry_maxheight;
        left = _mleft(data->area);
        top = _top(data->area) - msg->touchy
            - (pos.yoffset + data->entry_maxheight / 2);

        /* Allocate drag image structure */
        img = (struct MUI_DragImage *)
            AllocVec(sizeof(struct MUI_DragImage), MEMF_CLEAR);
        if (img == NULL)
            success = FALSE;
    }

    if (success)
    {
        /* Get drag frame */
        zframe = zune_zframe_get(obj,
            &muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Drag]);

        /* Allocate drag image buffer */
        img->width = width + zframe->ileft + zframe->iright;
        img->height = height + zframe->itop + zframe->ibottom;
        depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);
        img->bm = AllocBitMap(img->width, img->height, depth, BMF_MINPLANES,
            _screen(obj)->RastPort.BitMap);

        if (img->bm != NULL)
        {
            /* Render entry */
            struct RastPort temprp;
            InitRastPort(&temprp);
            temprp.BitMap = img->bm;
            ClipBlit(_rp(obj), left, top, &temprp,
                zframe->ileft, zframe->itop, width, height,
                0xc0);

            /* Render frame */
            struct RastPort *rp_save = muiRenderInfo(obj)->mri_RastPort;
            muiRenderInfo(obj)->mri_RastPort = &temprp;
            zframe->draw(zframe->customframe, muiRenderInfo(obj), 0, 0,
                img->width, img->height, 0, 0, img->width, img->height);
            muiRenderInfo(obj)->mri_RastPort = rp_save;
        }

        /* Ensure drag point matches where user clicked */
        img->touchx = msg->touchx - zframe->ileft + _addleft(obj);
        img->touchy = -(pos.yoffset + data->entry_maxheight / 2)
            - zframe->itop;
        img->flags = 0;
    }

    return (IPTR) img;
}

static void DoWheelMove(struct IClass *cl, Object *obj, LONG wheely)
{
    LONG new, first, entries, visible;

    new = first = XGET(obj, MUIA_List_First);
    entries = XGET(obj, MUIA_List_Entries);
    visible = XGET(obj, MUIA_List_Visible);

    new += wheely;

    if (new > entries - visible)
    {
        new = entries - visible;
    }

    if (new < 0)
    {
        new = 0;
    }

    if (new != first)
    {
        set(obj, MUIA_List_First, new);
    }
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
IPTR List__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct MUI_List_TestPos_Result pos;
    LONG seltype = MUIV_List_Select_On, old_active, new_active, visible,
        first, last, i;
    IPTR result = 0;
    BOOL select = FALSE, clear = FALSE, range_select = FALSE, changing;
    WORD delta;
    typeof(msg->muikey) muikey = msg->muikey;

    new_active = old_active = XGET(obj, MUIA_List_Active);
    visible = XGET(obj, MUIA_List_Visible);

    if (muikey != MUIKEY_NONE)
    {
        result = MUI_EventHandlerRC_Eat;

        /* Make keys behave differently in read-only mode */
        if (data->read_only)
        {
            switch (muikey)
            {
            case MUIKEY_TOP:
                muikey = MUIKEY_LINESTART;
                break;

            case MUIKEY_BOTTOM:
                muikey = MUIKEY_LINEEND;
                break;

            case MUIKEY_UP:
                muikey = MUIKEY_LEFT;
                break;

            case MUIKEY_DOWN:
            case MUIKEY_PRESS:
                muikey = MUIKEY_RIGHT;
                break;
            }
        }

        switch (muikey)
        {
        case MUIKEY_TOGGLE:
            if (data->multiselect != MUIV_Listview_MultiSelect_None
                && !data->read_only)
            {
                select = TRUE;
                data->click_column = data->def_click_column;
                new_active = MUIV_List_Active_Down;
            }
            else
            {
                DoMethod(obj, MUIM_List_Jump, 0);
                muikey = MUIKEY_NONE;
            }
            break;

        case MUIKEY_TOP:
            new_active = MUIV_List_Active_Top;
            break;

        case MUIKEY_BOTTOM:
            new_active = MUIV_List_Active_Bottom;
            break;

        case MUIKEY_LEFT:
        case MUIKEY_WORDLEFT:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Up);
            break;

        case MUIKEY_RIGHT:
        case MUIKEY_WORDRIGHT:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Down);
            break;

        case MUIKEY_LINESTART:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Top);
            break;

        case MUIKEY_LINEEND:
            DoMethod(obj, MUIM_List_Jump, MUIV_List_Jump_Bottom);
            break;

        case MUIKEY_UP:
            new_active = MUIV_List_Active_Up;
            break;

        case MUIKEY_DOWN:
            new_active = MUIV_List_Active_Down;
            break;

        case MUIKEY_PRESS:
            data->click_column = data->def_click_column;
            superset(cl, obj, MUIA_Listview_ClickColumn,
                data->click_column);
            set(obj, MUIA_Listview_DoubleClick, TRUE);
            break;

        case MUIKEY_PAGEUP:
            if (data->read_only)
                DoWheelMove(cl, obj, -visible);
            else
                new_active = MUIV_List_Active_PageUp;
            break;

        case MUIKEY_PAGEDOWN:
            if (data->read_only)
                DoWheelMove(cl, obj, visible);
            else
                new_active = MUIV_List_Active_PageDown;
            break;

        default:
            result = 0;
        }
    }
    else if (msg->imsg)
    {
        DoMethod(obj, MUIM_List_TestPos, msg->imsg->MouseX, msg->imsg->MouseY,
            (IPTR) &pos);

        switch (msg->imsg->Class)
        {
        case IDCMP_MOUSEBUTTONS:
            if (msg->imsg->Code == SELECTDOWN)
            {
                if (_isinobject(data->area, msg->imsg->MouseX,
                    msg->imsg->MouseY))
                {
                    data->mouse_click = MOUSE_CLICK_ENTRY;

                    if (!data->read_only && pos.entry != -1)
                    {
                        new_active = pos.entry;

                        clear = (data->multiselect
                            == MUIV_Listview_MultiSelect_Shifted
                            && (msg->imsg->Qualifier
                            & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) == 0);
                        seltype = clear ? MUIV_List_Select_On
                            : MUIV_List_Select_Toggle;
                        select = data->multiselect
                            != MUIV_Listview_MultiSelect_None;

                        /* Handle MUIA_Listview_ClickColumn */
                        data->click_column = pos.column;
                        superset(cl, obj, MUIA_Listview_ClickColumn,
                            data->click_column);

                        /* Handle double clicking */
                        if (data->last_active == pos.entry
                            && DoubleClick(data->last_secs, data->last_mics,
                            msg->imsg->Seconds, msg->imsg->Micros))
                        {
                            set(obj, MUIA_Listview_DoubleClick, TRUE);
                            data->last_active = -1;
                            data->last_secs = data->last_mics = 0;
                        }
                        else
                        {
                            data->last_active = pos.entry;
                            data->last_secs = msg->imsg->Seconds;
                            data->last_mics = msg->imsg->Micros;
                        }

                        /* Look out for mouse movement, timer and
                           inactive-window events while mouse button is
                           down */
                        DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                            (IPTR) &data->ehn);
                        data->ehn.ehn_Events |= (IDCMP_MOUSEMOVE
                            | IDCMP_INTUITICKS |IDCMP_INACTIVEWINDOW);
                        DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                            (IPTR) &data->ehn);
                    }
                }
            }
            else
            {
                if (msg->imsg->Code == SELECTUP && data->mouse_click)
                {
                    /* cache click position ... */
                    data->mouse_x = msg->imsg->MouseX;
                    data->mouse_y = msg->imsg->MouseY;

                    /* ... and activate the object */
                    set(_win(obj), MUIA_Window_ActiveObject, (IPTR)obj);
                    data->mouse_click = 0;
                }

                /* Restore normal event mask */
                DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                    (IPTR) &data->ehn);
                data->ehn.ehn_Events &= ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS
                    | IDCMP_INACTIVEWINDOW);
                DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                    (IPTR) &data->ehn);
            }
            break;

        case IDCMP_MOUSEMOVE:
        case IDCMP_INTUITICKS:
            if (pos.flags & MUI_LPR_ABOVE)
                new_active = MUIV_List_Active_Up;
            else if (pos.flags & MUI_LPR_BELOW)
                new_active = MUIV_List_Active_Down;
            else
                new_active = pos.entry;

            select = new_active != old_active
                && data->multiselect != MUIV_Listview_MultiSelect_None;
            if (select)
            {
                seltype = data->seltype;
                range_select = new_active >= 0;
            }

            break;

        case IDCMP_INACTIVEWINDOW:
            /* Stop listening for events we only listen to when mouse button is
               down: we will not be informed of the button being released */
            DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                (IPTR) &data->ehn);
            data->ehn.ehn_Events &=
                ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS | IDCMP_INACTIVEWINDOW);
            DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                (IPTR) &data->ehn);
            break;

        case IDCMP_RAWKEY:
            /* Scroll wheel */
            if (data->vert && _isinobject(data->vert, msg->imsg->MouseX,
                msg->imsg->MouseY))
                delta = 1;
            else if (_isinobject(data->area, msg->imsg->MouseX,
                msg->imsg->MouseY))
                delta = 4;
            else
                delta = 0;

            if (delta != 0)
            {
                switch (msg->imsg->Code)
                {
                case RAWKEY_NM_WHEEL_UP:
                    DoWheelMove(cl, obj, -delta);
                    break;

                case RAWKEY_NM_WHEEL_DOWN:
                    DoWheelMove(cl, obj, delta);
                    break;
                }
                result = MUI_EventHandlerRC_Eat;
            }
            break;
        }
    }

    /* Decide in advance if any selections may change */
    changing = clear || muikey == MUIKEY_TOGGLE || select;

    /* Change selected and active entries */
    if (clear)
    {
        DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All,
            MUIV_List_Select_Off, NULL);
    }

    if (muikey == MUIKEY_TOGGLE)
    {
        DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active,
            MUIV_List_Select_Toggle, NULL);
        select = FALSE;
    }

    if (new_active != old_active)
        set(obj, MUIA_List_Active, new_active);

    if (select)
    {
        if (range_select)
        {
            if (old_active < new_active)
                first = old_active + 1, last = new_active;
            else
                first = new_active, last = old_active - 1;
            for (i = first; i <= last; i++)
                DoMethod(obj, MUIM_List_Select, i, seltype, NULL);
        }
        else
        {
            DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active,
                seltype, NULL);
            DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active,
                MUIV_List_Select_Ask, &data->seltype);
        }
    }

    if (changing)
        superset(cl, obj, MUIA_Listview_SelectChange, TRUE);

    return result;
}

/**************************************************************************
 Dispatcher
**************************************************************************/
BOOPSI_DISPATCHER(IPTR, List_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return List__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return List__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return List__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return List__OM_GET(cl, obj, (struct opGet *)msg);

    case MUIM_Setup:
        return List__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return List__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
    case MUIM_HandleEvent:
        return List__MUIM_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    case MUIM_AskMinMax:
        return List__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    case MUIM_Show:
        return List__MUIM_Show(cl, obj, (struct MUIP_Show *)msg);
    case MUIM_Hide:
        return List__MUIM_Hide(cl, obj, (struct MUIP_Hide *)msg);
    case MUIM_Draw:
        return List__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    case MUIM_Layout:
        return List__MUIM_Layout(cl, obj, (struct MUIP_Layout *)msg);
    case MUIM_List_Clear:
        return List__MUIM_Clear(cl, obj, (struct MUIP_List_Clear *)msg);
    case MUIM_List_Sort:
        return List__MUIM_Sort(cl, obj, (struct MUIP_List_Sort *)msg);
    case MUIM_List_Exchange:
        return List__MUIM_Exchange(cl, obj,
            (struct MUIP_List_Exchange *)msg);
    case MUIM_List_Insert:
        return List__MUIM_Insert(cl, obj, (APTR) msg);
    case MUIM_List_InsertSingle:
        return List__MUIM_InsertSingle(cl, obj, (APTR) msg);
    case MUIM_List_GetEntry:
        return List__MUIM_GetEntry(cl, obj, (APTR) msg);
    case MUIM_List_Redraw:
        return List__MUIM_Redraw(cl, obj, (APTR) msg);
    case MUIM_List_Remove:
        return List__MUIM_Remove(cl, obj, (APTR) msg);
    case MUIM_List_Select:
        return List__MUIM_Select(cl, obj, (APTR) msg);
    case MUIM_List_Construct:
        return List__MUIM_Construct(cl, obj, (APTR) msg);
    case MUIM_List_Destruct:
        return List__MUIM_Destruct(cl, obj, (APTR) msg);
    case MUIM_List_Compare:
        return List__MUIM_Compare(cl, obj, (APTR) msg);
    case MUIM_List_Display:
        return List__MUIM_Display(cl, obj, (APTR) msg);
    case MUIM_List_SelectChange:
        return List__MUIM_SelectChange(cl, obj, (APTR) msg);
    case MUIM_List_CreateImage:
        return List__MUIM_CreateImage(cl, obj, (APTR) msg);
    case MUIM_List_DeleteImage:
        return List__MUIM_DeleteImage(cl, obj, (APTR) msg);
    case MUIM_List_Jump:
        return List__MUIM_Jump(cl, obj, (APTR) msg);
    case MUIM_List_Move:
        return List__MUIM_Move(cl, obj, (struct MUIP_List_Move *)msg);
    case MUIM_List_NextSelected:
        return List__MUIM_NextSelected(cl, obj,
           (struct MUIP_List_NextSelected *)msg);
    case MUIM_List_TestPos:
        return List__MUIM_TestPos(cl, obj, (APTR) msg);
    case MUIM_DragQuery:
        return List__MUIM_DragQuery(cl, obj, (APTR) msg);
    case MUIM_DragFinish:
        return List__MUIM_DragFinish(cl, obj, (APTR) msg);
    case MUIM_DragReport:
        return List__MUIM_DragReport(cl, obj, (APTR) msg);
    case MUIM_DragDrop:
        return List__MUIM_DragDrop(cl, obj, (APTR) msg);
    case MUIM_CreateDragImage:
        return List__MUIM_CreateDragImage(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_List_desc =
{
    MUIC_List,
    MUIC_Group,
    sizeof(struct MUI_ListData),
    (void *) List_Dispatcher
};
