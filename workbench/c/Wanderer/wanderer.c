/*
    Copyright � 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <dos/dostags.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef __AROS__
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <aros/debug.h>
#else
#include "mui.h"
ULONG xget(Object *obj, Tag attr);
#endif



#ifndef __AROS__
struct Library *MUIMasterBase;

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"
int openmuimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
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
    MUIMasterBase_instance.iffparsebase = OpenLibrary("iffparse.library",37);
    MUIMasterBase_instance.diskfontbase = OpenLibrary("diskfont.library",37);
    MUIMasterBase_instance.iconbase = OpenLibrary("icon.library",44);
    InitSemaphore(&MUIMasterBase_instance.ZuneSemaphore);

    return 1;
}
void closemuimaster(void)
{
}
#undef SysBase
#undef IntuitionBase
#undef GfxBase
#undef LayersBase
#undef UtilityBase
#else
int openmuimaster(void)
{
/*    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
 */
    return 0;
}
void closemuimaster(void)
{
/*    if (MUIMasterBase) CloseLibrary(MUIMasterBase);*/
}
#endif

enum
{
    MEN_WANDERER = 1,
    MEN_WANDERER_BACKDROP,
    MEN_WANDERER_EXECUTE,
    MEN_WANDERER_SHELL,
    MEN_WANDERER_GUISETTINGS,
    MEN_WANDERER_ABOUT,
    MEN_WANDERER_QUIT,
};

static struct NewMenu nm[] =
{
  {NM_TITLE, "Wanderer"              },
    {NM_ITEM,  "Backdrop",           "B", CHECKIT|MENUTOGGLE|CHECKED, NULL, (void*)MEN_WANDERER_BACKDROP},
    {NM_ITEM,  "Execute Command...", "E", NULL,               NULL, (void*)MEN_WANDERER_EXECUTE},
    {NM_ITEM,  "Redraw All" },
    {NM_ITEM,  "Update All" },
    {NM_ITEM,  "Last Message" },
    {NM_ITEM,  "Shell",              "W", NULL,               NULL, (void*)MEN_WANDERER_SHELL},
    {NM_ITEM,  "GUI Settings...",        NULL, NULL,          NULL, (void*)MEN_WANDERER_GUISETTINGS},
    {NM_ITEM,  "About...",           "?", NULL,               NULL, (void*)MEN_WANDERER_ABOUT},
    {NM_ITEM,  "Quit...",            "Q", NULL,               NULL, (void*)MEN_WANDERER_QUIT},

  {NM_TITLE, "Window",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "New Drawer", "N"},
    {NM_ITEM,  "Open Parent" },
    {NM_ITEM,  "Close", "K"},
    {NM_ITEM,  "Update" },
    {NM_ITEM,  "Select contents", "A"},
    {NM_ITEM,  "Clear selection", "Z"},
    {NM_ITEM,  "Clean Up", "."},
    {NM_ITEM,  "Snapshot" },
      {NM_SUB, "Window"},
      {NM_SUB, "All"},
    {NM_ITEM,  "Show" },
      {NM_SUB, "Only Icons", NULL, CHECKIT | CHECKED, 2},
      {NM_SUB, "All Files", NULL, CHECKIT, 1 },
    {NM_ITEM,  "View By" },
      {NM_SUB, "Icon", NULL, CHECKIT | CHECKED, 2 + 4 + 8},
      {NM_SUB, "Name",NULL, CHECKIT, 1 + 4 + 8},
      {NM_SUB, "Size",NULL, CHECKIT, 1 + 2 + 8},
      {NM_SUB, "Date", NULL, CHECKIT, 1 + 2 + 4},

  {NM_TITLE, "Icon",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "Open", "O"},
    {NM_ITEM,  "Close","C" },
    {NM_ITEM,  "Rename...", "R"},
    {NM_ITEM,  "Information...", "I" },
    {NM_ITEM,  "Snapshot", "S" },
    {NM_ITEM,  "Unsnapshot", "U" },
    {NM_ITEM,  "Leave Out", "L" },
    {NM_ITEM,  "Put Away", "P" },
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM,  "Delete..." },
    {NM_ITEM,  "Format Disk..." },
    {NM_ITEM,  "Empty Trash..." },

  {NM_TITLE, "Tools",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "ResetWanderer" },
  {NM_END}

};

