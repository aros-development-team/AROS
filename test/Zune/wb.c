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
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <aros/debug.h>

#include <libraries/mui.h>

static struct NewMenu nm[] =
{
  {NM_TITLE, "Workbench"              },
    {NM_ITEM,  "Backdrop",           "B", CHECKIT | CHECKED},
    {NM_ITEM,  "Execute Command...", "E"},
    {NM_ITEM,  "Redraw All" },
    {NM_ITEM,  "Update All" },
    {NM_ITEM,  "Last Message" },
    {NM_ITEM,  "Shell",              "Z"},
    {NM_ITEM,  "About...",           "?"},
    {NM_ITEM,  "Quit...",            "Q"},

  {NM_TITLE, "Window",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "New Drawer", "N"},
    {NM_ITEM,  "Open Parent" },
    {NM_ITEM,  "Close", "K"},
    {NM_ITEM,  "Update" },
    {NM_ITEM,  "Select Contents", "A"},
    {NM_ITEM,  "Clean Up", "."},
    {NM_ITEM,  "Snapshot" },
      {NM_SUB, "Window"},
      {NM_SUB, "All"},
    {NM_ITEM,  "Show" },
      {NM_SUB, "Only Icons", NULL, CHECKIT | CHECKED, 32},
      {NM_SUB, "All Files", NULL, CHECKIT, 16 },
    {NM_ITEM,  "View By" },
      {NM_SUB, "Icon", NULL, CHECKIT | CHECKED, 32 + 64 + 128},
      {NM_SUB, "Name",NULL, CHECKIT, 16 + 64 + 128},
      {NM_SUB, "Size",NULL, CHECKIT, 16 + 32 + 128},
      {NM_SUB, "Date", NULL, CHECKIT, 16 + 32 + 64},

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
    {NM_ITEM,  "ResetWB" },
  {NM_END}

};


/**************************************************************************
 Easily call the new method of super class with additional tags
**************************************************************************/
STATIC ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
  return (DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL));
}


struct Library *MUIMasterBase;

Object *app;
Object *root_iconwnd;

/**************************************************************************
 Easily get attributes
**************************************************************************/
ULONG xget(Object *obj, Tag attr)
{
  ULONG storage;
  GetAttr(attr, obj, &storage);
  return storage;
}
#if 0
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
    static char buf[1024];
    struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;

    DoMethod(drawer_iconlist, MUIM_IconList_NextSelected, &ent);
    if ((int)ent == MUIV_IconList_NextSelected_End) return;
    set(drawer_iconlist,MUIA_IconDrawerList_Drawer,ent->filename);
}
#endif

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
	title = "Workbench";
	iconlist = MUI_NewObject(MUIC_IconVolumeList, TAG_DONE);
    } else
    {
	title = (char*)GetTagData(MUIA_IconWindow_Drawer,NULL,msg->ops_AttrList);
	iconlist = MUI_NewObject(MUIC_IconDrawerList, MUIA_IconDrawerList_Drawer, title, TAG_DONE);
    }

    /* More than one GetTagData is not really efficient but since this is called very unoften... */
    action_hook = (struct Hook*)GetTagData(MUIA_IconWindow_ActionHook, NULL, msg->ops_AttrList);

    /* Now call the super methods new method with additional tags */
    obj = DoSuperNew(cl,obj,
	MUIA_Window_Title,title,
	MUIA_Window_Width,300,
	MUIA_Window_Height,300,
	WindowContents, VGroup,
		MUIA_Group_Child, iconlist,
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

	/* private methods */
	case MUIM_IconWindow_DoubleClicked: return IconWindow_DoubleClicked(cl,obj,msg);
    }
    return DoSuperMethodA(cl,obj,msg);
}

struct MUI_CustomClass *CL_IconWindow;

#define IconWindowObject (Object*)NewObject(CL_IconWindow->mcc_Class, NULL


/**************************************************************************
 This is the standard_hook for easy MUIM_CallHook callbacks
**************************************************************************/
static struct Hook hook_standard;

AROS_UFH3(void, hook_func_standard,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1))
{
	void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);

	if (func)
		func(funcptr + 1);
}

/**************************************************************************
 This is our action hook called by the IconWindow class.

 obj is the iconwindow object which initiated the action
**************************************************************************/
static struct Hook hook_action;

AROS_UFH3(void, hook_func_action,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct IconWindow_ActionMsg *, msg, A1))
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

	/* Create a new icon drawer window with the correct drawer being set */
	drawerwnd = IconWindowObject,
            MUIA_IconWindow_IsRoot, FALSE,
	    MUIA_IconWindow_ActionHook, &hook_action,
	    MUIA_IconWindow_Drawer, buf,
	    End;

	if (drawerwnd)
	{
	    /* We simply close the window here...the memory is not freed until wb is closed
	     * however */
	    DoMethod(drawerwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, drawerwnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);

	    /* Add the window to the application */
	    DoMethod(app,OM_ADDMEMBER,drawerwnd);

	    /* And now open it */
	    set(drawerwnd,MUIA_Window_Open,TRUE);
	}
    }
}

/**************************************************************************
 Our main entry
**************************************************************************/
int main(void)
{
    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;
    hook_action.h_Entry = (HOOKFUNC)hook_func_action;

    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    if (!MUIMasterBase)
	return 20;

    /* Create the IconWindow class which inherits from MUIC_Window (WindowClass) */
    CL_IconWindow = MUI_CreateCustomClass(NULL,MUIC_Window,NULL,sizeof(struct IconWindow_Data), IconWindow_Dispatcher);
    if (!CL_IconWindow)
    {
	CloseLibrary(MUIMasterBase);
	return 20;
    }

    app = ApplicationObject,
 	MUIA_Application_Menustrip, MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
    	SubWindow, root_iconwnd = IconWindowObject,
            MUIA_IconWindow_IsRoot, TRUE,
	    MUIA_IconWindow_ActionHook, &hook_action,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod(root_iconwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

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

    MUI_DeleteCustomClass(CL_IconWindow);
    CloseLibrary(MUIMasterBase);

    return 0;
}
