/*

CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S,PORT/K,QUIET/S

MAKEBIG
MAKESMALL
CYCLE
CYCLESCREEN
ZIPWINDOW
INSERT text
RUN command
AREXX script

popkey = ctrl alt f

FKey: Tastenbefehl = <control alt f>
Definierte Tasten
Neue Taste    Taste löschen
Befehl
Befehlsargumente
* Durch Fenster blättern
Durch Schirme blättern
Fenster vergrößern
Fenster verkleinern
Fenstergröße umschalten
Text einfügen
Programm starten
ARexx-Skript starten
Projekt
Belegung speichern S
Verbergen H
Ikonifizieren I
Beenden Q

FKey: Hot Key = <control alt f>
Defined Keys
New Key   Delete Key
Command
Command Parameters
* Cycle Windows
Cycle Screens
Enlarge Window
Shrink Window
Toggle Window Size
Insert Text
Run Program
Run Arexx Script
Project
Save Defined Keys S
Hide H
Iconify I
Quit Q

*/

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include <intuition/classusr.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <libraries/locale.h>
#include <libraries/gadtools.h>
#include <libraries/commodities.h>
#include <libraries/mui.h>
#include <devices/inputevent.h>
#include <aros/asmcall.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/alib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define CATCOMP_ARRAY
#include "strings.h"

#include <aros/debug.h>

/*********************************************************************************************/

#define VERSION 	1
#define REVISION 	0
#define DATESTR 	"15.08.2003"
#define VERSIONSTR	"$VER: FKey 1.0 (" DATESTR ")"

/*********************************************************************************************/

#define ARG_TEMPLATE "CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S,PORT/K,QUIET/S"

#define ARG_CXPRI   	0
#define ARG_CXPOPKEY 	1
#define ARG_CXPOPUP 	2
#define ARG_CXPORT  	3
#define ARG_QUIET   	4

#define NUM_ARGS    	5

/*********************************************************************************************/

#define ACTION_CYCLE_WIN    0
#define ACTION_CYCLE_SCR    1
#define ACTION_ENLARGE_WIN  2
#define ACTION_SHRINK_WIN   3
#define ACTION_TOGGLE_WIN   4
#define ACTION_INSERT_TEXT  5
#define ACTION_RUN_PROG     6
#define ACTION_RUN_AREXX    7

#define RETURNID_NEWKEY     1
#define RETURNID_DELKEY     2
#define RETURNID_STRINGACK  3
#define RETURNID_LVACK	    4
#define RETURNID_CMDACK     5

/*********************************************************************************************/

struct KeyInfo
{
    struct InputEvent *translist;
    CxObj   	      *filter, *trans, *custom;    
    WORD    	       action;
    char    	       descr[80];
    char    	       param[256];
};

/*********************************************************************************************/

struct LocaleBase    	*LocaleBase;

static char 	     	*cx_popkey = "ctrl alt f";
static LONG 	         cx_pri = 0;
static BOOL 	      	 cx_popup = FALSE;

static CxObj	     	*broker, *activated_custom_cobj;
static Object	     	*app, *wnd, *cmdcycle, *list, *liststr;
static Object	     	*insertstr, *runprogstr, *runarexxstr;
static Object	     	*cmdpage;
static struct Task   	*maintask;
static struct Hook   	 keylist_construct_hook, keylist_destruct_hook, keylist_disp_hook;
static struct Hook   	 broker_hook;
static struct MsgPort 	*brokermp;
static struct Catalog 	*catalog;
static struct RDArgs 	*myargs;
static LONG 	     	 prog_exitcode;
static UBYTE	       **wbargs;
static IPTR 	      	 args[NUM_ARGS];
static UBYTE	      	 s[257];

/*********************************************************************************************/

CONST_STRPTR MSG(ULONG id);
void FreeArguments(void);
void CleanupLocale(void);
void KillCX(void);
void KillGUI(void);
void ListToString(void);
void StringToKey(void);

/*********************************************************************************************/

WORD ShowMessage(CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return EasyRequestArgs(NULL, &es, NULL, NULL);  
}

/*********************************************************************************************/