/**************************************************************************
 This is the standard_hook for easy MUIM_CallHook callbacks
 It is initialized at the very beginning of the main program
**************************************************************************/
static struct Hook hook_standard;


#ifndef __AROS__
__asm __saveds void hook_func_standard(register __a0 struct Hook *h, register __a2 void *dummy, register __a1 **funcptr)
#else
AROS_UFH3(void, hook_func_standard,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1))
#endif
{
	void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);

	if (func)
		func((ULONG *)(funcptr + 1));
}

/**************************************************************************
 Easily call the new method of super class with additional tags
**************************************************************************/
STATIC ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
  return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

/**************************************************************************
 This function returns a Menu Object with the given id
**************************************************************************/
Object *FindMenuitem(Object* strip, int id)
{
    return (Object*)DoMethod(strip, MUIM_FindUData, id);
}

/**************************************************************************
 This connects a notify to the given menu entry id
**************************************************************************/
VOID DoMenuNotify(Object* strip, int id, void *function, void *arg)
{
    Object *entry;
    entry = FindMenuitem(strip,id);
    if (entry)
    {
	DoMethod(entry, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, entry, 4, MUIM_CallHook, &hook_standard, function, arg);
    }
}

/**************************************************************************
 Returns the screen title as a static buffer
**************************************************************************/
char *GetScreenTitle(void)
{
    static char title[256];
    /* AROS probably don't have graphics mem but without it look so empty */
    sprintf(title,"Wanderer  %ld graphics mem  %ld other mem",AvailMem(MEMF_CHIP),AvailMem(MEMF_FAST));
    return title;
}

/* Our global variables */

Object *app;
Object *root_iconwnd;
Object *root_menustrip;
Object *execute_wnd;
Object *execute_command_string;

/**************************************************************************
 Easily get attributes. (disabled for AmigaOS version because library is
 a linking library for debugging reasons and contains already xget)
**************************************************************************/
#ifdef __AROS__
ULONG xget(Object *obj, Tag attr)
{
  ULONG storage;
  GetAttr(attr, obj, &storage);
  return storage;
}
#endif

/**************************************************************************
 This is a custom class inheriting from the WindowClass.
**************************************************************************/

#define MUIA_IconWindow_IsRoot       (TAG_USER+0x1631313) /* i.g */
#define MUIA_IconWindow_Drawer       (TAG_USER+0x1631314) /* i.g */
#define MUIA_IconWindow_ActionHook   (TAG_USER+0x1631315) /* i.. */ /* Hook to call when some action happens */
#define MUIA_IconWindow_IsBackdrop   (TAG_USER+0x1631316) /* isg */ /* is Backdrop window ? */
#define MUIA_IconWindow_IconList     (TAG_USER+0x1631317) /* ..g */

#define MUIM_IconWindow_Open         (TAG_USER+0x12908f)
#define MUIM_IconWindow_UnselectAll  (TAG_USER+0x129090)

/* private methods, should be not called from outside */
#define MUIM_IconWindow_DoubleClicked (0x129090)
#define MUIM_IconWindow_IconsDropped (0x129091)
#define MUIM_IconWindow_Clicked (0x129092)

#define ICONWINDOW_ACTION_OPEN 1
#define ICONWINDOW_ACTION_CLICK 2
#define ICONWINDOW_ACTION_ICONDROP 3

struct IconWindow_ActionMsg
{
    int type;
    Object *iconlist;
    int isroot;
    struct IconList_Click *click;
    /* to be continued...*/
};

struct IconWindow_Data
{
    Object *iconlist; /* The iconlist it displays */
    int is_root;
    int is_backdrop;
    struct Hook *action_hook;
};

