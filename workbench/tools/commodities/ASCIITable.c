/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    ASCIITable -- Insert characters to clipboard from GUI.
 */

/******************************************************************************

    NAME

        ASCIITable

    SYNOPSIS

        CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S

    LOCATION

        SYS:Tools/Commodities

    FUNCTION

        Insert characters to clipboard from GUI

    INPUTS

        CX_PRIORITY  --  Priority of the ASCIITable broker
        CX_POPKEY    --  Hotkey combination for ASCIITable
        CX_POPUP     --  Appear at startup

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

//#define DEBUG 1
#include <aros/debug.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <libraries/iffparse.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <devices/clipboard.h>
#include <workbench/startup.h>

#include <proto/muimaster.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/utility.h>
#include <proto/icon.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Tools/Commodities.catalog"
#define CATALOG_VERSION  3

TEXT version[] = "$VER: ASCIITable 1.2 (14.01.2012)";

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
static struct Hook show_hook;

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

static struct IOClipReq *CBOpen(ULONG unit);
static void CBClose(struct IOClipReq *ior);
static BOOL CBWriteFTXT(struct IOClipReq *ior, CONST_STRPTR string);
static BOOL CBWriteLong(struct IOClipReq *ior, LONG *ldata);
static struct DiskObject *disko;

/*** ASCIITable class *******************************************************/

#define MAXLEN (60)

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
    TEXT buffer[MAXLEN + 1];
    struct IOClipReq *clip_req;
    TEXT shorthelp[192][20];
};

/*** CBOpen *****************************************************************/
static struct IOClipReq *CBOpen(ULONG unit)
{
    struct MsgPort *mp;
    struct IORequest *ior;

    if ((mp = CreatePort(0, 0)))
    {
        if ((ior = (struct IORequest *)CreateExtIO(mp, sizeof(struct IOClipReq))))
        {
            if (!(OpenDevice("clipboard.device", unit, ior, 0)))
            {
                return (struct IOClipReq *)ior;
            }
            DeleteExtIO(ior);
        }
        DeletePort(mp);
    }
    return NULL;
}

/*** CBCLose ****************************************************************/
static void CBClose(struct IOClipReq *ior)
{
    if (ior)
    {
        struct MsgPort *mp = ior->io_Message.mn_ReplyPort;

        CloseDevice((struct IORequest *)ior);
        DeleteExtIO((struct IORequest *)ior);
        DeletePort(mp);
    }
}

/*** CBWriteFTXT ************************************************************/
static BOOL CBWriteFTXT(struct IOClipReq *ior, CONST_STRPTR string)
{
    LONG length, slen, temp;
    BOOL odd;

    if (!ior || !string)
        return FALSE;

    slen = strlen(string);
    odd = (slen & 1);

    length = (odd) ? slen+1 : slen;

    ior->io_Offset = 0;
    ior->io_Error  = 0;
    ior->io_ClipID = 0;

    CBWriteLong(ior, (LONG *) "FORM");
    length += 12;

    temp = AROS_LONG2BE(length);
    CBWriteLong(ior, &temp);
    CBWriteLong(ior, (LONG *) "FTXT");
    CBWriteLong(ior, (LONG *) "CHRS");
    temp = AROS_LONG2BE(slen);
    CBWriteLong(ior, &temp);

    ior->io_Data    = (STRPTR)string;
    ior->io_Length  = slen;
    ior->io_Command = CMD_WRITE;
    DoIO((struct IORequest *)ior);

    if (odd)
    {
        ior->io_Data   = (APTR)"";
        ior->io_Length = 1;
        DoIO((struct IORequest *)ior);
    }

    ior->io_Command=CMD_UPDATE;
    DoIO ((struct IORequest *)ior);

    return ior->io_Error ? FALSE : TRUE;
}

/*** WriteLong **************************************************************/
static BOOL CBWriteLong(struct IOClipReq *ior, LONG *ldata)
{
    ior->io_Data    = (APTR)ldata;
    ior->io_Length  = 4;
    ior->io_Command = CMD_WRITE;
    DoIO( (struct IORequest *) ior);

    if (ior->io_Actual == 4)
    {
        return ior->io_Error ? FALSE : TRUE;
    }

    return FALSE;
}

