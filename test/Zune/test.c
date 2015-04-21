/*
    Copyright © 2002-2015, The AROS Development Team.
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

#define LIST_COUNT 5
#define MULTI_LIST_COUNT 2

#define NUMERIC_MIN 0
#define NUMERIC_MAX 100

static const ULONG default_color[] = {155 << 24, 180 << 24, 255 << 24};
static const struct MUI_PenSpec default_penspec = {"m0"};
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

Object *app;

Object *wheel;
Object *r_slider;
Object *g_slider;
Object *b_slider;
Object *hue_gauge;
Object *colorfield, *colorfield2, *colorfield_reset_button, *colorfield_pen;
Object *coloradjust;
Object *pendisplay, *pendisplay2, *pendisplay_pen, *pendisplay_spec,
    *reference_check, *shine_button, *shadow_button, *yellow_button, *poppen;
Object *group;
Object *editor_text;
Object *filename_string;
Object *save_button;
Object *list2;

Object *drawer_iconlist;
Object *volume_iconlist;

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

AROS_UFH0(void, about_function)
{
    AROS_USERFUNC_INIT

    static Object *about_wnd;

    if (!about_wnd)
    {
        about_wnd = AboutmuiObject, MUIA_Aboutmui_Application, app, End;
    }

    if (about_wnd)
        set(about_wnd, MUIA_Window_Open, TRUE);

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, changepen_function)
{
    AROS_USERFUNC_INIT

    SIPTR pen;

    GET(colorfield_pen, MUIA_String_Integer, &pen);
    if (pen >= 0)
        SET(colorfield, MUIA_Colorfield_Pen, pen);

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, ChangePendisplayPen)
{
    AROS_USERFUNC_INIT

    DoMethod(pendisplay, MUIM_Pendisplay_SetColormap,
        XGET(pendisplay_pen, MUIA_String_Integer));

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, display_function,
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

AROS_UFH3(void, display2_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, strings, A2),
    AROS_UFHA(struct list_entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    static char buf[100];

    if (entry)
    {
        sprintf(buf, "line num: %ld  id: %p", (long)*(strings - 1), entry);
        strings[0] = buf;
    }
    else
    {
        strings[0] = "Number";
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, save_function)
{
    AROS_USERFUNC_INIT

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

    AROS_USERFUNC_EXIT
}

static int id = 1;

AROS_UFH0(void, add_function)
{
    AROS_USERFUNC_INIT

    DoMethod(list2, MUIM_List_InsertSingle, id++,
        MUIV_List_Insert_Bottom);

    AROS_USERFUNC_EXIT
}

#if defined(TEST_ICONLIST)
/* IconList callbacks */
void volume_doubleclicked(void)
{
    char buf[200];
    struct IconList_Entry *ent = (void *)MUIV_IconList_NextIcon_Start;

    DoMethod(volume_iconlist, MUIM_IconList_NextIcon,
        MUIV_IconList_NextIcon_Selected, &ent);
    if ((IPTR) ent == MUIV_IconList_NextIcon_End)
        return;

    strcpy(buf, ent->label);
    strcat(buf, ":");
    set(drawer_iconlist, MUIA_IconDrawerList_Drawer, buf);
}

void drawer_doubleclicked(void)
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
                /* no MUIM_SetAsString yet */
            set(obj, MUIA_Text_Contents, buf);
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);

    AROS_USERFUNC_EXIT
}

struct MUI_CustomClass *CL_DropText;

#define DropTextObject BOOPSIOBJMACRO_START(CL_DropText->mcc_Class)

/* Main prog */

static struct Hook hook_standard;

