/*
    Copyright © 2002-2019, The AROS Development Team.
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
#include <proto/utility.h>
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

#define TEXT_RAW_IMAGE_SIZE 16
#define STRING_COUNT 6
#define LIST_COUNT 5
#define MULTI_LIST_COUNT 2

#define NUMERIC_MIN 0
#define NUMERIC_MAX 100
#define NUMERIC_DIV 2
#define NUMERIC_INIT 30

static const char *pages[] =
    {"General", "Text", "Boopsi", "Color", "Edit", "List", "Numeric", "Select",
#if defined(TEST_ICONLIST)
        "Icon List",
#endif
        "Balancing", NULL};
static const char *general_pages[] =
    {"Group", "Window", NULL};
static const char *text_pages[] =
    {"Text", "String", NULL};
static const char *color_pages[] =
    {"Palette", "Colors", "Pens", NULL};
static const char *list_pages[] =
    {"Single Column", "Multicolumn", NULL};
static const IPTR entries[] = {1, 2, 3, 4, 5, 6, (IPTR)NULL};
static const CONST_STRPTR window_left_modes[] =
    {"No change", "Pixels", "Centered", "Under pointer", NULL};
static const CONST_STRPTR window_top_modes[] =
    {"No change", "Pixels", "Centered", "Under pointer",
    "Pixels below screen bar", NULL};
static const CONST_STRPTR window_dim_modes[] =
    {"Default", "Pixels", "Growth percentage", "Total screen percentage",
    "Visible screen percentage", "Scaled", NULL};
static const CONST_STRPTR window_menu_modes[] =
    {"Parent", "Custom", "None", NULL};
static const TEXT digits[] = "-0123456789";
static const TEXT vowels[] = "aeiou";
static const TEXT default_accept_chars[] = "aeiou?.";
static const TEXT default_reject_chars[] = "*?";
static const ULONG default_color[] = {155 << 24, 180 << 24, 255 << 24};
static const struct MUI_PenSpec default_penspec = {"m0"};
static const char *fruits[] = {"Strawberry", "Apple", "Banana", "Orange",
    "Grapefruit", "Kumquat", "Plum", "Raspberry", "Apricot", "Grape",
    "Peach", "Lemon", "Lime", "Date", "Pineapple", "Blueberry", "Papaya",
    "Cranberry", "Gooseberry", "Pear", "Fig", "Coconut", "Melon",
    "Pumpkin", NULL};
static char *cities[] =
    {"Paris", "Pataya", "London", "New York", "Reykjavik", "Tokyo", NULL};
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

static Object *CreateGeneralGroup();
static Object *CreateTextGroup();
static Object *CreateBoopsiGroup();
static Object *CreateColorGroup();
static Object *CreateEditGroup();
static Object *CreateListGroup();
static Object *CreateNumericGroup();
static Object *CreateSelectGroup();
static Object *CreateBalancingGroup();

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
    NLEVELMETER,
    RLEVELMETER,
    NUMERIC_COUNT
};

enum
{
    HNGAUGE,
    HQGAUGE,
    VNGAUGE,
    VQGAUGE,
    GAUGE_COUNT
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
    Object *open_button,
        *menus_check;
}
general;

static struct
{
    Object *window,
        *title_string,
        *screen_string,
        *left_cycle,
        *left_index_string,
        *width_cycle,
        *width_index_string,
        *top_cycle,
        *top_index_string,
        *height_cycle,
        *height_index_string,
        *alt_left_cycle,
        *alt_left_index_string,
        *alt_width_cycle,
        *alt_width_index_string,
        *alt_top_cycle,
        *alt_top_index_string,
        *alt_height_cycle,
        *alt_height_index_string,
        *appwindow_check,
        *backdrop_check,
        *borderless_check,
        *close_check,
        *depth_check,
        *ref_check,
        *dragbar_check,
        *size_check,
        *activate_check,
        *id_check,
        *menu_cycle,
        *open_button,
        *close_button,
        *apply_button,
        *close_item,
        *left_text,
        *top_text,
        *width_text,
        *height_text,
        *alt_left_text,
        *alt_top_text,
        *alt_width_text,
        *alt_height_text;
}
window;

static struct
{
    STRPTR image_text;
}
text;

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
        *draggable_check,
        *showdropmarks_check,
        *multitest_check,
        *quiet_check,
        *dragsortable_check,
        *autovisible_check,
        *entries_text,
        *visible_text,
        *first_text,
        *insert_text,
        *active_text,
        *drop_text,
        *selected_text,
        *multi_lists[MULTI_LIST_COUNT],
        *colorfield,
        *format_string,
        *column_string,
        *column_text,
        *def_column_string,
        *showheadings_check,
        *showimage_check;
    LONG quiet[LIST_COUNT],
        destruct_count,
        has_multitest[LIST_COUNT];
    IPTR image;
}
list;

static struct
{
    Object *numerics[NUMERIC_COUNT],
        *gauges[GAUGE_COUNT],
        *min_string,
        *max_string,
        *slider_button;
}
numeric;

static struct
{
    Object *city_cycle,
        *prev_button,
        *next_button,
        *city_text;
}
cycle;

static Object *repeat_button;
static Object *objects_button;
static Object *context_menu;
static Object *popobject, *listview;
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
static struct Hook hook_standard;
static struct Hook hook;
static struct Hook hook_wheel;
static struct Hook hook_slider;
static struct Hook hook_objects;
static struct Hook hook_compare, hook_multitest;
static struct Hook hook_construct, hook_destruct, hook_display;
static struct Hook hook_objstr;
static Object *city_radios[3];

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

/* Get the test window position and dimension values input by the user */
static void GetWindowDimensions(LONG *left, LONG *top, LONG *width,
    LONG *height, LONG *alt_left, LONG *alt_top, LONG *alt_width,
    LONG *alt_height)
{
    LONG mode;

    /* Get left edge */
    mode = XGET(window.left_cycle, MUIA_Cycle_Active);
    if (mode == 0)
        *left = XGET(window.left_index_string, MUIA_String_Integer);
    else
        *left = 0 - mode;

    /* Get top edge */
    mode = XGET(window.top_cycle, MUIA_Cycle_Active);
    if (mode == 0)
        *top = XGET(window.top_index_string, MUIA_String_Integer);
    else if (mode == 3)
        *top = MUIV_Window_TopEdge_Delta(
            XGET(window.top_index_string, MUIA_String_Integer));
    else
        *top = 0 - mode;

    /* Get width */
    mode = XGET(window.width_cycle, MUIA_Cycle_Active);
    *width = XGET(window.width_index_string, MUIA_String_Integer);
    if (mode != 1 && *width > 100)
        *width = 100;
    switch (mode)
    {
    case 0:
        *width = MUIV_Window_Width_Default;
        break;
    case 2:
        *width = MUIV_Window_Width_MinMax(*width);
        break;
    case 3:
        *width = MUIV_Window_Width_Screen(*width);
        break;
    case 4:
        *width = MUIV_Window_Width_Visible(*width);
        break;
    case 5:
        *width = MUIV_Window_Width_Scaled;
        break;
    }

    /* Get height */
    mode = XGET(window.height_cycle, MUIA_Cycle_Active);
    *height = XGET(window.height_index_string, MUIA_String_Integer);
    if (mode != 1 && *height > 100)
        *height = 100;
    switch (mode)
    {
    case 0:
        *height = MUIV_Window_Height_Default;
        break;
    case 2:
        *height = MUIV_Window_Height_MinMax(*height);
        break;
    case 3:
        *height = MUIV_Window_Height_Screen(*height);
        break;
    case 4:
        *height = MUIV_Window_Height_Visible(*height);
        break;
    case 5:
        *height = MUIV_Window_Height_Scaled;
    }

    /* Get alternative left edge */
    mode = XGET(window.alt_left_cycle, MUIA_Cycle_Active);
    if (mode == 0)
        *alt_left = MUIV_Window_AltLeftEdge_NoChange;
    else if (mode == 1)
        *alt_left = XGET(window.alt_left_index_string, MUIA_String_Integer);
    else
        *alt_left = 1 - mode;

    /* Get alternative top edge */
    mode = XGET(window.alt_top_cycle, MUIA_Cycle_Active);
    if (mode == 0)
        *alt_top = MUIV_Window_AltTopEdge_NoChange;
    else if (mode == 1)
        *alt_top = XGET(window.alt_top_index_string, MUIA_String_Integer);
    else if (mode == 3)
        *alt_top = MUIV_Window_AltTopEdge_Delta(
            XGET(window.alt_top_index_string, MUIA_String_Integer));
    else
        *alt_top = 1 - mode;

    /* Get alternative width */
    mode = XGET(window.alt_width_cycle, MUIA_Cycle_Active);
    *alt_width = XGET(window.alt_width_index_string, MUIA_String_Integer);
    if (mode != 0 && *alt_width > 100)
        *alt_width = 100;
    switch (mode)
    {
    case 1:
        *alt_width = MUIV_Window_AltWidth_MinMax(*alt_width);
        break;
    case 2:
        *alt_width = MUIV_Window_AltWidth_Screen(*alt_width);
        break;
    case 3:
        *alt_width = MUIV_Window_AltWidth_Visible(*alt_width);
        break;
    case 4:
        *alt_width = MUIV_Window_AltWidth_Scaled;
    }

    /* Get alternative height */
    mode = XGET(window.alt_height_cycle, MUIA_Cycle_Active);
    *alt_height = XGET(window.alt_height_index_string, MUIA_String_Integer);
    if (mode != 0 && *alt_height > 100)
        *alt_height = 100;
    switch (mode)
    {
    case 1:
        *alt_height = MUIV_Window_AltHeight_MinMax(*alt_height);
        break;
    case 2:
        *alt_height = MUIV_Window_AltHeight_Screen(*alt_height);
        break;
    case 3:
        *alt_height = MUIV_Window_AltHeight_Visible(*alt_height);
        break;
    case 4:
        *alt_height = MUIV_Window_AltHeight_Scaled;
    }
}