STATIC IPTR IconWindow_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Hook *action_hook;
    int is_root, is_backdrop;
    struct IconWindow_Data *data;

    Object *iconlist;

    /* More than one GetTagData is not really efficient but since this is called very unoften... */
    is_backdrop = (int)GetTagData(MUIA_IconWindow_IsBackdrop, 0, msg->ops_AttrList);
    is_root = (int)GetTagData(MUIA_IconWindow_IsRoot, 0, msg->ops_AttrList);
    action_hook = (struct Hook*)GetTagData(MUIA_IconWindow_ActionHook, NULL, msg->ops_AttrList);

    if (is_root)
    {
	iconlist = MUI_NewObject(MUIC_IconVolumeList, MUIA_Background, MUII_RegisterBack, TAG_DONE);
    } else
    {
	char *drw = (char*)GetTagData(MUIA_IconWindow_Drawer,NULL,msg->ops_AttrList);
	iconlist = MUI_NewObject(MUIC_IconDrawerList,
				 MUIA_Background, MUII_PageBack,
				 MUIA_IconDrawerList_Drawer, drw, TAG_DONE);
    }

    /* Now call the super methods new method with additional tags */
    obj = (Object*)DoSuperNew(cl,obj,
	MUIA_Window_Width,300,
	MUIA_Window_Height,300,
	MUIA_Window_ScreenTitle, GetScreenTitle(),
	WindowContents, VGroup,
		InnerSpacing(0,0),
		MUIA_Group_Child, MUI_NewObject(MUIC_IconListview, MUIA_IconListview_UseWinBorder, TRUE, MUIA_IconListview_IconList, iconlist, TAG_DONE),
		End,
	TAG_MORE, (IPTR)msg->ops_AttrList);
    if (!obj) return NULL;

    /* Get and initialize the instance's data */
    data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    data->iconlist = iconlist;
    data->is_root = is_root;
    data->action_hook = action_hook;

    data->is_backdrop = -1;
    set(obj, MUIA_IconWindow_IsBackdrop, is_backdrop);

    /* If double clicked then we call our own private methods, that's easier then using Hooks */
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_DoubleClick,TRUE, obj, 1, MUIM_IconWindow_DoubleClicked);
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_IconsDropped, MUIV_EveryTime, obj, 1, MUIM_IconWindow_IconsDropped);
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_Clicked, MUIV_EveryTime, obj, 1, MUIM_IconWindow_Clicked);
    return (IPTR)obj;
}

STATIC IPTR IconWindow_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_IconWindow_IsBackdrop:
		    if ((!!tag->ti_Data) != data->is_backdrop)
		    {
		    	int is_open = xget(obj, MUIA_Window_Open);
		    	if (is_open) set(obj, MUIA_Window_Open, FALSE);
		        if (tag->ti_Data)
		        {
		            SetAttrs(obj,
				MUIA_Window_Title, NULL,
				MUIA_Window_UseBottomBorderScroller, FALSE,
				MUIA_Window_UseRightBorderScroller, FALSE,
				MUIA_Window_WandererBackdrop, TRUE,
				TAG_DONE);
				
		        } else
		        {
			    char *title;
			    if (data->is_root) title = "Wanderer";
			    else title = (char*)xget(data->iconlist, MUIA_IconDrawerList_Drawer);

			    SetAttrs(obj,
				MUIA_Window_Title,title,
				MUIA_Window_UseBottomBorderScroller, TRUE,
				MUIA_Window_UseRightBorderScroller, TRUE,
				MUIA_Window_WandererBackdrop, FALSE,
				TAG_DONE);
			}
		        data->is_backdrop = !!tag->ti_Data;
		    	if (is_open) set(obj, MUIA_Window_Open, TRUE);
		     }
		     break;

	}
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}


STATIC IPTR IconWindow_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    switch (msg->opg_AttrID)
    {
	case	MUIA_IconWindow_Drawer:
		if (!data->is_root)
		    *msg->opg_Storage = xget(data->iconlist,MUIA_IconDrawerList_Drawer);
		else *msg->opg_Storage = NULL;
		return 1;

	case    MUIA_IconWindow_IconList:
		*msg->opg_Storage = (ULONG)data->iconlist;
		return 1;
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC IPTR IconWindow_DoubleClicked(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    if (data->action_hook)
    {
	struct IconWindow_ActionMsg msg;
	msg.type = ICONWINDOW_ACTION_OPEN;
	msg.iconlist = data->iconlist;
	msg.isroot = data->is_root;
	msg.click = NULL;
	CallHookPkt(data->action_hook,obj,&msg);
    }
    return NULL; /* irrelevant */
}

STATIC IPTR IconWindow_Clicked(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);;
    if (data->action_hook)
    {
	struct IconWindow_ActionMsg msg;
	msg.type = ICONWINDOW_ACTION_CLICK;
	msg.iconlist = data->iconlist;
	msg.isroot = data->is_root;
	msg.click = (struct IconList_Click*)xget(data->iconlist, MUIA_IconList_Clicked);
	CallHookPkt(data->action_hook,obj,&msg);
    }
    return NULL; /* irrelevant */
}

