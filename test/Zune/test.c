/*
    Copyright © 2002, The AROS Development Team.
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
** constits of the public constants and members. Actually this is easiey
*/

#include <libraries/mui.h>

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
    
    nnset(r_slider,MUIA_Numeric_Value, (XGET(wheel,WHEEL_Red) >> 24) & 0xff);
    nnset(g_slider,MUIA_Numeric_Value, (XGET(wheel,WHEEL_Green) >> 24) & 0xff);
    nnset(b_slider,MUIA_Numeric_Value, (XGET(wheel,WHEEL_Blue) >> 24) & 0xff);

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, slider_function)
{
    AROS_USERFUNC_INIT
    
    struct ColorWheelRGB cw;
    ULONG red = XGET(r_slider,MUIA_Numeric_Value);
    ULONG green = XGET(g_slider,MUIA_Numeric_Value);
    ULONG blue = XGET(b_slider,MUIA_Numeric_Value);

    cw.cw_Red = (red<<24)|(red<<16)|(red<<8)|red;
    cw.cw_Green = (green<<24)|(green<<16)|(green<<8)|green;
    cw.cw_Blue = (blue<<24)|(blue<<16)|(blue<<8)|blue;

    nnset(wheel, WHEEL_RGB, &cw);
    set(hue_gauge, MUIA_Gauge_Current, XGET(wheel,WHEEL_Hue));

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, objects_function)
{
    AROS_USERFUNC_INIT
    
    Object *new_obj = MUI_MakeObject(MUIO_Button,"Button");
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
	about_wnd = AboutmuiObject,
		MUIA_Aboutmui_Application, app,
		End;
    }

    if (about_wnd) set(about_wnd,MUIA_Window_Open,TRUE);

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
    	sprintf(buf,"%ld", (LONG) *(strings-1));
        strings[0] = buf;
        strings[1] = entry->column1;
        strings[2] = entry->column2;
    } else
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
    	sprintf(buf,"line num: %ld  id: %p", (LONG) *(strings-1), entry);
        strings[0] = buf;
    } else
    {
        strings[0] = "Number";
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, save_function)
{
    AROS_USERFUNC_INIT

    char *text = (char*)XGET(editor_text, MUIA_Text_Contents);
    char *filename = (char*)XGET(filename_string, MUIA_String_Contents);
    BPTR fh;

    if (!strlen(filename)) return;

    if ((fh = Open(filename,MODE_NEWFILE)))
    {
    	Write(fh,text,strlen(text));
        Close(fh);
    }

    AROS_USERFUNC_EXIT
}

static int id = 1;

AROS_UFH0(void, add_function)
{
    AROS_USERFUNC_INIT

    DoMethod(list2,MUIM_List_InsertSingle, id++, MUIV_List_Insert_Bottom);
    
    AROS_USERFUNC_EXIT
}

AROS_UFH0(void, add_child_function)
{
    AROS_USERFUNC_INIT

    int act = XGET(list2,MUIA_List_Active);

    DoMethod(list2,MUIM_List_InsertSingleAsTree, id++, act /* parent */, MUIV_List_InsertSingleAsTree_Bottom, 0);

    AROS_USERFUNC_EXIT
}

/* IconList callbacks */
void volume_doubleclicked(void)
{
    char buf[200];
    struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
    DoMethod(volume_iconlist, MUIM_IconList_NextSelected, &ent);
    if ((int)ent == MUIV_IconList_NextSelected_End) return;

    strcpy(buf,ent->label);
    strcat(buf,":");
    set(drawer_iconlist,MUIA_IconDrawerList_Drawer,buf);
}

void drawer_doubleclicked(void)
{
    struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;

    DoMethod(drawer_iconlist, MUIM_IconList_NextSelected, &ent);
    if ((int)ent == MUIV_IconList_NextSelected_End) return;
    set(drawer_iconlist,MUIA_IconDrawerList_Drawer,ent->filename);
}