void Cleanup(CONST_STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("Fkey", msg, MSG(MSG_OK));     
	}
	else
	{
	    printf("FKey: %s\n", msg);
	}
    }
    
    KillGUI();
    FreeArguments();
    KillCX();
    CleanupLocale();
    
    exit(prog_exitcode);
}


/*********************************************************************************************/

void InitCX(void)
{
    maintask = FindTask(NULL);
}

/*********************************************************************************************/

void KillCX(void)
{
}

/*********************************************************************************************/

void InitLocale(STRPTR catname, ULONG version)
{
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 39);
    if (LocaleBase)
    {
	catalog = OpenCatalog(NULL, catname, OC_Version, version,
					     TAG_DONE);
    }
}

/*********************************************************************************************/

void CleanupLocale(void)
{
    if (catalog) CloseCatalog(catalog);
    if (LocaleBase) CloseLibrary((struct Library *)LocaleBase);
}

/*********************************************************************************************/

CONST_STRPTR MSG(ULONG id)
{
    if (catalog != NULL)
    {
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    }
    else 
    {
        return CatCompArray[id].cca_Str;
    }
}

/*********************************************************************************************/

struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_FKEY_MEN_PROJECT         },
     {NM_ITEM, (STRPTR)MSG_FKEY_MEN_PROJECT_SAVE    },
     {NM_ITEM, NM_BARLABEL  	    	    	    },
     {NM_ITEM, (STRPTR)MSG_FKEY_MEN_PROJECT_HIDE    },
     {NM_ITEM, (STRPTR)MSG_FKEY_MEN_PROJECT_ICONIFY },
     {NM_ITEM, NM_BARLABEL  	    	    	    },
     {NM_ITEM, (STRPTR)MSG_FKEY_MEN_PROJECT_QUIT    },
    {NM_END 	    	    	    	    	    }
};

/*********************************************************************************************/

void InitMenus(void)
{
    struct NewMenu *actnm = nm;
    
    for(actnm = nm; actnm->nm_Type != NM_END; actnm++)
    {
	if (actnm->nm_Label != NM_BARLABEL)
	{
	    ULONG  id = (ULONG)actnm->nm_Label;
	    CONST_STRPTR str = MSG(id);
	    
	    if (actnm->nm_Type == NM_TITLE)
	    {
		actnm->nm_Label = str;
	    } else {
		actnm->nm_Label = str + 2;
		if (str[0] != ' ') actnm->nm_CommKey = str;
	    }
	    actnm->nm_UserData = (APTR)id;
	    
	} /* if (actnm->nm_Label != NM_BARLABEL) */
	
    } /* for(actnm = nm; nm->nm_Type != NM_END; nm++) */

}

/*********************************************************************************************/

void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
    ArgArrayDone();
}

/*********************************************************************************************/

void KillGUI(void)
{
    DisposeObject(app);
}

/*********************************************************************************************/

void GetArguments(int argc, char **argv)
{
    if (argc)
    {
    	if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    	{
	    Fault(IoErr(), 0, s, 256);
	    Cleanup(s);
    	}
	
	if (args[ARG_CXPRI]) cx_pri = (LONG)*(IPTR *)args[ARG_CXPRI];
	if (args[ARG_CXPOPKEY]) cx_popkey = (STRPTR)args[ARG_CXPOPKEY];
	if (args[ARG_CXPOPUP]) cx_popup = TRUE;
    }
    else
    {
    	wbargs = ArgArrayInit(argc, (UBYTE **)argv);
	
	cx_pri = ArgInt(wbargs, "CX_PRIORITY", 0);
	cx_popkey = ArgString(wbargs, "CX_POPKEY", "ctrl alt f");
	
	if (strnicmp(ArgString(wbargs, "CX_POPUP", "YES"), "Y", 1) == 0)
	{
	    cx_popup = TRUE;
	}
    }    
}

/*********************************************************************************************/

static APTR keylist_construct_func(struct Hook *hook, APTR pool, struct KeyInfo *ki)
{
    struct KeyInfo *new;
    
    new = AllocPooled(pool, sizeof(*ki));
    if (new) *new = *ki;
    
    return new;
}

/*********************************************************************************************/

