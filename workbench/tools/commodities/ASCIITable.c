/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    ASCIITable -- Insert characters to clipboard from GUI.
 */

/******************************************************************************

    NAME

        Exchange

    SYNOPSIS

        CX_PRIORITY/K/N,CX_POPUP/K/S,CX_POPKEY/K

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Insert characters to clipboard from GUI

    INPUTS

        CX_PRIORITY  --  Priority of the Exchange broker
        CX_POPUP     --  Appear at startup
        CX_POPKEY    --  Hotkey combination for Exchange

    RESULT

    NOTES

        TODO: copy to clipboard, localisation, icon

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG 1
#include <aros/debug.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <libraries/iffparse.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <proto/muimaster.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/utility.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Tools/Commodities.catalog"
#define CATALOG_VERSION  3

TEXT version[] = "$VER: ASCIITable 1.0 (27.05.2009)";

#define ARG_TEMPLATE "CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S"
#define DEF_POPKEY "ctrl alt a"

enum {
    ARG_CXPRI,
    ARG_CXPOPKEY,
    ARG_CXPOPUP,
    NUM_ARGS
};

static Object *app, *wnd;
static struct Catalog *catalog;
static UBYTE s[257];

static struct Hook broker_hook;

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
static int Locale_Initialize(VOID);
static void MakeGUI(void);
static void showSimpleMessage(CONST_STRPTR msgString);
static CONST_STRPTR _(ULONG id);


/*** ASCIITable class *******************************************************/

#define MAXLEN (30)

#define ASCIITableObject BOOPSIOBJMACRO_START(ASCIITable_CLASS->mcc_Class)

#define MUIM_ASCIITable_Copy   (TAG_USER | 1)
#define MUIM_ASCIITable_Clear  (TAG_USER | 2)
#define MUIM_ASCIITable_Insert (TAG_USER | 3)
struct  MUIP_ASCIITable_Insert  {STACKED ULONG MethodID; STACKED LONG code;};

struct ASCIITable_DATA
{
    Object *copy_button;
    Object *clear_button;
    Object *ascii_string;
    Object *key_button[192];
    TEXT buffer[MAXLEN + 1];
};


/*** MakeButton *************************************************************/
static Object *MakeButton(int code)
{
    char buffer[2] = {0};
    buffer[0] = code;

    Object *btn = TextObject,
        ButtonFrame,
        MUIA_Font, MUIV_Font_Button,
        MUIA_Text_Contents, buffer,
        MUIA_Text_PreParse, "\33c",
        MUIA_InputMode    , MUIV_InputMode_RelVerify,
        MUIA_Background   , MUII_ButtonBack,
        MUIA_CycleChain,    TRUE,
    End;
    return btn;
}

/*** OM_NEW *****************************************************************/
IPTR ASCIITable__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct ASCIITable_DATA *data = NULL;
    Object *copy_button, *clear_button, *ascii_string, *key_group;
    struct TagItem key_group_tags[200];
    LONG i, code;

    for (code = 32 , i = 0 ; code < 128 ; code++ , i++)
    {
        key_group_tags[i].ti_Tag = Child;
        key_group_tags[i].ti_Data = MakeButton(code);
    }
    for (code = 160 ; code < 256 ; code++, i++)
    {
        key_group_tags[i].ti_Tag = Child;
        key_group_tags[i].ti_Data = MakeButton(code);
    }
    key_group_tags[i].ti_Tag = MUIA_Group_Columns;
    key_group_tags[i].ti_Data = 16;
    key_group_tags[++i].ti_Tag = TAG_DONE;
    key_group = MUI_NewObjectA(MUIC_Group, key_group_tags);
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        Child, key_group,
        Child, (IPTR) (RectangleObject, 
            MUIA_Rectangle_HBar, TRUE,
            MUIA_FixHeight,      2,
        End),
        Child, ascii_string = StringObject,
            StringFrame,
            MUIA_String_MaxLen, MAXLEN,
        End,
        Child, (IPTR) HGroup,
            MUIA_Weight,         0,
            MUIA_Group_SameSize, TRUE,
            
            Child, (IPTR) (copy_button    = SimpleButton("Copy")),
            Child, (IPTR) (clear_button   = SimpleButton("Clear")),
        End,
        TAG_DONE

    );

    if (self != NULL)
    {
        /*-- Store important variables -------------------------------------*/
        data = INST_DATA(CLASS, self);
        data->copy_button = copy_button;
        data->clear_button = clear_button;
        data->ascii_string = ascii_string;

        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        (
            copy_button, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) self, 1, MUIM_ASCIITable_Copy
        );
        DoMethod
        (
            clear_button, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) self, 1, MUIM_ASCIITable_Clear
        );
        
        for (i = 0 ; i < 192 ; i++)
        {
            code = (i < 96) ? i + 32 : i + 64;
            DoMethod
            (
                key_group_tags[i].ti_Data, MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR) self, 2, MUIM_ASCIITable_Insert, code
            );
        }
    }

    return (IPTR) self;
}

