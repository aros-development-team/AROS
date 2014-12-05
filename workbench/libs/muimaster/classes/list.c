/*
    Copyright � 2002-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdlib.h>

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/view.h>
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
    LONG *widths; /* Widths of the columns */
    LONG width;   /* Line width */
    LONG height;  /* Line height */
    WORD flags;   /* see below */
};

#define ENTRY_SELECTED   (1<<0)


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
    struct ColumnInfo *ci;
    STRPTR *preparses;
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

    LONG drop_mark_y;

    /* list images */
    struct MinList images;

    /* user prefs */
    ListviewRefresh prefs_refresh;
    UWORD prefs_linespacing;
    BOOL prefs_smoothed;
    UWORD prefs_smoothval;
};

#define LIST_ADJUSTWIDTH   (1<<0)
#define LIST_ADJUSTHEIGHT  (1<<1)
#define LIST_AUTOVISIBLE   (1<<2)
#define LIST_DRAGSORTABLE  (1<<3)
#define LIST_SHOWDROPMARKS (1<<4)
#define LIST_QUIET         (1<<5)


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

/**************************************************************************
 Allocate a single list entry, does not initialize it (except the pointer)
**************************************************************************/
static struct ListEntry *AllocListEntry(struct MUI_ListData *data)
{
    ULONG *mem;
    struct ListEntry *le;
    int size = sizeof(struct ListEntry) + sizeof(LONG) * data->columns + 4;
                                                               /* sizeinfo */
    LONG j;

    mem = AllocPooled(data->pool, size);
    if (!mem)
        return NULL;
    D(bug("List AllocListEntry %p, %ld bytes\n", mem, size));

    mem[0] = size;              /* Save the size */
    le = (struct ListEntry *)(mem + 1);
    le->widths = (LONG *) (le + 1);

    /* Initialize fields */
    le->height = 0;
    le->width = 0;
    le->flags = 0;
    for (j = 0; j < data->columns; j++)
        le->widths[j] = 0;

    return le;
}

/**************************************************************************
 Deallocate a single list entry, does not deinitialize it
**************************************************************************/
static void FreeListEntry(struct MUI_ListData *data,
    struct ListEntry *entry)
{
    ULONG *mem = ((ULONG *) entry) - 1;
    D(bug("FreeListEntry %p size=%ld\n", mem, mem[0]));
    FreePooled(data->pool, mem, mem[0]);
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
    if (data->strings)
    {
        FreeVec(data->strings - 1);
        data->strings = NULL;
    }
    data->columns = 0;
}