static void keylist_destruct_func(struct Hook *hook, APTR pool, struct KeyInfo *ki)
{
    if (ki)
    {
    	if (ki->filter) DeleteCxObjAll(ki->filter);
    	if (ki->translist) FreeIEvents(ki->translist);
	    	
    	FreePooled(pool, ki, sizeof(*ki));
    }
}

/*********************************************************************************************/

static void keylist_disp_func(struct Hook *hook, char **array, struct KeyInfo *ki)
{
    *array = ki->descr;
}

/*********************************************************************************************/

static void broker_func(struct Hook *hook, Object *obj, CxMsg *msg)
{
}


/*********************************************************************************************/

AROS_UFH2S(void, custom_func,
    AROS_UFHA(CxMsg *, msg, A0),
    AROS_UFHA(CxObj *, co, A1))
{
    AROS_USERFUNC_INIT
    
    activated_custom_cobj = co;
    Signal(maintask, SIGBREAKF_CTRL_E);
    
    AROS_USERFUNC_EXIT
}
    

/*********************************************************************************************/

static void MakeGUI(void)
{
    static CONST_STRPTR cmdarray[] =
    {
    	(CONST_STRPTR)MSG_FKEY_CMD_CYCLE_WIN,
    	(CONST_STRPTR)MSG_FKEY_CMD_CYCLE_SCR,
    	(CONST_STRPTR)MSG_FKEY_CMD_ENLARGE_WIN,
    	(CONST_STRPTR)MSG_FKEY_CMD_SHRINK_WIN,
    	(CONST_STRPTR)MSG_FKEY_CMD_TOGGLE_WIN_SIZE,
    	(CONST_STRPTR)MSG_FKEY_CMD_INSERT_TEXT,
    	(CONST_STRPTR)MSG_FKEY_CMD_RUN_PROG,
    	(CONST_STRPTR)MSG_FKEY_CMD_RUN_AREXX,
	0,
    };
    static TEXT wintitle[100];
    WORD i;
    Object *menu, *newkey, *delkey;
    
    for(i = 0; cmdarray[i]; i++)
    {
    	cmdarray[i] = MSG((ULONG) cmdarray[i]);
    }
    
    keylist_construct_hook.h_Entry = HookEntry;
    keylist_construct_hook.h_SubEntry = (HOOKFUNC)keylist_construct_func;
    
    keylist_destruct_hook.h_Entry = HookEntry;
    keylist_destruct_hook.h_SubEntry = (HOOKFUNC)keylist_destruct_func;

    keylist_disp_hook.h_Entry = HookEntry;
    keylist_disp_hook.h_SubEntry = (HOOKFUNC)keylist_disp_func;
    
    broker_hook.h_Entry = HookEntry;
    broker_hook.h_SubEntry = (HOOKFUNC)broker_func;
    
    menu = MUI_MakeObject(MUIO_MenustripNM, &nm, 0);
        
    snprintf(wintitle, sizeof(wintitle), MSG(MSG_FKEY_WINTITLE), cx_popkey);
    	
    app = ApplicationObject,
	MUIA_Application_Title, (IPTR)MSG(MSG_FKEY_CXNAME),
	MUIA_Application_Version, (IPTR)VERSIONSTR,
	MUIA_Application_Copyright, (IPTR)"Copyright © 1995-2003, The AROS Development Team",
	MUIA_Application_Author, (IPTR)"The AROS Development Team",
	MUIA_Application_Description, (IPTR)MSG(MSG_FKEY_CXDESCR),
	MUIA_Application_BrokerPri, cx_pri,
	MUIA_Application_BrokerHook, (IPTR)&broker_hook,
	MUIA_Application_Base, (IPTR)"FKey",
	MUIA_Application_SingleTask, TRUE,
	menu ? MUIA_Application_Menustrip : TAG_IGNORE, menu,
  	SubWindow, wnd = WindowObject,
	    MUIA_Window_Title, (IPTR)wintitle,
	    MUIA_Window_ID, MAKE_ID('F','W','I','N'),
	    WindowContents, HGroup,
	    	Child, VGroup,
		    GroupFrameT(MSG(MSG_FKEY_DEFINED_KEYS)),
		    Child, VGroup,
		    	GroupSpacing(0),
			Child, ListviewObject,
		    	    MUIA_Listview_List, list = ListObject,
				InputListFrame,
				MUIA_List_ConstructHook, (IPTR)&keylist_construct_hook,
				MUIA_List_DestructHook, (IPTR)&keylist_destruct_hook,
				MUIA_List_DisplayHook, (IPTR)&keylist_disp_hook,
				End,
			    End,
			Child, liststr = StringObject,
			    MUIA_Disabled, TRUE,
		    	    StringFrame,
			    End,
			End,
		    Child, HGroup,
		    	Child, newkey = SimpleButton(MSG(MSG_FKEY_NEW_KEY)),
			Child, delkey = SimpleButton(MSG(MSG_FKEY_DELETE_KEY)),
			End,
		    End,
		Child, VGroup,
		    GroupFrameT(MSG(MSG_FKEY_COMMAND)),
		    Child, cmdcycle = MUI_MakeObject(MUIO_Cycle, NULL, cmdarray),
		    Child, cmdpage = PageGroup,
		    	Child, HVSpace, /* cycle win */
			Child, HVSpace, /* cycle scr */
			Child, HVSpace, /* enlarge win */
			Child, HVSpace, /* shrink win */
			Child, HVSpace, /* Toggle win */
			Child, insertstr = StringObject, StringFrame, End, /* Insert text */
			Child, runprogstr = StringObject, StringFrame, End, /* Run prog */
			Child, runarexxstr = StringObject, StringFrame, End, /* Run ARexx */		
		    	End,
		    Child, HVSpace,
		    End,
		End,
	    End,
	End;
	
    if (!app) Cleanup(MSG(MSG_CANT_CREATE_GADGET));
    
    set(wnd, MUIA_Window_Open, TRUE);

    get(app, MUIA_Application_Broker, &broker);  
    get(app, MUIA_Application_BrokerPort, &brokermp);  
    
    if (!broker || !brokermp) Cleanup(MSG(MSG_CANT_CREATE_GADGET));

    {
    	CxObj *popfilter = CxFilter(cx_popkey);
    	
	if (popfilter)
	{
	    CxObj *popsig = CxSignal(maintask, SIGBREAKB_CTRL_F);

	    if (popsig)
	    {
	    	CxObj *trans;
		
	    	AttachCxObj(popfilter, popsig);
		
		trans = CxTranslate(NULL);
    	    	if (trans) AttachCxObj(popsig, trans);
	    }
	    
	    AttachCxObj(broker, popfilter);
	}
	
    }
    
    set(liststr, MUIA_String_AttachedList, (IPTR)list);

    //DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR) app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_FKEY_MEN_PROJECT_QUIT, (IPTR) app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_FKEY_MEN_PROJECT_HIDE, (IPTR) wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_FKEY_MEN_PROJECT_ICONIFY, (IPTR) wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);
    
    DoMethod(cmdcycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR)cmdpage, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
    DoMethod(cmdcycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    DoMethod(newkey, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_NEWKEY);
    DoMethod(delkey, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_DELKEY);
    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_LVACK);
    DoMethod(liststr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_STRINGACK);
    DoMethod(insertstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    DoMethod(runprogstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    DoMethod(runarexxstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    
}

/*********************************************************************************************/

void RethinkKey(struct KeyInfo *ki)
{
    if (ki->filter) DeleteCxObjAll(ki->filter);
    if (ki->translist)
    {
    	FreeIEvents(ki->translist);
	ki->translist = NULL;
    }
    ki->custom = ki->trans = ki->filter = NULL;
    
    if ((ki->filter = CxFilter(ki->descr)))
    {
    	switch(ki->action)
	{
	    case ACTION_INSERT_TEXT:
	    	strrev(ki->param);
	    	if ((ki->translist = InvertString(ki->param, NULL)))
		{		
	    	    if ((ki->trans = CxTranslate(ki->translist)))
		    {
		    	AttachCxObj(ki->filter, ki->trans);
		    }
		}
		strrev(ki->param);
	    	break;
		
	    default:
	    	/* This CxCustom thing is hacky/ugly. A CxSender
		   would be better, but if want to send a pointer
		   with it then this would require some fixes in
		   commodities.library and it's header in case we are
		   running on 64 bit machines :-\ */
		   
    		if ((ki->custom = CxCustom(custom_func, 0)))
		{
		    if ((ki->trans = CxTranslate(NULL)))
		    {
		    	AttachCxObj(ki->custom, ki->trans);
		    }
		    AttachCxObj(ki->filter, ki->custom);
		}
		break;
		
	}

	AttachCxObj(broker, ki->filter);

    }
}

/*********************************************************************************************/

void RethinkAction(void)
{
    struct KeyInfo *ki = NULL;
    
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&ki);
    
    if (ki)
    {
	Object *str = NULL;
    	IPTR val;
	
	get(cmdcycle, MUIA_Cycle_Active, &val);
    	ki->action = val;

	switch(ki->action)
	{
	    case ACTION_INSERT_TEXT:
	    	str = insertstr;
		break;
		
	    case ACTION_RUN_PROG:
	    	str = runprogstr;
		break;
		
	    case ACTION_RUN_AREXX:
	    	str = runarexxstr;
		break;
		
	}
	
	if (str)
	{
	    STRPTR s;
	    
	    get(str, MUIA_String_Contents, &s);
	    
	    strncpy(ki->param, s, sizeof(ki->param));
	}
	
	RethinkKey(ki);
    }
}

/*********************************************************************************************/

void NewKey(void)
{
    struct KeyInfo ki = {0};
    
    StringToKey();
    
    DoMethod(list, MUIM_List_InsertSingle, (IPTR)&ki, MUIV_List_Insert_Bottom);
    nnset(list, MUIA_List_Active, MUIV_List_Active_Bottom);
    nnset(liststr, MUIA_String_Contents, "");
    nnset(liststr, MUIA_Disabled, FALSE);
    set(wnd, MUIA_Window_ActiveObject, (IPTR)liststr);
    
    RethinkAction();
}

/*********************************************************************************************/

void DelKey(void)
{
    struct KeyInfo *ki = NULL;

    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&ki);

    if (ki)
    {
    	DoMethod(list, MUIM_List_Remove, MUIV_List_Remove_Active);
    	DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&ki);
    	if (!ki)
	{
	    nnset(liststr, MUIA_String_Contents, (IPTR)"");
	    nnset(liststr, MUIA_Disabled, TRUE);
	    
	    ListToString();
	}
    }
}