/*** MUIM_ASCIITable_Copy ***************************************************/
IPTR ASCIITable__MUIM_ASCIITable_Copy(Class *CLASS, Object *self, Msg msg)
{
    struct ASCIITable_DATA *data = INST_DATA(CLASS, self);
    bug("Copy method\n");
    return TRUE;
}

/*** MUIM_ASCIITable_Clear **************************************************/
IPTR ASCIITable__MUIM_ASCIITable_Clear(Class *CLASS, Object *self, Msg msg)
{
    struct ASCIITable_DATA *data = INST_DATA(CLASS, self);
    bug("Clear method\n");
    data->buffer[0] = '\0';
    set(data->ascii_string, MUIA_String_Contents, "");
    return TRUE;
}

/*** MUIM_ASCIITable_Insert *************************************************/
IPTR ASCIITable__MUIM_ASCIITable_Insert(Class *CLASS, Object *self, struct MUIP_ASCIITable_Insert *msg)
{
    struct ASCIITable_DATA *data = INST_DATA(CLASS, self);
    LONG len;
    bug("insert code %d\n", msg->code);
    strcpy(data->buffer, XGET(data->ascii_string, MUIA_String_Contents));
    len = strlen(data->buffer);
    if (len < MAXLEN)
    {
        data->buffer[len] = msg->code;
        data->buffer[len+1] = '\0';
        set(data->ascii_string, MUIA_String_Contents, data->buffer);
    }
    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    ASCIITable, NULL, MUIC_Group, NULL,
    OM_NEW,                   struct opSet *,
    MUIM_ASCIITable_Copy,     Msg,
    MUIM_ASCIITable_Clear,    Msg,
    MUIM_ASCIITable_Insert,   Msg
);



/*** Begin application ******************************************************/


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

static int Locale_Initialize(VOID)
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
    if (LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
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
    D(bug("ASCIITable Arguments pri %d popkey %s popup %d\n", cx_pri, cx_popkey, cx_popup));
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
            set(app, MUIA_Application_Iconified, FALSE);
        }
    }
    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

static void MakeGUI(void)
{
    Object *menu;
    static TEXT wintitle[100];
    CxObj *popfilter;

    menu = MUI_MakeObject(MUIO_MenustripNM, &nm, 0);

    broker_hook.h_Entry = (HOOKFUNC)broker_func;
    
    snprintf(wintitle, sizeof(wintitle), _(MSG_EXCHANGE_WINTITLE), cx_popkey);
    
    app = (Object *)ApplicationObject,
        MUIA_Application_Title, __(MSG_EXCHANGE_CXNAME),
        MUIA_Application_Version, (IPTR)version,
        MUIA_Application_Copyright, (IPTR)"Copyright  © 2009, The AROS Development TEAM",
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Description, __(MSG_EXCHANGE_CXDESCR),
        MUIA_Application_BrokerPri, cx_pri,
        MUIA_Application_BrokerHook, (IPTR)&broker_hook,
        MUIA_Application_Base, (IPTR)"ASCIITABLE",
        MUIA_Application_SingleTask, TRUE,
        MUIA_Application_Menustrip, (IPTR)menu,
        SubWindow, (IPTR)(wnd = (Object *)WindowObject,
            MUIA_Window_Title, (IPTR)wintitle,
            MUIA_Window_ID, MAKE_ID('A', 'I', 'T', 'B'),
            WindowContents, ASCIITableObject,
            End,
        End),
    End;
    
    if (! app)
        Cleanup(NULL); // Propably double start

    // enable hotkey
    maintask = FindTask(NULL);
    get(app, MUIA_Application_Broker, &broker);
    get(app, MUIA_Application_BrokerPort, &brokermp);
    if ( ! broker || ! brokermp)
        Cleanup(_(MSG_CANT_CREATE_BROKER));

    popfilter = CxFilter(cx_popkey);
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

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);

    DoMethod(app, MUIM_Notify, MUIA_Application_DoubleStart, TRUE,
        (IPTR)app, 3, MUIM_Set, MUIA_Application_Iconified, FALSE);

    // Open the window when app isn't iconified
    DoMethod(app, MUIM_Notify, MUIA_Application_Iconified, FALSE,
        (IPTR)wnd, 3, MUIM_Set, MUIA_Window_Open, TRUE);

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
    ULONG sigs = 0;

    set(app, MUIA_Application_Iconified, cx_popup ? FALSE : TRUE);
    
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
            }
        }
    }
}

/*********************************************************************************************/

static void Cleanup(CONST_STRPTR txt)
{
    MUI_DisposeObject(app);
    FreeVec(cx_popkey);
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
    D(bug("ASCIITable started\n"));
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