/* Refresh the current position and dimension information shown in the test
   window */
static void WindowUpdate(void)
{
    DoMethod(window.left_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_LeftEdge));
    DoMethod(window.width_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_Width));
    DoMethod(window.top_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_TopEdge));
    DoMethod(window.height_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_Height));

    DoMethod(window.alt_left_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_AltLeftEdge));
    DoMethod(window.alt_width_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_AltWidth));
    DoMethod(window.alt_top_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_AltTopEdge));
    DoMethod(window.alt_height_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", XGET(window.window, MUIA_Window_AltHeight));
}

/* Close and reopen the test window. Refresh its info while it's closed */
static void WindowReopen(void)
{
    SET(window.window, MUIA_Window_Open, FALSE);
    WindowUpdate();
    SET(window.window, MUIA_Window_Open, TRUE);
}

/* Close the test window */
static void WindowClose(void)
{
    SET(window.open_button, MUIA_Disabled, FALSE);
    SET(window.close_button, MUIA_Disabled, TRUE);
    SET(window.apply_button, MUIA_Disabled, TRUE);
    SET(window.window, MUIA_Window_Open, FALSE);

    DoMethod(app, OM_REMMEMBER, window.window);
    DoMethod(app, MUIM_Application_PushMethod, window.window, 1, OM_DISPOSE);
    window.window = NULL;
    window.close_item = NULL;
}

/* Open a new test window */
static void WindowOpen(void)
{
    LONG mode, left, top, width, height, alt_left, alt_top, alt_width,
        alt_height;
    Object *menus = NULL, *update_button, *reopen_button, *close_button;
    BOOL no_menus = FALSE;
    STRPTR title = NULL, screen_title = NULL;

    /* Get window and screen titles */
    GET(window.title_string, MUIA_String_Contents, &title);
    if (title[0] == '\0')
        title = NULL;
    else
        title = StrDup(title);

    GET(window.screen_string, MUIA_String_Contents, &screen_title);
    if (screen_title[0] == '\0')
        screen_title = NULL;
    else
        screen_title = StrDup(screen_title);

    GetWindowDimensions(&left, &top, &width, &height, &alt_left, &alt_top,
        &alt_width, &alt_height);

    /* Create menus */
    window.close_item = NULL;
    mode = XGET(window.menu_cycle, MUIA_Cycle_Active);
    if (mode == 1)
    {
        menus = MenustripObject,
            MUIA_Family_Child, MenuObject,
                MUIA_Menu_Title, "Test",
                MUIA_Family_Child, window.close_item = MenuitemObject,
                    MUIA_Menuitem_Title, "Close", End,
                End,
            End;
    }
    else if (mode == 2)
        no_menus = TRUE;

    /* Create window object */
    window.window = WindowObject,
        XGET(window.id_check, MUIA_Selected) ? MUIA_Window_ID : TAG_IGNORE,
            MAKE_ID('T','S','T','2'),
        MUIA_Window_Title, title,
        MUIA_Window_ScreenTitle, screen_title,
        MUIA_Window_LeftEdge, left,
        MUIA_Window_TopEdge, top,
        MUIA_Window_Width, width,
        MUIA_Window_Height, height,
        MUIA_Window_AltLeftEdge, alt_left,
        MUIA_Window_AltTopEdge, alt_top,
        MUIA_Window_AltWidth, alt_width,
        MUIA_Window_AltHeight, alt_height,
        MUIA_Window_Backdrop,
            XGET(window.backdrop_check, MUIA_Selected),
        MUIA_Window_Borderless,
            XGET(window.borderless_check, MUIA_Selected),
        MUIA_Window_CloseGadget,
            XGET(window.close_check, MUIA_Selected),
        MUIA_Window_DepthGadget,
            XGET(window.depth_check, MUIA_Selected),
        MUIA_Window_RefWindow,
            XGET(window.ref_check, MUIA_Selected) ? wnd : NULL,
        MUIA_Window_DragBar,
            XGET(window.dragbar_check, MUIA_Selected),
        MUIA_Window_SizeGadget,
            XGET(window.size_check, MUIA_Selected),
        MUIA_Window_Activate,
            XGET(window.activate_check, MUIA_Selected),
        MUIA_Window_Menustrip, menus,
        MUIA_Window_NoMenus, no_menus,
        WindowContents, VGroup,
            Child, ColGroup(4),
                GroupFrameT("Current dimensions"),
                Child, MUI_MakeObject(MUIO_Label, "Left Edge:", 0),
                Child, window.left_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Width:", 0),
                Child, window.width_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Top Edge:", 0),
                Child, window.top_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Height:", 0),
                Child, window.height_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                End,
            Child, ColGroup(4),
                GroupFrameT("Alternative dimensions"),
                Child, MUI_MakeObject(MUIO_Label, "Left Edge:", 0),
                Child, window.alt_left_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Width:", 0),
                Child, window.alt_width_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Top Edge:", 0),
                Child, window.alt_top_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Height:", 0),
                Child, window.alt_height_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                End,
            Child, HVSpace,
            Child, update_button =
                MUI_MakeObject(MUIO_Button, "Update"),
            Child, reopen_button =
                MUI_MakeObject(MUIO_Button, "Reopen"),
            Child, close_button =
                MUI_MakeObject(MUIO_Button, "Close"),
            End,
        End;

    if (window.window != NULL)
    {
        DoMethod(app, OM_ADDMEMBER, window.window);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            app, 3, MUIM_CallHook, &hook_standard, WindowClose);
        DoMethod(close_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, WindowClose);
        if (window.close_item != NULL)
            DoMethod(window.close_item, MUIM_Notify, MUIA_Menuitem_Trigger,
                MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
                WindowClose);

        /* Update dimensions when they change */
        DoMethod(window.window, MUIM_Notify, MUIA_Window_LeftEdge,
            MUIV_EveryTime, window.left_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_Width,
            MUIV_EveryTime, window.width_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_TopEdge,
            MUIV_EveryTime, window.top_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_Height,
            MUIV_EveryTime, window.height_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_AltLeftEdge,
            MUIV_EveryTime, window.alt_left_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_AltWidth,
            MUIV_EveryTime, window.alt_width_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_AltTopEdge,
            MUIV_EveryTime, window.alt_top_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);
        DoMethod(window.window, MUIM_Notify, MUIA_Window_AltHeight,
            MUIV_EveryTime, window.alt_height_text, 4, MUIM_SetAsString,
            MUIA_Text_Contents, "%ld", MUIV_TriggerValue);

        /* Update dimensions when asked */
        DoMethod(update_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, WindowUpdate);

        /* Close and reopen window when asked */
        DoMethod(reopen_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, WindowReopen);

        /* Get initial dimensions before window is opened */
        WindowUpdate();

        SET(window.window, MUIA_Window_Open, TRUE);
        SET(window.open_button, MUIA_Disabled, TRUE);
        SET(window.close_button, MUIA_Disabled, FALSE);
        SET(window.apply_button, MUIA_Disabled, FALSE);

    }

    /* Test getting title attributes */
    GET(window.window, MUIA_Window_Title, &title);
    if (title != NULL)
       SET(window.title_string, MUIA_String_Contents, title);
    GET(window.window, MUIA_Window_ScreenTitle, &screen_title);
    if (screen_title != NULL)
       SET(window.screen_string, MUIA_String_Contents, screen_title);
}

