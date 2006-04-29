/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Exchange -- controls commodities.
 */

/******************************************************************************

    NAME

        Exchange

    SYNOPSIS

        CX_PRIORITY/K/N,CX_POPUP/K/S,CX_POPKEY/K

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Manages the commodities in the system

    INPUTS

        CX_PRIORITY  --  Priority of the Exchange broker
	CX_POPUP     --  Appear at startup
	CX_POPKEY    --  Hotkey combination for Exchange

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cxintern.h>

#define DEBUG 1
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/utility.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Tools/Commodities.catalog"
#define CATALOG_VERSION  2

TEXT version[] = "$VER: Exchange 0.5 (29.04.2006)";

#define ARG_TEMPLATE "CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S"
#define DEF_POPKEY "ctrl alt h"

enum {
    ARG_CXPRI,
    ARG_CXPOPKEY,
    ARG_CXPOPUP,
    NUM_ARGS
};

static Object *app, *wnd, *listgad, *textgad1, *textgad2, *showgad, *hidegad, *cyclegad, *killgad;
static CONST_STRPTR cyclestrings[3];
static struct Catalog *catalog;
static UBYTE s[257];
static struct List brokerList;

static struct Hook broker_hook;
static struct Hook list_disp_hook;
static struct Hook list_select_hook;
static struct Hook inform_broker_hook;

static LONG cx_pri;
static char *cx_popkey;
static BOOL cx_popup = FALSE;
static CxObj *broker;
static struct MsgPort *brokermp;
static struct Task *maintask;

static void Cleanup(CONST_STRPTR txt);
static void GetArguments(int argc, char **argv);
static void HandleAll(void);
static void InitMenus(void);
static VOID Locale_Deinitialize(VOID);
static BOOL Locale_Initialize(VOID);
static void MakeGUI(void);
static void showSimpleMessage(CONST_STRPTR msgString);
static void update_list(void);
static CONST_STRPTR _(ULONG id);

/*********************************************************************************************/

static CONST_STRPTR _(ULONG id)
{
    if (LocaleBase != NULL && catalog != NULL)
    {
	return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } 
    else 
    {
	return CatCompArray[id].cca_Str;
    }
}

#define __(id) ((IPTR) _(id))   /* Get a message, as an IPTR */

/*********************************************************************************************/

static BOOL Locale_Initialize(VOID)
{
    if (LocaleBase != NULL)
    {
	catalog = OpenCatalog
	    ( 
	     NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE 
	    );
    }
    else
    {
	catalog = NULL;
    }

    return TRUE;
}

/*********************************************************************************************/

static VOID Locale_Deinitialize(VOID)
{
    if(LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}

/*********************************************************************************************/

static void GetArguments(int argc, char **argv)
{
    static struct RDArgs *myargs;
    static IPTR args[NUM_ARGS];
    static UBYTE **wbargs;
    if (argc)
    {
	if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
	{
	    Fault(IoErr(), 0, s, 256);
	    Cleanup(s);
	}
	if (args[ARG_CXPRI]) cx_pri = *(LONG*)args[ARG_CXPRI];
	if (args[ARG_CXPOPKEY])
	{
	    cx_popkey = StrDup((char *)args[ARG_CXPOPKEY]);
	}
	else
	{
	    cx_popkey = StrDup(DEF_POPKEY);
	}
	if (args[ARG_CXPOPUP]) cx_popup = TRUE;
	FreeArgs(myargs);
    }
    else
    {
	wbargs = ArgArrayInit(argc, (UBYTE**)argv);
	cx_pri = ArgInt(wbargs, "CX_PRIORITY", 0);
	cx_popkey = StrDup(ArgString(wbargs, "CX_POPKEY", DEF_POPKEY));
	if (strnicmp(ArgString(wbargs, "CX_POPUP", "NO"), "Y", 1) == 0)
	{
	    cx_popup = TRUE;
	}
	ArgArrayDone();
    }
    D(bug("Exchange Arguments pri %d popkey %s popup %d\n", cx_pri, cx_popkey, cx_popup));
}

/*********************************************************************************************/

static struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT         },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_HIDE    },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ICONIFY },
     {NM_ITEM, NM_BARLABEL    	               },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT    },
    {NM_END                                    }
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
	    CONST_STRPTR str = _(id);

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

static void showSimpleMessage(CONST_STRPTR msgString)
{
    struct EasyStruct easyStruct;

    easyStruct.es_StructSize	= sizeof(easyStruct);
    easyStruct.es_Flags		= 0;
    easyStruct.es_Title		= _(MSG_EXCHANGE_CXNAME);
    easyStruct.es_TextFormat	= msgString;
    easyStruct.es_GadgetFormat	= _(MSG_OK);		

    if (IntuitionBase != NULL && !Cli() )
    {
	EasyRequestArgs(NULL, &easyStruct, NULL, NULL);
    }
    else
    {
	PutStr(msgString);
    }
}