STATIC IPTR IconWindow_IconsDropped(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    if (data->action_hook)
    {
	struct IconWindow_ActionMsg msg;
	msg.type = ICONWINDOW_ACTION_ICONDROP;
	msg.iconlist = data->iconlist;
	msg.isroot = data->is_root;
	msg.click = (struct IconList_Click*)xget(data->iconlist, MUIA_IconList_Clicked);
	CallHookPkt(data->action_hook,obj,&msg);
    }
    return NULL;
}

STATIC IPTR IconWindow_Open(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    DoMethod(data->iconlist,MUIM_IconList_Clear);
    set(obj,MUIA_Window_Open,TRUE);
    DoMethod(data->iconlist,MUIM_IconList_Update);
    return 1;
}

STATIC IPTR IconWindow_UnselectAll(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    DoMethod(data->iconlist,MUIM_IconList_UnselectAll);
    return 1;
}

/* Use this macro for dispatchers if you don't want #ifdefs */
BOOPSI_DISPATCHER(IPTR,IconWindow_Dispatcher,cl,obj,msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconWindow_New(cl,obj,(struct opSet*)msg);
	case OM_SET: return IconWindow_Set(cl,obj,(struct opSet*)msg);
	case OM_GET: return IconWindow_Get(cl,obj,(struct opGet*)msg);

	case MUIM_IconWindow_Open: return IconWindow_Open(cl,obj,(APTR)msg);
	case MUIM_IconWindow_UnselectAll: return IconWindow_UnselectAll(cl,obj,(APTR)msg);

	/* private methods */
	case MUIM_IconWindow_DoubleClicked: return IconWindow_DoubleClicked(cl,obj,msg);
	case MUIM_IconWindow_IconsDropped: return IconWindow_IconsDropped(cl,obj,msg);
	case MUIM_IconWindow_Clicked: return IconWindow_Clicked(cl,obj,msg);
    }
    return DoSuperMethodA(cl,obj,msg);
}

struct MUI_CustomClass *CL_IconWindow;

#define IconWindowObject BOOPSIOBJMACRO_START(CL_IconWindow->mcc_Class)


/**************************************************************************
 This is a custom class inheriting from the Application Class.
 We need it support the (periodically) updating of the windows titles and
 we don't want to open timer.device by hand.
**************************************************************************/
struct Wanderer_Data
{
    struct MUI_InputHandlerNode timer_ihn;
    struct MUI_InputHandlerNode notify_ihn;
    struct MsgPort *notify_port;
};

#define MUIM_Wanderer_HandleTimer  0x8719129
#define MUIM_Wanderer_HandleNotify 0x871912a

STATIC IPTR Wanderer_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Wanderer_Data *data;

    obj = (Object*)DoSuperMethodA(cl,obj,(Msg)msg);
    if (!obj) return NULL;

    data = (struct Wanderer_Data*)INST_DATA(cl,obj);

    if (!(data->notify_port = CreateMsgPort()))
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

#ifdef __AROS__
    RegisterWorkbench(data->notify_port);
#endif

    /* Setup two input handlers */

    /* The first one is invoked every time we get a message from the Notify Port */
    data->notify_ihn.ihn_Signals = 1UL<<data->notify_port->mp_SigBit;
    data->notify_ihn.ihn_Object = obj;
    data->notify_ihn.ihn_Method = MUIM_Wanderer_HandleNotify;
    DoMethod(obj, MUIM_Application_AddInputHandler, &data->notify_ihn);

    /* The second one is a timer handler */
    data->timer_ihn.ihn_Flags = MUIIHNF_TIMER;
    /* called every second (this is only for timer input handlers) */
    data->timer_ihn.ihn_Millis = 3000;
    /* The following method of the given should be called if the
     * event happens */
    data->timer_ihn.ihn_Object = obj;
    data->timer_ihn.ihn_Method = MUIM_Wanderer_HandleTimer;

    DoMethod(obj, MUIM_Application_AddInputHandler, &data->timer_ihn);

    return (IPTR)obj;
}

