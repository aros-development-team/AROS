/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <dos/dostags.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef _AROS
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <aros/debug.h>
#else
#include "mui.h"
struct Library *MUIMasterBase;
#endif


#ifndef _AROS
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
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}
void closemuimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}
#endif

enum
{
    MEN_WANDERER = 1,
    MEN_WANDERER_BACKDROP,
    MEN_WANDERER_EXECUTE,
    MEN_WANDERER_SHELL,
    MEN_WANDERER_ABOUT,
    MEN_WANDERER_QUIT,
};

static struct NewMenu nm[] =
{
  {NM_TITLE, "Wanderer"              },
    {NM_ITEM,  "Backdrop",           "B", CHECKIT|MENUTOGGLE, NULL, (void*)MEN_WANDERER_BACKDROP},
    {NM_ITEM,  "Execute Command...", "E", NULL,               NULL, (void*)MEN_WANDERER_EXECUTE},
    {NM_ITEM,  "Redraw All" },
    {NM_ITEM,  "Update All" },
    {NM_ITEM,  "Last Message" },
    {NM_ITEM,  "Shell",              "W", NULL,               NULL, (void*)MEN_WANDERER_SHELL},
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


#ifndef _AROS
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
		func(funcptr + 1);
}

/**************************************************************************
 Easily call the new method of super class with additional tags
**************************************************************************/
STATIC ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
  return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}

/**************************************************************************
 Duplicate a string using AllocVec
**************************************************************************/
static char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, 0);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
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
struct Library *MUIMasterBase;

Object *app;
Object *root_iconwnd;
Object *root_menustrip;
Object *execute_wnd;
Object *execute_command_string;

/**************************************************************************
 Easily get attributes
**************************************************************************/
ULONG xget(Object *obj, Tag attr)
{
  ULONG storage;
  GetAttr(attr, obj, &storage);
  return storage;
}

/**************************************************************************
 This is a custom class inheriting from the WindowClass.
**************************************************************************/

#define MUIA_IconWindow_IsRoot       (TAG_USER+0x1631313) /* i.g */
#define MUIA_IconWindow_Drawer       (TAG_USER+0x1631314) /* i.g */
#define MUIA_IconWindow_ActionHook   (TAG_USER+0x1631315) /* i.. */ /* Hook to call when some action happens */

/* private methods, should be not called from outside */
#define MUIM_IconWindow_DoubleClicked (0x129090)

#define ICONWINDOW_ACTION_OPEN 1

struct IconWindow_ActionMsg
{
   int type;
   Object *iconlist;
   int isroot;
   /* to be continued...*/
};

struct IconWindow_Data
{
    Object *iconlist; /* The iconlist it displays */
    int is_root;
    struct Hook *action_hook;
};

STATIC IPTR IconWindow_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    char *title;
    struct Hook *action_hook;
    int is_root;
    struct IconWindow_Data *data;

    Object *iconlist;

    is_root = (int)GetTagData(MUIA_IconWindow_IsRoot,FALSE,msg->ops_AttrList);
    if (is_root)
    {
	title = "Wanderer";
	iconlist = MUI_NewObject(MUIC_IconVolumeList, TAG_DONE);
    } else
    {
	title = (char*)GetTagData(MUIA_IconWindow_Drawer,NULL,msg->ops_AttrList);
	iconlist = MUI_NewObject(MUIC_IconDrawerList, MUIA_IconDrawerList_Drawer, title, TAG_DONE);
    }

    /* More than one GetTagData is not really efficient but since this is called very unoften... */
    action_hook = (struct Hook*)GetTagData(MUIA_IconWindow_ActionHook, NULL, msg->ops_AttrList);

    /* Now call the super methods new method with additional tags */
    obj = (Object*)DoSuperNew(cl,obj,
	MUIA_Window_Title,title,
	MUIA_Window_Width,300,
	MUIA_Window_Height,300,
	MUIA_Window_UseBottomBorderScroller, TRUE,
	MUIA_Window_UseRightBorderScroller, TRUE,
	MUIA_Window_ScreenTitle, GetScreenTitle(),
	WindowContents, VGroup,
		MUIA_Group_Child, MUI_NewObject(MUIC_IconListview, MUIA_IconListview_UseWinBorder, TRUE, MUIA_IconListview_IconList, iconlist, TAG_DONE),
		End,
	TAG_MORE, (IPTR)msg->ops_AttrList);
    if (!obj) return NULL;

    /* Get and initialize the instance's data */
    data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    data->iconlist = iconlist;
    data->is_root = is_root;
    data->action_hook = action_hook;

    /* If double clicked then we call our own private methods, that's easier then using Hooks */
    DoMethod(iconlist,MUIM_Notify,MUIA_IconList_DoubleClick,TRUE,obj,1,MUIM_IconWindow_DoubleClicked);

    return (IPTR)obj;
}