/*********************************************************************************************/

static void update_list(void)
{
    struct BrokerCopy *node;
    LONG n = GetBrokerList(&brokerList);
    D(bug("Exchange: Number of Brokers %ld\n", n));

    nnset(textgad1, MUIA_Text_Contents, NULL);
    nnset(textgad2, MUIA_Text_Contents, NULL);

    set(listgad, MUIA_List_Quiet, TRUE);
    DoMethod(listgad, MUIM_List_Clear);
    ForeachNode(&brokerList, node)
    {
	D(bug("Exchange: Brokernode %s\n", node->bc_Name));
	DoMethod(listgad, MUIM_List_InsertSingle, node, MUIV_List_Insert_Bottom);
    }
    set(listgad, MUIA_List_Quiet, FALSE);
}

/*********************************************************************************************/

AROS_UFH3(void, broker_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(CxMsg *      , msg,    A1))
{
    AROS_USERFUNC_INIT

	D(bug("Exchange: Broker hook called\n"));
    if (CxMsgType(msg) == CXM_COMMAND)
    {
	if (CxMsgID(msg) == CXCMD_APPEAR)
	{
	    //This opens window if application was started with cx_popup=no
	    set(wnd, MUIA_Window_Open, TRUE);
	}
	else if (CxMsgID(msg) == CXCMD_LIST_CHG)
	{
	    update_list();
	}
    }
    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, list_display_func,
    AROS_UFHA(struct Hook *      , h,     A0),
    AROS_UFHA(char **            , array, A2),
    AROS_UFHA(struct BrokerCopy *, node,  A1))
{
    AROS_USERFUNC_INIT

    *array = node->bc_Name;
  
    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, list_select_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT

    struct BrokerCopy *bc;
    DoMethod(listgad, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&bc);
    if (bc)
    {
	BOOL showHide = (bc->bc_Flags & COF_SHOW_HIDE) == 0;
	BOOL active = (bc->bc_Flags & COF_ACTIVE) != 0;
	nnset(textgad1, MUIA_Text_Contents, bc->bc_Title);
	nnset(textgad2, MUIA_Text_Contents, bc->bc_Descr);
	nnset(hidegad, MUIA_Disabled, showHide);
	nnset(showgad, MUIA_Disabled, showHide);
	nnset(cyclegad, MUIA_Cycle_Active, active ? 0 : 1);
    }
  
    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, inform_broker_func,
    AROS_UFHA(struct Hook *, h,        A0),
    AROS_UFHA(Object*      , object,   A2),
    AROS_UFHA(LONG *       , command,  A1))
{
    AROS_USERFUNC_INIT

    struct BrokerCopy *bc;
    DoMethod(listgad, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR) &bc);
    if (bc)
    {
	D(bug("Exchange: Broker inform %s\n", bc->bc_Node.ln_Name));
	if (*command == CXCMD_ENABLE)
	{
	    if (XGET(cyclegad, MUIA_Cycle_Active) == 1)
		*command = CXCMD_DISABLE;
	}
	CxNotify(bc->bc_Node.ln_Name, *command);
    }

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