/*********************************************************************************************/

void StringToKey(void)
{
    struct KeyInfo *ki = NULL;
    STRPTR  	    text;

    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&ki);
    if (!ki) return;
    
    get(liststr, MUIA_String_Contents, &text);    
    strncpy(ki->descr, text, sizeof(ki->descr));

    DoMethod(list, MUIM_List_Redraw, MUIV_List_Redraw_Active);
    
    RethinkKey(ki);
}

/*********************************************************************************************/

void ListToString(void)
{
    struct KeyInfo *ki = NULL;

    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&ki);
    if (!ki) return;
    
    nnset(liststr, MUIA_String_Contents, ki->descr);
    
    switch(ki->action)
    {
    	case ACTION_INSERT_TEXT:
	    nnset(insertstr, MUIA_String_Contents, ki->param);
	    break;
	    
	case ACTION_RUN_PROG:
	    nnset(runprogstr, MUIA_String_Contents, ki->param);
	    break;
	    
	case ACTION_RUN_AREXX:
	    nnset(runarexxstr, MUIA_String_Contents, ki->param);
	    break;
    }
    
    nnset(cmdcycle, MUIA_Cycle_Active, ki->action);
    nnset(cmdpage, MUIA_Group_ActivePage, ki->action);
}

/*********************************************************************************************/