STATIC IPTR IconWindow_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct IconWindow_Data *data = (struct IconWindow_Data*)INST_DATA(cl,obj);
    switch (msg->opg_AttrID)
    {
	case	MUIA_IconWindow_Drawer: *msg->opg_Storage = xget(data->iconlist,MUIA_IconDrawerList_Drawer); return 1;
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
	CallHookPkt(data->action_hook,obj,&msg);
    }
    return NULL; /* irrelevant */
}

/* Use this macro for dispatchers if you don't want #ifdefs */
BOOPSI_DISPATCHER(IPTR,IconWindow_Dispatcher,cl,obj,msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return IconWindow_New(cl,obj,(struct opSet*)msg);
	case OM_GET: return IconWindow_Get(cl,obj,(struct opGet*)msg);

	/* private methods */
	case MUIM_IconWindow_DoubleClicked: return IconWindow_DoubleClicked(cl,obj,msg);
    }
    return DoSuperMethodA(cl,obj,msg);
}

struct MUI_CustomClass *CL_IconWindow;

#define IconWindowObject (Object*)NewObject(CL_IconWindow->mcc_Class, NULL


/**************************************************************************
 This is a custom class inheriting from the Application Class.
 We need it support the (periodically) updating of the windows titles and
 we don't want to open timer.device by hand.
**************************************************************************/
struct Wanderer_Data
{
    struct MUI_InputHandlerNode ihn;
};

#define MUIM_Wanderer_UpdateTitles 0x8719129

STATIC IPTR Wanderer_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Wanderer_Data *data;

    obj = (Object*)DoSuperMethodA(cl,obj,(Msg)msg);
    if (!obj) return NULL;

    data = (struct Wanderer_Data*)INST_DATA(cl,obj);

    /* A timer input handler */
    data->ihn.ihn_Flags = MUIIHNF_TIMER;

    /* called every second (this is only for timer input handlers) */
    data->ihn.ihn_Millis = 1000;

    /* The following method of the given should be called if the
     * event happens (one second has passed) */
    data->ihn.ihn_Object = obj;
    data->ihn.ihn_Method = MUIM_Wanderer_UpdateTitles;

    DoMethod(obj, MUIM_Application_AddInputHandler, &data->ihn);
    
    return (IPTR)obj;
}

STATIC IPTR Wanderer_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct Wanderer_Data *data = (struct Wanderer_Data*)INST_DATA(cl,obj);
    DoMethod(obj, MUIM_Application_RemInputHandler, &data->ihn);
    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC IPTR Wanderer_UpdateTitles(struct IClass *cl, Object *obj, Msg msg)
{
    Object *cstate = (Object*)(((struct List*)xget(obj, MUIA_Application_WindowList))->lh_Head);
    Object *child;
    char *scr_title = GetScreenTitle();

    while ((child = NextObject(&cstate)))
	set(child, MUIA_Window_ScreenTitle, scr_title);
    return 0; /* irrelevant */
}

/* Use this macro for dispatchers if you don't want #ifdefs */
BOOPSI_DISPATCHER(IPTR,Wanderer_Dispatcher,cl,obj,msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Wanderer_New(cl,obj,(APTR)msg);
	case OM_DISPOSE: return Wanderer_Dispose(cl,obj,(APTR)msg);
	case MUIM_Wanderer_UpdateTitles: return Wanderer_UpdateTitles(cl,obj,(APTR)msg);
    }
    return DoSuperMethodA(cl,obj,msg);
}

struct MUI_CustomClass *CL_Wanderer;

#define WandererObject (Object*)NewObject(CL_Wanderer->mcc_Class, NULL


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
#ifdef _AROS
	    	SYS_Error,  (IPTR)NULL,
#endif
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
#ifdef _AROS
    BPTR win = Open("CON:10/10/640/480/AROS-Shell/CLOSE", MODE_OLDFILE);
#else
    BPTR win = Open("CON:10/10/640/480/AROS-Shell/AUTO/CLOSE", MODE_OLDFILE);
#endif

