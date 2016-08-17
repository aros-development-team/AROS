/*
    Copyright � 2002-2016, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <aros/debug.h>

/* the following should go in a single include file which then only
** consists of the public constants and members. Actually this is easy
*/

#if 0
#define TEST_ICONLIST
#endif

#include <libraries/mui.h>
#if defined(TEST_ICONLIST)
#include "../../workbench/system/Wanderer/Classes/iconlist_attributes.h"
#include "../../workbench/system/Wanderer/Classes/iconlist.h"
#endif

#define STRING_COUNT 6
#define LIST_COUNT 5
#define MULTI_LIST_COUNT 2

#define NUMERIC_MIN 0
#define NUMERIC_MAX 100

static const TEXT digits[] = "-0123456789";
static const TEXT default_accept_chars[] = "aeiou?.";
static const TEXT default_reject_chars[] = "*?";
static const ULONG default_color[] = {155 << 24, 180 << 24, 255 << 24};
static const struct MUI_PenSpec default_penspec = {"m0"};
static const char *fruits[] = {"Strawberry", "Apple", "Banana", "Orange",
    "Grapefruit", "Kumquat", "Plum", "Raspberry", "Apricot", "Grape",
    "Peach", "Lemon", "Lime", "Date", "Pineapple", "Blueberry", "Papaya",
    "Cranberry", "Gooseberry", "Pear", "Fig", "Coconut", "Melon",
    "Pumpkin", NULL};
static const char *empty[] = {"", "", "", "", "", NULL};
static const LONG list_active_positions[] =
{
    MUIV_List_Active_Top,
    MUIV_List_Active_Bottom,
    MUIV_List_Active_Off,
    MUIV_List_Active_Off,
    MUIV_List_Active_Off
};
static const CONST_STRPTR list_move1_modes[] =
    {"Index", "Top", "Active", "Bottom", NULL};
static const CONST_STRPTR list_move2_modes[] =
    {"Index", "Top", "Active", "Bottom", "Next", "Previous", NULL};
static const CONST_STRPTR list_jump_modes[] =
    {"Index", "Top", "Active", "Bottom", "Down", "Up", NULL};
static const CONST_STRPTR list_insert_modes[] =
    {"Index", "Top", "Active", "Sorted", "Bottom", NULL};
static const CONST_STRPTR list_remove_modes[] =
    {"Index", "First", "Active", "Last", "Selected", "Unsafe Loop",
    "Safe Loop", NULL};
static const CONST_STRPTR list_activate_modes[] =
    {"Index", "Top", "Bottom", "Up", "Down", "Page Up", "Page Down", NULL};
static const CONST_STRPTR list_select_modes[] =
    {"Index", "Active", "All", NULL};
static const TEXT list_format[] = "BAR,BAR,";

enum
{
    HNSLIDER,
    HRSLIDER,
    HQSLIDER,
    VNSLIDER,
    VRSLIDER,
    VQSLIDER,
    NNUMERICBUTTON,
    RNUMERICBUTTON,
    NKNOB,
    RKNOB,
    NUMERIC_COUNT
};

struct Library *MUIMasterBase;
struct Library *ColorWheelBase;

struct list_entry
{
    char *column1;
    char *column2;
};

Object *app, *wnd;

static struct
{
    Object *strings[STRING_COUNT],
        *accept_string,
        *reject_string,
        *cursor_pos_slider,
        *display_pos_slider,
        *integer_string,
        *plaintext_string,
        *max_len_text,
        *cr_advance_check,
        *attach_list_check,
        *standard_hook_check,
        *custom_hook_check,
        *accept_all_check;
}
string;

static struct
{
    Object *lists[LIST_COUNT],
        *list_radios,
        *index1_string,
        *index2_string,
        *title_string,
        *move1_cycle,
        *move2_cycle,
        *jump_cycle,
        *select_cycle,
        *insert_cycle,
        *remove_cycle,
        *activate_cycle,
        *sort_button,
        *clear_button,
        *enable_button,
        *reset_button,
        *select_button,
        *deselect_button,
        *toggle_button,
        *move_button,
        *exchange_button,
        *jump_button,
        *redraw_button,
        *insert_single_button,
        *insert_multiple_button,
        *remove_button,
        *activate_button,
        *deactivate_button,
        *dragsortable_check,
        *showdropmarks_check,
        *quiet_check,
        *autovisible_check,
        *entries_text,
        *visible_text,
        *first_text,
        *insert_text,
        *active_text,
        *multi_lists[MULTI_LIST_COUNT],
        *format_string,
        *column_string,
        *column_text,
        *def_column_string,
        *showheadings_check;
    LONG quiet[LIST_COUNT];
}
list;

static Object *wheel;
static Object *r_slider;
static Object *g_slider;
static Object *b_slider;
static Object *hue_gauge;
static Object *colorfield, *colorfield2, *colorfield_reset_button,
    *colorfield_pen;
static Object *coloradjust;
static Object *pendisplay, *pendisplay2, *pendisplay_pen, *pendisplay_spec,
    *reference_check, *shine_button, *shadow_button, *yellow_button, *poppen;
static Object *group;
static Object *editor_text;
static Object *filename_string;
static Object *save_button;

#if defined(TEST_ICONLIST)
static Object *drawer_iconlist;
static Object *volume_iconlist;
#endif

AROS_UFH0(void, repeat_function)
{
    AROS_USERFUNC_INIT

    printf("MUI_Timer\n");

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, wheel_function)
{
    AROS_USERFUNC_INIT

    nnset(r_slider, MUIA_Numeric_Value, (XGET(wheel,
                WHEEL_Red) >> 24) & 0xff);
    nnset(g_slider, MUIA_Numeric_Value, (XGET(wheel,
                WHEEL_Green) >> 24) & 0xff);
    nnset(b_slider, MUIA_Numeric_Value, (XGET(wheel,
                WHEEL_Blue) >> 24) & 0xff);

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, slider_function)
{
    AROS_USERFUNC_INIT

    struct ColorWheelRGB cw;
    ULONG red = XGET(r_slider, MUIA_Numeric_Value);
    ULONG green = XGET(g_slider, MUIA_Numeric_Value);
    ULONG blue = XGET(b_slider, MUIA_Numeric_Value);

    cw.cw_Red = (red << 24) | (red << 16) | (red << 8) | red;
    cw.cw_Green = (green << 24) | (green << 16) | (green << 8) | green;
    cw.cw_Blue = (blue << 24) | (blue << 16) | (blue << 8) | blue;

    nnset(wheel, WHEEL_RGB, &cw);
    set(hue_gauge, MUIA_Gauge_Current, XGET(wheel, WHEEL_Hue));

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, objects_function)
{
    AROS_USERFUNC_INIT

    Object *new_obj = MUI_MakeObject(MUIO_Button, "Button");
    if (new_obj)
    {
        DoMethod(group, MUIM_Group_InitChange);
        DoMethod(group, OM_ADDMEMBER, new_obj);
        DoMethod(group, MUIM_Group_ExitChange);
    }

    AROS_USERFUNC_EXIT
}

static void About(void)
{
    static Object *about_wnd;

    if (!about_wnd)
    {
        about_wnd = AboutmuiObject, MUIA_Aboutmui_Application, app, End;
    }

    if (about_wnd)
        set(about_wnd, MUIA_Window_Open, TRUE);
}

static void ChangeStringAccept(void)
{
    STRPTR accept_chars = NULL, reject_chars = NULL;
    BOOL accept_all;
    UWORD i;

    accept_all = XGET(string.accept_all_check, MUIA_Selected);
    SET(string.accept_string, MUIA_Disabled, accept_all);
    SET(string.reject_string, MUIA_Disabled, accept_all);

    if (!accept_all)
    {
        GET(string.accept_string, MUIA_String_Contents, &accept_chars);
        GET(string.reject_string, MUIA_String_Contents, &reject_chars);
    }

    for (i = 0; i < STRING_COUNT; i++)
    {
        SET(string.strings[i], MUIA_String_Accept, accept_chars);
        SET(string.strings[i], MUIA_String_Reject, reject_chars);
    }
}

static void ChangePen(void)
{
    SIPTR pen = -1;

    GET(colorfield_pen, MUIA_String_Integer, &pen);
    if (pen >= 0)
        SET(colorfield, MUIA_Colorfield_Pen, pen);
}

static void ChangePendisplayPen(void)
{
    DoMethod(pendisplay, MUIM_Pendisplay_SetColormap,
        XGET(pendisplay_pen, MUIA_String_Integer));
}

static void ChangeListTitle(void)
{
    STRPTR title = NULL;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.lists[i], MUIA_List_Title, &title);
    FreeVec(title);

    GET(list.title_string, MUIA_String_Contents, &title);
    if (title[0] == '\0')
        title = NULL;

    title = StrDup(title);

    SET(list.lists[i], MUIA_List_Title, title);
}

static void UpdateListInfo(void)
{
    STRPTR title = NULL;
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.lists[i], MUIA_List_DragSortable, &value);
    NNSET(list.dragsortable_check, MUIA_Selected, value);
    GET(list.lists[i], MUIA_List_ShowDropMarks, &value);
    NNSET(list.showdropmarks_check, MUIA_Selected, value);
    value = list.quiet[i];    // MUIA_List_Quiet is not gettable!
    NNSET(list.quiet_check, MUIA_Selected, value);
    GET(list.lists[i], MUIA_List_AutoVisible, &value);
    NNSET(list.autovisible_check, MUIA_Selected, value);

    GET(list.lists[i], MUIA_List_Title, &title);
    NNSET(list.title_string, MUIA_String_Contents, title);

    GET(list.lists[i], MUIA_List_Entries, &value);
    DoMethod(list.entries_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
    GET(list.lists[i], MUIA_List_Visible, &value);
    DoMethod(list.visible_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
    GET(list.lists[i], MUIA_List_First, &value);
    DoMethod(list.first_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
    GET(list.lists[i], MUIA_List_InsertPosition, &value);
    DoMethod(list.insert_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
    GET(list.lists[i], MUIA_List_Active, &value);
    if (value == MUIV_List_Active_Off)
        SET(list.active_text, MUIA_Text_Contents, "N/A");
    else
        DoMethod(list.active_text, MUIM_SetAsString, MUIA_Text_Contents,
            "%ld", value);
}

static void ListGetVisible(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.lists[i], MUIA_List_Visible, &value);
    DoMethod(list.visible_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
}

static void ListGetFirst(void)
{
    UWORD i;
    LONG value = 0;
    Object *real_list;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    /* We fetch the Listview's list here to provide test coverage for that
       attribute. Please don't optimise */
    real_list = (Object *)XGET(list.lists[i], MUIA_Listview_List);
    GET(real_list, MUIA_List_First, &value);
    DoMethod(list.first_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
}

static void ListSetDragSortable(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.dragsortable_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_List_DragSortable, value);
}

static void ListSetShowDropMarks(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.showdropmarks_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_List_ShowDropMarks, value);
}

static void ListSetQuiet(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.quiet_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_List_Quiet, value);
    list.quiet[i] = value;
}

static void ListSetAutoVisible(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.autovisible_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_List_AutoVisible, value);
}