/* Apply new specifications to the current test window */
static void WindowApply(void)
{
    LONG left, top, width, height, alt_left, alt_top, alt_width,
        alt_height;
    STRPTR title = (APTR)~0;

    /* Update window and screen titles */
    GET(window.window, MUIA_Window_Title, &title);
    FreeVec(title);
    GET(window.title_string, MUIA_String_Contents, &title);
    if (title[0] == '\0')
        title = NULL;
    else
        title = StrDup(title);
    SET(window.window, MUIA_Window_Title, title);

    GET(window.window, MUIA_Window_ScreenTitle, &title);
    FreeVec(title);
    GET(window.screen_string, MUIA_String_Contents, &title);
    if (title[0] == '\0')
        title = NULL;
    else
        title = StrDup(title);
    SET(window.window, MUIA_Window_ScreenTitle, title);

    GetWindowDimensions(&left, &top, &width, &height, &alt_left, &alt_top,
        &alt_width, &alt_height);

    SET(window.window, MUIA_Window_LeftEdge, left);
    SET(window.window, MUIA_Window_Width, width);
    SET(window.window, MUIA_Window_TopEdge, top);
    SET(window.window, MUIA_Window_Height, height);

    SET(window.window, MUIA_Window_AltLeftEdge, alt_left);
    SET(window.window, MUIA_Window_AltWidth, alt_width);
    SET(window.window, MUIA_Window_AltTopEdge, alt_top);
    SET(window.window, MUIA_Window_AltHeight, alt_height);
}

AROS_UFH3(static void, ObjStrHook,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, group, A2),
    AROS_UFHA(Object *, str, A1))
{
    AROS_USERFUNC_INIT

    struct List *child_list = NULL;
    Object *child, *list;
    struct list_entry *entry;

    /* Find the listview object within its group */
    GET(group, MUIA_Group_ChildList, &child_list);
    child = (APTR)child_list->lh_Head;
    NextObject(&child);
    list = NextObject(&child);

    /* Copy one of the selected entry's fields to the string gadget */
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);
    SET(str, MUIA_String_Contents, entry->column1);

    AROS_USERFUNC_EXIT
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

AROS_UFH3(static IPTR, ListCompareHook,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(CONST_STRPTR, str1, A2),
    AROS_UFHA(CONST_STRPTR, str2, A1))
{
    AROS_USERFUNC_INIT

    IPTR len1, len2;

    len1 = strlen(str1);
    len2 = strlen(str2);

    /* Indicate which string is shorter */
    return len2 - len1;

    AROS_USERFUNC_EXIT
}