void CmdToKey(void)
{
    RethinkAction();
}

/*********************************************************************************************/

void HandleAction(void)
{
    struct KeyInfo  *ki;
    struct Window   *win;
    struct Screen   *scr;
    CxObj   	    *cobj;
    WORD    	     i;
        
    cobj = activated_custom_cobj;	
    activated_custom_cobj = NULL;
    
    for(i = 0; ; i++)
    {
	DoMethod(list, MUIM_List_GetEntry, i, (IPTR)&ki);
	if (!ki) break;
	
	if (ki->custom == cobj) break;
    }
    
    if (!ki) return;
    
    win = IntuitionBase->ActiveWindow;
    scr = IntuitionBase->FirstScreen;
    
    switch(ki->action)
    {
    	case ACTION_CYCLE_WIN:
	    if (win)
	    {
	    	struct Layer *lay;
		
	    	scr = win->WScreen;
		win = NULL;

		LockLayerInfo(&scr->LayerInfo);
		lay = scr->LayerInfo.top_layer;
		while(lay)
		{
		    if (lay->Window &&
		    	(lay != scr->BarLayer) &&
		    	!(lay->Flags & LAYERBACKDROP) &&
			!(((struct Window *)lay->Window)->Flags & WFLG_BORDERLESS))
		    {
		    	win = (struct Window *)lay->Window;
		    }
			
		    lay = lay->back;
		}		
		UnlockLayerInfo(&scr->LayerInfo);
		
		if (win)
		{
		    WindowToFront(win);
		    ActivateWindow(win);
		}
	    }
	    break;
	    
	case ACTION_CYCLE_SCR:
	    if (scr) ScreenToBack(scr);
	    break;
	    
	case ACTION_ENLARGE_WIN:
	    if (win && (win->Flags & WFLG_SIZEGADGET))
	    {
	    	WORD neww = win->MaxWidth;
		WORD newh = win->MaxHeight;
		
		if (neww > win->WScreen->Width) neww = win->WScreen->Width;
		if (newh > win->WScreen->Height) newh = win->WScreen->Height;
		
		ChangeWindowBox(win, win->LeftEdge, win->TopEdge, neww, newh);
	    }
	    break;
	    
	case ACTION_SHRINK_WIN:
	    if (win && (win->Flags & WFLG_SIZEGADGET))
	    {
	    	WORD neww = win->MinWidth;
		WORD newh = win->MinHeight;
		
		ChangeWindowBox(win, win->LeftEdge, win->TopEdge, neww, newh);
	    }
	    break;
	    
	case ACTION_TOGGLE_WIN:
	    if (win) ZipWindow(win);
	    break;
	    
	case ACTION_RUN_PROG:
	    if (ki->param)
	    {
	    	BPTR infh;
		
		infh = Open("CON:20/20/500/300/FKey/CLOSE/AUTO/WAIT", MODE_READWRITE);
		if (infh)
		{
		    struct TagItem systemtags[] =
		    {
		    	{SYS_Asynch , TRUE  	},
			{SYS_Input  , (IPTR)infh},
			{SYS_Output , 0	    	},
			{TAG_DONE   	    	}
		    };
		    
		    if (SystemTagList(ki->param, systemtags) == -1)
		    {
		    	/* Error */
		    	Close(infh);
		    }
		}
	    }
	    break;
	    
	case ACTION_RUN_AREXX:
	    break;

    } /* switch(ki->action) */
    
}