#ifdef _AROS
    if (SystemTags("",
#else
    if (SystemTags("newshell",
#endif
	SYS_Asynch,     TRUE,
#ifdef _AROS
	SYS_Background, FALSE,
#endif
	SYS_Input,	    (IPTR)win,
	SYS_Output,	    (IPTR)NULL,
#ifdef _AROS
	SYS_Error,	    (IPTR)NULL,
	SYS_UserShell,  TRUE,
#endif
	NP_CurrentDir, cd,
	TAG_DONE) == -1)
    {
    	Close(win);
    	UnLock(cd);
    }
}

void wanderer_about(void)
{
    MUI_RequestA(app,NULL,0,"About Wanderer", "*Better than ever",
	"AROS ROM version 0.7 (alpha)\n"
	"Wanderer version 0.1 (alpha)\n\n"
	"Copyright © 2002, The AROS Development Team.\n"
	"All rights reserved.\n\n"
	"\033cWe made it...\n\n"
	"\033bThe AROS Development Team\033n\n"
	"Aaron Digulla, Georg Steger, Nils Henrik Lorentzen,\n"
	"Henning Kiel, Staf Verhaegen, Henrik Berglund,\n"
	"Michal Schulz, Iain Templeton, Fabio Alemagna,\n"
	"Sebastian Heutling, Johan Grip, Tobias Seiler,\n"
	"Johan Alfredsson, Adam Chodorowski, Matt Parsons...\n"
	"\nTo be continued...",NULL);
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
    if (!strip) return;

    DoMenuNotify(strip,MEN_WANDERER_EXECUTE,execute_open, path);
    DoMenuNotify(strip,MEN_WANDERER_SHELL,shell_open,path);
    DoMenuNotify(strip,MEN_WANDERER_ABOUT,wanderer_about,NULL);
    DoMenuNotify(strip,MEN_WANDERER_QUIT,wanderer_quit,NULL);
}

/**************************************************************************
 This is our action hook called by the IconWindow class.

 obj is the iconwindow object which initiated the action
**************************************************************************/
static struct Hook hook_action;

#ifndef _AROS
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
	    Object *menustrip;

	    /* Create a new icon drawer window with the correct drawer being set */
	    drawerwnd = IconWindowObject,
		MUIA_Window_Menustrip, menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
		MUIA_IconWindow_IsRoot, FALSE,
		MUIA_IconWindow_ActionHook, &hook_action,
		MUIA_IconWindow_Drawer, buf,
		End;

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
		set(drawerwnd,MUIA_Window_Open,TRUE);
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
    }
}


/**************************************************************************
 Our main entry
**************************************************************************/
int main(void)
{
    Object *execute_execute_button, *execute_cancel_button;

    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;
    hook_action.h_Entry = (HOOKFUNC)hook_func_action;

    openmuimaster();

/*    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);*/

    if (!MUIMasterBase)
	return 20;

    /* Create the IconWindow class which inherits from MUIC_Window (WindowClass) */
    CL_IconWindow = MUI_CreateCustomClass(NULL,MUIC_Window,NULL,sizeof(struct IconWindow_Data), IconWindow_Dispatcher);
    if (!CL_IconWindow)
    {
	CloseLibrary(MUIMasterBase);
	return 20;
    }

    /* Create the Wanderer class which inherits from MUIC_Application (ApplicationClass) */
    CL_Wanderer = MUI_CreateCustomClass(NULL,MUIC_Application,NULL,sizeof(struct Wanderer_Data), Wanderer_Dispatcher);
    if (!CL_Wanderer)
    {
	MUI_DeleteCustomClass(CL_IconWindow);
	CloseLibrary(MUIMasterBase);
	return 20;
    }

    app = WandererObject,
    	SubWindow, root_iconwnd = IconWindowObject,
	    MUIA_Window_TopEdge, MUIV_Window_TopEdge_Delta(0), /* place the window below the bar layer */
	    MUIA_Window_LeftEdge, 0,
	    MUIA_Window_Width, MUIV_Window_Width_Screen(100),
	    MUIA_Window_Height, MUIV_Window_Height_Screen(100), /* won't take the barlayer into account */
	    MUIA_Window_Menustrip, root_menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
	    MUIA_Window_ScreenTitle, GetScreenTitle(),
            MUIA_IconWindow_IsRoot, TRUE,
	    MUIA_IconWindow_ActionHook, &hook_action,
	    End,
	SubWindow, execute_wnd = WindowObject,
	    MUIA_Window_Title, "Execute a file",
	    WindowContents, VGroup,
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
	End;

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

	set(root_iconwnd,MUIA_Window_Open,TRUE);

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
/*    CloseLibrary(MUIMasterBase);*/
    closemuimaster();
    return 0;
}