STATIC IPTR Wanderer_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct Wanderer_Data *data = (struct Wanderer_Data*)INST_DATA(cl,obj);
    if (data->notify_port)
    {
	/* They only have been added if the creation of the msg port was
	 * successful */
	DoMethod(obj, MUIM_Application_RemInputHandler, &data->timer_ihn);
	DoMethod(obj, MUIM_Application_RemInputHandler, &data->notify_ihn);
#ifdef __AROS__
	UnregisterWorkbench(data->notify_port);
#endif
	DeleteMsgPort(data->notify_port);
	data->notify_port = NULL;
    }
    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC IPTR Wanderer_HandleTimer(struct IClass *cl, Object *obj, Msg msg)
{
    Object *cstate = (Object*)(((struct List*)xget(obj, MUIA_Application_WindowList))->lh_Head);
    Object *child;
    char *scr_title = GetScreenTitle();

    while ((child = NextObject(&cstate)))
	set(child, MUIA_Window_ScreenTitle, scr_title);
    return 0; /* irrelevant */
}

STATIC IPTR Wanderer_HandleNotify(struct IClass *cl, Object *obj, Msg msg)
{
/*      struct Wanderer_Data *data = (struct Wanderer_Data*)INST_DATA(cl,obj); */
    return 0;
}

/* Use this macro for dispatchers if you don't want #ifdefs */
BOOPSI_DISPATCHER(IPTR,Wanderer_Dispatcher,cl,obj,msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Wanderer_New(cl,obj,(APTR)msg);
	case OM_DISPOSE: return Wanderer_Dispose(cl,obj,(APTR)msg);
	case MUIM_Wanderer_HandleTimer: return Wanderer_HandleTimer(cl,obj,(APTR)msg);
	case MUIM_Wanderer_HandleNotify: return Wanderer_HandleNotify(cl,obj,(APTR)msg);
    }
    return DoSuperMethodA(cl,obj,msg);
}

struct MUI_CustomClass *CL_Wanderer;

#define WandererObject BOOPSIOBJMACRO_START(CL_Wanderer->mcc_Class)


/******** Execute Window ********/

static char *execute_current_directory;
static char *execute_command;

/**************************************************************************
 Open the execute window

 This function will always get the current drawer as argument
**************************************************************************/
void execute_open(char **cd_ptr)
{
    char *cd;

    if (cd_ptr) cd = *cd_ptr;
    else cd = NULL;

    if (execute_current_directory) FreeVec(execute_current_directory);
    execute_current_directory = StrDup(cd);

    setstring(execute_command_string,execute_command);
    set(execute_wnd,MUIA_Window_Open,TRUE);
    set(execute_wnd,MUIA_Window_ActiveObject, execute_command_string);
}

/**************************************************************************
 Open the execute window. Simliar to above but you can also set the
 command. Called when item is openend
**************************************************************************/
void execute_open_with_command(char *cd, char *contents)
{
    if (execute_current_directory) FreeVec(execute_current_directory);
    execute_current_directory = StrDup(cd);

    setstring(execute_command_string,contents);
    set(execute_wnd,MUIA_Window_Open,TRUE);
    set(execute_wnd,MUIA_Window_ActiveObject, execute_command_string);
}

/**************************************************************************
 ...
**************************************************************************/
void execute_ok(void)
{
    char *cd;
    BPTR input;
    BPTR lock;

    set(execute_wnd,MUIA_Window_Open,FALSE);
    if (execute_command) FreeVec(execute_command);
    execute_command = StrDup((char*)xget(execute_command_string,MUIA_String_Contents));
    if (!execute_command) return;

    if (!execute_current_directory) cd = "RAM:";
    else cd = execute_current_directory;

    lock = Lock(cd,ACCESS_READ);
    if (!lock) return;

    input = Open("CON:////Output Window/CLOSE/AUTO/WAIT", MODE_OLDFILE);
    if (input)
    {
	if (SystemTags(execute_command,
	    	SYS_Asynch,	TRUE,
	    	SYS_Input,  (IPTR)input,
	    	SYS_Output, (IPTR)NULL,
                __AROS__ ?
	    	SYS_Error :
                TAG_IGNORE,  (IPTR)NULL,
		NP_CurrentDir, lock, /* Will be freed automatical if successful */
	    	TAG_DONE) == -1)
        {
            UnLock(lock);
            Close(input);
        }
    } else UnLock(lock);
}

