/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include <intuition/classusr.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <libraries/asl.h>
#include <libraries/locale.h>
#include <libraries/gadtools.h>
#include <libraries/commodities.h>
#include <libraries/mui.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
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
#include <proto/icon.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define CATCOMP_ARRAY
#include "strings.h"

#include <aros/debug.h>

/*********************************************************************************************/

#define VERSION 	1
#define REVISION 	4
#define DATESTR 	"11.04.2006"
#define VERSIONSTR	"$VER: FKey 1.4 (" DATESTR ")"

/*********************************************************************************************/

#define ARG_TEMPLATE "CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S,PORT/K,QUIET/S"

#define ARG_CXPRI   	0
#define ARG_CXPOPKEY 	1
#define ARG_CXPOPUP 	2
#define ARG_CXPORT  	3
#define ARG_QUIET   	4

#define NUM_ARGS    	5

/*********************************************************************************************/

#define ACTION_CYCLE_WIN     0
#define ACTION_CYCLE_SCR     1
#define ACTION_ENLARGE_WIN   2
#define ACTION_SHRINK_WIN    3
#define ACTION_TOGGLE_WIN    4
#define ACTION_RESCUE_WIN    5
#define ACTION_INSERT_TEXT   6
#define ACTION_RUN_PROG      7
#define ACTION_RUN_AREXX     8

#define RETURNID_NEWKEY      1
#define RETURNID_DELKEY      2
#define RETURNID_STRINGACK   3
#define RETURNID_LVACK	     4
#define RETURNID_CMDACK      5
#define RETURNID_SAVE	     6
#define RETURNID_DOUBLESTART 7

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
static struct WBStartup *wbstartup;
static LONG 	     	 prog_exitcode;
static UBYTE	       **wbargs;
static IPTR 	      	 args[NUM_ARGS];
static UBYTE	      	 s[257];

/*********************************************************************************************/

static void broker_func(struct Hook *hook, Object *obj, CxMsg *msg);
static UBYTE *BuildToolType(struct KeyInfo *ki);
static UBYTE **BuildToolTypes(UBYTE **src_ttypes);
static void Cleanup(CONST_STRPTR msg);
static void CleanupLocale(void);
static void CmdToKey(void);
static void DelKey(void);
static struct DiskObject *LoadProgIcon(BPTR *icondir, STRPTR iconname);
static void FreeArguments(void);
static void FreeToolTypes(UBYTE **ttypes);
static void GetArguments(int argc, char **argv);
static void HandleAction(void);
static void HandleAll(void);
static void InitCX(void);
static void InitLocale(STRPTR catname, ULONG version);
static void InitMenus(void);
static APTR keylist_construct_func(struct Hook *hook, APTR pool, struct KeyInfo *ki);
static void keylist_destruct_func(struct Hook *hook, APTR pool, struct KeyInfo *ki);
static void keylist_disp_func(struct Hook *hook, char **array, struct KeyInfo *ki);
static void KillCX(void);
static void KillGUI(void);
static void ListToString(void);
static void LoadSettings(void);
static void MakeGUI(void);
static CONST_STRPTR MSG(ULONG id);
static void NewKey(void);
static void RethinkAction(void);
static void RethinkKey(struct KeyInfo *ki);
static void SaveSettings(void);
static WORD ShowMessage(CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadtext);
static void StringToKey(void);

/*********************************************************************************************/

static WORD ShowMessage(CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadtext)
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

static void Cleanup(CONST_STRPTR msg)
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

static void InitCX(void)
{
    maintask = FindTask(NULL);
}

/*********************************************************************************************/

static void KillCX(void)
{
}

/*********************************************************************************************/

static void InitLocale(STRPTR catname, ULONG version)
{
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 39);
    if (LocaleBase)
    {
	catalog = OpenCatalog(NULL, catname, OC_Version, version,
					     TAG_DONE);
    }
}

/*********************************************************************************************/

static void CleanupLocale(void)
{
    if (catalog) CloseCatalog(catalog);
    if (LocaleBase) CloseLibrary((struct Library *)LocaleBase);
}

/*********************************************************************************************/

static CONST_STRPTR MSG(ULONG id)
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