/*********************************************************************************************/

void HandleAll(void)
{
    ULONG sigs = 0;
    LONG  returnid;
    
    set (wnd, MUIA_Window_Open, TRUE);
    
    for(;;)
    {
    	returnid = (LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs);

	if (returnid == MUIV_Application_ReturnID_Quit) break;
	
	switch(returnid)
	{
	    case RETURNID_NEWKEY:
	    	NewKey();
	    	break;
		
	    case RETURNID_DELKEY:
    	    	DelKey();
		break;
		
	    case RETURNID_LVACK:
	    	ListToString();
		break;
		
	    case RETURNID_STRINGACK:
	    	StringToKey();
		break;
		
	    case RETURNID_CMDACK:
	    	CmdToKey();
		break;
	}
	
	if (sigs)
	{
	    sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);
	    if (sigs & SIGBREAKF_CTRL_C) break;
	    if (sigs & SIGBREAKF_CTRL_E) HandleAction();
	    if (sigs & SIGBREAKF_CTRL_F)
	    {
	    	set(wnd, MUIA_Window_Open, TRUE);
		DoMethod(wnd, MUIM_Window_ToFront);
	    }
	}
    }

}

/*********************************************************************************************/

int main(int argc, char **argv)
{
    GetArguments(argc, argv);
    InitLocale("System/Tools/Commodities.catalog", 0);
    InitCX();
    InitMenus();
    MakeGUI();
    HandleAll();       
    Cleanup(NULL);
    
    return 0;
}