static IPTR create_balance_column(void)
{
    return 	(IPTR)  VGroup,
	   Child, RectangleObject, GaugeFrame, End,
Child, BalanceObject, End,
	   Child, RectangleObject, GaugeFrame, End,
Child, BalanceObject, End,
	   Child, RectangleObject, GaugeFrame, End,
Child, BalanceObject, End,
	   Child, RectangleObject, GaugeFrame, End,
Child, BalanceObject, End,
	   Child, RectangleObject, GaugeFrame, End,
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
__saveds __asm IPTR dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    AROS_USERFUNC_INIT
    
    switch (msg->MethodID)
    {
    	case MUIM_DragQuery: return MUIV_DragQuery_Accept;
    	case MUIM_DragDrop:
    	{
	    struct DropText_Data *data = (struct DropText_Data*)INST_DATA(cl,obj);
	    char buf[100];
	    data->times++;
	    sprintf(buf,"%ld times",data->times); /* no MUIM_SetAsString yet */
	    set(obj,MUIA_Text_Contents,buf);
    	}
    }
    
    return DoSuperMethodA(cl,obj,(Msg)msg);

    AROS_USERFUNC_EXIT
}

struct MUI_CustomClass *CL_DropText;

#define DropTextObject BOOPSIOBJMACRO_START(CL_DropText->mcc_Class)

/* Main prog */

static struct Hook hook_standard;

AROS_UFH3(void, hook_func_standard,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1))
{
    AROS_USERFUNC_INIT

    void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);

    if (func) func((ULONG *) funcptr + 1);

    AROS_USERFUNC_EXIT
}

