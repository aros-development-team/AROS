/*
 *----------------------------------------------------------------------------
 *                     btsdp class for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * The SDP server role, the classic counterpart of btgatt.class. The service
 * records are registered with the stack by the classes and programs that
 * serve them (btserial.class and its Serial Port, for one), and
 * bluetooth.library answers the devices that browse them. This class is
 * where the user decides what those devices get: its settings window lists
 * the registered classic services so that each can be switched off - it is
 * then missing from the records a device is shown, and a connection to it
 * is refused - and on again. Unlike a GATT service a classic one is offered
 * from the moment it is registered, so that nothing stops working where
 * this class is not installed.
 *
 * It can also have the radios put their name and the offered services into
 * the extended inquiry response, which a scanning device sees before it has
 * connected.
 */

#include "debug.h"

#include "btsdp.h"

static const STRPTR libname = MOD_NAME_STRING;

#define UtilityBase nh->nh_UtilityBase

static BOOL bOpenCfgWindow(struct BTSdpBase *nh);

/* /// "bRecordKey()" */
/* FALSE for a record that is none of ours (a GATT service). */
static BOOL bRecordKey(struct Library *BluetoothBase, APTR rec, struct BTSdpKey *key)
{
    IPTR proto = 0, uuid16 = 0, channel = 0, psm = 0;

    btGetAttrs(BGA_SERVICERECORD, rec, BSRA_Protocol, &proto, BSRA_UUID16, &uuid16,
               BSRA_RFCOMMChannel, &channel, BSRA_PSM, &psm, TAG_END);
    if((proto != BSVP_RFCOMM) && (proto != BSVP_L2CAP))
    {
        return(FALSE);
    }
    key->sk_UUID16 = uuid16;
    key->sk_Protocol = proto;
    key->sk_Port = (proto == BSVP_RFCOMM) ? channel : psm;
    key->sk_Pad = 0;
    return(TRUE);
}
/* \\\ */

/* /// "bDisabledIndex()" */
static LONG bDisabledIndex(struct BTSdpCfg *cfg, const struct BTSdpKey *key)
{
    ULONG n;

    for(n = 0; (n < cfg->sc_NumDisabled) && (n < BTSDP_MAXDISABLED); n++)
    {
        if(!memcmp(&cfg->sc_Disabled[n], key, sizeof(struct BTSdpKey)))
        {
            return((LONG) n);
        }
    }
    return(-1);
}
/* \\\ */

/* /// "bSetDisabled()" */
/* FALSE if there is no room to remember one more service as switched off. */
static BOOL bSetDisabled(struct BTSdpCfg *cfg, const struct BTSdpKey *key, BOOL disabled)
{
    LONG idx = bDisabledIndex(cfg, key);

    if(disabled)
    {
        if(idx >= 0)
        {
            return(TRUE);
        }
        if(cfg->sc_NumDisabled >= BTSDP_MAXDISABLED)
        {
            return(FALSE);
        }
        cfg->sc_Disabled[cfg->sc_NumDisabled++] = *key;
    }
    else if(idx >= 0)
    {
        cfg->sc_NumDisabled--;
        if((ULONG) idx != cfg->sc_NumDisabled)
        {
            cfg->sc_Disabled[idx] = cfg->sc_Disabled[cfg->sc_NumDisabled];
        }
    }
    return(TRUE);
}
/* \\\ */

/* /// "bApplyRecord()" */
/* Offer a classic service to connecting devices, or not. */
static void bApplyRecord(struct BTSdpBase *nh, struct Library *BluetoothBase, APTR rec, BOOL all)
{
    struct BTSdpKey key;

    if(bRecordKey(BluetoothBase, rec, &key))
    {
        btSetAttrs(BGA_SERVICERECORD, rec,
                   BSRA_Enabled, all || (bDisabledIndex(&nh->nh_Cfg, &key) < 0),
                   TAG_END);
    }
}
/* \\\ */

/* /// "bApplyPolicy()" */
/* Put the configuration into effect - or, with all set, what holds without
   this class: every service offered and nothing said to scanning devices. */
static void bApplyPolicy(struct BTSdpBase *nh, struct Library *BluetoothBase, BOOL all)
{
    struct List *list = NULL;
    struct Node *rec;

    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_ServiceRecordList, &list, TAG_END);
    if(list)
    {
        for(rec = list->lh_Head; rec->ln_Succ; rec = rec->ln_Succ)
        {
            bApplyRecord(nh, BluetoothBase, rec, all);
        }
    }
    btUnlockBase();
    btSetAttrs(BGA_STACK, NULL, BSA_EIRServices, (!all && nh->nh_Cfg.sc_EIRServices) ? TRUE : FALSE, TAG_END);
}
/* \\\ */