static void ListReset(void)
{
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    SET(list.lists[i], MUIA_List_Active, list_active_positions[i]);
}

static void ListMove(void)
{
    LONG mode, pos1, pos2;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.move1_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos1 = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos1 = 1 - mode;

    mode = XGET(list.move2_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos2 = XGET(list.index2_string, MUIA_String_Integer);
    else
        pos2 = 1 - mode;

    DoMethod(list.lists[i], MUIM_List_Move, pos1, pos2);
}

static void ListSort(void)
{
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    DoMethod(list.lists[i], MUIM_List_Sort);
}

static void ListEnable(void)
{
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    SET(list.lists[i], MUIA_Disabled, FALSE);
}

static void ListExchange(void)
{
    LONG mode, pos1, pos2;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.move1_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos1 = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos1 = 1 - mode;

    mode = XGET(list.move2_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos2 = XGET(list.index2_string, MUIA_String_Integer);
    else
        pos2 = 1 - mode;

    DoMethod(list.lists[i], MUIM_List_Exchange, pos1, pos2);
}

static void ListJump(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.jump_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = 1 - mode;

    DoMethod(list.lists[i], MUIM_List_Jump, pos);
}

static void ListSelect(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.select_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = 0 - mode;

    DoMethod(list.lists[i], MUIM_List_Select, pos, MUIV_List_Select_On,
        NULL);
}

static void ListDeselect(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.select_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = 0 - mode;

    DoMethod(list.lists[i], MUIM_List_Select, pos, MUIV_List_Select_Off,
        NULL);
}

static void ListToggle(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.select_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = 0 - mode;

    DoMethod(list.lists[i], MUIM_List_Select, pos,
        MUIV_List_Select_Toggle, NULL);
}

static void ListRedraw(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.select_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = 0 - mode;

    DoMethod(list.lists[i], MUIM_List_Redraw, pos);
}

static void ListInsertSingle(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.insert_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = 1 - mode;

    DoMethod(list.lists[i], MUIM_List_InsertSingle, "Tomato", pos);
}

static void ListInsert(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.insert_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = 1 - mode;

    DoMethod(list.lists[i], MUIM_List_Insert, fruits, -1, pos);
}

static void ListRemove(void)
{
    LONG mode, pos, count, j, *selections;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.remove_cycle, MUIA_Cycle_Active);

    if (mode == 5)
    {
        /* Remove selected entries in a loop to test MUIM_List_NextSelected.
           This doesn't work as expected in MUI or Zune */
        pos = MUIV_List_NextSelected_Start - 1;
        while (pos != MUIV_List_NextSelected_End)
        {
            if (pos == MUIV_List_NextSelected_Start - 1)
                pos++;
            DoMethod(list.lists[i], MUIM_List_NextSelected, (IPTR) &pos);
            if (pos != MUIV_List_NextSelected_End)
                DoMethod(list.lists[i], MUIM_List_Remove, pos);
        }
    }
    else if (mode == 6)
    {
        /* Remove selected entries safely by first retrieving them with
           MUIM_List_NextSelected and then removing them one by one */
        DoMethod(list.lists[i], MUIM_List_Select, MUIV_List_Select_All,
            MUIV_List_Select_Ask, &count);
        selections = AllocVec(sizeof(LONG) * count, MEMF_ANY);
        if (selections != NULL)
        {
            pos = MUIV_List_NextSelected_Start;
            for (j = 0; j < count; j++)
            {
                DoMethod(list.lists[i], MUIM_List_NextSelected, (IPTR) &pos);
                selections[j] = pos;
            }

            /* We must remove the entries in reverse order; otherwise the
               later indices will become invalid */
            while (count > 0)
                DoMethod(list.lists[i], MUIM_List_Remove, selections[--count]);
            FreeVec(selections);
        }
    }
    else
    {
        if (mode == 0)
            pos = XGET(list.index1_string, MUIA_String_Integer);
        else
            pos = 1 - mode;
        DoMethod(list.lists[i], MUIM_List_Remove, pos);
    }
}

static void ListClear(void)
{
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    DoMethod(list.lists[i], MUIM_List_Clear);
}

static void ListActivate(void)
{
    LONG mode, pos;
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    mode = XGET(list.activate_cycle, MUIA_Cycle_Active);

    if (mode == 0)
        pos = XGET(list.index1_string, MUIA_String_Integer);
    else
        pos = -1 - mode;

    SET(list.lists[i], MUIA_List_Active, pos);
}

static void ListDeactivate(void)
{
    UWORD i;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    SET(list.lists[i], MUIA_List_Active, MUIV_List_Active_Off);
}

static void CheckListDoubleClick(void)
{
    LONG value = -1;

    GET(list.multi_lists[1], MUIA_Listview_DoubleClick, &value);
    MUI_Request(app, wnd, 0, "Test", "OK",
        "MUIA_Listview_Doubleclick = %ld", value);
}

AROS_UFH3(static APTR, ListConstructHook,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(IPTR, n, A1))
{
    AROS_USERFUNC_INIT

    struct list_entry *entry;

    entry = AllocPooled(pool, sizeof(struct list_entry) + 40);

    if (entry != NULL)
    {
        entry->column1 = (char *)(entry + 1);
        entry->column2 = entry->column1 + 20;
        sprintf(entry->column1, "Entry%ld", (long)n);
        sprintf(entry->column2, "Col2: Entry%ld", (long)n);
    }

    return entry;

    AROS_USERFUNC_EXIT
}

AROS_UFH3(static void, ListDestructHook,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct list_entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct list_entry) + 40);
    printf("List item destroyed\n");

    AROS_USERFUNC_EXIT
}

AROS_UFH3(static void, display_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, strings, A2),
    AROS_UFHA(struct list_entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    static char buf[100];

    if (entry)
    {
        sprintf(buf, "%ld", (long)*(strings - 1));
        strings[0] = buf;
        strings[1] = entry->column1;
        strings[2] = entry->column2;
    }
    else
    {
        strings[0] = "Number";
        strings[1] = "Column 1";
        strings[2] = "Column 2";
    }

    AROS_USERFUNC_EXIT
}

static void Save(void)
{
    char *text = (char *)XGET(editor_text, MUIA_String_Contents);
    char *filename = (char *)XGET(filename_string, MUIA_String_Contents);
    BPTR fh;

    if (!strlen(filename))
        return;

    if ((fh = Open(filename, MODE_NEWFILE)))
    {
        Write(fh, text, strlen(text));
        Close(fh);
    }
}

#if defined(TEST_ICONLIST)
/* IconList callbacks */
static void volume_doubleclicked(void)
{
    char buf[200];
    struct IconList_Entry *ent = (void *)MUIV_IconList_NextIcon_Start;

    DoMethod(volume_iconlist, MUIM_IconList_NextIcon,
        MUIV_IconList_NextIcon_Selected, &ent);
    if ((IPTR) ent == MUIV_IconList_NextIcon_End)
        return;

    strcpy(buf, ent->label);
    strcat(buf, ":");
}

static void drawer_doubleclicked(void)
{
    struct IconList_Entry *ent = (void *)MUIV_IconList_NextIcon_Start;

    DoMethod(drawer_iconlist, MUIM_IconList_NextIcon,
        MUIV_IconList_NextIcon_Selected, &ent);
    if ((IPTR) ent == MUIV_IconList_NextIcon_End)
        return;
    set(drawer_iconlist, MUIA_IconDrawerList_Drawer,
        ent->ile_IconEntry->ie_IconNode.ln_Name);
}
#endif

static IPTR create_balance_column(void)
{
    return (IPTR) VGroup,
        Child, RectangleObject,
            GaugeFrame,
            End,
        Child, BalanceObject,
            End,
        Child, RectangleObject,
            GaugeFrame,
            End,
        Child, BalanceObject,
            End,
        Child, RectangleObject,
            GaugeFrame,
            End,
        Child, BalanceObject,
            End,
        Child, RectangleObject,
            GaugeFrame,
            End,
        Child, BalanceObject,
            End,
        Child, RectangleObject,
            GaugeFrame,
            End,
        End;
}

/* The custom class */

struct DropText_Data
{
    ULONG times;
};

#ifdef __MAXON__
#undef KeymapBase
struct Library *KeymapBase;
#endif

#ifndef __AROS__
__saveds __asm IPTR dispatcher(register __a0 struct IClass *cl,
    register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, dispatcher,
    AROS_UFHA(Class *, cl, A0),
    AROS_UFHA(Object *, obj, A2), AROS_UFHA(Msg, msg, A1))
#endif
{
    AROS_USERFUNC_INIT

    switch (msg->MethodID)
    {
    case MUIM_DragQuery:
        return MUIV_DragQuery_Accept;
    case MUIM_DragDrop:
        {
            struct DropText_Data *data =
                (struct DropText_Data *)INST_DATA(cl, obj);
            char buf[100];
            data->times++;
            sprintf(buf, "%ld times", (long)data->times);
            set(obj, MUIA_Text_Contents, buf);
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);

    AROS_USERFUNC_EXIT
}

static struct MUI_CustomClass *CL_DropText;

#define DropTextObject BOOPSIOBJMACRO_START(CL_DropText->mcc_Class)

/* Main prog */

static struct Hook hook_standard;

AROS_UFH3S(void, hook_func_standard,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2), AROS_UFHA(void **, funcptr, A1))
{
    AROS_USERFUNC_INIT

    void (*func) (ULONG *) = (void (*)(ULONG *))(*funcptr);

    if (func)
        func((ULONG *) funcptr + 1);

    AROS_USERFUNC_EXIT
}