AROS_UFH3(static IPTR, ListMultiTestHook,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(APTR, unused, A2),
    AROS_UFHA(CONST_STRPTR, str, A1))
{
    AROS_USERFUNC_INIT

    /* Indicate whether the string doesn't begin with a vowel */
    return strchr(vowels, tolower(str[0])) == NULL;

    AROS_USERFUNC_EXIT
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

    GET(list.lists[i], MUIA_Listview_DragType, &value);
    NNSET(list.draggable_check, MUIA_Selected,
        value != MUIV_Listview_DragType_None);
    NNSET(list.dragsortable_check, MUIA_Disabled,
        value == MUIV_Listview_DragType_None);
    if (value == MUIV_Listview_DragType_None)
        value = FALSE;
    else
        GET(list.lists[i], MUIA_List_DragSortable, &value);
    NNSET(list.dragsortable_check, MUIA_Selected, value);
    GET(list.lists[i], MUIA_List_ShowDropMarks, &value);
    NNSET(list.showdropmarks_check, MUIA_Selected, value);
    value = list.has_multitest[i];    // MUIA_List_MultiTestHook isn't gettable!
    NNSET(list.multitest_check, MUIA_Selected, value);
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
    DoMethod(list.lists[i], MUIM_List_Select, MUIV_List_Select_All,
        MUIV_List_Select_Ask, (IPTR) &value);
    DoMethod(list.selected_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
    GET(list.lists[i], MUIA_List_DropMark, &value);
    DoMethod(list.drop_text, MUIM_SetAsString, MUIA_Text_Contents,
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

static void ListGetSelected(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    DoMethod(list.lists[i], MUIM_List_Select, MUIV_List_Select_All,
        MUIV_List_Select_Ask, (IPTR) &value);
    DoMethod(list.selected_text, MUIM_SetAsString, MUIA_Text_Contents,
        "%ld", value);
}

static void ListSetDraggable(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.draggable_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_Listview_DragType, value ?
        MUIV_Listview_DragType_Immediate : MUIV_Listview_DragType_None);
}

static void ListSetShowDropMarks(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.showdropmarks_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_List_ShowDropMarks, value);
}

static void ListSetMultiTest(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.multitest_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_List_MultiTestHook, value ? &hook_multitest : NULL);
    list.has_multitest[i] = value;
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

static void ListSetDragSortable(void)
{
    UWORD i;
    LONG value = 0;

    i = XGET(list.list_radios, MUIA_Radio_Active);

    GET(list.dragsortable_check, MUIA_Selected, &value);
    SET(list.lists[i], MUIA_List_DragSortable, value);
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
        if (count == 0)
            count = 1;    /* There may still be an active entry */
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
    list.destruct_count++;

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
        if (XGET(list.showimage_check, MUIA_Selected))
            sprintf(buf, "%ld \33O[%08lx]", (long)*(strings - 1),
                (long)list.image);
        else
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

/* Update numeric minimums */
static void NumericSetMin(void)
{
    UWORD min, i;

    min = XGET(numeric.min_string, MUIA_String_Integer);

    for (i = 0; i < NUMERIC_COUNT; i++)
        SET(numeric.numerics[i], MUIA_Numeric_Min, min);
}

/* Update numeric/gauge maximums. Note that the maximum is set for some gauges
 * elsewhere through notification */
static void NumericSetMax(void)
{
    UWORD max, div, i;
    TEXT info_text[10];

    max = XGET(numeric.max_string, MUIA_String_Integer);

    for (i = 0; i < NUMERIC_COUNT; i++)
        SET(numeric.numerics[i], MUIA_Numeric_Max, max);

    sprintf(info_text, "%%ld/%d", (int)max);
    FreeVec((APTR)XGET(numeric.gauges[HNGAUGE], MUIA_Gauge_InfoText));
    SET(numeric.gauges[HNGAUGE], MUIA_Gauge_InfoText, StrDup(info_text));

    SET(numeric.gauges[VNGAUGE], MUIA_Gauge_Max, max);

    max = XGET(numeric.gauges[VNGAUGE], MUIA_Gauge_Max);
    div = XGET(numeric.gauges[VQGAUGE], MUIA_Gauge_Divide);
    SET(numeric.gauges[VQGAUGE], MUIA_Gauge_Max, max / div);
}

static void GaugeCopyCurrent(void)
{
    UWORD i;

    i = XGET(numeric.gauges[HNGAUGE], MUIA_Gauge_Current);
    SET(numeric.gauges[HQGAUGE], MUIA_Gauge_Current, i);
}

static void SliderCopyValue(void)
{
    UWORD i;

    i = XGET(numeric.numerics[HRSLIDER], MUIA_Numeric_Value);
    nnset(numeric.numerics[HQSLIDER], MUIA_Numeric_Value, i);
    i = XGET(numeric.numerics[VRSLIDER], MUIA_Numeric_Value);
    nnset(numeric.numerics[VQSLIDER], MUIA_Numeric_Value, i);
}

static void RadioCopyActive(void)
{
    UWORD i;

    i = XGET(city_radios[1], MUIA_Radio_Active);
    nnset(city_radios[2], MUIA_Radio_Active, i);
}

static void CycleCopyActive(void)
{
    UWORD i;

    i = XGET(cycle.city_cycle, MUIA_Cycle_Active);
    SET(cycle.city_text, MUIA_Text_Contents, cities[i]);
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

/* Custom Window subclass */

struct TestWindowData
{
    ULONG x;
};

IPTR TestWindow__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct TagItem *tags;
    struct TagItem *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Window_Open:
            if (tag->ti_Data)
                SET(obj, MUIA_Window_ActiveObject, general.open_button);
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

AROS_UFH3S(IPTR, TestWindowDispatcher,
    AROS_UFHA(Class *, cl, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    switch (msg->MethodID)
    {
    case OM_SET:
        return TestWindow__OM_SET(cl, obj, (struct opSet *)msg);
    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    AROS_USERFUNC_EXIT
}

static struct MUI_CustomClass *test_window_class;

#define TestWindowObject BOOPSIOBJMACRO_START(test_window_class->mcc_Class)

/* Main prog */

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
    LONG result = RETURN_OK;
    APTR pool;
    Object *second_wnd;
    Object *about_button;
    Object *quit_button;
    Object *about_item, *quit_item;
    STRPTR title;
    UWORD i;
    LONG value = 0;

    hook_standard.h_Entry = (HOOKFUNC) hook_func_standard;
    hook_objstr.h_Entry = (HOOKFUNC) ObjStrHook;

    pool = CreatePool(MEMF_ANY, 4096, 4096);

    MUIMasterBase = (struct Library *)OpenLibrary("muimaster.library", 0);

    hook.h_Entry = (HOOKFUNC) repeat_function;
    hook_wheel.h_Entry = (HOOKFUNC) wheel_function;
    hook_slider.h_Entry = (HOOKFUNC) slider_function;
    hook_objects.h_Entry = (HOOKFUNC) objects_function;
    hook_compare.h_Entry = (HOOKFUNC) ListCompareHook;
    hook_multitest.h_Entry = (HOOKFUNC) ListMultiTestHook;
    hook_construct.h_Entry = (HOOKFUNC) ListConstructHook;
    hook_destruct.h_Entry = (HOOKFUNC) ListDestructHook;
    hook_display.h_Entry = (HOOKFUNC) display_function;
    list.destruct_count = 0;
    list.has_multitest[3] = TRUE;

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
    test_window_class = MUI_CreateCustomClass(NULL, MUIC_Window, NULL,
        sizeof(struct TestWindowData), TestWindowDispatcher);
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
            MUIA_List_CompareHook, &hook_compare,
            MUIA_ShortHelp,
                "Default scroller\nTop entry active\nSorted by length",
            End,
        MUIA_Listview_MultiSelect,
            MUIV_Listview_MultiSelect_None,
        MUIA_Listview_DragType, MUIV_Listview_DragType_Immediate,
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
            MUIA_List_AutoVisible, TRUE,
            MUIA_ShortHelp,
                "Left scroller\nBottom entry active\nSorted alphabetically",
            End,
        MUIA_Listview_ScrollerPos,
            MUIV_Listview_ScrollerPos_Left,
        MUIA_Listview_DragType, MUIV_Listview_DragType_None,
        MUIA_CycleChain, 1,
        End;
    list.lists[2] = ListviewObject,
        MUIA_Listview_List,
            ListObject,
            InputListFrame,
            MUIA_List_SourceArray, fruits,
            MUIA_List_Active, MUIV_List_Active_Off,
            MUIA_List_Pool, pool,
            MUIA_ShortHelp,
                "Right scroller\nNo active entry\nSorted alphabetically",
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
            MUIA_List_MultiTestHook, &hook_multitest,
            MUIA_ShortHelp,
                "No scroller\nDefault active entry\nSorted alphabetically",
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
            MUIA_List_Active, MUIV_List_Active_Bottom,
            MUIA_List_MinLineHeight, 20,
            MUIA_ShortHelp,
                "Default scroller\nBottom entry active\nSorted by length",
            End,
        MUIA_Listview_Input, FALSE,
        MUIA_CycleChain, 1,
        End;

    list.colorfield = ColorfieldObject,
        MUIA_Colorfield_RGB, default_color,
        End;

    if (CL_DropText == NULL || test_window_class == NULL)
        result = RETURN_FAIL;

    if (result == RETURN_OK)
    {
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

            SubWindow, wnd = TestWindowObject,
                MUIA_Window_Title, "test",
                MUIA_Window_ID, MAKE_ID('T','E','S','T'),
                MUIA_Window_Activate, TRUE,

                WindowContents, VGroup,
                    Child, RegisterGroup(pages),
                        //MUIA_Background, "5:SYS:Prefs/Presets/Backdrops/StuccoBlue.pic",
                        Child, CreateGeneralGroup(),
                        Child, CreateTextGroup(),
                        Child, CreateBoopsiGroup(),
                        Child, CreateColorGroup(),
                        Child, CreateEditGroup(),
                        Child, CreateListGroup(),
                        Child, CreateNumericGroup(),
                        Child, CreateSelectGroup(),
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
                        Child, CreateBalancingGroup(),

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

        if (app == NULL)
            result = RETURN_FAIL;
    }

    if (result == RETURN_OK)
    {
        ULONG sigs = 0;

        DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        DoMethod(second_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            second_wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);
        DoMethod(general.open_button, MUIM_Notify, MUIA_Pressed, FALSE,
            second_wnd, 3, MUIM_Set, MUIA_Window_Open, TRUE);
        SET(general.menus_check, MUIA_Selected, TRUE);
        DoMethod(general.menus_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, wnd, 3, MUIM_Set, MUIA_Window_NoMenus,
            MUIV_NotTriggerValue);
        DoMethod(about_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2,
            MUIM_Application_AboutMUI, NULL);
        DoMethod(quit_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        DoMethod(objects_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2,
            MUIM_CallHook, &hook_objects);
        DoMethod(repeat_button, MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
            app, 2, MUIM_CallHook, &hook);

        /* Notifications and set-up for window tab */
        set(window.close_check, MUIA_Selected, TRUE);
        set(window.depth_check, MUIA_Selected, TRUE);
        set(window.close_check, MUIA_Selected, TRUE);
        set(window.dragbar_check, MUIA_Selected, TRUE);
        set(window.size_check, MUIA_Selected, TRUE);
        set(window.activate_check, MUIA_Selected, TRUE);
        set(window.close_button, MUIA_Disabled, TRUE);
        set(window.apply_button, MUIA_Disabled, TRUE);
        DoMethod(window.open_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, WindowOpen);
        DoMethod(window.close_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, WindowClose);
        DoMethod(window.apply_button, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 3, MUIM_CallHook, &hook_standard, WindowApply);

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
            DoMethod(list.lists[i], MUIM_Notify, MUIA_Listview_SelectChange,
                MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
                ListGetSelected);
        }
        DoMethod(list.draggable_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetDraggable);
        DoMethod(list.draggable_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, list.dragsortable_check, 3, MUIM_Set,
            MUIA_Disabled, MUIV_NotTriggerValue);
        DoMethod(list.draggable_check, MUIM_Notify, MUIA_Selected,
            FALSE, list.dragsortable_check, 3, MUIM_Set,
            MUIA_Selected, FALSE);
        DoMethod(list.showdropmarks_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetShowDropMarks);
        DoMethod(list.multitest_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetMultiTest);
        DoMethod(list.quiet_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetQuiet);
        DoMethod(list.dragsortable_check, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            ListSetDragSortable);
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
        SET(list.lists[4], MUIA_List_CompareHook, &hook_compare);
        SET(list.lists[4], MUIA_List_AutoVisible, TRUE);

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
            DoMethod(list.showimage_check, MUIM_Notify, MUIA_Selected,
                MUIV_EveryTime, list.multi_lists[i], 2, MUIM_List_Redraw,
                MUIV_List_Redraw_All);
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
        SET(list.multi_lists[1], MUIA_List_ConstructHook, &hook_construct);
        SET(list.multi_lists[1], MUIA_List_DestructHook, &hook_destruct);
        SET(list.multi_lists[1], MUIA_List_DisplayHook, &hook_display);
        DoMethod(list.multi_lists[1], MUIM_List_Insert, entries, -1,
            MUIV_List_Insert_Top);

        DoMethod(listview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            popobject, 2, MUIM_Popstring_Close, TRUE);

        /* numeric */

        /* Update min/max values of sliders and gauges whenever the min/max
         * fields are changed */
        DoMethod(numeric.min_string, MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            NumericSetMin);
        DoMethod(numeric.max_string, MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            NumericSetMax);
        DoMethod(numeric.numerics[HNSLIDER], MUIM_Notify, MUIA_Numeric_Max,
            MUIV_EveryTime, numeric.gauges[HNGAUGE], 3, MUIM_Set,
            MUIA_Gauge_Max, MUIV_TriggerValue);
        DoMethod(numeric.gauges[HNGAUGE], MUIM_Notify, MUIA_Gauge_Max,
            MUIV_EveryTime, numeric.gauges[HQGAUGE], 3, MUIM_Set,
            MUIA_Gauge_Max, MUIV_TriggerValue);

        DoMethod(numeric.numerics[NKNOB], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[NLEVELMETER], 3, MUIM_Set,
            MUIA_Numeric_Value, MUIV_TriggerValue);
        DoMethod(numeric.numerics[RKNOB], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[RLEVELMETER], 3, MUIM_Set,
            MUIA_Numeric_Value, MUIV_TriggerValue);
        DoMethod(numeric.slider_button, MUIM_Notify, MUIA_Pressed, FALSE,
            numeric.numerics[HRSLIDER], 3, MUIM_Set, MUIA_Slider_Horiz, FALSE);

        /* Get horizontal sliders to follow each other */
        DoMethod(numeric.numerics[HNSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[HRSLIDER], 3, MUIM_Set,
            MUIA_Numeric_Value, MUIV_TriggerValue);
        DoMethod(numeric.numerics[HRSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[HNSLIDER], 3, MUIM_NoNotifySet,
            MUIA_Numeric_Value, MUIV_TriggerValue);
        DoMethod(numeric.numerics[HRSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            SliderCopyValue);
        DoMethod(numeric.numerics[HQSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[HNSLIDER], 3, MUIM_Set,
            MUIA_Numeric_Value, MUIV_TriggerValue);

        /* Get normal horizontal gauge to follow horizontal sliders */
        for (i = HNSLIDER; i <= HRSLIDER; i++)
        {
            DoMethod(numeric.numerics[i], MUIM_Notify, MUIA_Numeric_Value,
                MUIV_EveryTime, numeric.gauges[HNGAUGE], 3, MUIM_Set,
                MUIA_Gauge_Current, MUIV_TriggerValue);
        }

        /* Get quiet horizontal gauge to follow normal horizontal gauge */
        DoMethod(numeric.gauges[HNGAUGE], MUIM_Notify, MUIA_Gauge_Current,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            GaugeCopyCurrent);

        /* Get vertical sliders to follow each other */
        DoMethod(numeric.numerics[VNSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[VRSLIDER], 3, MUIM_Set,
            MUIA_Numeric_Value, MUIV_TriggerValue);
        DoMethod(numeric.numerics[VRSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[VNSLIDER], 3, MUIM_NoNotifySet,
            MUIA_Numeric_Value, MUIV_TriggerValue);
        DoMethod(numeric.numerics[VRSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            SliderCopyValue);
        DoMethod(numeric.numerics[VQSLIDER], MUIM_Notify, MUIA_Numeric_Value,
            MUIV_EveryTime, numeric.numerics[VNSLIDER], 3, MUIM_Set,
            MUIA_Numeric_Value, MUIV_TriggerValue);

        /* Get vertical gauge to follow vertical sliders */
        for (i = VNSLIDER; i <= VRSLIDER; i++)
        {
            DoMethod(numeric.numerics[i], MUIM_Notify, MUIA_Numeric_Value,
                MUIV_EveryTime, numeric.gauges[VNGAUGE], 3, MUIM_Set,
                MUIA_Gauge_Current, MUIV_TriggerValue);
        }

        /* Get quiet vertical gauge to follow normal vertical gauge */
        DoMethod(numeric.gauges[VNGAUGE], MUIM_Notify, MUIA_Gauge_Current,
            MUIV_EveryTime, numeric.gauges[VQGAUGE], 3, MUIM_Set,
            MUIA_Gauge_Current, MUIV_TriggerValue);

        /* radio */
        DoMethod(city_radios[0], MUIM_Notify, MUIA_Radio_Active,
            MUIV_EveryTime, city_radios[1], 3, MUIM_Set,
            MUIA_Radio_Active, MUIV_TriggerValue);
        DoMethod(city_radios[1], MUIM_Notify, MUIA_Radio_Active,
            MUIV_EveryTime, city_radios[0], 3, MUIM_NoNotifySet,
            MUIA_Radio_Active, MUIV_TriggerValue);
        DoMethod(city_radios[1], MUIM_Notify, MUIA_Radio_Active,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            RadioCopyActive);
        DoMethod(city_radios[2], MUIM_Notify, MUIA_Radio_Active,
            MUIV_EveryTime, city_radios[0], 3, MUIM_Set,
            MUIA_Radio_Active, MUIV_TriggerValue);

        /* cycle */
        DoMethod(cycle.city_cycle, MUIM_Notify, MUIA_Cycle_Active,
            MUIV_EveryTime, city_radios[0], 3, MUIM_Set,
            MUIA_Radio_Active, MUIV_TriggerValue);
        DoMethod(cycle.prev_button, MUIM_Notify, MUIA_Pressed, FALSE,
            cycle.city_cycle, 3, MUIM_Set, MUIA_Cycle_Active,
            MUIV_Cycle_Active_Prev);
        DoMethod(cycle.next_button, MUIM_Notify, MUIA_Pressed, FALSE,
            cycle.city_cycle, 3, MUIM_Set, MUIA_Cycle_Active,
            MUIV_Cycle_Active_Next);
        DoMethod(cycle.city_cycle, MUIM_Notify, MUIA_Cycle_Active,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            CycleCopyActive);

#if defined(TEST_ICONLIST)
        /* iconlist */
        DoMethod(volume_iconlist, MUIM_Notify, MUIA_IconList_DoubleClick,
            TRUE, volume_iconlist, 3, MUIM_CallHook, &hook_standard,
            volume_doubleclicked);
        DoMethod(drawer_iconlist, MUIM_Notify, MUIA_IconList_DoubleClick,
            TRUE, drawer_iconlist, 3, MUIM_CallHook, &hook_standard,
            drawer_doubleclicked);
#endif

        /* pre-display automatic tests */
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
        set(wnd, MUIA_Window_ScreenTitle, "Zune Test Application");

        /* post-display automatic tests */
        set(list.lists[0], MUIA_Listview_SelectChange, TRUE);
        get(list.lists[0], MUIA_Listview_SelectChange, &value);
        if (value)
            printf("MUIA_Listview_SelectChange is settable,"
                " although it should not be.\n");

        list.image = DoMethod(list.multi_lists[0], MUIM_List_CreateImage,
            list.colorfield, 0);

        /* Set pen fields */
        set(colorfield_pen, MUIA_String_Integer,
            XGET(colorfield, MUIA_Colorfield_Pen));

        char pen_str[10];
        nnset(pendisplay_pen, MUIA_String_Integer,
            XGET(pendisplay, MUIA_Pendisplay_Pen));

        struct MUI_PenSpec *pen_spec = NULL;
        GET(pendisplay, MUIA_Pendisplay_Spec, &pen_spec);
        CopyMem(pen_spec->buf, pen_str, 10);
        pen_str[9] = '0';
        set(pendisplay_spec, MUIA_String_Contents, pen_str);

        /* Main loop: wait until quit */
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

        /* clean up */
        DoMethod(list.multi_lists[0], MUIM_List_DeleteImage, list.image);

        for (i = 0; i < LIST_COUNT; i++)
        {
            GET(list.lists[i], MUIA_List_Title, &title);
            SET(list.lists[i], MUIA_List_Title, NULL);
            FreeVec(title);
        }

        FreeVec(text.image_text);

        if (window.window != NULL)
        {
            GET(window.window, MUIA_Window_Title, &title);
            SET(window.window, MUIA_Window_Title, NULL);
            FreeVec(title);
        }

        MUI_DisposeObject(app);
    }
    if (context_menu)
        MUI_DisposeObject(context_menu);
    CloseLibrary(ColorWheelBase);
    MUI_DeleteCustomClass(test_window_class);
    MUI_DeleteCustomClass(CL_DropText);

    /* shutdown-related automatic tests */
    if (list.destruct_count != 18)
        printf("The hook installed through MUIA_List_DestructHook has been"
            " called %ld times, but should have been called 18 times.\n",
            (long)list.destruct_count);

    CloseLibrary(MUIMasterBase);

    DeletePool(pool);

    return result;
}

static Object *CreateGeneralGroup()
{
    Object *group;

    group = RegisterGroup(general_pages),

        /* group */
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
                    Child, general.open_button = TextObject,
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
                        Child, general.menus_check =
                            MUI_MakeObject(MUIO_Checkmark,
                            "_Enable menus"),
                        End,
                    Child, HGroup,
                        Child, MUI_MakeObject(MUIO_Label,
                            "_Enable menus", 0),
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
                                "Line1\nLine2\nLine3\n"
                                "Line4\nLine5\nLine6\n"
                                "Line7\nLine8\n\n\n\n"
                                "Line9\nLine10\nLine11\n",
                            End,
                        Child, HGroup,
                            Child, MUI_MakeObject(MUIO_Button, "Button9"),
                            Child, MUI_MakeObject(MUIO_Button, "Button10"),
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
                            MUIA_List_ConstructHook, &hook_construct,
                            MUIA_List_DestructHook, &hook_destruct,
                            MUIA_List_DisplayHook, &hook_display,
                            MUIA_List_Format, ",,",
                            MUIA_List_SourceArray, entries,
                            MUIA_List_Title, TRUE,
                            End,
                        End,
                    End,
                MUIA_Popobject_ObjStrHook, &hook_objstr,
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

        /* window */
        Child, VGroup,

            Child, ColGroup(4),
                Child, MUI_MakeObject(MUIO_Label, "Window title:", 0),
                Child, window.title_string =
                    StringObject,
                    StringFrame,
                    MUIA_String_Contents, "Test window",
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Screen title:", 0),
                Child, window.screen_string =
                    StringObject,
                    StringFrame,
                    MUIA_String_Contents, "Test window",
                    End,
                End,

            Child, ColGroup(2),
                GroupFrameT("Initial dimensions"),
                Child, ColGroup(2),
                    GroupFrameT("Left Edge"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.left_cycle = CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_left_modes + 1,
                        End,
                    Child, MUI_MakeObject(MUIO_Label, "Pixels:", 0),
                    Child, window.left_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Width"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.width_cycle = CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_dim_modes,
                        End,
                    Child, MUI_MakeObject(MUIO_Label,
                        "Pixels/percentage:", 0),
                    Child, window.width_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Top Edge"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.top_cycle = CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_top_modes + 1,
                        End,
                    Child, MUI_MakeObject(MUIO_Label, "Pixels:", 0),
                    Child, window.top_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Height"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.height_cycle =
                        CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_dim_modes,
                        End,
                    Child, MUI_MakeObject(MUIO_Label,
                        "Pixels/percentage:", 0),
                    Child, window.height_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                End,

            Child, ColGroup(2),
                GroupFrameT("Alternative dimensions"),
                Child, ColGroup(2),
                    GroupFrameT("Left Edge"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.alt_left_cycle =
                        CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_left_modes,
                        End,
                    Child, MUI_MakeObject(MUIO_Label, "Pixels:", 0),
                    Child, window.alt_left_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Width"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.alt_width_cycle =
                        CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_dim_modes + 1,
                        End,
                    Child, MUI_MakeObject(MUIO_Label,
                        "Pixels/percentage:", 0),
                    Child, window.alt_width_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Top Edge"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.alt_top_cycle =
                        CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_top_modes,
                        End,
                    Child, MUI_MakeObject(MUIO_Label, "Pixels:", 0),
                    Child, window.alt_top_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Height"),
                    Child, MUI_MakeObject(MUIO_Label, "Mode:", 0),
                    Child, window.alt_height_cycle =
                        CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_dim_modes + 1,
                        End,
                    Child, MUI_MakeObject(MUIO_Label,
                        "Pixels/percentage:", 0),
                    Child, window.alt_height_index_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)digits,
                        MUIA_String_Integer, 100,
                        End,
                    End,
                End,

            Child, ColGroup(6),
                Child, HGroup,
                    Child, window.appwindow_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "AppWindow", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.backdrop_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Backdrop", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.borderless_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Borderless", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.close_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Close gadget", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.depth_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Depth gadget", 0),
                    Child, HVSpace,
                    End,
                Child, HVSpace,
                Child, HGroup,
                    Child, window.dragbar_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label,
                        "Draggable", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.ref_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Reference parent", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.size_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Size gadget", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.activate_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Active", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, window.id_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label, "Use ID", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, MUI_MakeObject(MUIO_Label, "Menus:", 0),
                    Child, window.menu_cycle = CycleObject,
                        ButtonFrame,
                        MUIA_Cycle_Entries, window_menu_modes,
                        End,
                    End,
                End,

            Child, HGroup,
                Child, window.open_button =
                    MUI_MakeObject(MUIO_Button, "Open"),
                Child, window.close_button =
                    MUI_MakeObject(MUIO_Button, "Close"),
                Child, window.apply_button =
                    MUI_MakeObject(MUIO_Button, "Apply"),
                End,

            Child, HVSpace,
            End,
        End;

    return group;
}

static Object *CreateTextGroup()
{
    Object *group;
    UWORD i, j;
    ULONG colour;
    struct MUI_AlphaData *raw_image;
    ULONG *pixels;
    TEXT image_text[100];

    /* Allocate image data */
    raw_image = AllocVec(sizeof(struct MUI_AlphaData)
        + TEXT_RAW_IMAGE_SIZE * TEXT_RAW_IMAGE_SIZE * sizeof(ULONG), MEMF_ANY);
    if (raw_image == NULL)
        return NULL;

    /* Create a pattern where each quadrant of a square is a different colour */
    raw_image->width = TEXT_RAW_IMAGE_SIZE;
    raw_image->height = TEXT_RAW_IMAGE_SIZE;
    pixels = raw_image->data;
    for (i = 0; i < TEXT_RAW_IMAGE_SIZE / 2; i++)
    {
        colour = 0xff0000ff;
        for (j = 0; j < TEXT_RAW_IMAGE_SIZE / 2; j++)
            *pixels++ = AROS_LONG2LE(colour);
        colour = 0x00ff00ff;
        for (j = 0; j < TEXT_RAW_IMAGE_SIZE / 2; j++)
            *pixels++ = AROS_LONG2LE(colour);
    }
    for (i = 0; i < TEXT_RAW_IMAGE_SIZE / 2; i++)
    {
        colour = 0x0000ffff;
        for (j = 0; j < TEXT_RAW_IMAGE_SIZE / 2; j++)
            *pixels++ = AROS_LONG2LE(colour);
        colour = 0x00000000;
        for (j = 0; j < TEXT_RAW_IMAGE_SIZE / 2; j++)
            *pixels++ = AROS_LONG2LE(colour);
    }

    /* Embed image in a string */
    sprintf(image_text,
        "This is a text object with a raw ARGB image: \33A[%lx]",
        (IPTR)raw_image);

    group = RegisterGroup(text_pages),

        /* text */
        Child, VGroup,
            Child, TextObject,
                MUIA_Background, "2:cfffffff,cfffffff,10000000",
                TextFrame,
                MUIA_Text_Contents, "\33cHello World!\n"
                    "This is a text object\n\33lLeft "
                    "\33bbold\33n\n"
                    "\33rRight",
                End,
            Child, TextObject,
                TextFrame,
                /* Test for a bug with pen specifications as
                   reported here:
                   https://sourceforge.net/p/aros/bugs/487/
                */
                MUIA_Text_Contents,
                    "This is a \33P[3]text \33P[]object "
                    "\33P[1]with pen specifications",
                End,
            Child, TextObject,
                TextFrame,
                MUIA_Text_Contents, (text.image_text = StrDup(image_text)),
                End,
            Child, RectangleObject,
                MUIA_VertWeight, 0,
                    /* Seems to be not supported properly as
                       orginal MUI doesn't allow to alter the
                       height of the window */
                MUIA_Rectangle_HBar, TRUE,
                MUIA_Rectangle_BarTitle, "Enter a string",
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
                Child, VGroup,
                    GroupFrameT("Default Alignment"),
                    Child, string.strings[0] = StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)default_accept_chars,
                        MUIA_String_Reject, (IPTR)default_reject_chars,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_MaxLen, 9,
                        MUIA_CycleChain, 1,
                        MUIA_String_AttachedList, list.lists[0],
                        End,
                    End,
                Child, VGroup, GroupFrameT("Left Aligned"),
                    Child, string.strings[1] = StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)default_accept_chars,
                        MUIA_String_Reject, (IPTR)default_reject_chars,
                        MUIA_String_Format, MUIV_String_Format_Left,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_MaxLen, 9,
                        MUIA_CycleChain, 1,
                        MUIA_String_Contents, (IPTR)default_accept_chars,
                        MUIA_String_BufferPos, 3,
                        MUIA_String_AttachedList, list.lists[0],
                        End,
                    End,
                Child, VGroup, GroupFrameT("Right Aligned"),
                    Child, string.strings[2] = StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)default_accept_chars,
                        MUIA_String_Reject, (IPTR)default_reject_chars,
                        MUIA_String_Format, MUIV_String_Format_Right,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_MaxLen, 9,
                        MUIA_CycleChain, 1,
                        MUIA_String_AttachedList, list.lists[0],
                        End,
                    End,
                Child, VGroup, GroupFrameT("Centered"),
                    Child, string.strings[3] = StringObject,
                        StringFrame,
                        MUIA_String_Accept, (IPTR)default_accept_chars,
                        MUIA_String_Reject, (IPTR)default_reject_chars,
                        MUIA_String_Format, MUIV_String_Format_Center,
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
                        MUIA_String_Accept, (IPTR)default_accept_chars,
                        MUIA_String_Reject, (IPTR)default_reject_chars,
                        MUIA_String_Format, MUIV_String_Format_Center,
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
                        MUIA_String_Accept, (IPTR)default_accept_chars,
                        MUIA_String_Reject, (IPTR)default_reject_chars,
                        MUIA_String_Format, MUIV_String_Format_Center,
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
                    Child, string.accept_string =
                        StringObject,
                        StringFrame,
                        MUIA_String_Contents, (IPTR)default_accept_chars,
                        End,
                    Child, MUI_MakeObject(MUIO_Label,
                        "Unacceptable characters:", 0),
                    Child, string.reject_string =
                        StringObject,
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
                            MUI_MakeObject(MUIO_Checkmark, NULL),
                        Child, MUI_MakeObject(MUIO_Label,
                            "Advance on CR", 0),
                        Child, HVSpace,
                        End,
                    Child, HGroup,
                        Child, string.attach_list_check =
                            MUI_MakeObject(MUIO_Checkmark, NULL),
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
                            MUI_MakeObject(MUIO_Checkmark, NULL),
                        Child, MUI_MakeObject(MUIO_Label,
                            "Use custom edit hook", 0),
                        Child, HVSpace,
                        End,
#endif
                    End,
                Child, HVSpace,
                End,

            End,
        End;

    return group;
}

static Object *CreateBoopsiGroup()
{
    Object *group;

    group = VGroup,
        Child, wheel = BoopsiObject,
            /* MUI/Boopsi tags mixed */
            GroupFrame,
            /* boopsi objects don't know their sizes, so we help keep
             * important values during window resize */
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
        End;

    return group;
}

static Object *CreateColorGroup()
{
    Object *group;

    group = RegisterGroup(color_pages),
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
                    MUI_MakeObject(MUIO_Button, "Reset"),
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
                        Child, pendisplay2 =
                            PendisplayObject,
                            MUIA_Pendisplay_RGBcolor, default_color,
                            End,
                        End,
                    Child, ColGroup(2),
                        Child,
                            MUI_MakeObject(MUIO_Label, "Pen:", 0),
                        Child, HGroup,
                            Child, pendisplay_pen =
                                StringObject, StringFrame,
                                End,
                            Child, HVSpace,
                            Child, reference_check =
                                MUI_MakeObject(MUIO_Checkmark, NULL),
                            Child, MUI_MakeObject(MUIO_Label,
                                "Reference", 0),
                            End,
                        Child,
                            MUI_MakeObject(MUIO_Label, "Penspec:", 0),
                        Child, pendisplay_spec =
                            StringObject,
                            StringFrame,
                            End,
                        End,
                    Child, HGroup,
                        Child, shine_button =
                            MUI_MakeObject(MUIO_Button, "Shine"),
                        Child, shadow_button =
                            MUI_MakeObject(MUIO_Button, "Shadow"),
                        Child, yellow_button =
                            MUI_MakeObject(MUIO_Button, "Yellow"),
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
        End;

    return group;
}

static Object *CreateEditGroup()
{
    Object *group;

    group = VGroup,
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
        End;

    return group;
}

static Object *CreateListGroup()
{
    Object *group;

    group = RegisterGroup(list_pages),
        Child, VGroup,
            Child, ColGroup(LIST_COUNT),
                Child, VGroup,
                    GroupFrameT("No Multiselect"),
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
                    Child, list.draggable_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label,
                        "Draggable", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, list.showdropmarks_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label,
                        "Show drop marks", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, list.autovisible_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label,
                        "Auto visible", 0),
                    Child, HVSpace,
                    End,
                Child, list.reset_button =
                    MUI_MakeObject(MUIO_Button, "Reset"),

                Child, MUI_MakeObject(MUIO_Label,
                    "Affected index 2:", 0),
                Child, list.index2_string =
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
                    Child, list.multitest_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label,
                        "Filter multiselect", 0),
                    Child, HVSpace,
                    End,
                Child, HGroup,
                    Child, list.quiet_check =
                        MUI_MakeObject(MUIO_Checkmark, NULL),
                    Child, MUI_MakeObject(MUIO_Label,
                        "Quiet", 0),
                    Child, HVSpace,
                    End,
                Child, list.enable_button =
                    MUI_MakeObject(MUIO_Button, "Enable"),

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
                Child, MUI_MakeObject(MUIO_Label,
                    "Title:", 0),
                Child, list.title_string =
                    StringObject,
                    StringFrame,
                    End,

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
                    "Entries:", 0),
                Child, list.entries_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
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
                    "Visible entries:", 0),
                Child, list.visible_text = TextObject,
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
                    "First visible index:", 0),
                Child, list.first_text = TextObject,
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
                    "Last insertion index:", 0),
                Child, list.insert_text = TextObject,
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
                    "Active index:", 0),
                Child, list.active_text = TextObject,
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
                    "Selected entries:", 0),
                Child, list.selected_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,

                Child, HVSpace,
                Child, HVSpace,
                Child, HVSpace,
                Child, HVSpace,
                Child, MUI_MakeObject(MUIO_Label,
                    "Last drop index:", 0),
                Child, list.drop_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                End,
            End,
        Child, VGroup,
            Child, ColGroup(MULTI_LIST_COUNT),
                Child, VGroup,
                    GroupFrameT("Standard Format"),
                    Child, list.multi_lists[0] =
                        ListviewObject,
                        MUIA_Listview_List,
                            ListObject,
                            InputListFrame,
                            MUIA_List_ConstructHook, &hook_construct,
                            MUIA_List_DestructHook, &hook_destruct,
                            MUIA_List_DisplayHook, &hook_display,
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
                    Child, list.multi_lists[1] =
                        ListviewObject,
                        MUIA_Listview_List,
                            ListObject,
                            InputListFrame,
                            MUIA_List_Format, list_format,
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
                Child, MUI_MakeObject(
                    MUIO_Label, "Format:", 0),
                Child, list.format_string = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                    End,
                Child, list.showheadings_check =
                    MUI_MakeObject(MUIO_Checkmark, NULL),
                Child, MUI_MakeObject(MUIO_Label,
                    "Show column headings", 0),
                Child, list.showimage_check =
                    MUI_MakeObject(MUIO_Checkmark, NULL),
                Child, MUI_MakeObject(MUIO_Label,
                    "Show image", 0),
                End,
            Child, HGroup,
                Child, MUI_MakeObject(MUIO_Label,
                    "Default clicked column:", 0),
                Child, list.def_column_string =
                    StringObject,
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
        End;

    return group;
}