AROS_UFH3(void, hook_func_standard,
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
    Object *wnd, *second_wnd;
    Object *open_button;
    Object *about_button;
    Object *quit_button;
    Object *repeat_button;
    Object *objects_button;
    Object *about_item, *quit_item;
    Object *context_menu;
    Object *popobject, *listview;
    Object *lists[LIST_COUNT];
    Object *list_add_button, *list_remove_button, *list_clear_button;
    Object *dragsortable_check, *showdropmarks_check, *quiet_check;
    Object *sort_button, *clear_button, *fill_button, *enable_button;
    Object *multi_lists[MULTI_LIST_COUNT];
    Object *format_string;
    Object *column_string;
    Object *column_text;
    Object *showheadings_check;
    Object *numerics[NUMERIC_COUNT];
    Object *min_string, *max_string;
    Object *slider_button;
    Object *country_radio[2];
    UWORD i;

    static char *pages[] =
        {"General", "Text", "Boopsi", "Color", "Edit", "List", "Gauges",
            "Numeric", "Radio",
#if defined(TEST_ICONLIST)
            "Icon List",
#endif
            "Balancing", NULL};
    static char *color_pages[] =
        {"Palette", "Colors", "Pens", NULL};
    static char *list_pages[] =
        {"Single Column (1)", "Single Column (2)", "Multicolumn", NULL};
    static char **radio_entries1 = pages;
    static char *radio_entries2[] =
        {"Paris", "Pataya", "London", "New York", "Reykjavik", NULL};
    static char *fruits[] = {"Strawberry", "Apple", "Banana", "Orange",
        "Grapefruit", "Kumquat", "Plum", "Raspberry", "Apricot", "Grape",
        "Peach", "Lemon", "Lime", "Date", "Pineapple", "Blueberry", "Papaya",
        "Cranberry", "Gooseberry", "Pear", "Fig", "Coconut", "Melon",
        "Pumpkin", NULL};

    static struct list_entry entry1 = {"Testentry1", "Col2: Entry1"};
    static struct list_entry entry2 = {"Entry2", "Col2: Entry2"};
    static struct list_entry entry3 = {"Entry3", "Col2: Entry3"};
    static struct list_entry entry4 = {"Entry4", "Col2: Entry4"};
    static struct list_entry entry5 = {"Entry5", "Col2: Entry5"};
    static struct list_entry entry6 = {"Entry6", "Col2: Entry6"};

    static struct list_entry *entries[] =
        {&entry1, &entry2, &entry3, &entry4, &entry5, &entry6, NULL};

    struct Hook hook;
    struct Hook hook_wheel;
    struct Hook hook_slider;
    struct Hook hook_objects;
    struct Hook hook_display;
    struct Hook hook_display2;

    hook_standard.h_Entry = (HOOKFUNC) hook_func_standard;

    MUIMasterBase = (struct Library *)OpenLibrary("muimaster.library", 0);

    hook.h_Entry = (HOOKFUNC) repeat_function;
    hook_wheel.h_Entry = (HOOKFUNC) wheel_function;
    hook_slider.h_Entry = (HOOKFUNC) slider_function;
    hook_objects.h_Entry = (HOOKFUNC) objects_function;
    hook_display.h_Entry = (HOOKFUNC) display_function;
    hook_display2.h_Entry = (HOOKFUNC) display2_function;

    context_menu = MenuitemObject,
        MUIA_Family_Child, MenuitemObject,
            MUIA_Menuitem_Title, "Menutest",
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

    app = ApplicationObject,
        MUIA_Application_Menustrip, MenuitemObject,
            MUIA_Family_Child, MenuitemObject,
                MUIA_Menuitem_Title, "Project",
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
                    Child, VGroup,
                        Child, TextObject,
                            MUIA_Background, "2:cfffffff,cfffffff,10000000",
                            TextFrame,
                            MUIA_Text_Contents, "\33cHello World!\n"
                                "This is a text object\n\33lLeft \33bbold\33n\n"
                                "\33rRight",
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
                                    Child, lists[0] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_Title, "Fruits",
                                            MUIA_List_SourceArray, fruits,
                                            MUIA_ShortHelp, "Default scroller",
                                            End,
                                        MUIA_Listview_MultiSelect,
                                            MUIV_Listview_MultiSelect_None,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Default Multiselect"),
                                    Child, lists[1] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_SourceArray, fruits,
                                            MUIA_ShortHelp, "Left scroller",
                                            End,
                                        MUIA_Listview_ScrollerPos,
                                            MUIV_Listview_ScrollerPos_Left,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Shifted Multiselect"),
                                    Child, lists[2] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_SourceArray, fruits,
                                            MUIA_ShortHelp, "Right scroller",
                                            End,
                                        MUIA_Listview_MultiSelect,
                                            MUIV_Listview_MultiSelect_Shifted,
                                        MUIA_Listview_ScrollerPos,
                                            MUIV_Listview_ScrollerPos_Right,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Unconditional Multiselect"),
                                    Child, lists[3] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_SourceArray, fruits,
                                            MUIA_ShortHelp, "No scroller",
                                            End,
                                        MUIA_Listview_MultiSelect,
                                            MUIV_Listview_MultiSelect_Always,
                                        MUIA_Listview_ScrollerPos,
                                            MUIV_Listview_ScrollerPos_None,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                Child, VGroup,
                                    GroupFrameT("Read Only"),
                                    Child, lists[4] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            ReadListFrame,
                                            MUIA_List_SourceArray, fruits,
                                            MUIA_ShortHelp, "Default scroller",
                                            End,
                                        MUIA_Listview_Input, FALSE,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                End,
                            Child, RectangleObject,
                                MUIA_VertWeight, 0,
                                MUIA_Rectangle_HBar, TRUE,
                                MUIA_Rectangle_BarTitle, "List Controls",
                                End,
                            Child, HGroup,
                                Child, sort_button =
                                    MUI_MakeObject(MUIO_Button, "Sort"),
                                Child, clear_button =
                                    MUI_MakeObject(MUIO_Button, "Clear"),
                                Child, fill_button =
                                    MUI_MakeObject(MUIO_Button, "Fill"),
                                Child, enable_button =
                                    MUI_MakeObject(MUIO_Button, "Enable"),
                                End,
                            Child, HGroup,
                                Child, HGroup,
                                    Child, dragsortable_check =
                                        MUI_MakeObject(MUIO_Checkmark, NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Drag sortable", 0),
                                    Child, HVSpace,
                                    End,
                                Child, HGroup,
                                    Child, showdropmarks_check =
                                        MUI_MakeObject(MUIO_Checkmark, NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Show drop marks", 0),
                                    Child, HVSpace,
                                    End,
                                Child, HGroup,
                                    Child, quiet_check =
                                        MUI_MakeObject(MUIO_Checkmark,NULL),
                                    Child, MUI_MakeObject(MUIO_Label,
                                        "Quiet", 0),
                                    Child, HVSpace,
                                    End,
                                End,
                            End,
                        Child, VGroup,
                            Child, ListviewObject,
                                MUIA_Listview_List, list2 = ListObject,
                                    InputListFrame,
                                    MUIA_List_DisplayHook, &hook_display2,
                                    End,
                                End,
                            Child, HGroup,
                                Child, list_add_button =
                                    MUI_MakeObject(MUIO_Button,"_Add"),
                                Child, list_remove_button =
                                    MUI_MakeObject(MUIO_Button,"_Remove"),
                                Child, list_clear_button =
                                    MUI_MakeObject(MUIO_Button,"_Clear"),
                                End,
                            End,
                        Child, VGroup,
                            Child, ColGroup(LIST_COUNT / 2),
                                Child, VGroup, GroupFrameT("Standard Format"),
                                    Child, multi_lists[0] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_DisplayHook,
                                                &hook_display,
                                            MUIA_List_Format, list_format,
                                            MUIA_List_SourceArray, entries,
                                            MUIA_List_Title, TRUE,
                                            End,
                                        MUIA_Listview_MultiSelect,
                                            MUIV_Listview_MultiSelect_None,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                Child, VGroup, GroupFrameT("Custom Format"),
                                    Child, multi_lists[1] = ListviewObject,
                                        MUIA_Listview_List,
                                            ListObject,
                                            InputListFrame,
                                            MUIA_List_DisplayHook,
                                                &hook_display,
                                            MUIA_List_Format, list_format,
                                            MUIA_List_SourceArray, entries,
                                            MUIA_List_Title, TRUE,
                                            End,
                                        MUIA_Listview_MultiSelect,
                                            MUIV_Listview_MultiSelect_None,
                                        MUIA_CycleChain, 1,
                                        End,
                                    End,
                                End,
                            Child, RectangleObject,
                                MUIA_VertWeight, 0,
                                MUIA_Rectangle_HBar, TRUE,
                                MUIA_Rectangle_BarTitle, "List Controls",
                                End,
                            Child, HGroup,
                                Child, MUI_MakeObject(MUIO_Label, "Format:", 0),
                                Child, format_string = StringObject,
                                    StringFrame,
                                    MUIA_String_Contents, list_format,
                                    MUIA_CycleChain, 1,
                                    End,
                                Child, showheadings_check =
                                    MUI_MakeObject(MUIO_Checkmark, NULL),
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Show column headings", 0),
                                End,
                            Child, HGroup,
                                Child, MUI_MakeObject(MUIO_Label,
                                    "Clicked column:", 0),
                                Child, column_string = StringObject,
                                    MUIA_ShowMe, FALSE,
                                    End,
                                Child, column_text = TextObject,
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
                                MUIA_String_Accept, (IPTR)"-0123456789",
                                MUIA_String_Integer, NUMERIC_MIN,
                                MUIA_CycleChain, 1,
                                End,
                            Child, MUI_MakeObject(MUIO_Label,
                                "Maximum Value:", 0),
                            Child, max_string = (Object *)StringObject,
                                StringFrame,
                                MUIA_String_Accept, (IPTR)"-0123456789",
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
            MUIM_CallHook, &hook_standard, save_function);

        DoMethod(quit_item, MUIM_Notify, MUIA_Menuitem_Trigger,
            MUIV_EveryTime, app, 2, MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit);
        DoMethod(about_item, MUIM_Notify, MUIA_Menuitem_Trigger,
            MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard,
            about_function);

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
            changepen_function);
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

        /* list */
        set(showdropmarks_check, MUIA_Selected, TRUE);
        for (i = 0; i < LIST_COUNT; i++)
        {
            DoMethod(lists[i], MUIM_Notify, MUIA_Listview_DoubleClick,
                MUIV_EveryTime, lists[i], 3, MUIM_Set, MUIA_Disabled, TRUE);
            DoMethod(sort_button, MUIM_Notify, MUIA_Pressed, FALSE,
                lists[i], 1, MUIM_List_Sort);
            DoMethod(clear_button, MUIM_Notify, MUIA_Pressed, FALSE,
                lists[i], 1, MUIM_List_Clear);
            DoMethod(fill_button, MUIM_Notify, MUIA_Pressed, FALSE,
                lists[i], 4, MUIM_List_Insert, fruits, -1,
                MUIV_List_Insert_Bottom);
            DoMethod(enable_button, MUIM_Notify, MUIA_Pressed, FALSE,
                lists[i], 3, MUIM_Set, MUIA_Disabled, FALSE);
            DoMethod(dragsortable_check, MUIM_Notify, MUIA_Selected,
                MUIV_EveryTime, lists[i], 3, MUIM_Set,
                MUIA_List_DragSortable, MUIV_TriggerValue);
            DoMethod(showdropmarks_check, MUIM_Notify, MUIA_Selected,
                MUIV_EveryTime, lists[i], 3, MUIM_Set,
                MUIA_List_ShowDropMarks, MUIV_TriggerValue);
            DoMethod(quiet_check, MUIM_Notify, MUIA_Selected,
                MUIV_EveryTime, lists[i], 3, MUIM_Set, MUIA_List_Quiet,
                MUIV_TriggerValue);
        }

        set(showheadings_check, MUIA_Selected, TRUE);
        DoMethod(format_string, MUIM_Notify, MUIA_String_Acknowledge,
            MUIV_EveryTime, multi_lists[1], 3, MUIM_Set, MUIA_List_Format,
            MUIV_TriggerValue);
        for (i = 0; i < MULTI_LIST_COUNT; i++)
        {
            DoMethod(showheadings_check, MUIM_Notify, MUIA_Selected,
                MUIV_EveryTime, multi_lists[i], 3, MUIM_Set,
                MUIA_List_Title, MUIV_TriggerValue);
            DoMethod(multi_lists[i], MUIM_Notify, MUIA_Listview_ClickColumn,
                MUIV_EveryTime, column_string, 3, MUIM_Set,
                MUIA_String_Integer, MUIV_TriggerValue);
        }
        DoMethod(column_string, MUIM_Notify, MUIA_String_Contents,
            MUIV_EveryTime, column_text, 3, MUIM_Set, MUIA_Text_Contents,
            MUIV_TriggerValue);

        DoMethod(listview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            popobject, 2, MUIM_Popstring_Close, TRUE);

        /* The callbacks of the buttons within the list page */
        DoMethod(list_add_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3,
            MUIM_CallHook, &hook_standard, add_function);
        DoMethod(list_remove_button, MUIM_Notify, MUIA_Pressed, FALSE,
            list2, 2, MUIM_List_Remove, MUIV_List_Remove_Active);
        DoMethod(list_clear_button, MUIM_Notify, MUIA_Pressed, FALSE, list2,
            1, MUIM_List_Clear);

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

        set(wnd, MUIA_Window_Open, TRUE);
        set(wnd, MUIA_Window_ScreenTitle, "Zune Test application");

        /* Set pen fields */
        set(colorfield_pen, MUIA_String_Integer,
            XGET(colorfield, MUIA_Colorfield_Pen));

        char pen_str[10];
        nnset(pendisplay_pen, MUIA_String_Integer,
            XGET(pendisplay, MUIA_Pendisplay_Pen));

        struct MUI_PenSpec *pen_spec;
        GET(pendisplay, MUIA_Pendisplay_Spec, &pen_spec);
        strncpy(pen_str, pen_spec->ps_buf, 10);
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
    return 0;
}