/*** MakeButton *************************************************************/
static Object *MakeButton(LONG code)
{
    char buffer[2] = {0};
    buffer[0] = code;

    Object *btn = (Object *)TextObject,
        ButtonFrame,
        MUIA_Font, MUIV_Font_Button,
        MUIA_Text_Contents, (IPTR)buffer,
        MUIA_Text_PreParse, (IPTR)"\33c",
        MUIA_InputMode    , MUIV_InputMode_RelVerify,
        MUIA_Background   , MUII_ButtonBack,
        MUIA_CycleChain   , TRUE,
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
        key_group_tags[i].ti_Data = (IPTR)MakeButton(code);
    }
    for (code = 160 ; code < 256 ; code++, i++)
    {
        key_group_tags[i].ti_Tag = Child;
        key_group_tags[i].ti_Data = (IPTR)MakeButton(code);
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
        Child, (IPTR)(ascii_string = (Object *)StringObject,
            StringFrame,
            MUIA_String_MaxLen, MAXLEN,
        End),
        Child, (IPTR) (HGroup,
            MUIA_Weight,         0,
            MUIA_Group_SameSize, TRUE,
            
            Child, (IPTR) (copy_button    = SimpleButton(_(MSG_ASCIITABLE_GAD_COPY))),
            Child, (IPTR) (clear_button   = SimpleButton(_(MSG_ASCIITABLE_GAD_CLEAR))),
        End),
        TAG_MORE, message->ops_AttrList
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
            sprintf(data->shorthelp[i], "%c\n%d\n0x%x", (int)code, (int)code, (unsigned int)code);
            set((Object *)key_group_tags[i].ti_Data, MUIA_ShortHelp, data->shorthelp[i]);
            DoMethod
            (
                (Object *)key_group_tags[i].ti_Data, MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR) self, 2, MUIM_ASCIITable_Insert, code
            );
        }
        data->clip_req = CBOpen(0);
        if (!data->clip_req)
        {
            showSimpleMessage(_(MSG_CANT_OPEN_CLIPDEVICE));
        }
    }

    return (IPTR) self;
}

/*** OM_DISPOSE *************************************************************/
IPTR ASCIITable__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct ASCIITable_DATA *data = INST_DATA(CLASS, self);

    CBClose(data->clip_req);
    
    return DoSuperMethodA(CLASS, self, message);
}

/*** MUIM_ASCIITable_Copy ***************************************************/
IPTR ASCIITable__MUIM_ASCIITable_Copy(Class *CLASS, Object *self, Msg msg)
{
    struct ASCIITable_DATA *data = INST_DATA(CLASS, self);
    CBWriteFTXT(data->clip_req, (CONST_STRPTR)XGET(data->ascii_string, MUIA_String_Contents));
    return TRUE;
}

/*** MUIM_ASCIITable_Clear **************************************************/
IPTR ASCIITable__MUIM_ASCIITable_Clear(Class *CLASS, Object *self, Msg msg)
{
    struct ASCIITable_DATA *data = INST_DATA(CLASS, self);
    data->buffer[0] = '\0';
    set(data->ascii_string, MUIA_String_Contents, "");
    return TRUE;
}

/*** MUIM_ASCIITable_Insert *************************************************/
IPTR ASCIITable__MUIM_ASCIITable_Insert(Class *CLASS, Object *self, struct MUIP_ASCIITable_Insert *msg)
{
    struct ASCIITable_DATA *data = INST_DATA(CLASS, self);
    LONG len;
    D(bug("insert code %d\n", msg->code));
    strcpy(data->buffer, (CONST_STRPTR)XGET(data->ascii_string, MUIA_String_Contents));
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
ZUNE_CUSTOMCLASS_5
(
    ASCIITable, NULL, MUIC_Group, NULL,
    OM_NEW,                   struct opSet *,
    OM_DISPOSE,               Msg,
    MUIM_ASCIITable_Copy,     Msg,
    MUIM_ASCIITable_Clear,    Msg,
    MUIM_ASCIITable_Insert,   struct MUIP_ASCIITable_Insert *
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

/*** Locale_Initialize ******************************************************/
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

/*** Locale_Deinitialize ****************************************************/
static VOID Locale_Deinitialize(VOID)
{
    if (LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}

/*** GetArguments ***********************************************************/
static void GetArguments(int argc, char **argv)
{
    static struct RDArgs *myargs;
    static IPTR args[NUM_ARGS];
    static UBYTE **wbargs;
    static STRPTR cxname;
    static struct WBStartup *argmsg;
    static struct WBArg *wb_arg;

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
        cxname = argv[0];
    }
    else
    {
        argmsg = (struct WBStartup *)argv;
        wb_arg = argmsg->sm_ArgList;
        cxname = wb_arg->wa_Name;
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
    disko = GetDiskObject(cxname);
}

/****************************************************************************/
static struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT         },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_HIDE    },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ICONIFY },
     {NM_ITEM, NM_BARLABEL    	               },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT    },
    {NM_END                                    }
};

