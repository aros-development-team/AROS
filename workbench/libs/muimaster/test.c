#include <stdio.h>

#include <clib/alib_protos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

/* define this if this should be compiled with mui */
#ifdef COMPILE_WITH_MUI

#include <libraries/mui.h>
#include <proto/muimaster.h>

typedef unsigned long IPTR;

#else

/* the following should go in a single include file which then only
** constits of the public constants and members. Actually this is easiey
*/

#include "mui.h"

/* muimaster.library is not yet a library */
#include "muimaster_intern.h"

struct MUIMasterBase_intern MUIMasterBase_instance;

#endif

struct Library *MUIMasterBase;
struct Library *ColorWheelBase;


Object *wheel;
Object *r_slider;
Object *g_slider;
Object *b_slider;

ULONG xget(Object *obj, Tag attr)
{
  ULONG storage;
  GetAttr(attr, obj, &storage);
  return storage;
}

__saveds void repeat_function(void)
{
    printf("MUI_Timer\n");
}

__saveds void wheel_function(void)
{
    nnset(r_slider,MUIA_Numeric_Value, (xget(wheel,WHEEL_Red) >> 24) & 0xff);
    nnset(g_slider,MUIA_Numeric_Value, (xget(wheel,WHEEL_Green) >> 24) & 0xff);
    nnset(b_slider,MUIA_Numeric_Value, (xget(wheel,WHEEL_Blue) >> 24) & 0xff);
}

__saveds void slider_function(void)
{
    struct ColorWheelRGB cw;
    ULONG red = xget(r_slider,MUIA_Numeric_Value);
    ULONG green = xget(g_slider,MUIA_Numeric_Value);
    ULONG blue = xget(b_slider,MUIA_Numeric_Value);

    cw.cw_Red = (red<<24)|(red<<16)|(red<<8)|red;
    cw.cw_Green = (green<<24)|(green<<16)|(green<<8)|green;
    cw.cw_Blue = (blue<<24)|(blue<<16)|(blue<<8)|blue;

    nnset(wheel, WHEEL_RGB, &cw);
}

/* The custom class */

struct DropText_Data
{
    ULONG times;
};

#ifndef _AROS
__saveds __asm IPTR dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
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
}

struct MUI_CustomClass *CL_DropText;

#define DropTextObject (Object*)NewObject(CL_DropText->mcc_Class, NULL


/* Main prog */

void main(void)
{
    Object *app;
    Object *wnd,*second_wnd;
    Object *open_button;
    Object *quit_button;
    Object *repeat_button;
    Object *about_item, *quit_item;
    Object *context_menu;

    static char *pages[] = {"Groups","Colorwheel","Virtual Group",NULL};

    struct Hook hook;
    struct Hook hook_wheel;
    struct Hook hook_slider;

#ifndef COMPILE_WITH_MUI
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase = (APTR)OpenLibrary("dos.library",37);
    MUIMasterBase_instance.utilitybase = (APTR)OpenLibrary("utility.library",37);
    MUIMasterBase_instance.aslbase = OpenLibrary("asl.library",37);
    MUIMasterBase_instance.gfxbase = (APTR)OpenLibrary("graphics.library",37);
    MUIMasterBase_instance.layersbase = OpenLibrary("layers.library",37);
    MUIMasterBase_instance.intuibase = (APTR)OpenLibrary("intuition.library",37);
    MUIMasterBase_instance.cxbase = OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase = OpenLibrary("keymap.library",37);
    MUIMasterBase_instance.gadtoolsbase = OpenLibrary("gadtools.library",37);
    __zune_prefs_init(&__zprefs);
#else
    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);
#endif

    hook.h_Entry = (HOOKFUNC)repeat_function;
    hook_wheel.h_Entry = (HOOKFUNC)wheel_function;
    hook_slider.h_Entry = (HOOKFUNC)slider_function;

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
    	    	Child, TextObject, TextFrame, MUIA_Text_Contents, "\33cHello World!!\nThis is a text object\n\33lLeft \33bbold\33n\n\33rRight",End,
    	    	Child, RegisterGroup(pages),
		    Child, HGroup,
		        GroupFrameT("A horizontal group"),
		        Child, ColGroup(2),
			    GroupFrameT("A column group"),
			    Child, repeat_button = TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Repeat", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Drag Me", MUIA_Draggable, TRUE, MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, open_button = TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Open Window", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, TextObject, MUIA_ContextMenu, context_menu, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Press Right", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button5", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, HVSpace, //TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button6", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    End,
		        Child, VGroup,
			    GroupFrameT("A vertical group"),
			    Child, DropTextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Drop Here", MUIA_Dropable, TRUE, MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			    Child, TextObject, MUIA_CycleChain, 1, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button8", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			   End,
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
		        End,
		    Child, VGroupV,
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
		Child, DropTextObject, MUIA_Text_Contents, "Drop here", MUIA_Dropable, TRUE, End,
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
	DoMethod(repeat_button, MUIM_Notify, MUIA_Timer, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook);

	DoMethod(wheel, MUIM_Notify,WHEEL_Hue       , MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_wheel);
	DoMethod(wheel, MUIM_Notify,WHEEL_Saturation, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_wheel);
	DoMethod(r_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_slider);
	DoMethod(g_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_slider);
	DoMethod(b_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, app, 2, MUIM_CallHook, &hook_slider);

	DoMethod(quit_item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	set(wnd,MUIA_Window_Open,TRUE);

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

#ifdef COMPILE_WITH_MUI
    CloseLibrary(MUIMasterBase);
#endif
}

// ---- old test -------

#if 0
__asm __saveds void hook_function(register __a1 int *pval)
{
    printf("get notification of the userdata: %ld\n",*pval);
}

void main(void)
{
    struct Hook hook;
    Object *notify;

    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.utilitybase = OpenLibrary("utility.library",37);

    printf("Creating object\n");
    notify = MUI_NewObjectA(MUIC_Notify, NULL);

    if (notify)
    {
	LONG ud;
	printf("Object created at 0x%lx. Now setting MUIA_UserData to 10\n",notify);
	set(notify, MUIA_UserData, 10);
	get(notify, MUIA_UserData, &ud);
	printf("UserData is now %ld\n",ud);

	printf("Create a notify for the MUIA_UserData tag\n");
	hook.h_Entry = (HOOKFUNC)hook_function;
	DoMethod(notify, MUIM_Notify, MUIA_UserData, MUIV_EveryTime, notify, 3, MUIM_CallHook, &hook, MUIV_TriggerValue);
	printf("Setting User Data to 20\n");
	set(notify, MUIA_UserData, 20);
	printf("Done. Now disposing object\n");

	MUI_DisposeObject(notify);
    }
}

#endif