/* /// "bLoadClassConfig()" */
static void bLoadClassConfig(struct BTSdpBase *nh, struct Library *BluetoothBase)
{
    struct BTSdpCfg *cfg = &nh->nh_Cfg;
    struct BTSdpCfg *chunk;
    APTR pic;

    Forbid();
    /* default: what the stack does without us */
    memset(cfg, 0, sizeof(struct BTSdpCfg));
    cfg->sc_ChunkID = AROS_LONG2BE(MAKE_ID('S','D','P','S'));
    cfg->sc_Length = AROS_LONG2BE(sizeof(struct BTSdpCfg) - 8);
    nh->nh_UsingDefaultCfg = TRUE;
    if((pic = btGetClsCfg(libname)))
    {
        if((chunk = btGetCfgChunk(pic, AROS_LONG2BE(cfg->sc_ChunkID))))
        {
            ULONG len = AROS_LONG2BE(chunk->sc_Length);
            if(len > sizeof(struct BTSdpCfg) - 8)
            {
                len = sizeof(struct BTSdpCfg) - 8;
            }
            CopyMem(((UBYTE *) chunk) + 8, ((UBYTE *) cfg) + 8, len);
            btFreeVec(chunk);
            if(cfg->sc_NumDisabled > BTSDP_MAXDISABLED)
            {
                cfg->sc_NumDisabled = BTSDP_MAXDISABLED;
            }
            nh->nh_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
}
/* \\\ */

/* /// "bStoreClassConfig()" */
static void bStoreClassConfig(struct BTSdpBase *nh, struct Library *BluetoothBase, BOOL todisk)
{
    APTR pic;

    if(!(pic = btGetClsCfg(libname)))
    {
        btSetClsCfg(libname, NULL);
        pic = btGetClsCfg(libname);
    }
    if(pic && btAddCfgEntry(pic, &nh->nh_Cfg))
    {
        nh->nh_UsingDefaultCfg = FALSE;
        if(todisk)
        {
            btSaveCfgToDisk(NULL, FALSE);
        }
    }
}
/* \\\ */

/* /// "Lib Stuff" */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh)
{
    struct Library *BluetoothBase;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    if(!(nh->nh_UtilityBase = OpenLibrary("utility.library", 39)))
    {
        return(FALSE);
    }
    /* service records can be disabled since bluetooth.library 45.16 */
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        CloseLibrary(nh->nh_UtilityBase);
        return(FALSE);
    }
    bLoadClassConfig(nh, BluetoothBase);
    bApplyPolicy(nh, BluetoothBase, FALSE);
    CloseLibrary(BluetoothBase);
    return(TRUE);
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR nh)
{
    struct Library *BluetoothBase;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_GUITask)
    {
        return(FALSE);
    }
    if((BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        bApplyPolicy(nh, BluetoothBase, TRUE);
        CloseLibrary(BluetoothBase);
    }
    CloseLibrary(nh->nh_UtilityBase);
    return(TRUE);
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
/* \\\ */

/* /// "btcGetAttrsA()" */
AROS_LH3(LONG, btcGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, btsdp)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    LONG count = 0;

    switch(type)
    {
        case BCGA_CLASS:
            if((ti = FindTagItem(BCCA_Priority, tags)))
            {
                *((SIPTR *) ti->ti_Data) = 0;
                count++;
            }
            if((ti = FindTagItem(BCCA_Description, tags)))
            {
                *((STRPTR *) ti->ti_Data) = "SDP server: classic services offered to connecting devices";
                count++;
            }
            if((ti = FindTagItem(BCCA_HasClassCfgGUI, tags)))
            {
                *((IPTR *) ti->ti_Data) = TRUE;
                count++;
            }
            if((ti = FindTagItem(BCCA_HasBindingCfgGUI, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(BCCA_AfterDOSRestart, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(BCCA_UsingDefaultCfg, tags)))
            {
                *((IPTR *) ti->ti_Data) = nh->nh_UsingDefaultCfg;
                count++;
            }
            break;
    }
    return(count);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcSetAttrsA()" */
AROS_LH3(LONG, btcSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, btsdp)
{
    AROS_LIBFUNC_INIT
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcDoMethodA()" */
AROS_LH2(SIPTR, btcDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, nh, 7, btsdp)
{
    AROS_LIBFUNC_INIT
    struct Library *BluetoothBase;

    switch(methodid)
    {
        case BCM_OpenCfgWindow:
            return(bOpenCfgWindow(nh));

        case BCM_ConfigChangedEvent:
            /* not while the window edits it; its Use and Save apply it */
            if(!nh->nh_GUITask && (BluetoothBase = OpenLibrary("bluetooth.library", 45)))
            {
                bLoadClassConfig(nh, BluetoothBase);
                bApplyPolicy(nh, BluetoothBase, FALSE);
                CloseLibrary(BluetoothBase);
            }
            return(TRUE);

        case BCM_ServiceRecordEvent:
            /* somebody registered a service - which the user may have
               switched off before - or is taking one away */
            if(methoddata[1] && (BluetoothBase = OpenLibrary("bluetooth.library", 45)))
            {
                bApplyRecord(nh, BluetoothBase, (APTR) methoddata[0], FALSE);
                CloseLibrary(BluetoothBase);
            }
            Forbid();
            if(nh->nh_GUITask)
            {
                Signal(nh->nh_GUITask, SIGBREAKF_CTRL_F);
            }
            Permit();
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bOpenCfgWindow()" */
static BOOL bOpenCfgWindow(struct BTSdpBase *nh)
{
    struct Library *BluetoothBase;
    BOOL ret = FALSE;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        return(FALSE);
    }
    Forbid();
    if(!nh->nh_GUITask)
    {
        if((nh->nh_GUITask = btSpawnSubTask(MOD_NAME_STRING " GUI", bGUITask, nh)))
        {
            ret = TRUE;
        }
    }
    Permit();
    CloseLibrary(BluetoothBase);
    return(ret);
}
/* \\\ */

#undef UtilityBase

/*
 * ***********************************************************************
 * * The settings window                                                 *
 * ***********************************************************************
 */

#define BluetoothBase nh->nh_BtBase
#define IntuitionBase nh->nh_IntBase
#define MUIMasterBase nh->nh_MUIBase

/* /// "bDispHook()" */
AROS_UFH3(LONG, bDispHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(char **, strarr, A2),
          AROS_UFHA(struct BTSdpEntry *, se, A1))
{
    AROS_USERFUNC_INIT

    if(se)
    {
        strarr[0] = se->se_Offered ? "Yes" : "No";
        strarr[1] = se->se_Name;
        strarr[2] = se->se_UUID;
        strarr[3] = se->se_Via;
        strarr[4] = se->se_Owner;
    } else {
        strarr[0] = "\33bOffered";
        strarr[1] = "\33bService";
        strarr[2] = "\33bUUID";
        strarr[3] = "\33bReached via";
        strarr[4] = "\33bProvided by";
    }
    return(0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bHex16()" */
static char * bHex16(char *buf, ULONG val)
{
    static const char hex[] = "0123456789ABCDEF";
    LONG n;

    *buf++ = '0';
    *buf++ = 'x';
    for(n = 12; n >= 0; n -= 4)
    {
        *buf++ = hex[(val >> n) & 15];
    }
    *buf = 0;
    return(buf);
}
/* \\\ */

/* /// "bFillList()" */
/* The classic services registered right now, and what the edited
   configuration says about each. */
static void bFillList(struct BTSdpBase *nh)
{
    struct List *list = NULL;
    struct Node *rec;
    ULONG num = 0;

    set(nh->nh_ListObj, MUIA_List_Quiet, TRUE);
    DoMethod(nh->nh_ListObj, MUIM_List_Clear);
    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_ServiceRecordList, &list, TAG_END);
    if(list)
    {
        for(rec = list->lh_Head; rec->ln_Succ && (num < BTSDP_MAXENTRIES); rec = rec->ln_Succ)
        {
            struct BTSdpEntry *se = &nh->nh_Entries[num];
            STRPTR name = NULL, owner = NULL;

            if(!bRecordKey(BluetoothBase, rec, &se->se_Key))
            {
                continue;
            }
            btGetAttrs(BGA_SERVICERECORD, rec, BSRA_Name, &name, BSRA_Owner, &owner, TAG_END);
            se->se_Offered = (bDisabledIndex(&nh->nh_GUICfg, &se->se_Key) < 0);
            strncpy(se->se_Name, name ? (char *) name : "Unnamed service", sizeof(se->se_Name) - 1);
            se->se_Name[sizeof(se->se_Name) - 1] = 0;
            strncpy(se->se_Owner, owner ? (char *) owner : "", sizeof(se->se_Owner) - 1);
            se->se_Owner[sizeof(se->se_Owner) - 1] = 0;
            bHex16(se->se_UUID, se->se_Key.sk_UUID16);
            if(se->se_Key.sk_Protocol == BSVP_RFCOMM)
            {
                btSafeRawDoFmt((STRPTR) se->se_Via, sizeof(se->se_Via), "RFCOMM channel %ld",
                                (ULONG) se->se_Key.sk_Port);
            } else {
                strcpy(se->se_Via, "L2CAP PSM ");
                bHex16(se->se_Via + strlen(se->se_Via), se->se_Key.sk_Port);
            }
            DoMethod(nh->nh_ListObj, MUIM_List_InsertSingle, se, MUIV_List_Insert_Bottom);
            num++;
        }
    }
    btUnlockBase();
    set(nh->nh_ListObj, MUIA_List_Quiet, FALSE);
}
/* \\\ */

/* /// "bShowSelected()" */
/* The checkmark under the list follows the selected service. */
static void bShowSelected(struct BTSdpBase *nh)
{
    struct BTSdpEntry *se = NULL;

    DoMethod(nh->nh_ListObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &se);
    nnset(nh->nh_OfferObj, MUIA_Selected, se ? se->se_Offered : FALSE);
    set(nh->nh_OfferObj, MUIA_Disabled, !se);
}
/* \\\ */

/* /// "bGUITaskCleanup()" */
static void bGUITaskCleanup(struct BTSdpBase *nh)
{
    if(nh->nh_App)
    {
        MUI_DisposeObject(nh->nh_App);
        nh->nh_App = NULL;
    }
    if(MUIMasterBase)
    {
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
    }
    if(IntuitionBase)
    {
        CloseLibrary(IntuitionBase);
        IntuitionBase = NULL;
    }
    if(BluetoothBase)
    {
        CloseLibrary(BluetoothBase);
        BluetoothBase = NULL;
    }
    Forbid();
    nh->nh_GUITask = NULL;
    --nh->nh_Library.lib_OpenCnt;
}
/* \\\ */

/* /// "bGUITask()" */
AROS_UFH0(void, bGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct BTSdpBase *nh;

    thistask = FindTask(NULL);
    nh = thistask->tc_UserData;

    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        bGUITaskCleanup(nh);
        return;
    }
    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        bGUITaskCleanup(nh);
        return;
    }
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        KPRINTF(10, ("Couldn't open bluetooth.library.\n"));
        bGUITaskCleanup(nh);
        return;
    }

    /* the window edits a copy; Use and Save put it into effect */
    CopyMem(&nh->nh_Cfg, &nh->nh_GUICfg, sizeof(struct BTSdpCfg));
    nh->nh_DispHook.h_Entry = (HOOKFUNC) bDispHook;
    nh->nh_DispHook.h_Data = nh;

    nh->nh_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)libname,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)"Copyright 2026 The AROS Development Team",
        MUIA_Application_Author     , (IPTR)"The AROS Development Team",
        MUIA_Application_Description, (IPTR)"Settings for the Bluetooth SDP server",
        MUIA_Application_Base       , (IPTR)"BTSDP",
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(nh->nh_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(nh->nh_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            End,

        SubWindow, (IPTR)(nh->nh_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR)"Bluetooth classic services (SDP server)",
            MUIA_HelpNode, (IPTR)libname,

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)VGroup, GroupFrameT("Classic services"),
                    Child, (IPTR)ListviewObject,
                        MUIA_CycleChain, 1,
                        MUIA_Listview_List, (IPTR)(nh->nh_ListObj = ListObject,
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, (IPTR)"BAR,BAR,BAR,BAR,",
                            MUIA_List_DisplayHook, (IPTR)&nh->nh_DispHook,
                            MUIA_ShortHelp, (IPTR)"The classic Bluetooth services registered with the stack.\n"
                                                  "A device that connects only finds, and can only use,\n"
                                                  "the ones offered.",
                            End),
                        End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(nh->nh_OfferObj = MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)Label1((IPTR)"Offer the selected service to connecting devices"),
                        Child, (IPTR)HSpace(0),
                        End,
                    End,
                Child, (IPTR)HGroup, GroupFrameT("Visibility"),
                    Child, (IPTR)(nh->nh_EIRObj = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)Label1((IPTR)"Tell devices that scan for this machine which services it offers"),
                    Child, (IPTR)HSpace(0),
                    End,
                Child, (IPTR)HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(nh->nh_SaveObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Save ",
                        End),
                    Child, (IPTR)(nh->nh_UseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Use ",
                        End),
                    Child, (IPTR)(nh->nh_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Cancel ",
                        End),
                    End,
                End,
            End),
        End;

    if(!nh->nh_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        bGUITaskCleanup(nh);
        return;
    }

    set(nh->nh_OfferObj, MUIA_CycleChain, 1);
    set(nh->nh_EIRObj, MUIA_CycleChain, 1);
    set(nh->nh_EIRObj, MUIA_ShortHelp, (IPTR)"The radio answers a scan with its name and the list of offered\n"
                                             "services, so a device knows what is here before it connects.\n"
                                             "It only answers scans while the machine is discoverable.");
    nnset(nh->nh_EIRObj, MUIA_Selected, nh->nh_GUICfg.sc_EIRServices ? TRUE : FALSE);

    DoMethod(nh->nh_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nh->nh_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(nh->nh_SaveObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(nh->nh_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nh->nh_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nh->nh_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(nh->nh_ListObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_SELECT);
    DoMethod(nh->nh_ListObj, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_TOGGLE);
    DoMethod(nh->nh_OfferObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_OFFER);
    DoMethod(nh->nh_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(nh->nh_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_OpenConfigWindow, 0);

    bFillList(nh);
    bShowSelected(nh);
    {
        IPTR  isopen = 0;
        IPTR  iconify = 0;
        ULONG sigs;
        LONG retid;

        get(nh->nh_App, MUIA_Application_Iconified, &iconify);
        set(nh->nh_MainWindow, MUIA_Window_Open, TRUE);
        get(nh->nh_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            bGUITaskCleanup(nh);
            return;
        }
        do
        {
            retid = DoMethod(nh->nh_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_SELECT:
                    bShowSelected(nh);
                    break;

                case ID_OFFER:
                case ID_TOGGLE:
                {
                    struct BTSdpEntry *se = NULL;
                    IPTR sel = 0;

                    DoMethod(nh->nh_ListObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &se);
                    if(se)
                    {
                        if(retid == ID_OFFER)
                        {
                            get(nh->nh_OfferObj, MUIA_Selected, &sel);
                        } else {
                            sel = !se->se_Offered;
                        }
                        if(bSetDisabled(&nh->nh_GUICfg, &se->se_Key, sel ? FALSE : TRUE))
                        {
                            se->se_Offered = sel ? TRUE : FALSE;
                        }
                        DoMethod(nh->nh_ListObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
                    }
                    bShowSelected(nh);
                    break;
                }

                case ID_DEF_CONFIG:
                case ID_STORE_CONFIG:
                {
                    IPTR eir = 0;

                    get(nh->nh_EIRObj, MUIA_Selected, &eir);
                    nh->nh_GUICfg.sc_EIRServices = eir ? TRUE : FALSE;
                    Forbid();
                    CopyMem(&nh->nh_GUICfg, &nh->nh_Cfg, sizeof(struct BTSdpCfg));
                    Permit();
                    bStoreClassConfig(nh, BluetoothBase, retid == ID_DEF_CONFIG);
                    bApplyPolicy(nh, BluetoothBase, FALSE);
                    retid = MUIV_Application_ReturnID_Quit;
                    break;
                }

                case ID_ABOUT:
                    MUI_RequestA(nh->nh_App, nh->nh_MainWindow, 0, NULL, "OK", VERSION_STRING, NULL);
                    break;
            }
            if(retid == MUIV_Application_ReturnID_Quit)
            {
                break;
            }
            if(sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
                if(sigs & SIGBREAKF_CTRL_F)
                {
                    /* services were registered or removed meanwhile */
                    bFillList(nh);
                    bShowSelected(nh);
                }
            }
        } while(TRUE);
        set(nh->nh_MainWindow, MUIA_Window_Open, FALSE);
    }
    bGUITaskCleanup(nh);

    AROS_USERFUNC_EXIT
}
/* \\\ */