/**************************************************************************
 Execute operation was canceld
**************************************************************************/
void execute_cancel(void)
{
    set(execute_wnd,MUIA_Window_Open,FALSE);
}

/*******************************/

void shell_open(char **cd_ptr)
{
    BPTR cd = Lock(*cd_ptr,ACCESS_READ);
#ifdef __AROS__
    BPTR win = Open("CON:10/10/640/480/AROS-Shell/CLOSE", MODE_OLDFILE);
    BPTR from = Open("S:Shell-Startup", MODE_OLDFILE);
#else
    BPTR win = Open("CON:10/10/640/480/AROS-Shell/AUTO/CLOSE", MODE_OLDFILE);
#endif

#ifdef __AROS__
    if (SystemTags("",
	SYS_Asynch,     TRUE,
	SYS_Input,	    (IPTR)win,
	SYS_Output,	    (IPTR)NULL,
	SYS_Background, FALSE,
	SYS_Error,	    (IPTR)NULL,
	SYS_ScriptInput, (IPTR)from,
	SYS_UserShell,  TRUE,
	NP_CurrentDir, cd,
	TAG_DONE) == -1)
#else
    if (SystemTags("newshell",
	SYS_Asynch,     TRUE,
	SYS_Input,	    (IPTR)win,
	SYS_Output,	    (IPTR)NULL,
	NP_CurrentDir, cd,
	TAG_DONE) == -1)
#endif
    {
    	Close(win);
    #ifdef __AROS__
    	Close(from);
    #endif
    	UnLock(cd);
    }
}

void wanderer_backdrop(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip,MEN_WANDERER_BACKDROP);
    if (item)
    {
    	set(root_iconwnd,MUIA_Window_Open,FALSE);
	set(root_iconwnd,MUIA_IconWindow_IsBackdrop, xget(item, MUIA_Menuitem_Checked));
    	set(root_iconwnd,MUIA_Window_Open,TRUE);
    }
}

void wanderer_guisettings(void)
{
    DoMethod(app, MUIM_Application_OpenConfigWindow);
}

void wanderer_about(void)
{
    STRPTR base;
    STRPTR params[] = { NULL, NULL };

    get(app, MUIA_Application_Base, (IPTR)&base);
    params[0] = base;

    MUI_RequestA(app,NULL,0,"About Wanderer", "*Better than ever",
	"AROS ROM version 0.7 (alpha)\n"
	"Wanderer version 0.1 (alpha)\n\n"
	"Copyright � 2003, The AROS Development Team.\n"
	"All rights reserved.\n\n"
	"ARexx Port: %s\n\n"
	"\033cWe made it...\n\n"
	"\033bThe AROS Development Team\033n\n"
	"Aaron Digulla, Georg Steger, Nils Henrik Lorentzen,\n"
	"Henning Kiel, Staf Verhaegen, Henrik Berglund,\n"
	"Michal Schulz, Iain Templeton, Fabio Alemagna,\n"
	"Sebastian Heutling, Johan Grip, Tobias Seiler,\n"
	"Johan Alfredsson, Adam Chodorowski, Sebastian Bauer,\n"
        "Matt Parsons, Stefan Berger, Bernardo Innocenti,\n"
	"Joerg Dietrich, Kjetil Svalastog Matheussen, Peter Eriksson,\n"
	"David Le Corfec, Paul Smith, ...\n"
	"\nTo be continued..."
	, params);
}