static void MakeGUI(void)
{
    cyclestrings[0] = _(MSG_EXCHANGE_CYCLE_ACTIVE);
    cyclestrings[1] = _(MSG_EXCHANGE_CYCLE_INACTIVE);
    Object *menu = MUI_MakeObject(MUIO_MenustripNM, &nm, 0);

    broker_hook.h_Entry = (HOOKFUNC)broker_func;
    list_disp_hook.h_Entry = (HOOKFUNC)list_display_func;
    list_select_hook.h_Entry = (HOOKFUNC)list_select_func;
    inform_broker_hook.h_Entry = (HOOKFUNC)inform_broker_func;
    
    static TEXT wintitle[100];
    snprintf(wintitle, sizeof(wintitle), _(MSG_EXCHANGE_WINTITLE), cx_popkey);
    
    app = ApplicationObject,
	MUIA_Application_Title, __(MSG_EXCHANGE_CXNAME),
	MUIA_Application_Version, (IPTR)version,
	MUIA_Application_Copyright, (IPTR)"Copyright  © 1995-2006, The AROS Development TEAM",
	MUIA_Application_Author, (IPTR)"The AROS Development Team",
	MUIA_Application_Description, __(MSG_EXCHANGE_CXDESCR),
	MUIA_Application_BrokerPri, cx_pri,
	MUIA_Application_BrokerHook, (IPTR)&broker_hook,
	MUIA_Application_Base, (IPTR)"EXCHANGE",
	MUIA_Application_SingleTask, TRUE,
	MUIA_Application_Menustrip, (IPTR)menu,
	SubWindow, (IPTR)(wnd = WindowObject,
	    MUIA_Window_Title, (IPTR)wintitle,
	    MUIA_Window_ID, MAKE_ID('E', 'X', 'C', 'H'),
	    WindowContents, (IPTR)(HGroup,
		Child, (IPTR)(VGroup,
		    GroupFrameT(_(MSG_EXCHANGE_LISTVIEW)),
		    Child, (IPTR)(ListviewObject,
			MUIA_Listview_List, (IPTR)(listgad = ListObject,
			    InputListFrame,
			    MUIA_List_DisplayHook, (IPTR)&list_disp_hook,
			    MUIA_CycleChain, 1,
			End),
		    End),
		End),
		Child, (IPTR)(BalanceObject, MUIA_CycleChain, 1, End),
		Child, (IPTR)(VGroup,
		    MUIA_HorizWeight, 150,
		    Child, (IPTR)(VGroup,
			GroupFrameT(_(MSG_EXCHANGE_INFO)),
			Child, (IPTR)(textgad1 = TextObject, StringFrame, End),
			Child, (IPTR)(textgad2 = TextObject, StringFrame, End),
		    End),
		    Child, (IPTR)HVSpace,
		    Child, (IPTR)(ColGroup(2),
			MUIA_Group_SameSize, TRUE,
			Child, (IPTR)(hidegad = SimpleButton(_(MSG_EXCHANGE_GAD_HIDE))),
			Child, (IPTR)(showgad = SimpleButton(_(MSG_EXCHANGE_GAD_SHOW))),
			Child, (IPTR)(cyclegad = MUI_MakeObject(MUIO_Cycle, NULL, cyclestrings)),
			Child, (IPTR)(killgad = SimpleButton(_(MSG_EXCHANGE_GAD_REMOVE))),
		    End),
		End),
	    End),
	End),
    End;
    
    if (! app)
	Cleanup(NULL); // Propably double start

    // enable hotkey
    maintask = FindTask(NULL);
    get(app, MUIA_Application_Broker, &broker);
    get(app, MUIA_Application_BrokerPort, &brokermp);
    if ( ! broker || ! brokermp)
	Cleanup(_(MSG_CANT_CREATE_GADGET));

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

    // initial entry of brokers to listgadget
    update_list();

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);

    DoMethod(app, MUIM_Notify, MUIA_Application_DoubleStart, TRUE,
	(IPTR)app, 3, MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(listgad, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
	    (IPTR)listgad, 2, MUIM_CallHook, (IPTR)&list_select_hook);

    DoMethod(cyclegad, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
	(IPTR)app, 3, MUIM_CallHook, (IPTR)&inform_broker_hook, CXCMD_ENABLE);

    DoMethod(hidegad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 3, MUIM_CallHook, (IPTR)&inform_broker_hook, CXCMD_DISAPPEAR);

    DoMethod(showgad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 3, MUIM_CallHook, (IPTR)&inform_broker_hook, CXCMD_APPEAR);

    DoMethod(killgad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 3, MUIM_CallHook, (IPTR)&inform_broker_hook, CXCMD_KILL);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_QUIT,
	(IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_HIDE,
	(IPTR)app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_ICONIFY,
	(IPTR)app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);
     
}

/*********************************************************************************************/

static void HandleAll(void)
{
    if (cx_popup)
	set(wnd, MUIA_Window_Open, TRUE);
    
    ULONG sigs = 0;
    while((LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
	    != MUIV_Application_ReturnID_Quit)
    {
	if (sigs)
	{
	    sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
	    if (sigs & SIGBREAKF_CTRL_C)
	    {
		break;
	    }
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

static void Cleanup(CONST_STRPTR txt)
{
    MUI_DisposeObject(app);
    FreeVec(cx_popkey);
    FreeBrokerList(&brokerList);
    if (txt)
    {
	showSimpleMessage(txt);
	exit(RETURN_ERROR);
    }
    exit(RETURN_OK);
}

/*********************************************************************************************/

int main(int argc, char **argv)
{
    NewList(&brokerList);
    GetArguments(argc, argv);
    InitMenus();    
    MakeGUI();
    HandleAll();
    Cleanup(NULL);
    return RETURN_OK;
}

/*********************************************************************************************/

ADD2INIT(Locale_Initialize,   90);
ADD2EXIT(Locale_Deinitialize, 90);