/**************************************************************************
 Parses the given format string (also frees a previously parsed format).
 Return 0 on failure.
**************************************************************************/
static int ParseListFormat(struct MUI_ListData *data, STRPTR format)
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
            AllocVec((new_columns + 10) * sizeof(STRPTR), 0)))
        return 0;

    if (!(data->strings = AllocVec((new_columns + 1 + 10)
        * sizeof(STRPTR), 0)))    /* hold enough space also for the entry pos,
                                   * used by orginal MUI and also some
                                   * security space */
        return 0;

    if (!(data->ci = AllocVec(new_columns * sizeof(struct ColumnInfo), 0)))
        return 0;

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

    data->columns = new_columns;
    data->strings++;            /* Skip entry pos */

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
        entry_data = data->entries[entry_pos]->data;

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
                /* This columns width is bigger than the other in the same
                 * columns, so we store this value
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
        data->entries_maxwidth += data->ci[j].entries_width;

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

    data->entries_visible = (_mheight(obj) - data->title_height)
        / (data->entry_maxheight /* + data->prefs_linespacing */ );

    /* Distribute extra vertical space evenly between top and bottom of
     * list */

    data->entries_top_pixel = _mtop(obj) + data->title_height
        + (_mheight(obj) - data->title_height
        -
        data->entries_visible *
        (data->entry_maxheight /* + data->prefs_linespacing */ )) / 2;

    return (old_entries_visible != data->entries_visible)
        || (old_entries_top_pixel != data->entries_top_pixel);
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

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Font, MUIV_Font_List,
        MUIA_ShowSelState, FALSE,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Background, MUII_ListBack, TAG_MORE, (IPTR) msg->ops_AttrList);
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
            data->format = StrDup((STRPTR) tag->ti_Data);
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
            set(obj, MUIA_Draggable, tag->ti_Data);
            break;
        }
    }

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
    if (!(ParseListFormat(data, data->format)))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    /* This is neccessary for at least the title */
    if (!SetListSize(data, 0))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (data->title)
    {
        if (!(data->entries[ENTRY_TITLE] = AllocListEntry(data)))
        {
            CoerceMethod(cl, obj, OM_DISPOSE);
            return 0;
        }
    }
    else
        data->entries[ENTRY_TITLE] = NULL;

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

    D(bug("List_New(%lx)\n", obj));

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
    FreeVec(data->format);

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
            data->format = StrDup((STRPTR) tag->ti_Data);
            ParseListFormat(data, data->format);
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

                    /* Selectchange stuff */
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

                    data->update = 2;
                    data->update_pos = old;
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                    data->update = 2;
                    data->update_pos = data->entries_active;
                    MUI_Redraw(obj, MADF_DRAWUPDATE);

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
            data->update = 3;
            data->entries_first = tag->ti_Data;

            MUI_Redraw(obj, MADF_DRAWUPDATE);
            if ((data->vertprop_first != tag->ti_Data)
                && (!(data->flags & LIST_QUIET)))
            {
                set(obj, MUIA_List_VertProp_First, tag->ti_Data);
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
            if (!tag->ti_Data)
            {
                DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
                if (data->entries_num != XGET(obj, MUIA_List_VertProp_Entries))
                    set(obj, MUIA_List_VertProp_Entries, data->entries_num);
                if (data->vertprop_first !=
                    XGET(obj, MUIA_List_VertProp_First))
                    set(obj, MUIA_List_VertProp_First, data->vertprop_first);
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
            set(obj, MUIA_Draggable, tag->ti_Data);
            break;

        case MUIA_Selected:
            /* Swallow this so the Area class doesn't redraw us */
            tag->ti_Tag = TAG_IGNORE;
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
        break;
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

    CalcWidths(cl, obj);

    data->list_cursor =
        zune_imspec_setup(MUII_ListCursor, muiRenderInfo(obj));
    data->list_select =
        zune_imspec_setup(MUII_ListSelect, muiRenderInfo(obj));
    data->list_selcur =
        zune_imspec_setup(MUII_ListSelCur, muiRenderInfo(obj));

    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
IPTR List__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListImage *li = List_First(&data->images);

    while (li)
    {
        struct ListImage *next = Node_Next(li);
        DoMethod(obj, MUIM_List_DeleteImage, (IPTR) li);
        li = next;
    }

    zune_imspec_cleanup(data->list_cursor);
    zune_imspec_cleanup(data->list_select);
    zune_imspec_cleanup(data->list_selcur);

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


    if ((data->flags & LIST_ADJUSTWIDTH) && (data->entries_num > 0))
    {
        msg->MinMaxInfo->MinWidth += data->entries_maxwidth;
        msg->MinMaxInfo->DefWidth += data->entries_maxwidth;
        msg->MinMaxInfo->MaxWidth += data->entries_maxwidth;
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
            msg->MinMaxInfo->DefHeight += data->entries_totalheight;
            msg->MinMaxInfo->MaxHeight += data->entries_totalheight;
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
    D(bug("List %p minheigh=%d, line maxh=%d\n",
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
    x1 = _mleft(obj);

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

    if (data->flags & LIST_QUIET)
        return 0;

    DoSuperMethodA(cl, obj, (Msg) msg);

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

    if ((msg->flags & MADF_DRAWUPDATE) == 0 || data->update == 1)
    {
        DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
            _mwidth(obj), _mheight(obj),
            0, data->entries_first * data->entry_maxheight, 0);
    }

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
        _mwidth(obj), _mheight(obj));

    if ((msg->flags & MADF_DRAWUPDATE) == 0 || data->update == 1)
    {
        y = _mtop(obj);
        /* Draw Title
         */
        if (data->title_height && data->title)
        {
            List_DrawEntry(cl, obj, ENTRY_TITLE, y);
            y += data->entries[ENTRY_TITLE]->height;
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
            Move(_rp(obj), _mleft(obj), y);
            Draw(_rp(obj), _mright(obj), y);
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
            y++;
            Move(_rp(obj), _mleft(obj), y);
            Draw(_rp(obj), _mright(obj), y);
        }
    }

    y = data->entries_top_pixel;

    start = data->entries_first;
    end = data->entries_first + data->entries_visible;

    if ((msg->flags & MADF_DRAWUPDATE) && data->update == 3)
    {
        int diffy = data->entries_first - data->update_pos;
        int top, bottom;
        if (abs(diffy) < data->entries_visible)
        {
            scroll_caused_damage =
                (_rp(obj)->Layer->Flags & LAYERREFRESH) ? FALSE : TRUE;

            ScrollRaster(_rp(obj), 0, diffy * data->entry_maxheight,
                _mleft(obj), y,
                _mright(obj),
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

        DoMethod(obj, MUIM_DrawBackground, _mleft(obj), top,
            _mwidth(obj), bottom - top + 1,
            0,
            top - _mtop(obj) + data->entries_first * data->entry_maxheight,
            0);
    }

    for (entry_pos = start;
        entry_pos < end && entry_pos < data->entries_num; entry_pos++)
    {
        struct ListEntry *entry = data->entries[entry_pos];

        if (!(msg->flags & MADF_DRAWUPDATE) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == 1) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == 3) ||
            ((msg->flags & MADF_DRAWUPDATE) && data->update == 2
                && data->update_pos == entry_pos))
        {
            /* Choose appropriate highlight image */

            if (entry_pos == data->entries_active
                && entry->flags & ENTRY_SELECTED)
                highlight = data->list_selcur;
            else if (entry_pos == data->entries_active)
                highlight = data->list_cursor;
            else if (entry->flags & ENTRY_SELECTED)
                highlight = data->list_select;
            else
                highlight = NULL;

            /* Draw highlight or background */

            if (highlight != NULL)
            {
                zune_imspec_draw(highlight, muiRenderInfo(obj),
                    _mleft(obj), y, _mwidth(obj), data->entry_maxheight,
                    0, y - data->entries_top_pixel, 0);
            }
            else if ((msg->flags & MADF_DRAWUPDATE) && data->update == 2
                && data->update_pos == entry_pos)
            {
                DoMethod(obj, MUIM_DrawBackground, _mleft(obj), y,
                    _mwidth(obj), data->entry_maxheight, 0,
                    y - _mtop(obj) +
                    data->entries_first * data->entry_maxheight, 0);
            }

            List_DrawEntry(cl, obj, entry_pos, y);
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

    ULONG x1 = _mleft(obj);
    ULONG col;
    y = _mtop(obj);

    if (data->title_height && data->title)
    {
        for (col = 0; col < data->columns; col++)
        {
            ULONG halfdelta = data->ci[col].delta / 2;
            x1 += data->ci[col].entries_width + halfdelta;

            if (x1 + (data->ci[col].bar ? BAR_WIDTH : 0) > _mright(obj))
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

    x1 = _mleft(obj);

    for (col = 0; col < data->columns; col++)
    {
        ULONG halfdelta = data->ci[col].delta / 2;
        x1 += data->ci[col].entries_width + halfdelta;

        if (x1 + (data->ci[col].bar ? BAR_WIDTH : 0) > _mright(obj))
            break;

        if (data->ci[col].bar)
        {
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
            Move(_rp(obj), x1, y);
            Draw(_rp(obj), x1, _mbottom(obj));
            SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
            Move(_rp(obj), x1 + 1, y);
            Draw(_rp(obj), x1 + 1, _mbottom(obj));

            x1 += BAR_WIDTH;
        }

        x1 += data->ci[col].delta - halfdelta;
    }

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

        data->update = 1;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return 0;
}

/**************************************************************************
 MUIM_List_Exchange
**************************************************************************/
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

        data->update = 2;
        data->update_pos = pos1;
        MUI_Redraw(obj, MADF_DRAWUPDATE);

        data->update = 2;
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

    if (!(data->flags & LIST_QUIET))
    {
        if (msg->pos == MUIV_List_Redraw_All)
        {
            data->update = 1;
            CalcWidths(cl, obj);
            MUI_Redraw(obj, MADF_DRAWUPDATE);
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
                if (CalcDimsOfEntry(cl, obj, pos))
                    data->update = 1;
                else
                {
                    data->update = 2;
                    data->update_pos = pos;
                }
                MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
        }
    }
    return 0;
}

/**************************************************************************
 MUIM_List_Remove
**************************************************************************/
IPTR List__MUIM_Remove(struct IClass *cl, Object *obj,
    struct MUIP_List_Remove *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, cur;
    LONG new_act;
    struct ListEntry *lentry;
    //int rem_count = 1;

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
        /* TODO: needs special handling */
        pos = data->entries_active;
        break;

    default:
        pos = msg->pos;
        break;
    }

    if (pos < 0 || pos >= data->entries_num)
        return 0;

    new_act = data->entries_active;

    if (pos == new_act && new_act == data->entries_num - 1)
        new_act--;              /* might become MUIV_List_Active_Off */

    lentry = data->entries[pos];
    DoMethod(obj, MUIM_List_Destruct, (IPTR) lentry->data,
        (IPTR) data->pool);

    cur = pos + 1;

    RemoveListEntries(data, pos, cur - pos);
    data->confirm_entries_num -= cur - pos;

    /* ensure that the active element is in a valid range */
    if (new_act >= data->entries_num)
        new_act = data->entries_num - 1;

    SetAttrs(obj, MUIA_List_Entries, data->confirm_entries_num,
        (new_act >= pos) || (new_act != data->entries_active) ?
        MUIA_List_Active : TAG_DONE,
        new_act,   /* Inform only if neccessary (for notify) */
        TAG_DONE);

    data->update = 1;
    MUI_Redraw(obj, MADF_DRAWUPDATE);

    return 0;
}

/**************************************************************************
 MUIM_List_Select
**************************************************************************/
IPTR List__MUIM_Select(struct IClass *cl, Object *obj,
    struct MUIP_List_Select *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, i, count, selcount=0, state=0;
    BOOL multi_allowed = TRUE, new_select_state = FALSE;

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
        /* Disallow selection of an additional entry if there is a currently
           selected entry that is not multi-selectable (in such case there
           will only be one entry currently selected, so no need to iterate) */
        i = MUIV_List_NextSelected_Start;
        DoMethod(obj, MUIM_List_NextSelected, (IPTR) &i);
        if (i != MUIV_List_NextSelected_End)
            selcount++;
        if (data->multi_test_hook != NULL && selcount != 0)
            multi_allowed = CallHookPkt(data->multi_test_hook, NULL,
                data->entries[i]->data);
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
            /* Disallow selection if entry is not multi-selectable and
             * there are already selected entries */
            if (data->multi_test_hook != NULL && new_select_state)
                new_select_state = multi_allowed && (selcount == 0 ||
                    CallHookPkt(data->multi_test_hook, NULL,
                        data->entries[i]->data));

            if (new_select_state)
                data->entries[i]->flags |= ENTRY_SELECTED;
            else
                data->entries[i]->flags &= ~ENTRY_SELECTED;
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
            data->update = 1;
        else
        {
            data->update = 2;
            data->update_pos = pos;
        }
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
   
    return 0;
}

/**************************************************************************
 MUIM_List_Insert
**************************************************************************/

IPTR List__MUIM_Insert(struct IClass *cl, Object *obj,
    struct MUIP_List_Insert *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    LONG pos, count, sort;

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

        data->entries[pos] = lentry;
        data->confirm_entries_num++;

        if (_flags(obj) & MADF_SETUP)
        {
            /* We have to calculate the width and height of the newly
             * inserted entry. This has to be done after inserting the
             * element into the list */
            CalcDimsOfEntry(cl, obj, pos);
        }

        toinsert++;
        pos++;
    }

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
        DoMethod(obj, MUIM_List_Sort);
        /* TODO: which pos to return here !?        */
        /* MUIM_List_Sort already called MUI_Redraw */
    }
    else
    {
        data->update = 1;
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    data->insert_position = pos;

    return (ULONG) pos;
}

/**************************************************************************
 MUIM_List_InsertSingle
**************************************************************************/
IPTR List__MUIM_InsertSingle(struct IClass *cl, Object *obj,
    struct MUIP_List_InsertSingle *msg)
{
    return DoMethod(obj, MUIM_List_Insert, (IPTR) & msg->entry, 1,
        msg->pos);
}

/**************************************************************************
 MUIM_List_GetEntry
**************************************************************************/
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

/**************************************************************************
 MUIM_List_CreateImage
Called by a List subclass in its Setup method.
Connects an Area subclass object to the list, much like an object gets
connected to a window. List calls Setup and AskMinMax on that object,
keeps a reference to it (that reference will be returned).
Text engine will dereference that pointer and draw the object with its
default size.
**************************************************************************/
IPTR List__MUIM_CreateImage(struct IClass *cl, Object *obj,
    struct MUIP_List_CreateImage *msg)
{
    struct MUI_ListData *data = INST_DATA(cl, obj);
    struct ListImage *li;

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

/**************************************************************************
 MUIM_List_DeleteImage
**************************************************************************/
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
                data->entries[max] = tmp;
            }
        }
    }

    data->update = 1;
    MUI_Redraw(obj, MADF_DRAWUPDATE);

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

    /* Update index of active entry */
    if (from == data->entries_active)
        data->entries_active = to;
    else if (data->entries_active > from && data->entries_active < to)
        data->entries_active--;
    else if (data->entries_active < from && data->entries_active >= to)
        data->entries_active++;

    /* Reflect list changes visually */
    data->update = 1;
    MUI_Redraw(obj, MADF_DRAWUPDATE);

    return TRUE;
}