static Object *CreateNumericGroup()
{
    Object *group;

    group = HGroup,
        Child, VGroup,
            Child, ColGroup(2),
                GroupFrameT("Horizontal Sliders"),
                Child, MUI_MakeObject(MUIO_Label, "Normal:", 0),
                Child, numeric.numerics[HNSLIDER] = SliderObject,
                    MUIA_Slider_Horiz, TRUE,
                    MUIA_Numeric_Min, NUMERIC_MIN,
                    MUIA_Numeric_Max, NUMERIC_MAX,
                    MUIA_CycleChain, 1,
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Reverse:", 0),
                Child, numeric.numerics[HRSLIDER] = SliderObject,
                    MUIA_Numeric_Reverse, TRUE,
                    MUIA_Numeric_Min, NUMERIC_MIN,
                    MUIA_Numeric_Max, NUMERIC_MAX,
                    MUIA_CycleChain, 1,
                    End,
                Child, MUI_MakeObject(MUIO_Label, "Quiet:", 0),
                Child, numeric.numerics[HQSLIDER] = SliderObject,
                    MUIA_Slider_Quiet, TRUE,
                    MUIA_Numeric_Min, NUMERIC_MIN,
                    MUIA_Numeric_Max, NUMERIC_MAX,
                    MUIA_CycleChain, 1,
                    End,
                End,
            Child, ColGroup(2),
                Child, ColGroup(5),
                    GroupFrameT("Knobs"),
                    Child, HVSpace,
                    Child, MUI_MakeObject(MUIO_Label, "Normal", 0),
                    Child, HVSpace,
                    Child, MUI_MakeObject(MUIO_Label, "Reverse", 0),
                    Child, HVSpace,
                    Child, HVSpace,
                    Child, numeric.numerics[NKNOB] = KnobObject,
                        MUIA_Numeric_Min, NUMERIC_MIN,
                        MUIA_Numeric_Max, NUMERIC_MAX,
                        MUIA_CycleChain, 1,
                        End,
                    Child, HVSpace,
                    Child, numeric.numerics[RKNOB] = KnobObject,
                        MUIA_Numeric_Reverse, TRUE,
                        MUIA_Numeric_Min, NUMERIC_MIN,
                        MUIA_Numeric_Max, NUMERIC_MAX,
                        MUIA_CycleChain, 1,
                        End,
                    Child, HVSpace,
                    End,
                Child, VGroup,
                    Child, ColGroup(2),
                        Child, MUI_MakeObject(MUIO_Label,
                            "Minimum Value:", 0),
                        Child, numeric.min_string = (Object *)StringObject,
                            StringFrame,
                            MUIA_String_Accept, (IPTR)digits,
                            MUIA_String_Integer, NUMERIC_MIN,
                            MUIA_CycleChain, 1,
                            End,
                        Child, MUI_MakeObject(MUIO_Label,
                            "Maximum Value:", 0),
                        Child, numeric.max_string = (Object *)StringObject,
                            StringFrame,
                            MUIA_String_Accept, (IPTR)digits,
                            MUIA_String_Integer, NUMERIC_MAX,
                            MUIA_CycleChain, 1,
                            End,
                        End,
                    Child, numeric.slider_button = TextObject,
                        ButtonFrame,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_CycleChain, 1,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, "Change Slider Orientations",
                        End,
                    End,
                Child, ColGroup(2),
                    GroupFrameT("Level Meters"),
                    Child, MUI_MakeObject(MUIO_Label, "Normal", 0),
                    Child, MUI_MakeObject(MUIO_Label, "Reverse", 0),
                    Child, numeric.numerics[NLEVELMETER] = LevelmeterObject,
                        MUIA_Numeric_Min, NUMERIC_MIN,
                        MUIA_Numeric_Max, NUMERIC_MAX,
                        MUIA_CycleChain, 1,
                        End,
                    Child, numeric.numerics[RLEVELMETER] = LevelmeterObject,
                        MUIA_Numeric_Reverse, TRUE,
                        MUIA_Numeric_Min, NUMERIC_MIN,
                        MUIA_Numeric_Max, NUMERIC_MAX,
                        MUIA_CycleChain, 1,
                        End,
                    End,
                Child, ColGroup(3),
                    GroupFrameT("Numeric Buttons"),
                    Child, MUI_MakeObject(MUIO_Label, "Normal:", 0),
                    Child, numeric.numerics[NNUMERICBUTTON] =
                        NumericbuttonObject,
                        MUIA_Numeric_Min, NUMERIC_MIN,
                        MUIA_Numeric_Max, NUMERIC_MAX,
                        MUIA_CycleChain, 1,
                        End,
                    Child, HVSpace,
                    Child, MUI_MakeObject(MUIO_Label, "Reverse:", 0),
                    Child, numeric.numerics[RNUMERICBUTTON] =
                        NumericbuttonObject,
                        MUIA_Numeric_Reverse, TRUE,
                        MUIA_Numeric_Min, NUMERIC_MIN,
                        MUIA_Numeric_Max, NUMERIC_MAX,
                        MUIA_CycleChain, 1,
                        End,
                    Child, HVSpace,
                    Child, VSpace(0),
                    Child, VSpace(0),
                    Child, HVSpace,
                    End,
                End,
            End,
        Child, ColGroup(3),
            GroupFrameT("Vertical Sliders"),
            Child, MUI_MakeObject(MUIO_Label, "Normal", 0),
            Child, MUI_MakeObject(MUIO_Label, "Reverse", 0),
            Child, MUI_MakeObject(MUIO_Label, "Quiet", 0),
            Child, numeric.numerics[VNSLIDER] = SliderObject,
                MUIA_Slider_Horiz, FALSE,
                MUIA_Numeric_Min, NUMERIC_MIN,
                MUIA_Numeric_Max, NUMERIC_MAX,
                MUIA_Numeric_Value, NUMERIC_INIT,
                MUIA_CycleChain, 1,
                End,
            Child, numeric.numerics[VRSLIDER] = SliderObject,
                MUIA_Slider_Horiz, FALSE,
                MUIA_Numeric_Reverse, TRUE,
                MUIA_Numeric_Min, NUMERIC_MIN,
                MUIA_Numeric_Max, NUMERIC_MAX,
                MUIA_Numeric_Value, NUMERIC_INIT,
                MUIA_CycleChain, 1,
                End,
            Child, numeric.numerics[VQSLIDER] = SliderObject,
                MUIA_Slider_Horiz, FALSE,
                MUIA_Slider_Quiet, TRUE,
                MUIA_Numeric_Min, NUMERIC_MIN,
                MUIA_Numeric_Max, NUMERIC_MAX,
                MUIA_Numeric_Value, NUMERIC_INIT,
                MUIA_CycleChain, 1,
                End,
            End,

        Child, VGroup,
            GroupFrameT("Gauges"),
            Child, numeric.gauges[HNGAUGE] = GaugeObject,
                GaugeFrame,
                MUIA_Gauge_Horiz, TRUE,
                MUIA_Gauge_InfoText, StrDup("%ld/100"),
                End,
            Child, numeric.gauges[HQGAUGE] = GaugeObject,
                GaugeFrame,
                MUIA_Gauge_Horiz, TRUE,
                End,
            Child, ScaleObject,
                End,
            Child, HGroup,
                Child, numeric.gauges[VNGAUGE] = GaugeObject,
                    GaugeFrame,
                    MUIA_Gauge_Current, NUMERIC_INIT,
                    MUIA_Gauge_Max, NUMERIC_MAX,
                    MUIA_Gauge_InfoText, StrDup("%ld"),
                    End,
                Child, numeric.gauges[VQGAUGE] = GaugeObject,
                    GaugeFrame,
                    MUIA_Gauge_Horiz, FALSE,
                    MUIA_Gauge_Current, NUMERIC_INIT,
                    MUIA_Gauge_Max, NUMERIC_MAX / NUMERIC_DIV,
                    MUIA_Gauge_Divide, NUMERIC_DIV,
                    End,
                End,
            End,

        End;

    return group;
}