void wanderer_quit(void)
{
    if (MUI_RequestA(app,NULL,0,"Wanderer", "*Ok|Cancel", "Do you really want to quit Wanderer?",NULL))
	DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

/**************************************************************************
 Start all menu notifies
**************************************************************************/
VOID DoAllMenuNotifies(Object *strip, char *path)
{
    Object *item;

    if (!strip) return;

    DoMenuNotify(strip,MEN_WANDERER_EXECUTE,execute_open, path);
    DoMenuNotify(strip,MEN_WANDERER_SHELL,shell_open,path);
    DoMenuNotify(strip,MEN_WANDERER_GUISETTINGS,wanderer_guisettings,NULL);
    DoMenuNotify(strip,MEN_WANDERER_ABOUT,wanderer_about,NULL);
    DoMenuNotify(strip,MEN_WANDERER_QUIT,wanderer_quit,NULL);

    if ((item = FindMenuitem(strip,MEN_WANDERER_BACKDROP)))
    {
	DoMethod(item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 7, MUIM_Application_PushMethod, app, 4, MUIM_CallHook, &hook_standard, wanderer_backdrop, strip);
    }
}

/**************************************************************************
 This is our action hook called by the IconWindow class.

 obj is the iconwindow object which initiated the action
**************************************************************************/
static struct Hook hook_action;

#ifndef __AROS__
__asm __saveds void hook_func_action(register __a0 struct Hook *h, register __a2 Object *obj, register __a1 struct IconWindow_ActionMsg *msg)
#else
AROS_UFH3(void, hook_func_action,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct IconWindow_ActionMsg *, msg, A1))
#endif
{
    if (msg->type == ICONWINDOW_ACTION_OPEN)
    {
	static char buf[1024];
	struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
	Object *drawerwnd;

	DoMethod(msg->iconlist, MUIM_IconList_NextSelected, &ent);
	if ((int)ent == MUIV_IconList_NextSelected_End) return;

	if (msg->isroot)
	{
	    strcpy(buf,ent->label);
	    strcat(buf,":");
	} else
	{
	    strcpy(buf,ent->filename);
	}

	if (ent->type == ST_ROOT || ent->type == ST_USERDIR)
	{
	    Object *cstate = (Object*)(((struct List*)xget(app, MUIA_Application_WindowList))->lh_Head);
	    Object *child;

	    while ((child = NextObject(&cstate)))
	    {
	    	if (xget(child, MUIA_UserData))
	    	{
		    char *child_drawer = (char*)xget(child, MUIA_IconWindow_Drawer);
		    if (child_drawer && !Stricmp(buf,child_drawer))
		    {
		    	int is_open = xget(child, MUIA_Window_Open);
		    	if (!is_open)
			    DoMethod(child, MUIM_IconWindow_Open);
			else
			{
			    DoMethod(child, MUIM_Window_ToFront);
			    set(child, MUIA_Window_Activate, TRUE);
			}
		    	return;
		    }
	    	}
	    }

	    {
		/* Check if the window for this drawer is already opened */
		Object *menustrip;

		/* Create a new icon drawer window with the correct drawer being set */
		drawerwnd = IconWindowObject,
		    MUIA_UserData, 1,
		    MUIA_Window_Menustrip, menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
		    MUIA_IconWindow_IsRoot, FALSE,
		    MUIA_IconWindow_ActionHook, &hook_action,
		    MUIA_IconWindow_Drawer, buf,
		EndBoopsi;

		if (drawerwnd)
		{
	    	    /* Get the drawer path back so we can use it also outside this function */
		    char *drw = (char*)xget(drawerwnd,MUIA_IconWindow_Drawer);

		    /* We simply close the window here in case somebody like to to this...
		     * the memory is not freed until wb is closed however */
		    DoMethod(drawerwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, drawerwnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);

		    /* If "Execute Command" entry is clicked open the execute window */
		    DoAllMenuNotifies(menustrip,drw);

		    /* Add the window to the application */
		    DoMethod(app,OM_ADDMEMBER,drawerwnd);

		    /* And now open it */
		    DoMethod(drawerwnd, MUIM_IconWindow_Open);
		}
	    }
	} else if (ent->type == ST_FILE)
	{
	    static char buf[1024];
	    strncpy(buf,ent->filename,1023);
	    buf[1023] = 0;
	    /* truncate the path */
	    PathPart(buf)[0] = 0;
	    execute_open_with_command(buf, FilePart(ent->filename));
	}
    } else
    if (msg->type == ICONWINDOW_ACTION_CLICK)
    {
	if (!msg->click->shift)
	{
	    Object *cstate = (Object*)(((struct List*)xget(app, MUIA_Application_WindowList))->lh_Head);
	    Object *child;

	    while ((child = NextObject(&cstate)))
	    {
	    	if (xget(child, MUIA_UserData))
	    	{
		    if (child != obj)
		    {
			DoMethod(child, MUIM_IconWindow_UnselectAll);
		    }
		}
	    }
	}
    } else
    if (msg->type == ICONWINDOW_ACTION_ICONDROP)
    {
	Object *cstate = (Object*)(((struct List*)xget(app, MUIA_Application_WindowList))->lh_Head);
	Object *child;

	while ((child = NextObject(&cstate)))
	{
	    if (xget(child, MUIA_UserData))
	    {
		struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
		Object *iconlist = (Object*)xget(child, MUIA_IconWindow_IconList);

		do
		{
		    DoMethod(iconlist, MUIM_IconList_NextSelected, &ent);
		    if ((int)ent == MUIV_IconList_NextSelected_End) break;

		    Printf("%s\n",ent->filename);
		} while(1);
	    }
	}
    }
}