/*** InitMenus **************************************************************/
static void InitMenus(void)
{
    struct NewMenu *actnm = nm;

    for(actnm = nm; actnm->nm_Type != NM_END; actnm++)
    {
        if (actnm->nm_Label != NM_BARLABEL)
        {
            ULONG  id = (IPTR)actnm->nm_Label;
            CONST_STRPTR str = _(id);

            if (actnm->nm_Type == NM_TITLE)
            {
                actnm->nm_Label = str;
            } else {
                actnm->nm_Label = str + 2;
                if (str[0] != ' ') actnm->nm_CommKey = str;
            }
            actnm->nm_UserData = (APTR)(IPTR)id;

        } /* if (actnm->nm_Label != NM_BARLABEL) */

    } /* for(actnm = nm; nm->nm_Type != NM_END; nm++) */
}

/*** showSimpleMessage ******************************************************/
static void showSimpleMessage(CONST_STRPTR msgString)
{
    struct EasyStruct easyStruct;

    easyStruct.es_StructSize	= sizeof(easyStruct);
    easyStruct.es_Flags		= 0;
    easyStruct.es_Title		= _(MSG_ASCIITABLE_CXNAME);
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

/*** broker_func ************************************************************/
AROS_UFH3(void, broker_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(CxMsg *      , msg,    A1))
{
    AROS_USERFUNC_INIT

    D(bug("ASCIITable: Broker hook called\n"));
    if (CxMsgType(msg) == CXM_COMMAND)
    {
        if (CxMsgID(msg) == CXCMD_APPEAR)
        {
            CallHookPkt(&show_hook, NULL, NULL);
        }
        else if (CxMsgID(msg) == CXCMD_DISAPPEAR)
        {
            set(wnd, MUIA_Window_Open, FALSE);
        }
    }
    AROS_USERFUNC_EXIT
}

/*** show_func ************************************************************/
AROS_UFH3(
    void, show_func,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    if (XGET(app, MUIA_Application_Iconified) == TRUE)
        set(app, MUIA_Application_Iconified, FALSE);
    else
        set(wnd, MUIA_Window_Open, TRUE);

    AROS_USERFUNC_EXIT
}

/*** MakeGUI ****************************************************************/
static void MakeGUI(void)
{
    Object *menu;
    static TEXT wintitle[100];
    CxObj *popfilter;

    menu = MUI_MakeObject(MUIO_MenustripNM, &nm, 0);

    broker_hook.h_Entry = (HOOKFUNC)broker_func;
    show_hook.h_Entry = (HOOKFUNC)show_func;
    
    snprintf(wintitle, sizeof(wintitle), _(MSG_ASCIITABLE_WINTITLE), cx_popkey);
    
    app = (Object *)ApplicationObject,
        MUIA_Application_Title, __(MSG_ASCIITABLE_CXNAME),
        MUIA_Application_Version, (IPTR)version,
        MUIA_Application_Copyright, (IPTR)"Copyright  © 2012, The AROS Development TEAM",
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Description, __(MSG_ASCIITABLE_CXDESCR),
        MUIA_Application_BrokerPri, cx_pri,
        MUIA_Application_BrokerHook, (IPTR)&broker_hook,
        MUIA_Application_Base, (IPTR)"ASCIITABLE",
        MUIA_Application_SingleTask, TRUE,
        MUIA_Application_Menustrip, (IPTR)menu,
        MUIA_Application_DiskObject, (IPTR)disko,
        SubWindow, (IPTR)(wnd = (Object *)WindowObject,
            MUIA_Window_Title, (IPTR)wintitle,
            MUIA_Window_ID, MAKE_ID('A', 'I', 'T', 'B'),
            WindowContents, (IPTR)ASCIITableObject,
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
            if (trans) AttachCxObj(popfilter, trans);
        }
        AttachCxObj(broker, popfilter);
    }

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(app, MUIM_Notify, MUIA_Application_DoubleStart, TRUE,
        (IPTR)wnd, 2, MUIM_CallHook, &show_hook);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_QUIT,
        (IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_HIDE,
        (IPTR)wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_ICONIFY,
        (IPTR)app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);
}

/*** HandleAll **************************************************************/
static void HandleAll(void)
{
    ULONG sigs = 0;

    set(wnd, MUIA_Window_Open, cx_popup);
    
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
                CallHookPkt(&show_hook, NULL, NULL);
            }
        }
    }
}

/*** Cleanup ****************************************************************/
static void Cleanup(CONST_STRPTR txt)
{
    MUI_DisposeObject(app);
    FreeVec(cx_popkey);
    FreeDiskObject(disko);
    if (txt)
    {
        showSimpleMessage(txt);
        exit(RETURN_ERROR);
    }
    exit(RETURN_OK);
}

/*** main *******************************************************************/
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

/****************************************************************************/
ADD2INIT(Locale_Initialize,   90);
ADD2EXIT(Locale_Deinitialize, 90);