static struct NewMenu nm[] =
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

static void InitMenus(void)
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

static void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
    ArgArrayDone();
}

/*********************************************************************************************/

static void KillGUI(void)
{
    DisposeObject(app);
}

/*********************************************************************************************/

static void GetArguments(int argc, char **argv)
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
    	wbstartup = (struct WBStartup *)argv;
    	wbargs = ArgArrayInit(argc, (UBYTE **)argv);

	cx_pri = ArgInt(wbargs, "CX_PRIORITY", 0);
	cx_popkey = ArgString(wbargs, "CX_POPKEY", cx_popkey);
	
	if (strnicmp(ArgString(wbargs, "CX_POPUP", "NO"), "Y", 1) == 0)
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
    D(bug("FKey: broker_func called\n"));
    if ( (CxMsgType(msg) == CXM_COMMAND) && (CxMsgID(msg) == CXCMD_APPEAR) )
    {
	// This opens the window if FKey was started with CX_POPUP=NO
	set(wnd, MUIA_Window_Open, TRUE);
	D(bug("FKey: CXCMD_APPEAR message\n"));
    }
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
	(CONST_STRPTR)MSG_FKEY_CMD_RESCUE_WIN,
    	(CONST_STRPTR)MSG_FKEY_CMD_INSERT_TEXT,
    	(CONST_STRPTR)MSG_FKEY_CMD_RUN_PROG,
    	(CONST_STRPTR)MSG_FKEY_CMD_RUN_AREXX,
	0,
    };
    static TEXT wintitle[100];
    WORD i;
    Object *menu, *newkey, *delkey, *savekey;
    
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
	MUIA_Application_Copyright, (IPTR)"Copyright © 1995-2006, The AROS Development Team",
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
		    Child, savekey = SimpleButton(MSG(MSG_FKEY_SAVE_KEY)),
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
			Child, HVSpace, /* rescue win */
                        Child, insertstr = StringObject, StringFrame, End, /* Insert text */
			Child, PopaslObject, /* Run prog */
				MUIA_Popstring_String, runprogstr = StringObject, StringFrame, End,
				MUIA_Popstring_Button, PopButton(MUII_PopFile),
				ASLFR_RejectIcons, TRUE,
			End,		
			Child, PopaslObject, /* Run AREXX */
				MUIA_Popstring_String, runarexxstr = StringObject, StringFrame, End,
				MUIA_Popstring_Button, PopButton(MUII_PopFile),
				ASLFR_RejectIcons, TRUE,
			End,	
		    End,
		    Child, HVSpace,
		    End,
		End,
	    End,
	End;
	
    if (!app)
    {
    #if 1
    	Cleanup(NULL); /* Make no noise. Is ugly if FKey is double-started. */
    #else
    	Cleanup(MSG(MSG_CANT_CREATE_GADGET));
    #endif
    }
    
    get(app, MUIA_Application_Broker, &broker);  
    get(app, MUIA_Application_BrokerPort, &brokermp);  
    
    if (!broker || !brokermp)
	Cleanup(MSG(MSG_CANT_CREATE_GADGET));

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

    DoMethod(app, MUIM_Notify, MUIA_Application_DoubleStart, TRUE, (IPTR) app, 2, MUIM_Application_ReturnID, RETURNID_DOUBLESTART);
    
    //DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR) app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_FKEY_MEN_PROJECT_QUIT, (IPTR) app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_FKEY_MEN_PROJECT_HIDE, (IPTR) app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_FKEY_MEN_PROJECT_ICONIFY, (IPTR) app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_FKEY_MEN_PROJECT_SAVE, (IPTR) app, 2, MUIM_Application_ReturnID, RETURNID_SAVE);
    
    DoMethod(cmdcycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR)cmdpage, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
    DoMethod(cmdcycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    DoMethod(newkey, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_NEWKEY);
    DoMethod(delkey, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_DELKEY);
    DoMethod(savekey, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_SAVE);
    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_LVACK);
    DoMethod(liststr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_STRINGACK);
    DoMethod(insertstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    DoMethod(runprogstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    DoMethod(runarexxstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, RETURNID_CMDACK);
    
}

/*********************************************************************************************/

static void RethinkKey(struct KeyInfo *ki)
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

static void RethinkAction(void)
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

static void NewKey(void)
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

static void DelKey(void)
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

static void StringToKey(void)
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

static void ListToString(void)
{
    struct KeyInfo *ki = NULL;

    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&ki);
    if (!ki) return;
    
    nnset(liststr, MUIA_Disabled, FALSE);
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

static void CmdToKey(void)
{
    RethinkAction();
}

/*********************************************************************************************/

static void HandleAction(void)
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
	    
	case ACTION_RESCUE_WIN:
	    if (win)
	    {
		WORD dx = 0, dy = 0;
		if (win->LeftEdge < 0)
			dx = -win->LeftEdge;
		else if (win->LeftEdge + win->Width > win->WScreen->Width)
			dx = win->WScreen->Width - win->Width - win->LeftEdge;
		
		if (win->TopEdge + win->Height > win->WScreen->Height)
			dy = win->WScreen->Height - win->Height - win->TopEdge;
		else if (win->TopEdge < win->WScreen->BarHeight)
        {
			// try to keep the screen title bar visible
			if (win->WScreen->BarHeight + win->Height < win->WScreen->Height)
				dy = -win->TopEdge + win->WScreen->BarHeight;
			else
				dy = win->WScreen->Height - win->Height - win->TopEdge;
		}
		MoveWindow(win, dx, dy);
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

#define QUOTE_START 0xAB
#define QUOTE_END   0xBB

/*********************************************************************************************/

static UBYTE *BuildToolType(struct KeyInfo *ki)
{
    static UBYTE  ttstring[500];
    UBYTE   	 *param1 = "";
    UBYTE   	 *param2 = "";

    switch(ki->action)
    {
    	case ACTION_CYCLE_WIN:
	    param1 = "CYCLE";
	    break;
	    
	case ACTION_CYCLE_SCR:
	    param1 = "CYCLESCREEN";
	    break;
	    
	case ACTION_ENLARGE_WIN:
	    param1 = "MAKEBIG";
	    break;
	    
	case ACTION_SHRINK_WIN:
	    param1 = "MAKESMALL";
	    break;
	    
	case ACTION_TOGGLE_WIN:
	    param1 = "ZIPWINDOW";
	    break;
	    
	case ACTION_RESCUE_WIN:
	    param1 = "RESCUEWIN";
	    break;
	    
	case ACTION_INSERT_TEXT:
	    param1 = "INSERT ";
	    param2 = ki->param;
	    break;
	    
	case ACTION_RUN_PROG:
	    param1 = "RUN ";
	    param2 = ki->param;
	    break;
	    
	case ACTION_RUN_AREXX:
	    param1 = "AREXX ";
    	    param2 = ki->param;
	    break;
	       
    }
    
    snprintf(ttstring, sizeof(ttstring), "%c%s%c %s%s",
    	     QUOTE_START,
	     ki->descr,
	     QUOTE_END,
	     param1,
	     param2);
	     
    return ttstring;
}

/*********************************************************************************************/

static UBYTE **BuildToolTypes(UBYTE **src_ttypes)
{
    APTR     pool = CreatePool(MEMF_CLEAR, 200, 200);
    Object  *listobj = list;
    UBYTE   *tt;
    WORD     list_index = 0, num_ttypes = 0, alloc_ttypes = 10;
    
    UBYTE **dst_ttypes;
    
    if (!pool) return NULL;
    
    dst_ttypes = AllocPooled(pool, (alloc_ttypes + 2) * sizeof(UBYTE *));
    if (!dst_ttypes)
    {
    	DeletePool(pool);
	return NULL;
    }
    
    /* Put together final tooltypes list based on old tooltypes and
       new tooltypes all in one loop */
       
    for(;;)
    {
    	tt = NULL;
	
    	if (listobj)
	{
	    /* New tooltypes */
	    
    	    struct KeyInfo *ki = NULL;

    	    DoMethod(listobj, MUIM_List_GetEntry, list_index, (IPTR)&ki);
    	    list_index++;

	    if (ki)
	    {
	    	tt = BuildToolType(ki);
	    }
	    else
	    {
	    	listobj = NULL;
	    }	    
	}
	
	if (!listobj)
	{
	    /* Old tooltypes */
	    
	    if (src_ttypes) tt = *src_ttypes++;
	    if (!tt) break; /* Done. Skip out of "for(;;)" loop */

    	    if (tt[0] == QUOTE_START) continue; /* skip tooltype containing old settings */
	}
	
	if (!tt) break; /* Paranoia. Should not happen. */
	
	if (num_ttypes >= alloc_ttypes)
	{
	    UBYTE **new_dst_ttypes = AllocPooled(pool, (alloc_ttypes + 10 + 2) * sizeof(UBYTE *));
	    
	    if (!new_dst_ttypes)
	    {
	    	DeletePool(pool);
		return NULL;
	    }
	    
	    CopyMem(dst_ttypes + 1, new_dst_ttypes + 1, alloc_ttypes * sizeof(UBYTE *));
	    dst_ttypes = new_dst_ttypes;
	    alloc_ttypes += 10;
	}
	
	dst_ttypes[num_ttypes + 1] = AllocPooled(pool, strlen(tt) + 1);
	if (!dst_ttypes[num_ttypes + 1])
	{
    	    DeletePool(pool);
    	    return NULL;
	}
	
	CopyMem(tt, dst_ttypes[num_ttypes + 1], strlen(tt) + 1);
	num_ttypes++;
	
    }
    
    dst_ttypes[0] = (APTR)pool;
    
    return dst_ttypes + 1;
    
}

/*********************************************************************************************/

static void FreeToolTypes(UBYTE **ttypes)
{
    if (ttypes)
    {
    	DeletePool((APTR)ttypes[-1]);
    }
}

/*********************************************************************************************/

static struct DiskObject *LoadProgIcon(BPTR *icondir, STRPTR iconname)
{
    struct DiskObject *progicon;
    
    if (wbstartup)
    {
    	BPTR olddir;
	
	*icondir = wbstartup->sm_ArgList[0].wa_Lock;
	
	olddir = CurrentDir(*icondir);	
    	progicon = GetDiskObject(wbstartup->sm_ArgList[0].wa_Name);		
	CurrentDir(olddir);

	strncpy(iconname, wbstartup->sm_ArgList[0].wa_Name, 255);
    }
    else
    {	
	if (GetProgramName(iconname, 255))
	{
    	    BPTR olddir;
	    
	    *icondir = GetProgramDir();
	    
	    olddir = CurrentDir(*icondir);
    	    progicon = GetDiskObject(iconname);	    
	    CurrentDir(olddir);
	}	    
    }
    
    return progicon;
}

/*********************************************************************************************/

static void SaveSettings(void)
{
    struct DiskObject 	 *progicon;
    UBYTE   	    	**ttypes, **old_ttypes;
    UBYTE   	    	  iconname[256];
    BPTR    	    	  icondir = NULL;

    progicon = LoadProgIcon(&icondir, iconname);
        
    if (!progicon) return;

    old_ttypes = (UBYTE **)progicon->do_ToolTypes;
    if ((ttypes = BuildToolTypes(old_ttypes)))
    {
    	BPTR olddir;
	
#if 0 /* DEBUG */
    	UBYTE *tt, **ttypes_copy = ttypes;

	while((tt = *ttypes_copy++))
	{
	    kprintf("TT: %s\n", tt);
	}
#endif

    	olddir = CurrentDir(icondir);
		
    	progicon->do_ToolTypes = ttypes;
	PutDiskObject(iconname, progicon);
	progicon->do_ToolTypes = old_ttypes;
	
	CurrentDir(olddir);
		
    	FreeToolTypes(ttypes);
    }
    
    FreeDiskObject(progicon);
    
}

/*********************************************************************************************/

static void LoadSettings(void)
{
    struct DiskObject *progicon;
    UBYTE   	       iconname[256];
    BPTR    	       icondir = NULL;
    UBYTE   	      **ttypes, *tt;
    
    progicon = LoadProgIcon(&icondir, iconname);
    if (!progicon) return;

    if ((ttypes = (UBYTE **)progicon->do_ToolTypes))
    {
    	while((tt = *ttypes++))
	{
	    struct KeyInfo   ki = {0};
	    UBYTE   	    *quote_end;
	    
	    ki.action = 0xFF;

	    if ((tt[0] == QUOTE_START) && ((quote_end = strchr(tt, (char)QUOTE_END))))
	    {
	    	WORD len = quote_end - tt - 1;

		if (len >= sizeof(ki.descr)) continue;
		if (quote_end[1] != ' ') continue;

	    	strncpy(ki.descr, tt + 1, len);
    	    		
	    	if (strncmp(quote_end + 2, "CYCLE", 5 + 1) == 0)
		{
		    ki.action = ACTION_CYCLE_WIN;
		}
		else if (strncmp(quote_end + 2, "CYCLESCREEN", 11 + 1) == 0)
		{
		    ki.action = ACTION_CYCLE_SCR;
		}
		else if (strncmp(quote_end + 2, "MAKEBIG", 7 + 1) == 0)
		{
		    ki.action = ACTION_ENLARGE_WIN;
		}
		else if (strncmp(quote_end + 2, "MAKESMALL", 9 + 1) == 0)
		{
		    ki.action = ACTION_SHRINK_WIN;
		}
		else if (strncmp(quote_end + 2, "ZIPWINDOW", 9 + 1) == 0)
		{
		    ki.action = ACTION_TOGGLE_WIN;
		}
		else if (strncmp(quote_end + 2, "RESCUEWIN", 9 + 1) == 0)
		{
			ki.action = ACTION_RESCUE_WIN;
		}
		else if (strncmp(quote_end + 2, "INSERT ", 7) == 0)
		{
		    ki.action = ACTION_INSERT_TEXT;
		    strncpy(ki.param, quote_end + 2 + 7, sizeof(ki.param) - 1);
		    
		}
		else if (strncmp(quote_end + 2, "RUN ", 4) == 0)
		{
		    ki.action = ACTION_RUN_PROG;
		    strncpy(ki.param, quote_end + 2 + 4, sizeof(ki.param) - 1);
		}
		else if (strncmp(quote_end + 2, "AREXX ", 6) == 0)
		{
		    ki.action = ACTION_RUN_AREXX;
		    strncpy(ki.param, quote_end + 2 + 6, sizeof(ki.param) - 1);
		}
		
		if (ki.action != 0xFF)
		{
    	    	    DoMethod(list, MUIM_List_InsertSingle, (IPTR)&ki, MUIV_List_Insert_Bottom);
		}
		
	    } /* if ((tt[0] == QUOTE_START) && ((quote_end = strchr(tt, QUOTE_END)))) */
	    
	} /* while((tt = *ttypes++)) */
	
	{
	    LONG index;
	    
	    for(index = 0; ; index++)
	    {
	    	struct KeyInfo *ki = NULL;
		
		DoMethod(list, MUIM_List_GetEntry, index, (IPTR)&ki);
		if (!ki) break;
		
		RethinkKey(ki);
	    }
	}

    } /* if ((ttypes = (UBYTE **)progicon->do_ToolTypes)) */
    
    FreeDiskObject(progicon);
}

/*********************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs = 0;
    LONG  returnid;
    IPTR  num_list_entries = 0;
    
    get(list, MUIA_List_Entries, &num_list_entries);
    if ((num_list_entries == 0) || cx_popup)
    {
    	set (wnd, MUIA_Window_Open, TRUE);
    }
    
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
		
	    case RETURNID_SAVE:
    	    	SaveSettings();
		break;
		
	    case RETURNID_DOUBLESTART:
	    	set(wnd, MUIA_Window_Open, TRUE);
		break;
	}
	
	if (sigs)
	{
	    sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);
	    if (sigs & SIGBREAKF_CTRL_C) break;
	    if (sigs & SIGBREAKF_CTRL_E) HandleAction();
	    if (sigs & SIGBREAKF_CTRL_F)
	    {
	    	set(app, MUIA_Application_Iconified, FALSE);
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
    LoadSettings();
    HandleAll();       
    Cleanup(NULL);
    
    return 0;
}