int main(void)
{
    APTR pool;
    Object *second_wnd;
    Object *open_button;
    Object *about_button;
    Object *quit_button;
    Object *repeat_button;
    Object *objects_button;
    Object *about_item, *quit_item;
    Object *context_menu;
    Object *popobject, *listview;
    Object *numerics[NUMERIC_COUNT];
    Object *min_string, *max_string;
    Object *slider_button;
    Object *country_radio[2];
    CONST_STRPTR title;
    UWORD i;
    LONG value = 0;

    static char *pages[] =
        {"General", "Text", "Boopsi", "Color", "Edit", "List", "Gauges",
            "Numeric", "Radio",
#if defined(TEST_ICONLIST)
            "Icon List",
#endif
            "Balancing", NULL};
    static char *text_pages[] =
        {"Text", "String", NULL};
    static char *color_pages[] =
        {"Palette", "Colors", "Pens", NULL};
    static char *list_pages[] =
        {"Single Column", "Multicolumn", NULL};
    static char **radio_entries1 = pages;
    static char *radio_entries2[] =
        {"Paris", "Pataya", "London", "New York", "Reykjavik", NULL};

    static IPTR entries[] = {1, 2, 3, 4, 5, 6, (IPTR)NULL};

    struct Hook hook;
    struct Hook hook_wheel;
    struct Hook hook_slider;
    struct Hook hook_objects;
    struct Hook hook_construct, hook_destruct, hook_display;

    hook_standard.h_Entry = (HOOKFUNC) hook_func_standard;

    pool = CreatePool(MEMF_ANY, 4096, 4096);

    MUIMasterBase = (struct Library *)OpenLibrary("muimaster.library", 0);

    hook.h_Entry = (HOOKFUNC) repeat_function;
    hook_wheel.h_Entry = (HOOKFUNC) wheel_function;
    hook_slider.h_Entry = (HOOKFUNC) slider_function;
    hook_objects.h_Entry = (HOOKFUNC) objects_function;
    hook_construct.h_Entry = (HOOKFUNC) ListConstructHook;
    hook_destruct.h_Entry = (HOOKFUNC) ListDestructHook;
    hook_display.h_Entry = (HOOKFUNC) display_function;

    context_menu = MenustripObject,
        MUIA_Family_Child, MenuObject,
            MUIA_Menu_Title, "Menutest",
            MUIA_Family_Child, about_item = MenuitemObject,
                MUIA_Menuitem_Title, "First Test Entry", End,
            MUIA_Family_Child, quit_item = MenuitemObject,
                MUIA_Menuitem_Title, "Second Test Entry", End,
            End,
        End;

    /* should check the result in a real program! */
    CL_DropText = MUI_CreateCustomClass(NULL, MUIC_Text, NULL,
        sizeof(struct DropText_Data), dispatcher);
    ColorWheelBase = OpenLibrary("gadgets/colorwheel.gadget", 0);

    pendisplay = PendisplayObject, MUIA_Pendisplay_Spec, &default_penspec, End;

    title = StrDup("Fruits");
    list.lists[0] = ListviewObject,
        MUIA_Listview_List,
            ListObject,
            InputListFrame,
            MUIA_List_Title, title,
            MUIA_List_SourceArray, fruits,
            MUIA_List_Active, MUIV_List_Active_Top,
            MUIA_List_PoolThreshSize, 256,
            MUIA_List_DragSortable, TRUE,
            MUIA_ShortHelp, "Default scroller\nTop entry active",
            End,
        MUIA_Listview_MultiSelect,
            MUIV_Listview_MultiSelect_None,
        MUIA_Listview_DoubleClick, TRUE,
        MUIA_CycleChain, 1,
        End;
    list.lists[1] = ListviewObject,
        MUIA_Listview_List,
            ListObject,
            InputListFrame,
            MUIA_List_SourceArray, fruits,
            MUIA_List_Active, MUIV_List_Active_Bottom,
            MUIA_List_PoolPuddleSize, 512,
            MUIA_ShortHelp, "Left scroller\nBottom entry active",
            End,
        MUIA_Listview_ScrollerPos,
            MUIV_Listview_ScrollerPos_Left,
        MUIA_CycleChain, 1,
        End;
    list.lists[2] = ListviewObject,
        MUIA_Listview_List,
            ListObject,
            InputListFrame,
            MUIA_List_SourceArray, fruits,
            MUIA_List_Active, MUIV_List_Active_Off,
            MUIA_List_Pool, pool,
            MUIA_ShortHelp, "Right scroller\nNo active entry",
            End,
        MUIA_Listview_MultiSelect,
            MUIV_Listview_MultiSelect_Shifted,
        MUIA_Listview_ScrollerPos,
            MUIV_Listview_ScrollerPos_Right,
        MUIA_CycleChain, 1,
        End;
    list.lists[3] = ListviewObject,
        MUIA_Listview_List,
            ListObject,
            InputListFrame,
            MUIA_List_SourceArray, fruits,
            MUIA_List_Pool, NULL,
            MUIA_ShortHelp, "No scroller\nDefault active entry",
            End,
        MUIA_Listview_MultiSelect,
            MUIV_Listview_MultiSelect_Always,
        MUIA_Listview_ScrollerPos,
            MUIV_Listview_ScrollerPos_None,
        MUIA_CycleChain, 1,
        End;
    list.lists[4] = ListviewObject,
        MUIA_Listview_List,
            ListObject,
            ReadListFrame,
            MUIA_List_SourceArray, fruits,
            MUIA_List_MinLineHeight, 20,
            MUIA_ShortHelp, "Default scroller\nDefault active entry",
            End,
        MUIA_Listview_Input, FALSE,
        MUIA_CycleChain, 1,
        End;

    app = ApplicationObject,
        MUIA_Application_Menustrip, MenustripObject,
            MUIA_Family_Child, MenuObject,
                MUIA_Menu_Title, "Project",
                MUIA_Family_Child, about_item = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title, (SIPTR)-1,
                    End,
                MUIA_Family_Child, quit_item = MenuitemObject,
                    MUIA_Menuitem_Title, "Quit",
                    MUIA_Menuitem_Shortcut, "Q",
                    End,
                End,
            End,

        SubWindow, wnd = WindowObject,
            MUIA_Window_Title, "test",
            MUIA_Window_Activate, TRUE,

            WindowContents, VGroup,
                Child, RegisterGroup(pages),
                    //MUIA_Background, "5:SYS:Prefs/Presets/Backdrops/StuccoBlue.pic",

                    /* general */
                    Child, VGroup,
                        Child, HGroup,
                            GroupFrameT("A horizontal group"),
                            Child, ColGroup(2),
                                GroupFrameT("A column group"),
                                Child, repeat_button = TextObject,
                                    MUIA_CycleChain, 1,
                                    ButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_Text_PreParse, "\33c",
                                    MUIA_Text_Contents, "Repeat",
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    End,
                                Child, TextObject,
                                    MUIA_CycleChain, 1,
                                    ButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_Text_PreParse, "\33c",
                                    MUIA_Text_Contents, "Drag Me",
                                    MUIA_Draggable, TRUE,
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    End,
                                Child, open_button = TextObject,
                                    MUIA_CycleChain, 1,
                                    ButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_Text_PreParse, "\33c",
                                    MUIA_Text_Contents, "Open Window",
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    End,
                                Child, TextObject,
                                    MUIA_ContextMenu, context_menu,
                                    MUIA_CycleChain, 1,
                                    ButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_Text_PreParse, "\33c",
                                    MUIA_Text_Contents, "Press Right",
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    End,
                                Child, objects_button = TextObject,
                                    MUIA_CycleChain, 1,
                                    ButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_Text_PreParse, "\33c",
                                    MUIA_Text_Contents, "Add Objects",
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    End,
                                Child, HVSpace,
                                Child, HGroup,
                                    Child, HVSpace,
                                    Child, MUI_MakeObject(MUIO_Checkmark,
                                        "_Checkmark"),
                                    End,
                                Child, HGroup,
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "_Checkmark", 0),
                                    Child, HVSpace,
                                    End,
                                Child, HVSpace,
                                Child, HVSpace,
                                End,
                            Child, group = VGroup,
                                GroupFrameT("A vertical group"),
                                Child, DropTextObject,
                                    MUIA_Dropable, TRUE,
                                    MUIA_CycleChain, 1,
                                    ButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_Text_PreParse, "\33c",
                                    MUIA_Text_Contents, "Drop Here",
                                    MUIA_Dropable, TRUE,
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    End,
                                Child, TextObject,
                                    TextFrame,
                                    MUIA_Background, MUII_MARKHALFSHINE,
                                    MUIA_CycleChain, 1,
                                    MUIA_Text_PreParse, "\33c",
                                    MUIA_Text_Contents, "Pattern",
                                    End,
                                Child, MUI_NewObject(MUIC_Popimage, TAG_DONE),
                                End,
                            Child, ScrollgroupObject,
                                GroupFrameT("A scroll group"),
                                MUIA_Scrollgroup_Contents, VGroupV,
                                    VirtualFrame,
                                    Child, TextObject,
                                        TextFrame,
                                        MUIA_Text_Contents,
                                            "Line1\nLine2\nLine3\nLine4\n"
                                            "Line5\nLine6\nLine7\nLine8\n"
                                            "\n\n\n"
                                            "Line9\nLine10\nLine11\n",
                                        End,
                                    Child, HGroup,
                                        Child, MUI_MakeObject(MUIO_Button,
                                            "Button9"),
                                        Child, MUI_MakeObject(MUIO_Button,
                                            "Button10"),
                                        End,
                                    End,
                                End,
                            End,
                        Child, popobject = PopobjectObject,
                            MUIA_Popstring_String,
                                MUI_MakeObject(MUIO_String, NULL, 200),
                            MUIA_Popstring_Button, PopButton(MUII_PopUp),
                            MUIA_Popobject_Object, VGroup,
                                Child, TextObject,
                                    MUIA_Text_Contents, "test",
                                    End,
                                Child, listview = ListviewObject,
                                    MUIA_Listview_List, ListObject,
                                        InputListFrame,
                                        MUIA_List_ConstructHook,
                                            &hook_construct,
                                        MUIA_List_DestructHook,
                                            &hook_destruct,
                                        MUIA_List_DisplayHook, &hook_display,
                                        MUIA_List_Format, ",,",
                                        MUIA_List_SourceArray, entries,
                                        MUIA_List_Title, TRUE,
                                        End,
                                    End,
                                End,
                            End,
                        Child, CycleObject,
                            ButtonFrame,
                            MUIA_Cycle_Entries, pages,
                            End,
                        Child, ScrollbarObject,
                            MUIA_Group_Horiz, TRUE,
                            MUIA_Prop_Visible, 100,
                            MUIA_Prop_Entries, 300,
                            MUIA_Prop_First, 50,
                            End,
                        End,

                    /* text */
                    Child, RegisterGroup(text_pages),

                    Child, VGroup,
                        Child, TextObject,
                            MUIA_Background, "2:cfffffff,cfffffff,10000000",
                            TextFrame,
                            MUIA_Text_Contents, "\33cHello World!\n"
                                "This is a text object\n\33lLeft \33bbold\33n\n"
                                "\33rRight",
                            End,
                        Child, TextObject,
                            TextFrame,
                            /* Test for a bug with pen specifications as reported here:
                               https://sourceforge.net/p/aros/bugs/487/
                            */
                            MUIA_Text_Contents, "This is a \33P[3]text \33P[]object \33P[1]with pen specifications",
                            End,
                        Child, RectangleObject,
                            MUIA_VertWeight, 0,
                                /* Seems to be not supported properly as orginal MUI
                                   doesn't allow to alter the height of the window */
                            MUIA_Rectangle_HBar, TRUE,
                            MUIA_Rectangle_BarTitle,"Enter a string",
                            End,
                        Child, StringObject,
                            StringFrame,
                            MUIA_CycleChain,1,
                            MUIA_String_AdvanceOnCR, TRUE,
                            End,
                        Child, HVSpace,
                        End,

                        /* string */
                        Child, HGroup,
                            Child, VGroup,
                                Child, VGroup, GroupFrameT("Default Alignment"),
                                    Child, string.strings[0] = StringObject,
                                        StringFrame,
                                        MUIA_String_Accept,
                                            (IPTR)default_accept_chars,
                                        MUIA_String_Reject,
                                            (IPTR)default_reject_chars,
                                        MUIA_String_AdvanceOnCR, TRUE,
                                        MUIA_String_MaxLen, 9,
                                        MUIA_CycleChain, 1,
                                        MUIA_String_AttachedList, list.lists[0],
                                        End,
                                    End,
                                Child, VGroup, GroupFrameT("Left Aligned"),
                                    Child, string.strings[1] = StringObject,
                                        StringFrame,
                                        MUIA_String_Accept,
                                            (IPTR)default_accept_chars,
                                        MUIA_String_Reject,
                                            (IPTR)default_reject_chars,
                                        MUIA_String_Format,
                                            MUIV_String_Format_Left,
                                        MUIA_String_AdvanceOnCR, TRUE,
                                        MUIA_String_MaxLen, 9,
                                        MUIA_CycleChain, 1,
                                        MUIA_String_Contents,
                                            (IPTR)default_accept_chars,
                                        MUIA_String_BufferPos, 3,
                                        MUIA_String_AttachedList, list.lists[0],
                                        End,
                                    End,
                                Child, VGroup, GroupFrameT("Right Aligned"),
                                    Child, string.strings[2] = StringObject,
                                        StringFrame,
                                        MUIA_String_Accept,
                                            (IPTR)default_accept_chars,
                                        MUIA_String_Reject,
                                            (IPTR)default_reject_chars,
                                        MUIA_String_Format,
                                            MUIV_String_Format_Right,
                                        MUIA_String_AdvanceOnCR, TRUE,
                                        MUIA_String_MaxLen, 9,
                                        MUIA_CycleChain, 1,
                                        MUIA_String_AttachedList, list.lists[0],
                                        End,
                                    End,
                                Child, VGroup, GroupFrameT("Centered"),
                                    Child, string.strings[3] = StringObject,
                                        StringFrame,
                                        MUIA_String_Accept,
                                            (IPTR)default_accept_chars,
                                        MUIA_String_Reject,
                                            (IPTR)default_reject_chars,
                                        MUIA_String_Format,
                                            MUIV_String_Format_Center,
                                        MUIA_String_AdvanceOnCR, TRUE,
                                        MUIA_String_MaxLen, 9,
                                        MUIA_CycleChain, 1,
                                        MUIA_String_Integer, 123,
                                        MUIA_String_AttachedList, list.lists[0],
                                        End,
                                    End,
                                Child, VGroup, GroupFrameT("Secret"),
                                    Child, string.strings[4] = StringObject,
                                        StringFrame,
                                        MUIA_String_Accept,
                                            (IPTR)default_accept_chars,
                                        MUIA_String_Reject,
                                            (IPTR)default_reject_chars,
                                        MUIA_String_Format,
                                            MUIV_String_Format_Center,
                                        MUIA_String_Secret, TRUE,
                                        MUIA_String_AdvanceOnCR, TRUE,
                                        MUIA_String_MaxLen, 9,
                                        MUIA_CycleChain, 1,
                                        MUIA_String_AttachedList, list.lists[0],
                                        End,
                                    End,
                                Child, HGroup, GroupFrameT("Narrow"),
                                    Child, HVSpace,
                                    Child, string.strings[5] = StringObject,
                                        StringFrame,
                                        MUIA_String_Accept,
                                            (IPTR)default_accept_chars,
                                        MUIA_String_Reject,
                                            (IPTR)default_reject_chars,
                                        MUIA_String_Format,
                                            MUIV_String_Format_Center,
                                        MUIA_String_AdvanceOnCR, TRUE,
                                        MUIA_MaxWidth, 20,
                                        MUIA_String_MaxLen, 9,
                                        MUIA_CycleChain, 1,
                                        MUIA_String_AttachedList, list.lists[0],
                                        End,
                                    Child, HVSpace,
                                    End,
                                Child, HVSpace,
                                End,
                            Child, VGroup, GroupFrameT("Controls"),
                                Child, HGroup,
                                    Child, string.accept_all_check =
                                        MUI_MakeObject(MUIO_Checkmark, NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Accept all characters", 0),
                                    Child, HVSpace,
                                    End,
                                Child, ColGroup(2),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Acceptable characters:", 0),
                                    Child, string.accept_string = StringObject,
                                        StringFrame,
                                        MUIA_String_Contents,
                                            (IPTR)default_accept_chars,
                                        End,
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Unacceptable characters:", 0),
                                    Child, string.reject_string = StringObject,
                                        StringFrame,
                                        End,
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Cursor position:", 0),
                                    Child, string.cursor_pos_slider =
                                        SliderObject,
                                        MUIA_Group_Horiz, TRUE,
                                        MUIA_Numeric_Min, 0,
                                        End,
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Display position:", 0),
                                    Child, string.display_pos_slider =
                                        SliderObject,
                                        MUIA_Group_Horiz, TRUE,
                                        MUIA_Numeric_Min, 0,
                                        End,
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Integer value:", 0),
                                    Child, string.integer_string =
                                        StringObject,
                                        StringFrame,
                                        MUIA_String_Accept, (IPTR)digits,
                                        End,
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Plaintext:", 0),
                                    Child, string.plaintext_string =
                                        StringObject,
                                        StringFrame,
                                        End,
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Maximum string length:", 0),
                                    Child, string.max_len_text = TextObject,
                                        TextFrame,
                                        MUIA_Text_Contents, "Unknown",
                                        End,
                                    End,
                                Child, VGroup,
                                    Child, HGroup,
                                        Child, string.cr_advance_check =
                                            MUI_MakeObject(MUIO_Checkmark,
                                                NULL),
                                        Child, MUI_MakeObject(MUIO_Label,
                                            "Advance on CR", 0),
                                        Child, HVSpace,
                                        End,
                                    Child, HGroup,
                                        Child, string.attach_list_check =
                                            MUI_MakeObject(MUIO_Checkmark,
                                                NULL),
                                        Child, MUI_MakeObject(MUIO_Label,
                                            "Attach list", 0),
                                        Child, HVSpace,
                                        End,
#if 0
                                    Child, HGroup,
                                        Child, string.standard_hook_check =
                                            MUI_MakeObject(MUIO_Checkmark,
                                                NULL),
                                        Child, MUI_MakeObject(MUIO_Label,
                                            "Use standard edit hook", 0),
                                        Child, HVSpace,
                                        End,
                                    Child, HGroup,
                                        Child, string.custom_hook_check =
                                            MUI_MakeObject(MUIO_Checkmark,
                                                NULL),
                                        Child, MUI_MakeObject(MUIO_Label,
                                            "Use custom edit hook", 0),
                                        Child, HVSpace,
                                        End,
#endif
                                    End,
                                Child, HVSpace,
                                End,

                            End,
                        End,

                    /* boopsi */
                    Child, VGroup,
                        Child, wheel = BoopsiObject, /* MUI/Boopsi tags mixed */
                            GroupFrame,
                            /* boopsi objects don't know */
                            /* their sizes, so we help */
                            /* keep important values */
                            /* during window resize */
                            MUIA_Boopsi_ClassID, "colorwheel.gadget",
                            MUIA_Boopsi_MinWidth , 30, 
                            MUIA_Boopsi_MinHeight, 30,
                            MUIA_Boopsi_Remember , WHEEL_Saturation,
                            MUIA_Boopsi_Remember , WHEEL_Hue,
                            /* this magic fills in the screen pointer */
                            MUIA_Boopsi_TagScreen, WHEEL_Screen,
                            WHEEL_Screen, NULL,
                            GA_Left, 0,
                            GA_Top, 0, /* MUI will automatically */
                            GA_Width, 0, /* fill in the correct values */
                            GA_Height, 0,
                            ICA_TARGET, ICTARGET_IDCMP,
                                /* needed for notification */
                            WHEEL_Saturation, 0, /* start in the center */
                            MUIA_FillArea, TRUE,
                                /* use this because it defaults to FALSE
                                   for boopsi gadgets but the colorwheel
                                   doesn't bother about redrawing its
                                   background */
                            End,

                        Child, r_slider = SliderObject,
                            MUIA_Group_Horiz, TRUE,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 255,
                            End,
                        Child, g_slider = SliderObject,
                            MUIA_Group_Horiz, TRUE,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 255,
                            End,
                        Child, b_slider = SliderObject,
                            MUIA_Group_Horiz, TRUE,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 255,
                            End,
                        Child, hue_gauge = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Max, 255,
                            MUIA_Gauge_Divide, 1<<24,
                            MUIA_Gauge_InfoText, "Hue: %ld",
                            End,
                        End,

                    /* color */
                    Child, RegisterGroup(color_pages),
                        Child, VGroup, GroupFrameT("Palette"),
                            Child, PaletteObject,
                                End,
                            End,
                        Child, HGroup,
                            Child, VGroup, GroupFrameT("Colorfield"),

                                Child, HGroup,
                                    Child, colorfield = ColorfieldObject,
                                        MUIA_Colorfield_RGB, default_color,
                                        MUIA_Colorfield_Pen, 253,
                                        End,
                                    Child, colorfield2 = ColorfieldObject,
                                        MUIA_Colorfield_Red, default_color[0],
                                        MUIA_Colorfield_Green, default_color[1],
                                        MUIA_Colorfield_Blue, default_color[2],
                                        End,
                                    End,
                                Child, colorfield_reset_button =
                                    MUI_MakeObject(MUIO_Button,
                                        "Reset"),
                                Child, HGroup,
                                    Child,
                                        MUI_MakeObject(MUIO_Label, "Pen:", 0),
                                    Child, colorfield_pen = StringObject,
                                        StringFrame,
                                        End,
                                    End,
                                End,
                            Child, VGroup, GroupFrameT("Coloradjust"),
                                Child, coloradjust = ColoradjustObject,
                                    MUIA_Coloradjust_RGB, default_color,
                                    End,
                                End,
                            End,
                        Child, HGroup,
                            Child, VGroup,
                                Child, VGroup, GroupFrameT("Pendisplay"),
                                    Child, HGroup,
                                        Child, pendisplay,
                                        Child, pendisplay2 = PendisplayObject,
                                            MUIA_Pendisplay_RGBcolor,
                                                default_color,
                                            End,
                                        End,
                                    Child, ColGroup(2),
                                        Child,
                                            MUI_MakeObject(
                                                MUIO_Label, "Pen:", 0),
                                        Child, HGroup,
                                            Child, pendisplay_pen =
                                                StringObject, StringFrame,
                                                End,
                                            Child, HVSpace,
                                            Child, reference_check =
                                                MUI_MakeObject(MUIO_Checkmark,
                                                    NULL),
                                            Child, MUI_MakeObject(MUIO_Label,
                                                "Reference", 0),
                                            End,
                                        Child,
                                            MUI_MakeObject(
                                                MUIO_Label, "Penspec:", 0),
                                        Child, pendisplay_spec = StringObject,
                                            StringFrame,
                                            End,
                                        End,
                                    Child, HGroup,
                                        Child, shine_button =
                                            MUI_MakeObject(MUIO_Button,
                                                "Shine"),
                                        Child, shadow_button =
                                            MUI_MakeObject(MUIO_Button,
                                                "Shadow"),
                                        Child, yellow_button =
                                            MUI_MakeObject(MUIO_Button,
                                                "Yellow"),
                                        End,
                                    End,
                                Child, VGroup, GroupFrameT("Poppen"),
                                    Child, poppen = PoppenObject,
                                        End,
                                    End,
                                End,
                            Child, VGroup, GroupFrameT("Penadjust"),
                                Child, PenadjustObject,
                                    End,
                                End,
                            End,
                        End,

                    /* edit */
                    Child, VGroup,
                        Child, editor_text = StringObject,
                            StringFrame,
                            End,
                        Child, PopaslObject,
                            ASLFR_DoSaveMode, TRUE,
                            MUIA_Popstring_String, filename_string =
                                MUI_MakeObject(MUIO_String, NULL, 200),
                            MUIA_Popstring_Button, PopButton(MUII_PopFile),
                            End,
                        Child, save_button =
                            MUI_MakeObject(MUIO_Button, "Save"),
                        End,

                    /* lists */
                    Child, RegisterGroup(list_pages),
                        Child, VGroup,
                            Child, ColGroup(LIST_COUNT),
                                Child, VGroup, GroupFrameT("No Multiselect"),
                                    Child, list.lists[0],
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Default Multiselect"),
                                    Child, list.lists[1],
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Shifted Multiselect"),
                                    Child, list.lists[2],
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Unconditional Multiselect"),
                                    Child, list.lists[3],
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Read Only"),
                                    Child, list.lists[4],
                                    End,
                                End,
                            Child, HGroup,
                                MUIA_Group_HorizSpacing, 0,
                                Child, RectangleObject,
                                    MUIA_HorizWeight, 1,
                                    End,
                                Child, list.list_radios = RadioObject,
                                    MUIA_Radio_Entries, empty,
                                    MUIA_Group_Horiz, TRUE,
                                    MUIA_HorizWeight, 1000,
                                    End,
                                Child, RectangleObject,
                                    MUIA_HorizWeight, 1,
                                    End,
                                End,
                            Child, RectangleObject,
                                MUIA_VertWeight, 0,
                                MUIA_Rectangle_HBar, TRUE,
                                MUIA_Rectangle_BarTitle, "List Controls",
                                End,
                            Child, ColGroup(6),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Affected index 1:", 0),
                                Child, list.index1_string =
                                    StringObject,
                                    StringFrame,
                                    MUIA_String_Accept, (IPTR)digits,
                                    MUIA_String_Integer, 0,
                                    End,
                                Child, HGroup,
                                    Child, list.dragsortable_check =
                                        MUI_MakeObject(MUIO_Checkmark, NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Drag sortable", 0),
                                    Child, HVSpace,
                                    End,
                                Child, HGroup,
                                    Child, list.showdropmarks_check =
                                        MUI_MakeObject(MUIO_Checkmark, NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Show drop marks", 0),
                                    Child, HVSpace,
                                    End,
                                Child, HVSpace,
                                Child, HVSpace,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Affected index 2:", 0),
                                Child, list.index2_string =
                                    StringObject,
                                    StringFrame,
                                    MUIA_String_Accept, (IPTR)digits,
                                    MUIA_String_Integer, 0,
                                    End,
                                Child, HGroup,
                                    Child, list.quiet_check =
                                        MUI_MakeObject(MUIO_Checkmark,NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Quiet", 0),
                                    Child, HVSpace,
                                    End,
                                Child, HGroup,
                                    Child, list.autovisible_check =
                                        MUI_MakeObject(MUIO_Checkmark,NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Auto visible", 0),
                                    Child, HVSpace,
                                    End,
                                Child, list.reset_button =
                                    MUI_MakeObject(MUIO_Button, "Reset"),
                                Child, HVSpace,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Move/exchange mode 1:", 0),
                                Child, list.move1_cycle = CycleObject,
                                    ButtonFrame,
                                    MUIA_Cycle_Entries, list_move1_modes,
                                    End,
                                Child, list.move_button =
                                    MUI_MakeObject(MUIO_Button, "Move"),
                                Child, list.sort_button =
                                    MUI_MakeObject(MUIO_Button, "Sort"),
                                Child, list.enable_button =
                                    MUI_MakeObject(MUIO_Button, "Enable"),
                                Child, HVSpace,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Move/exchange mode 2:", 0),
                                Child, list.move2_cycle = CycleObject,
                                    ButtonFrame,
                                    MUIA_Cycle_Entries, list_move2_modes,
                                    End,
                                Child, list.exchange_button =
                                    MUI_MakeObject(MUIO_Button, "Exchange"),
                                Child, list.redraw_button =
                                    MUI_MakeObject(MUIO_Button, "Redraw"),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Title:", 0),
                                Child, list.title_string =
                                    StringObject,
                                    StringFrame,
                                    End,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Jump mode:", 0),
                                Child, list.jump_cycle = CycleObject,
                                    ButtonFrame,
                                    MUIA_Cycle_Entries, list_jump_modes,
                                    End,
                                Child, list.jump_button =
                                    MUI_MakeObject(MUIO_Button, "Jump"),
                                Child, list.toggle_button =
                                    MUI_MakeObject(MUIO_Button, "Toggle"),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Entries:", 0),
                                Child, list.entries_text = TextObject,
                                    TextFrame,
                                    MUIA_Text_Contents, "N/A",
                                    End,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Select/redraw mode:", 0),
                                Child, list.select_cycle = CycleObject,
                                    ButtonFrame,
                                    MUIA_Cycle_Entries, list_select_modes,
                                    End,
                                Child, list.select_button =
                                    MUI_MakeObject(MUIO_Button, "Select"),
                                Child, list.deselect_button =
                                    MUI_MakeObject(MUIO_Button, "Deselect"),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Visible entries:", 0),
                                Child, list.visible_text = TextObject,
                                    TextFrame,
                                    MUIA_Text_Contents, "N/A",
                                    End,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Insert mode:", 0),
                                Child, list.insert_cycle = CycleObject,
                                    ButtonFrame,
                                    MUIA_Cycle_Entries, list_insert_modes,
                                    End,
                                Child, list.insert_single_button =
                                    MUI_MakeObject(MUIO_Button, "Insert Single"),
                                Child, list.insert_multiple_button =
                                    MUI_MakeObject(MUIO_Button, "Insert Multiple"),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "First visible index:", 0),
                                Child, list.first_text = TextObject,
                                    TextFrame,
                                    MUIA_Text_Contents, "N/A",
                                    End,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Remove mode:", 0),
                                Child, list.remove_cycle = CycleObject,
                                    ButtonFrame,
                                    MUIA_Cycle_Entries, list_remove_modes,
                                    End,
                                Child, list.remove_button =
                                    MUI_MakeObject(MUIO_Button, "Remove"),
                                Child, list.clear_button =
                                    MUI_MakeObject(MUIO_Button, "Clear"),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Last insertion index:", 0),
                                Child, list.insert_text = TextObject,
                                    TextFrame,
                                    MUIA_Text_Contents, "N/A",
                                    End,

                                Child, MUI_MakeObject(MUIO_Label,
                                    "Activate mode:", 0),
                                Child, list.activate_cycle = CycleObject,
                                    ButtonFrame,
                                    MUIA_Cycle_Entries, list_activate_modes,
                                    End,
                                Child, list.activate_button =
                                    MUI_MakeObject(MUIO_Button, "Activate"),
                                Child, list.deactivate_button =
                                    MUI_MakeObject(MUIO_Button, "Deactivate"),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Active index:", 0),
                                Child, list.active_text = TextObject,
                                    TextFrame,
                                    MUIA_Text_Contents, "N/A",
                                    End,
                                End,
                            End,
                        Child, VGroup,
                            Child, ColGroup(LIST_COUNT / 2),
                                Child, VGroup, GroupFrameT("Standard Format"),
                                    Child, list.multi_lists[0] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_ConstructHook,
                                                &hook_construct,
                                            MUIA_List_DestructHook,
                                                &hook_destruct,
                                            MUIA_List_DisplayHook,
                                                &hook_display,
                                            MUIA_List_Format, list_format,
                                            MUIA_List_SourceArray, entries,
                                            MUIA_List_Title, TRUE,
                                            MUIA_List_AdjustWidth, TRUE,
                                            End,
                                        MUIA_Listview_MultiSelect,
                                            MUIV_Listview_MultiSelect_None,
                                        MUIA_Listview_DefClickColumn, 1,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                Child, VGroup, GroupFrameT("Custom Format"),
                                    Child, list.multi_lists[1] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_ConstructHook,
                                                &hook_construct,
                                            MUIA_List_DestructHook,
                                                &hook_destruct,
                                            MUIA_List_DisplayHook,
                                                &hook_display,
                                            MUIA_List_Format, list_format,
                                            MUIA_List_SourceArray, entries,
                                            MUIA_List_Title, TRUE,
                                            MUIA_List_AdjustHeight, TRUE,
                                            End,
                                        MUIA_Listview_MultiSelect,
                                            MUIV_Listview_MultiSelect_None,
                                        MUIA_CycleChain, 1,
                                        End,
                                    Child, HVSpace,
                                    End,
                                End,
                            Child, RectangleObject,
                                MUIA_VertWeight, 0,
                                MUIA_Rectangle_HBar, TRUE,
                                MUIA_Rectangle_BarTitle, "List Controls",
                                End,
                            Child, HGroup,
                                Child, MUI_MakeObject(MUIO_Label, "Format:", 0),
                                Child, list.format_string = StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    End,
                                Child, list.showheadings_check =
                                    MUI_MakeObject(MUIO_Checkmark, NULL),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Show column headings", 0),
                                End,
                            Child, HGroup,
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Default clicked column:", 0),
                                Child, list.def_column_string = StringObject,
                                    StringFrame,
                                    MUIA_String_Accept, (IPTR)digits,
                                    MUIA_String_Integer, -1,
                                    MUIA_CycleChain, 1,
                                    End,
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Clicked column:", 0),
                                Child, list.column_string = StringObject,
                                    MUIA_ShowMe, FALSE,
                                    End,
                                Child, list.column_text = TextObject,
                                    TextFrame,
                                    MUIA_Text_Contents, "N/A",
                                    End,
                                End,
                            End,
                        End,

                    /* gauges */
                    Child, HGroup,
                        Child, VGroup,
                            Child, VGroup,
                                GroupFrame,
                                Child, GaugeObject,
                                    GaugeFrame,
                                    MUIA_Gauge_InfoText, "%ld %%",
                                    MUIA_Gauge_Horiz, TRUE,
                                    MUIA_Gauge_Current, 25,
                                    End,
                                Child, ScaleObject,
                                    End,
                                End,
                            Child, VGroup,
                                GroupFrame,
                                Child, GaugeObject,
                                    GaugeFrame,
                                    MUIA_Gauge_InfoText, "%ld %%",
                                    MUIA_Gauge_Horiz, TRUE,
                                    MUIA_Gauge_Current, 50,
                                    End,
                                Child, ScaleObject,
                                    End,
                                End,
                            Child, VGroup,
                                GroupFrame,
                                Child, GaugeObject,
                                    GaugeFrame,
                                    MUIA_Gauge_InfoText, "%ld %%",
                                    MUIA_Gauge_Horiz, TRUE,
                                    MUIA_Gauge_Current, 75,
                                    End,
                                Child, ScaleObject,
                                    End,
                                End,
                            End,
                        Child, HGroup,
                            Child, HVSpace,
                            Child, GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_InfoText, "%ld %%",
                                MUIA_Gauge_Current, 25,
                                End,
                            Child, GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_InfoText, "%ld %%",
                                MUIA_Gauge_Current, 50,
                                End,
                            Child, GaugeObject,
                                GaugeFrame,
                                MUIA_Gauge_InfoText, "%ld %%",
                                MUIA_Gauge_Current, 75,
                                End,
                            Child, HVSpace,
                            End,
                        End,

                    /* numeric */
                    Child, HGroup,
                        Child, ColGroup(2),
                            GroupFrameT("Horizontal Sliders"),
                            Child, MUI_MakeObject(MUIO_Label, "Normal:", 0),
                            Child, numerics[HNSLIDER] = SliderObject,
                                MUIA_Slider_Horiz, TRUE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            Child, MUI_MakeObject(MUIO_Label, "Reverse:", 0),
                            Child, numerics[HRSLIDER] = SliderObject,
                                MUIA_Numeric_Reverse, TRUE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            Child, MUI_MakeObject(MUIO_Label, "Quiet:", 0),
                            Child, numerics[HQSLIDER] = SliderObject,
                                MUIA_Slider_Quiet, TRUE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            End,
                        Child, ColGroup(3),
                            GroupFrameT("Vertical Sliders"),
                            Child, MUI_MakeObject(MUIO_Label, "Normal", 0),
                            Child, MUI_MakeObject(MUIO_Label, "Reverse", 0),
                            Child, MUI_MakeObject(MUIO_Label, "Quiet", 0),
                            Child, numerics[VNSLIDER] = SliderObject,
                                MUIA_Slider_Horiz, FALSE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            Child, numerics[VRSLIDER] = SliderObject,
                                MUIA_Slider_Horiz, FALSE,
                                MUIA_Numeric_Reverse, TRUE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            Child, numerics[VQSLIDER] = SliderObject,
                                MUIA_Slider_Horiz, FALSE,
                                MUIA_Slider_Quiet, TRUE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            End,
                        Child, ColGroup(2),
                            GroupFrameT("Numeric Buttons"),
                            Child, MUI_MakeObject(MUIO_Label, "Normal:", 0),
                            Child, numerics[NNUMERICBUTTON] =
                                NumericbuttonObject,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            Child, MUI_MakeObject(MUIO_Label, "Reverse:", 0),
                            Child, numerics[RNUMERICBUTTON] =
                                NumericbuttonObject,
                                MUIA_Numeric_Reverse, TRUE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            End,
                        Child, ColGroup(2),
                            GroupFrameT("Knobs"),
                            Child, MUI_MakeObject(MUIO_Label, "Normal", 0),
                            Child, MUI_MakeObject(MUIO_Label, "Reverse", 0),
                            Child, numerics[NKNOB] = KnobObject,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            Child, numerics[RKNOB] = KnobObject,
                                MUIA_Numeric_Reverse, TRUE,
                                MUIA_Numeric_Min, NUMERIC_MIN,
                                MUIA_Numeric_Max, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            End,
                        Child, ColGroup(2),
                            Child, MUI_MakeObject(MUIO_Label,
                                "Minimum Value:", 0),
                            Child, min_string = (Object *)StringObject,
                                StringFrame,
                                MUIA_String_Accept, (IPTR)digits,
                                MUIA_String_Integer, NUMERIC_MIN,
                                MUIA_CycleChain, 1,
                                End,
                            Child, MUI_MakeObject(MUIO_Label,
                                "Maximum Value:", 0),
                            Child, max_string = (Object *)StringObject,
                                StringFrame,
                                MUIA_String_Accept, (IPTR)digits,
                                MUIA_String_Integer, NUMERIC_MAX,
                                MUIA_CycleChain, 1,
                                End,
                            Child, slider_button = TextObject,
                                ButtonFrame,
                                MUIA_InputMode, MUIV_InputMode_RelVerify,
                                MUIA_CycleChain, 1,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_Text_PreParse, "\33c",
                                MUIA_Text_Contents,
                                    "Change Slider Orientations",
                                End,
                            Child, MUI_MakeObject(MUIO_Label, "", 0),
                            End,
                        End,

                    /* radios */
                    Child, HGroup,
                        Child, VGroup, 
                            Child, RadioObject,
                            GroupFrame,
                            MUIA_Radio_Entries, radio_entries1,
                            End,
                        Child, country_radio[0] = RadioObject,
                            GroupFrame,
                            MUIA_Radio_Entries, radio_entries2,
                            MUIA_Radio_Active, 1,
                            End,
                        End,
                    Child, HGroup,
                        Child, RadioObject,
                            GroupFrame,
                            MUIA_Radio_Entries, radio_entries1,
                            End,
                            Child, country_radio[1] = RadioObject,
                                GroupFrame,
                                MUIA_Radio_Entries, radio_entries2,
                                MUIA_Radio_Active, 1,
                                End,
                            End,
                        End,

#if defined(TEST_ICONLIST)
                    /* iconlist */
                    Child, HGroup,
                        Child, volume_iconlist =
                            MUI_NewObject(MUIC_IconVolumeList, GroupFrame,
                                TAG_DONE),
                        Child, drawer_iconlist =
                            MUI_NewObject(MUIC_IconDrawerList, GroupFrame,
                                MUIA_IconDrawerList_Drawer, "SYS:",
                                TAG_DONE),
                        End,
#endif

                    /* balancing */
                    Child, HGroup,
                        Child, create_balance_column(),
                        Child, BalanceObject,
                            End,

                        Child, create_balance_column(),
                        Child, BalanceObject,
                            End,

                        Child, create_balance_column(),
                        Child, BalanceObject,
                            End,

                        Child, create_balance_column(),
                        Child, BalanceObject,
                            End,

                        Child, create_balance_column(),

                        End,

                    End,

                Child, HGroup,
                    Child, about_button = TextObject,
                        ButtonFrame,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_CycleChain, 1,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, "About",
                        End,
                    Child, quit_button = TextObject,
                        ButtonFrame,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_CycleChain, 1,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_ControlChar, 'q',
                        MUIA_Text_HiChar, 'q',
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, "Quit",
                        End,
                    End,
                End,
            End,

        SubWindow, second_wnd = WindowObject,
            MUIA_Window_Title, "Second window",
            WindowContents, VGroup,
                Child, DropTextObject,
                    MUIA_Text_Contents, "Drop here", 
                    MUIA_Dropable, TRUE,
                    End,
                End,
            End,

        End;

    if (app)
    {
        ULONG sigs = 0;

        DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        DoMethod(second_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            second_wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);
        DoMethod(open_button, MUIM_Notify, MUIA_Pressed, FALSE, second_wnd,
            3, MUIM_Set, MUIA_Window_Open, TRUE);
        DoMethod(about_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2,
            MUIM_Application_AboutMUI, NULL);
        DoMethod(quit_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        DoMethod(objects_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2,
            MUIM_CallHook, &hook_objects);
        DoMethod(repeat_button, MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
            app, 2, MUIM_CallHook, &hook);

        /* Notifications and set-up for string objects */
        DoMethod(string.max_len_text, MUIM_SetAsString, MUIA_Text_Contents,
            "%ld", XGET(string.strings[0], MUIA_String_MaxLen) - 1);
        set(string.cursor_pos_slider, MUIA_Numeric_Max,
            XGET(string.strings[0], MUIA_String_MaxLen) - 1);
        set(string.display_pos_slider, MUIA_Numeric_Max,
            XGET(string.strings[0], MUIA_String_MaxLen) - 1);
        set(string.cr_advance_check, MUIA_Selected, TRUE);
        set(string.attach_list_check, MUIA_Selected, TRUE);
        set(string.strings[1], MUIA_Disabled,
            XGET(string.strings[1], MUIA_String_Format)
            != MUIV_String_Format_Left);
        set(string.strings[2], MUIA_Disabled,
            XGET(string.strings[2], MUIA_String_Format)
            != MUIV_String_Format_Right);
        set(string.strings[3], MUIA_Disabled,
            XGET(string.strings[3], MUIA_String_Format)
            != MUIV_String_Format_Center);
        set(string.strings[4], MUIA_Disabled,
            !XGET(string.strings[4], MUIA_String_Secret));
        set(string.strings[0], MUIA_String_MaxLen, 100);
        set(string.reject_string, MUIA_String_Contents,
            (IPTR)default_reject_chars);

        for (i = 0; i < STRING_COUNT; i++)
        {
            if (i == 0)
            {
                DoMethod(string.accept_string, MUIM_Notify,
                    MUIA_String_Contents, MUIV_EveryTime, string.strings[i],
                    3, MUIM_Set, MUIA_String_Accept, MUIV_TriggerValue);
                DoMethod(string.cr_advance_check, MUIM_Notify, MUIA_Selected,
                    MUIV_EveryTime, string.strings[i], 3, MUIM_Set,
                    MUIA_String_AdvanceOnCR, MUIV_TriggerValue);
                DoMethod(string.cursor_pos_slider, MUIM_Notify,
                    MUIA_Numeric_Value, MUIV_EveryTime, string.strings[i], 3,
                    MUIM_Set, MUIA_String_BufferPos, MUIV_TriggerValue);
                DoMethod(string.display_pos_slider, MUIM_Notify,
                    MUIA_Numeric_Value, MUIV_EveryTime, string.strings[i], 3,
                    MUIM_Set, MUIA_String_DisplayPos, MUIV_TriggerValue);
                DoMethod(string.attach_list_check, MUIM_Notify, MUIA_Selected,
                    TRUE, string.strings[i], 3, MUIM_Set,
                    MUIA_String_AttachedList, list.lists[0]);
                DoMethod(string.attach_list_check, MUIM_Notify, MUIA_Selected,
                    FALSE, string.strings[i], 3, MUIM_Set,
                    MUIA_String_AttachedList, NULL);
            }
            else
            {
                DoMethod(string.strings[0], MUIM_Notify, MUIA_String_Accept,
                    MUIV_EveryTime, string.strings[i], 3, MUIM_Set,
                    MUIA_String_Accept, MUIV_TriggerValue);
                DoMethod(string.strings[0], MUIM_Notify,
                    MUIA_String_AdvanceOnCR, MUIV_EveryTime,
                    string.strings[i], 3, MUIM_Set, MUIA_String_AdvanceOnCR,
                    MUIV_TriggerValue);
                DoMethod(string.strings[0], MUIM_Notify,
                    MUIA_String_BufferPos, MUIV_EveryTime,
                    string.strings[i], 3, MUIM_Set, MUIA_String_BufferPos,
                    MUIV_TriggerValue);
                DoMethod(string.strings[0], MUIM_Notify,
                    MUIA_String_DisplayPos, MUIV_EveryTime,
                    string.strings[i], 3, MUIM_Set, MUIA_String_DisplayPos,
                    MUIV_TriggerValue);
                DoMethod(string.strings[0], MUIM_Notify,
                    MUIA_String_AttachedList, MUIV_EveryTime,
                    string.strings[i], 3, MUIM_Set,
                    MUIA_String_AttachedList, MUIV_TriggerValue);
            }
            DoMethod(string.reject_string, MUIM_Notify, MUIA_String_Contents,
                MUIV_EveryTime, string.strings[i], 3, MUIM_Set,
                MUIA_String_Reject, MUIV_TriggerValue);
            DoMethod(string.strings[i], MUIM_Notify, MUIA_String_Integer,
                MUIV_EveryTime, string.integer_string, 3, MUIM_NoNotifySet,
                MUIA_String_Integer, MUIV_TriggerValue);
            DoMethod(string.integer_string, MUIM_Notify, MUIA_String_Integer,
                MUIV_EveryTime, string.strings[i], 3, MUIM_NoNotifySet,
                MUIA_String_Integer, MUIV_TriggerValue);
        }
        DoMethod(string.accept_all_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ChangeStringAccept);
        DoMethod(string.strings[4], MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, string.plaintext_string, 3, MUIM_NoNotifySet,
            MUIA_String_Contents, MUIV_TriggerValue);
        DoMethod(string.plaintext_string, MUIM_Notify,
            MUIA_String_Acknowledge, MUIV_EveryTime, string.strings[4], 3,
            MUIM_NoNotifySet, MUIA_String_Contents, MUIV_TriggerValue);

        DoMethod(wheel, MUIM_Notify, WHEEL_Hue, MUIV_EveryTime, app, 2,
            MUIM_CallHook, &hook_wheel);
        DoMethod(wheel, MUIM_Notify, WHEEL_Saturation, MUIV_EveryTime, app,
            2, MUIM_CallHook, &hook_wheel);
        DoMethod(r_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            app, 2, MUIM_CallHook, &hook_slider);
        DoMethod(g_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            app, 2, MUIM_CallHook, &hook_slider);
        DoMethod(b_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
            app, 2, MUIM_CallHook, &hook_slider);

        DoMethod(save_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3,
            MUIM_CallHook, &hook_standard, Save);

        DoMethod(quit_item, MUIM_Notify, MUIA_Menuitem_Trigger,
            MUIV_EveryTime, app, 2, MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit);
        DoMethod(about_item, MUIM_Notify, MUIA_Menuitem_Trigger,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            About);

        /* Notifications for color objects */
        DoMethod(colorfield, MUIM_Notify, MUIA_Colorfield_Red,
            MUIV_EveryTime, colorfield2, 3, MUIM_Set, MUIA_Colorfield_Red,
            MUIV_TriggerValue);
        DoMethod(colorfield, MUIM_Notify, MUIA_Colorfield_Green,
            MUIV_EveryTime, colorfield2, 3, MUIM_Set, MUIA_Colorfield_Green,
            MUIV_TriggerValue);
        DoMethod(colorfield, MUIM_Notify, MUIA_Colorfield_Blue,
            MUIV_EveryTime, colorfield2, 3, MUIM_Set, MUIA_Colorfield_Blue,
            MUIV_TriggerValue);
        DoMethod(coloradjust, MUIM_Notify, MUIA_Coloradjust_Red,
            MUIV_EveryTime, colorfield, 3, MUIM_Set, MUIA_Colorfield_Red,
            MUIV_TriggerValue);
        DoMethod(coloradjust, MUIM_Notify, MUIA_Coloradjust_Green,
            MUIV_EveryTime, colorfield, 3, MUIM_Set, MUIA_Colorfield_Green,
            MUIV_TriggerValue);
        DoMethod(coloradjust, MUIM_Notify, MUIA_Coloradjust_Blue,
            MUIV_EveryTime, colorfield, 3, MUIM_Set, MUIA_Colorfield_Blue,
            MUIV_TriggerValue);
        DoMethod(colorfield, MUIM_Notify, MUIA_Colorfield_Pen,
            MUIV_EveryTime, colorfield_pen, 3, MUIM_Set,
            MUIA_String_Integer, MUIV_TriggerValue);
        DoMethod(colorfield_pen, MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ChangePen);
        DoMethod(colorfield_reset_button, MUIM_Notify, MUIA_Pressed, FALSE,
            colorfield, 3, MUIM_Set, MUIA_Colorfield_RGB, default_color);

        /* Notifications for pen objects */
        DoMethod(pendisplay, MUIM_Notify, MUIA_Pendisplay_Pen,
            MUIV_EveryTime, pendisplay_pen, 3, MUIM_NoNotifySet,
            MUIA_String_Integer, MUIV_TriggerValue);
        DoMethod(pendisplay_pen, MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ChangePendisplayPen);
        DoMethod(pendisplay, MUIM_Notify, MUIA_Pendisplay_Spec,
            MUIV_EveryTime, pendisplay_spec, 3, MUIM_Set,
            MUIA_String_Contents, MUIV_TriggerValue);
        DoMethod(colorfield, MUIM_Notify, MUIA_Colorfield_RGB,
            MUIV_EveryTime, pendisplay, 3, MUIM_Set,
            MUIA_Pendisplay_RGBcolor, MUIV_TriggerValue);
        DoMethod(pendisplay, MUIM_Notify, MUIA_Pendisplay_RGBcolor,
            MUIV_EveryTime, poppen, 3, MUIM_Set, MUIA_Pendisplay_RGBcolor,
            MUIV_TriggerValue);
        DoMethod(pendisplay_spec, MUIM_Notify, MUIA_String_Contents,
            MUIV_EveryTime, pendisplay, 3, MUIM_Set, MUIA_Pendisplay_Spec,
            MUIV_TriggerValue);
        DoMethod(reference_check, MUIM_Notify, MUIA_Selected, TRUE,
            pendisplay2, 3, MUIM_Set, MUIA_Pendisplay_Reference, pendisplay);
        DoMethod(reference_check, MUIM_Notify, MUIA_Selected, FALSE,
            pendisplay2, 3, MUIM_Set, MUIA_Pendisplay_Reference, NULL);
        DoMethod(shine_button, MUIM_Notify, MUIA_Pressed, FALSE, pendisplay,
            2, MUIM_Pendisplay_SetMUIPen, MPEN_SHINE);
        DoMethod(shadow_button, MUIM_Notify, MUIA_Pressed, FALSE, pendisplay,
            2, MUIM_Pendisplay_SetMUIPen, MPEN_SHADOW);
        DoMethod(yellow_button, MUIM_Notify, MUIA_Pressed, FALSE, pendisplay,
            4, MUIM_Pendisplay_SetRGB, 0xffffffff, 0xffffffff, 0);

        /* Notifications and set-up for list objects */
        set(list.showdropmarks_check, MUIA_Selected, TRUE);
        UpdateListInfo();
        for (i = 0; i < LIST_COUNT; i++)
        {
            DoMethod(list.lists[i], MUIM_Notify, MUIA_Listview_DoubleClick,
                MUIV_EveryTime, list.lists[i], 3, MUIM_Set, MUIA_Disabled,
                TRUE);
            DoMethod(list.lists[i], MUIM_Notify, MUIA_List_Entries,
                MUIV_EveryTime, list.entries_text, 4, MUIM_SetAsString,
                MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
            DoMethod(list.lists[i], MUIM_Notify, MUIA_List_Visible,
                MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
                ListGetVisible);
            DoMethod(list.lists[i], MUIM_Notify, MUIA_List_First,
                MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
                ListGetFirst);
            DoMethod(list.lists[i], MUIM_Notify, MUIA_List_InsertPosition,
                MUIV_EveryTime, list.insert_text, 4, MUIM_SetAsString,
                MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
            DoMethod(list.lists[i], MUIM_Notify, MUIA_List_Active,
                MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
                UpdateListInfo);
        }
        DoMethod(list.dragsortable_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetDragSortable);
        DoMethod(list.showdropmarks_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetShowDropMarks);
        DoMethod(list.quiet_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetQuiet);
        DoMethod(list.autovisible_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetAutoVisible);
        DoMethod(list.reset_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListReset);
        DoMethod(list.move_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListMove);
        DoMethod(list.sort_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListSort);
        DoMethod(list.enable_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListEnable);
        DoMethod(list.exchange_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListExchange);
        DoMethod(list.jump_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListJump);
        DoMethod(list.select_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListSelect);
        DoMethod(list.deselect_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListDeselect);
        DoMethod(list.toggle_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListToggle);
        DoMethod(list.redraw_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListRedraw);
        DoMethod(list.insert_single_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListInsertSingle);
        DoMethod(list.insert_multiple_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListInsert);
        DoMethod(list.remove_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListRemove);
        DoMethod(list.clear_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListClear);
        DoMethod(list.activate_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListActivate);
        DoMethod(list.deactivate_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, ListDeactivate);
        DoMethod(list.title_string, MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ChangeListTitle);
        DoMethod(list.list_radios, MUIM_Notify, MUIA_Radio_Active,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            UpdateListInfo);

        SET(list.showheadings_check, MUIA_Selected,
            XGET(list.multi_lists[0], MUIA_List_Title));
        SET(list.format_string, MUIA_String_Contents,
            XGET(list.multi_lists[1], MUIA_List_Format));
        SET(list.def_column_string, MUIA_String_Integer,
            XGET(list.multi_lists[0], MUIA_Listview_DefClickColumn));
        DoMethod(list.format_string, MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, list.multi_lists[1], 3, MUIM_Set,
            MUIA_List_Format, MUIV_TriggerValue);
        DoMethod(list.multi_lists[1], MUIM_Notify, MUIA_List_Format,
            MUIV_EveryTime, wnd, 3, MUIM_Set,
            MUIA_Window_ActiveObject, list.format_string);
        for (i = 0; i < MULTI_LIST_COUNT; i++)
        {
            DoMethod(list.showheadings_check, MUIM_Notify, MUIA_Selected,
                MUIV_EveryTime, list.multi_lists[i], 3, MUIM_Set,
                MUIA_List_Title, MUIV_TriggerValue);
            DoMethod(list.def_column_string, MUIM_Notify,
                MUIA_String_Integer,
                MUIV_EveryTime, list.multi_lists[i], 3, MUIM_Set,
                MUIA_Listview_DefClickColumn, MUIV_TriggerValue);
            DoMethod(list.multi_lists[i], MUIM_Notify,
                MUIA_Listview_ClickColumn, MUIV_EveryTime,
                list.column_string, 3, MUIM_Set, MUIA_String_Integer,
                MUIV_TriggerValue);
        }
        DoMethod(list.column_string, MUIM_Notify, MUIA_String_Contents,
            MUIV_EveryTime, list.column_text, 3, MUIM_Set, MUIA_Text_Contents,
            MUIV_TriggerValue);
        DoMethod(list.multi_lists[1], MUIM_Notify, MUIA_Listview_DoubleClick,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            CheckListDoubleClick);

        DoMethod(listview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            popobject, 2, MUIM_Popstring_Close, TRUE);

        /* numeric */
        for (i = 0; i < NUMERIC_COUNT; i++)
        {
            DoMethod(min_string, MUIM_Notify, MUIA_String_Integer,
                MUIV_EveryTime, numerics[i], 3, MUIM_Set, MUIA_Numeric_Min,
                MUIV_TriggerValue);
            DoMethod(max_string, MUIM_Notify, MUIA_String_Integer,
                MUIV_EveryTime, numerics[i], 3, MUIM_Set, MUIA_Numeric_Max,
                MUIV_TriggerValue);
        }
        DoMethod(slider_button, MUIM_Notify, MUIA_Pressed, FALSE,
            numerics[HRSLIDER], 3, MUIM_Set, MUIA_Slider_Horiz, FALSE);

        /* radio */
        DoMethod(country_radio[0], MUIM_Notify, MUIA_Radio_Active,
            MUIV_EveryTime, country_radio[1], 3, MUIM_NoNotifySet,
            MUIA_Radio_Active, MUIV_TriggerValue);
        DoMethod(country_radio[1], MUIM_Notify, MUIA_Radio_Active,
            MUIV_EveryTime, country_radio[0], 3, MUIM_NoNotifySet,
            MUIA_Radio_Active, MUIV_TriggerValue);

#if defined(TEST_ICONLIST)
        /* iconlist */
        DoMethod(volume_iconlist, MUIM_Notify, MUIA_IconList_DoubleClick,
            TRUE, volume_iconlist, 3, MUIM_CallHook, &hook_standard,
            volume_doubleclicked);
        DoMethod(drawer_iconlist, MUIM_Notify, MUIA_IconList_DoubleClick,
            TRUE, drawer_iconlist, 3, MUIM_CallHook, &hook_standard,
            drawer_doubleclicked);
#endif

        /* automatic tests */
        get(list.lists[0], MUIA_List_Visible, &value);
        if (value != -1)
            printf("MUIA_List_Visible equals %ld before display,"
                " but it should be -1.\n", (long)value);
        get(list.multi_lists[0], MUIA_Listview_ClickColumn, &value);
        if (value != 0)
            printf("MUIA_Listview_ClickColumn equals %ld before display,"
                " but it should be 0.\n", (long)value);
        get(list.lists[0], MUIA_Listview_DoubleClick, &value);
        if (value != 0)
            printf("MUIA_Listview_DoubleClick equals %ld before display,"
                " but it should be 0.\n", (long)value);

        set(wnd, MUIA_Window_Open, TRUE);
        set(wnd, MUIA_Window_ScreenTitle, "Zune Test application");

        /* Set pen fields */
        set(colorfield_pen, MUIA_String_Integer,
            XGET(colorfield, MUIA_Colorfield_Pen));

        char pen_str[10];
        nnset(pendisplay_pen, MUIA_String_Integer,
            XGET(pendisplay, MUIA_Pendisplay_Pen));

        struct MUI_PenSpec *pen_spec = NULL;
        GET(pendisplay, MUIA_Pendisplay_Spec, &pen_spec);
        strncpy(pen_str, pen_spec->buf, 10);
        set(pendisplay_spec, MUIA_String_Contents, pen_str);

        while ((LONG) DoMethod(app, MUIM_Application_NewInput,
                &sigs) != MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
                if (sigs & SIGBREAKF_CTRL_C)
                    break;
                if (sigs & SIGBREAKF_CTRL_D)
                    break;
            }
        }

        MUI_DisposeObject(app);
    }
    if (context_menu)
        MUI_DisposeObject(context_menu);
    CloseLibrary(ColorWheelBase);
    MUI_DeleteCustomClass(CL_DropText);

    CloseLibrary(MUIMasterBase);

    DeletePool(pool);

    return 0;
}