/**************************************************************************
 MUIM_List_NextSelected
**************************************************************************/
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

    /* Return index of selected entry, or indicate there are no more */
    if (!found)
        pos = MUIV_List_NextSelected_End;
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
    UWORD flags = 0;
    LONG mx = msg->x - _left(obj);
    LONG entries_visible;

    if (data->entries_visible <= data->entries_num)
        entries_visible = data->entries_visible;
    else
        entries_visible = data->entries_num;
    LONG ey = msg->y - data->entries_top_pixel;
        /* y coordinates transformed to the entries */

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
    else if (mx >= _width(obj))
        flags |= MUI_LPR_RIGHT;
    else
    {
        /* Identify column */
        if (data->entries_num > 0 && data->columns > 0)
        {
            LONG width_sum = 0;
            for (col = 0; col < data->columns; col++)
            {
                result->xoffset = mx - width_sum;
                width_sum +=
                    data->ci[col].entries_width +
                    data->ci[col].delta +
                    (data->ci[col].bar ? BAR_WIDTH : 0);
                D(bug("[List/MUIM_TestPos] col %d "
                    "width %d width_sum %d mx %d\n",
                    col, data->ci[col].entries_width, width_sum, mx));
                if (mx < width_sum)
                {
                    D(bug("[List/MUIM_TestPos] Column hit %d\n", col));
                    break;
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
    if (msg->obj == obj)
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
            DoMethod(obj, MUIM_DrawBackground, _mleft(obj), data->drop_mark_y,
                _mwidth(obj), 1, 0, 0, 0);

            /* Draw new drop mark and store its position */

            SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], _pens(obj)[MPEN_SHADOW],
                JAM2);
            old_pattern = rp->LinePtrn;
            SetDrPt(rp, 0xF0F0);
            Move(rp, _mleft(obj), y);
            Draw(rp, _mright(obj), y);
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

        /* Ensure that dropped entry will be positioned between the two
         * entries that are above and below the drop mark, rather than
         * strictly at the numeric index shown */

        if (n > data->entries_active)
            n--;
    }
    else if ((pos.flags & MUI_LPR_ABOVE) != 0)
        n = MUIV_List_Move_Top;
    else
        n = MUIV_List_Move_Bottom;

    DoMethod(msg->obj, MUIM_List_Move, MUIV_List_Move_Active, n);

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

    /* Get info on dragged entry */
    DoMethod(obj, MUIM_List_TestPos, _left(obj) - msg->touchx,
        _top(obj) - msg->touchy, (IPTR) &pos);
    if (pos.entry == -1)
        success = FALSE;

    if (success)
    {
        /* Get boundaries of entry */
        width = _mwidth(obj);
        height = data->entry_maxheight;
        left = _mleft(obj);
        top = _top(obj) - msg->touchy
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
    MUIC_Area,
    sizeof(struct MUI_ListData),
    (void *) List_Dispatcher
};