static Object *CreateSelectGroup()
{
    Object *group;

    group = HGroup,
        Child, VGroup,
            GroupFrameT("Radio"),
            Child, city_radios[0] = RadioObject,
                GroupFrameT("Vertical"),
                MUIA_Radio_Entries, cities,
                MUIA_Radio_Active, 1,
                End,
            Child, city_radios[1] = RadioObject,
                GroupFrameT("Horizontal"),
                MUIA_Group_Horiz, TRUE,
                MUIA_Radio_Entries, cities,
                MUIA_Radio_Active, 1,
                End,
            Child, city_radios[2] = RadioObject,
                GroupFrameT("Grid"),
                MUIA_Group_Rows, 2,
                MUIA_Radio_Entries, cities,
                MUIA_Radio_Active, 1,
                End,
            Child, HVSpace,
            End,
        Child, VGroup,
            GroupFrameT("Cycle"),
            Child, ColGroup(3),
                Child, MUI_MakeObject(
                    MUIO_Label, "Choose city:", 0),
                Child, cycle.city_cycle = CycleObject,
                    ButtonFrame,
                    MUIA_Cycle_Entries, cities,
                    MUIA_Radio_Active, 1,
                    End,
                Child, cycle.prev_button =
                    MUI_MakeObject(MUIO_Button, "Previous"),
                Child, MUI_MakeObject(MUIO_Label,
                    "Chosen city:", 0),
                Child, cycle.city_text = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, "N/A",
                    End,
                Child, cycle.next_button =
                    MUI_MakeObject(MUIO_Button, "Next"),
                End,
            Child, HVSpace,
            End,
        End;

    return group;
}

static Object *CreateBalancingGroup()
{
    Object *group;

    group = HGroup,
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

        End;

    return group;
}