int main(void)
{
    Object *wnd,*second_wnd;
    Object *open_button;
    Object *quit_button;
    Object *repeat_button;
    Object *objects_button;
    Object *about_item, *quit_item;
    Object *context_menu;
    Object *popobject, *listview;
    Object *list_add_button, *list_add_child_button, *list_remove_button, *list_clear_button;
    Object *country_radio[2];

    static char *pages[] = {"Groups","Colorwheel","Virtual Group","Edit","List","Gauges","Radio","Icon List","Balancing",NULL};
    static char **radio_entries1 = pages;
    static char *radio_entries2[] = {"Paris","Pataya","London","New-York","Reykjavik",NULL};

    static struct list_entry entry1 = {"Testentry1","Col2: Entry1"};
    static struct list_entry entry2 = {"Entry2","Col2: Entry2"};
    static struct list_entry entry3 = {"Entry3","Col2: Entry3"};
    static struct list_entry entry4 = {"Entry4","Col2: Entry4"};
    static struct list_entry entry5 = {"Entry5","Col2: Entry5"};
    static struct list_entry entry6 = {"Entry6","Col2: Entry6"};

    static struct list_entry *entries[] =
    	{&entry1,&entry2,&entry3,&entry4,&entry5,&entry6,NULL};

    struct Hook hook;
    struct Hook hook_wheel;
    struct Hook hook_slider;
    struct Hook hook_objects;
    struct Hook hook_display;
    struct Hook hook_display2;

    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;

    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    hook.h_Entry = (HOOKFUNC)repeat_function;
    hook_wheel.h_Entry = (HOOKFUNC)wheel_function;
    hook_slider.h_Entry = (HOOKFUNC)slider_function;
    hook_objects.h_Entry = (HOOKFUNC)objects_function;
    hook_display.h_Entry = (HOOKFUNC)display_function;
    hook_display2.h_Entry = (HOOKFUNC)display2_function;

    context_menu = MenuitemObject,
	    MUIA_Family_Child, MenuitemObject,
		MUIA_Menuitem_Title, "Menutest",
		MUIA_Family_Child, about_item = MenuitemObject, MUIA_Menuitem_Title, "First Test Entry", End,
		MUIA_Family_Child, quit_item = MenuitemObject, MUIA_Menuitem_Title, "Second Test Entry", End,
		End,
	    End;

    /* should check the result in a real program! */
    CL_DropText = MUI_CreateCustomClass(NULL,MUIC_Text,NULL,sizeof(struct DropText_Data), dispatcher);
    ColorWheelBase = OpenLibrary("gadgets/colorwheel.gadget",0);

    app = ApplicationObject,
	MUIA_Application_Menustrip, MenuitemObject,
	    MUIA_Family_Child, MenuitemObject,
	    	MUIA_Menuitem_Title, "Project",
	    	MUIA_Family_Child, about_item = MenuitemObject, MUIA_Menuitem_Title, "About...", MUIA_Menuitem_Shortcut, "?", End,
	    	MUIA_Family_Child, MenuitemObject, MUIA_Menuitem_Title, ~0, End,
	    	MUIA_Family_Child, quit_item = MenuitemObject, MUIA_Menuitem_Title, "Quit", MUIA_Menuitem_Shortcut, "Q", End,
	    	End,
	    End,
    	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "test",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
    	    	Child, TextObject, MUIA_Background, "2:cfffffff,cfffffff,10000000", TextFrame, MUIA_Text_Contents, "\33cHello World!!\nThis is a text object\n\33lLeft \33bbold\33n\n\33rRight",End,
    	    	Child, popobject = PopobjectObject,
    	    	    MUIA_Popstring_String, MUI_MakeObject(MUIO_String, NULL, 200),
    	    	    MUIA_Popstring_Button, PopButton(MUII_PopUp),
    	    	    MUIA_Popobject_Object, VGroup,
    	    	    	Child, TextObject,MUIA_Text_Contents,"test",End,
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
    	    	Child, RegisterGroup(pages),
//		    MUIA_Background, "5:SYS:Prefs/Presets/Backdrops/StuccoBlue.pic",
		    Child, HGroup,
		        GroupFrameT("A horizontal group"),
		        Child, ColGroup(2),
			    GroupFrameT("A column group"),
			    Child, repeat_button = TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Repeat", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Drag Me", MUIA_Draggable, TRUE, MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, open_button = TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Open Window", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, TextObject, MUIA_ContextMenu, context_menu, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Press Right", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, objects_button = TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Add Objects", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, HVSpace, //TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button6", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, MUI_MakeObject(MUIO_Label,"_Checkmark",0),
			    Child, MUI_MakeObject(MUIO_Checkmark,"_Checkmark"),
			    End,
		        Child, group = VGroup,
			    GroupFrameT("A vertical group"),
			    Child, DropTextObject, MUIA_Dropable, TRUE, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Drop Here", MUIA_Dropable, TRUE, MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, TextObject, TextFrame, MUIA_Background, MUII_MARKHALFSHINE, MUIA_CycleChain, 1, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Pattern", End,
    	    	    	Child, ColoradjustObject, End,

			   End,
			Child, MUI_NewObject(MUIC_Popimage, TAG_DONE),
		       End,

		    Child, VGroup,
		        Child, wheel = BoopsiObject,  /* MUI and Boopsi tags mixed */
		            GroupFrame,
		            MUIA_Boopsi_ClassID  , "colorwheel.gadget",
		            MUIA_Boopsi_MinWidth , 30, /* boopsi objects don't know */
		            MUIA_Boopsi_MinHeight, 30, /* their sizes, so we help   */
		            MUIA_Boopsi_Remember , WHEEL_Saturation, /* keep important values */
		            MUIA_Boopsi_Remember , WHEEL_Hue,        /* during window resize  */
		            MUIA_Boopsi_TagScreen, WHEEL_Screen, /* this magic fills in */
		            WHEEL_Screen         , NULL,         /* the screen pointer  */
		            GA_Left     , 0,
		            GA_Top      , 0, /* MUI will automatically     */
		            GA_Width    , 0, /* fill in the correct values */
		            GA_Height   , 0,
		            ICA_TARGET  , ICTARGET_IDCMP, /* needed for notification */
		            WHEEL_Saturation, 0, /* start in the center */
		            MUIA_FillArea, TRUE, /* use this because it defaults to FALSE
					        for boopsi gadgets but the colorwheel
					        doesnt bother about redrawing its backgorund */
		            End,

		        Child, r_slider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End,
		        Child, g_slider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End,
		        Child, b_slider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 255, End,

			Child, hue_gauge = GaugeObject, GaugeFrame, MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Max, 255, MUIA_Gauge_Divide, 1<<24, MUIA_Gauge_InfoText, "Hue: %ld",End,
		        End,

		    Child, ScrollgroupObject,
			MUIA_Scrollgroup_Contents, VGroupV,
			    VirtualFrame,
			    Child, TextObject,
				TextFrame,
				MUIA_Text_Contents, "Line1\nLine2\nLine3\nLine4\nLine5\nLine6\nLine7\nLine8\n\n\n\nLine9\nLine10\nLine11\n",
				End,
			    Child, HGroup,
				Child, MUI_MakeObject(MUIO_Button,"Button9"),
				Child, MUI_MakeObject(MUIO_Button,"Button10"),
				End,
			    End,
		    	End,
		    Child, VGroup,
			Child, editor_text = StringObject,
			    StringFrame,
			    MUIA_Text_Multiline, TRUE,
			    End,
		    	Child, PopaslObject,
		    	    ASLFR_DoSaveMode, TRUE,
    		    	    MUIA_Popstring_String, filename_string = MUI_MakeObject(MUIO_String, NULL, 200),
    	    		    MUIA_Popstring_Button, PopButton(MUII_PopFile),
			    End,
			Child, save_button = MUI_MakeObject(MUIO_Button, "Save"),
			End,
		    Child, VGroup,
		    	Child, ListviewObject,
			    MUIA_Listview_List, list2 = ListObject,
				InputListFrame,
			        MUIA_List_DisplayHook, &hook_display2,
			    	End,
			    End,
			Child, HGroup,
			    Child, list_add_button = MUI_MakeObject(MUIO_Button,"_Add"),
			    Child, list_add_child_button = MUI_MakeObject(MUIO_Button,"_Add Child"),
			    Child, list_remove_button = MUI_MakeObject(MUIO_Button,"_Remove"),
			    Child, list_clear_button = MUI_MakeObject(MUIO_Button,"_Clear"),
			    End,
			End,

		    /* gauges */
		    Child, HGroup,
	                Child, VGroup,
	                    Child, VGroup, GroupFrame,
		                Child, GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 25, End,
		                Child, ScaleObject, End,
	                        End,
	                    Child, VGroup, GroupFrame,
		                Child, GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 50, End,
		                Child, ScaleObject, End,
	                        End,
	                    Child, VGroup, GroupFrame,
		                Child, GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 75, End,
		                Child, ScaleObject, End,
	                        End,
	                    End,
	                Child, HGroup,
			    Child, HVSpace,
		            Child, GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Current, 25, End,
		            Child, GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Current, 50, End,
		            Child, GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Current, 75, End,
			    Child, HVSpace,
	                    End,
		        End,

		    /* radios */
		    Child, HGroup,
	                Child, VGroup, 
		            Child, RadioObject, GroupFrame, MUIA_Radio_Entries, radio_entries1, End,
		            Child, country_radio[0] = RadioObject, GroupFrame, MUIA_Radio_Entries, radio_entries2, MUIA_Radio_Active, 1, End,
	                    End,
	                Child, HGroup,
		            Child, RadioObject, GroupFrame, MUIA_Radio_Entries, radio_entries1, End,
		            Child, country_radio[1] = RadioObject, GroupFrame, MUIA_Radio_Entries, radio_entries2, MUIA_Radio_Active, 1, End,
	                    End,
		        End,
		    /* iconlist */
	            Child, HGroup,
	            	Child, volume_iconlist = MUI_NewObject(MUIC_IconVolumeList, GroupFrame, TAG_DONE),
	            	Child, drawer_iconlist = MUI_NewObject(MUIC_IconDrawerList, GroupFrame, MUIA_IconDrawerList_Drawer,"SYS:",TAG_DONE),
	            	End,

	Child,HGroup,

	Child, create_balance_column(),
Child, BalanceObject, End,
	  Child, create_balance_column(),
Child, BalanceObject, End,

	  Child, create_balance_column(),
Child, BalanceObject, End,

	  Child, create_balance_column(),
Child, BalanceObject, End,

	  Child, create_balance_column(),

	End,
	End,
	
		Child, RectangleObject,
		    MUIA_VertWeight,0, /* Seems to be not supported properly as orginal MUI doesn't allow to alter the height of the window */
		    MUIA_Rectangle_HBar, TRUE,
		    MUIA_Rectangle_BarTitle,"Enter a string",
		    End,

		Child, StringObject,
		    StringFrame,
		    MUIA_CycleChain,1,
		    MUIA_String_AdvanceOnCR, TRUE,
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

		Child, HGroup,
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
                    MUIA_Dropable, TRUE,  End,
		End,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(second_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, second_wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);
	DoMethod(open_button, MUIM_Notify, MUIA_Pressed, FALSE, second_wnd, 3,  MUIM_Set, MUIA_Window_Open, TRUE);
	DoMethod(quit_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(objects_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_CallHook, &hook_objects);
	DoMethod(repeat_button, MUIM_Notify, MUIA_Timer, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook);

	DoMethod(wheel, MUIM_Notify,WHEEL_Hue       , MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_wheel);
	DoMethod(wheel, MUIM_Notify,WHEEL_Saturation, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_wheel);
	DoMethod(r_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_slider);
	DoMethod(g_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_slider);
	DoMethod(b_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_slider);

	DoMethod(save_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, save_function);

	DoMethod(quit_item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(about_item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard, about_function);

	DoMethod(listview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, popobject, 2, MUIM_Popstring_Close, TRUE);

        /* The callbacks of the buttons within the list page */
	DoMethod(list_add_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, add_function);
	DoMethod(list_add_child_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, add_child_function);
        DoMethod(list_remove_button, MUIM_Notify, MUIA_Pressed, FALSE, list2, 2, MUIM_List_Remove, MUIV_List_Remove_Active);
	DoMethod(list_clear_button, MUIM_Notify, MUIA_Pressed, FALSE, list2, 1, MUIM_List_Clear);

	/* radio */
	DoMethod(country_radio[0], MUIM_Notify, MUIA_Radio_Active, MUIV_EveryTime, country_radio[1], 3, MUIM_NoNotifySet, MUIA_Radio_Active, MUIV_TriggerValue);
	DoMethod(country_radio[1], MUIM_Notify, MUIA_Radio_Active, MUIV_EveryTime, country_radio[0], 3, MUIM_NoNotifySet, MUIA_Radio_Active, MUIV_TriggerValue);

        /* iconlist */
        DoMethod(volume_iconlist, MUIM_Notify, MUIA_IconList_DoubleClick, TRUE, volume_iconlist, 3, MUIM_CallHook, &hook_standard, volume_doubleclicked);
        DoMethod(drawer_iconlist, MUIM_Notify, MUIA_IconList_DoubleClick, TRUE, drawer_iconlist, 3, MUIM_CallHook, &hook_standard, drawer_doubleclicked);

	set(wnd,MUIA_Window_Open,TRUE);
	set(wnd,MUIA_Window_ScreenTitle, "Zune Test application");

	while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}

	MUI_DisposeObject(app);
    }
    if (context_menu) MUI_DisposeObject(context_menu);
    CloseLibrary(ColorWheelBase);
    MUI_DeleteCustomClass(CL_DropText);

    CloseLibrary(MUIMasterBase);
    return 0;
}