#ifdef __AROS__
LONG            __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process              = NULL;
STRPTR          __detached_name                 = "Wanderer";

void DoDetach(void)
{
    kprintf("LoadWB.DoDetach\n");

    /* If there's a detacher, tell it to go away */
    if (__detacher_process)
    {
	Signal((struct Task *)__detacher_process, __detacher_must_wait_for_signal);
    }
}
#endif

/**************************************************************************
 Our main entry
**************************************************************************/
int main(void)
{
    Object *execute_execute_button, *execute_cancel_button;

    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;
    hook_action.h_Entry = (HOOKFUNC)hook_func_action;

    openmuimaster();

    if (!MUIMasterBase)
	return 20;

    /* Create the IconWindow class which inherits from MUIC_Window (WindowClass) */
    CL_IconWindow = MUI_CreateCustomClass(NULL,MUIC_Window,NULL,sizeof(struct IconWindow_Data), IconWindow_Dispatcher);

    if (!CL_IconWindow)
    {
        closemuimaster();
	return 20;
    }

    /* Create the Wanderer class which inherits from MUIC_Application (ApplicationClass) */
    CL_Wanderer = MUI_CreateCustomClass(NULL,MUIC_Application,NULL,sizeof(struct Wanderer_Data), Wanderer_Dispatcher);
    if (!CL_Wanderer)
    {
	MUI_DeleteCustomClass(CL_IconWindow);
        closemuimaster();
	return 20;
    }

    app = WandererObject,
	MUIA_Application_Title, "Wanderer",
	MUIA_Application_Base, "WANDERER",
	MUIA_Application_Version, "$VER: Wanderer 0.1 (10.12.02)",
	MUIA_Application_Description, "The AROS filesystem GUI",
	MUIA_Application_SingleTask, TRUE,
    	SubWindow, root_iconwnd = IconWindowObject,
	    MUIA_UserData, 1,
	    MUIA_Window_Menustrip, root_menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
	    MUIA_Window_ScreenTitle, GetScreenTitle(),
            MUIA_IconWindow_IsRoot, TRUE,
	    MUIA_IconWindow_IsBackdrop, TRUE,
	    MUIA_IconWindow_ActionHook, &hook_action,
	    EndBoopsi,
	SubWindow, execute_wnd = WindowObject,
	    MUIA_Window_Title, "Execute a file",
	    WindowContents, VGroup,
	    MUIA_Background, MUII_GroupBack,
		Child, TextObject, MUIA_Text_Contents,"Enter command and its arguments:",End,
		Child, HGroup,
		    Child, Label("Command:"),
		    Child, PopaslObject,
			MUIA_Popstring_Button, PopButton(MUII_PopFile),
			MUIA_Popstring_String, execute_command_string = StringObject, StringFrame, End,
			End,
		    End,
		Child, HGroup,
		    Child, execute_execute_button = SimpleButton("_Execute"),
		    Child, HVSpace,
		    Child, execute_cancel_button = SimpleButton("_Cancel"),
		    End,
		End,
	    End,
	EndBoopsi;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod(root_iconwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 3, MUIM_CallHook, &hook_standard, wanderer_quit);

	/* If "Execute Command" entry is clicked open the execute window */
	DoAllMenuNotifies(root_menustrip,"RAM:");

        /* Execute Window Notifies */
        DoMethod(execute_command_string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard, execute_ok);
        DoMethod(execute_execute_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, execute_ok);
        DoMethod(execute_cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, execute_cancel);
        DoMethod(execute_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 3, MUIM_CallHook, &hook_standard, execute_cancel);

	/* And now open it */
	DoMethod(root_iconwnd, MUIM_IconWindow_Open);

#ifdef __AROS__
	DoDetach();
#endif

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

    MUI_DeleteCustomClass(CL_Wanderer);
    MUI_DeleteCustomClass(CL_IconWindow);
    closemuimaster();
    return 0;
}
