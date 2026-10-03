/*
 *----------------------------------------------------------------------------
 *                    btgatt class for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * The GATT server role. GATT services are registered with the stack the way
 * SDP records are - btAddServiceRecord() with BSVP_ATT - by this class and
 * by any other class or program that has one to offer, and bluetooth.library
 * answers the requests of the devices that connect. What those devices can
 * discover is up to this class: a registered service is only offered once
 * it is enabled here, and the settings window lists them all so that each
 * can be switched on and off. It also adds the services every GATT server
 * has (Generic Access, Generic Attribute, Device Information) and has the
 * radios advertise, so that LE devices can find the machine and connect.
 */

#include "debug.h"

#include "btgatt.h"

static const STRPTR libname = MOD_NAME_STRING;

#define UtilityBase nh->nh_UtilityBase

#define UUID_GENERIC_ACCESS    0x1800
#define UUID_GENERIC_ATTRIBUTE 0x1801
#define UUID_DEVICE_INFO       0x180a

static BOOL bOpenCfgWindow(struct BTGattBase *nh);

/* /// "bRecordKey()" */
/* What a service is known by in the configuration: its UUID in the 128 bit
   form, little-endian. Returns the 16 bit UUID if it has one. */
static ULONG bRecordKey(struct Library *BluetoothBase, APTR rec, UBYTE *key)
{
    static const UBYTE base[16] = { 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
                                    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    IPTR uuid16 = 0;
    UBYTE *uuid128 = NULL;
    ULONG n;

    btGetAttrs(BGA_SERVICERECORD, rec, BSRA_UUID16, &uuid16, BSRA_UUID128, &uuid128, TAG_END);
    if(uuid128)
    {
        for(n = 0; n < 16; n++)
        {
            key[n] = uuid128[15 - n];
        }
        return(0);
    }
    CopyMem((APTR) base, key, 16);
    key[12] = uuid16 & 0xff;
    key[13] = uuid16 >> 8;
    return((ULONG) uuid16);
}
/* \\\ */

/* /// "bMandatory()" */
/* The two services a GATT server may not be without. */
static BOOL bMandatory(ULONG uuid16)
{
    return((uuid16 == UUID_GENERIC_ACCESS) || (uuid16 == UUID_GENERIC_ATTRIBUTE));
}
/* \\\ */

/* /// "bDisabledIndex()" */
static LONG bDisabledIndex(struct BTGattCfg *cfg, const UBYTE *key)
{
    ULONG n;

    for(n = 0; (n < cfg->gc_NumDisabled) && (n < BTGATT_MAXDISABLED); n++)
    {
        if(!memcmp(cfg->gc_Disabled[n], key, 16))
        {
            return((LONG) n);
        }
    }
    return(-1);
}
/* \\\ */

/* /// "bSetDisabled()" */
/* FALSE if there is no room to remember one more service as switched off. */
static BOOL bSetDisabled(struct BTGattCfg *cfg, const UBYTE *key, BOOL disabled)
{
    LONG idx = bDisabledIndex(cfg, key);

    if(disabled)
    {
        if(idx >= 0)
        {
            return(TRUE);
        }
        if(cfg->gc_NumDisabled >= BTGATT_MAXDISABLED)
        {
            return(FALSE);
        }
        CopyMem((APTR) key, cfg->gc_Disabled[cfg->gc_NumDisabled++], 16);
    }
    else if(idx >= 0)
    {
        cfg->gc_NumDisabled--;
        if((ULONG) idx != cfg->gc_NumDisabled)
        {
            CopyMem(cfg->gc_Disabled[cfg->gc_NumDisabled], cfg->gc_Disabled[idx], 16);
        }
    }
    return(TRUE);
}
/* \\\ */

/* /// "bApplyRecord()" */
/* Offer a GATT service to connecting devices, or not. */
static void bApplyRecord(struct BTGattBase *nh, struct Library *BluetoothBase, APTR rec)
{
    IPTR proto = 0;
    UBYTE key[16];
    ULONG uuid16;

    btGetAttrs(BGA_SERVICERECORD, rec, BSRA_Protocol, &proto, TAG_END);
    if(proto != BSVP_ATT)
    {
        return;
    }
    uuid16 = bRecordKey(BluetoothBase, rec, key);
    btSetAttrs(BGA_SERVICERECORD, rec,
               BSRA_Enabled, bMandatory(uuid16) || (bDisabledIndex(&nh->nh_Cfg, key) < 0),
               TAG_END);
}
/* \\\ */

/* /// "bApplyPolicy()" */
/* Put the configuration into effect: which services are offered, and
   whether the radios advertise. */
static void bApplyPolicy(struct BTGattBase *nh, struct Library *BluetoothBase)
{
    struct List *list = NULL;
    struct Node *rec;

    btLockReadBase();
    btGetAttrs(BGA_STACK, NULL, BSA_ServiceRecordList, &list, TAG_END);
    if(list)
    {
        for(rec = list->lh_Head; rec->ln_Succ; rec = rec->ln_Succ)
        {
            bApplyRecord(nh, BluetoothBase, rec);
        }
    }
    btUnlockBase();
    btSetAttrs(BGA_STACK, NULL, BSA_LEAdvertising, nh->nh_Cfg.gc_Advertise ? TRUE : FALSE, TAG_END);
}
/* \\\ */

/* /// "bLoadClassConfig()" */
static void bLoadClassConfig(struct BTGattBase *nh, struct Library *BluetoothBase)
{
    struct BTGattCfg *cfg = &nh->nh_Cfg;
    struct BTGattCfg *chunk;
    APTR pic;

    Forbid();
    /* default: every service is offered, but nobody is invited in */
    memset(cfg, 0, sizeof(struct BTGattCfg));
    cfg->gc_ChunkID = AROS_LONG2BE(MAKE_ID('G','A','T','T'));
    cfg->gc_Length = AROS_LONG2BE(sizeof(struct BTGattCfg) - 8);
    nh->nh_UsingDefaultCfg = TRUE;
    if((pic = btGetClsCfg(libname)))
    {
        if((chunk = btGetCfgChunk(pic, AROS_LONG2BE(cfg->gc_ChunkID))))
        {
            ULONG len = AROS_LONG2BE(chunk->gc_Length);
            if(len > sizeof(struct BTGattCfg) - 8)
            {
                len = sizeof(struct BTGattCfg) - 8;
            }
            CopyMem(((UBYTE *) chunk) + 8, ((UBYTE *) cfg) + 8, len);
            btFreeVec(chunk);
            if(cfg->gc_NumDisabled > BTGATT_MAXDISABLED)
            {
                cfg->gc_NumDisabled = BTGATT_MAXDISABLED;
            }
            nh->nh_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
}
/* \\\ */

/* /// "bStoreClassConfig()" */
static void bStoreClassConfig(struct BTGattBase *nh, struct Library *BluetoothBase, BOOL todisk)
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

/* /// "bAddServices()" */
/* The services of our own. An empty Device Name is answered by the stack
   with the name of the radio the device is talking to. */
static void bAddServices(struct BTGattBase *nh, struct Library *BluetoothBase)
{
    static const UBYTE appearance[2] = { 0x80, 0x00 }; /* a computer */
    static const char maker[] = "AROS Development Team";
    static const char model[] = "AROS";
    struct BtGattCharDef gap[2];
    struct BtGattCharDef dis[2];

    memset(gap, 0, sizeof(gap));
    gap[0].bgd_UUID16 = 0x2a00;          /* Device Name */
    gap[0].bgd_Properties = BGDP_READ;
    gap[1].bgd_UUID16 = 0x2a01;          /* Appearance */
    gap[1].bgd_Properties = BGDP_READ;
    gap[1].bgd_Len = sizeof(appearance);
    gap[1].bgd_Value = appearance;
    nh->nh_GAPRecord = btAddServiceRecord(BSRA_Protocol, BSVP_ATT, BSRA_UUID16, UUID_GENERIC_ACCESS,
                                          BSRA_Name, (IPTR) "Generic Access", BSRA_Owner, (IPTR) libname,
                                          BSRA_Characteristics, (IPTR) gap, BSRA_NumCharacteristics, 2,
                                          TAG_END);
    nh->nh_GATTRecord = btAddServiceRecord(BSRA_Protocol, BSVP_ATT, BSRA_UUID16, UUID_GENERIC_ATTRIBUTE,
                                           BSRA_Name, (IPTR) "Generic Attribute", BSRA_Owner, (IPTR) libname,
                                           TAG_END);
    memset(dis, 0, sizeof(dis));
    dis[0].bgd_UUID16 = 0x2a29;          /* Manufacturer Name String */
    dis[0].bgd_Properties = BGDP_READ;
    dis[0].bgd_Len = sizeof(maker) - 1;
    dis[0].bgd_Value = (CONST UBYTE *) maker;
    dis[1].bgd_UUID16 = 0x2a24;          /* Model Number String */
    dis[1].bgd_Properties = BGDP_READ;
    dis[1].bgd_Len = sizeof(model) - 1;
    dis[1].bgd_Value = (CONST UBYTE *) model;
    nh->nh_DISRecord = btAddServiceRecord(BSRA_Protocol, BSVP_ATT, BSRA_UUID16, UUID_DEVICE_INFO,
                                          BSRA_Name, (IPTR) "Device Information", BSRA_Owner, (IPTR) libname,
                                          BSRA_Characteristics, (IPTR) dis, BSRA_NumCharacteristics, 2,
                                          TAG_END);
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
    /* BSVP_ATT service records came with bluetooth.library 45.15 */
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        CloseLibrary(nh->nh_UtilityBase);
        return(FALSE);
    }
    bLoadClassConfig(nh, BluetoothBase);
    bAddServices(nh, BluetoothBase);
    bApplyPolicy(nh, BluetoothBase);
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
        /* without a GATT server there is nothing to advertise for */
        btSetAttrs(BGA_STACK, NULL, BSA_LEAdvertising, FALSE, TAG_END);
        btRemServiceRecord(nh->nh_DISRecord);
        btRemServiceRecord(nh->nh_GATTRecord);
        btRemServiceRecord(nh->nh_GAPRecord);
        nh->nh_DISRecord = nh->nh_GATTRecord = nh->nh_GAPRecord = NULL;
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
         LIBBASETYPEPTR, nh, 5, btgatt)
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
                *((STRPTR *) ti->ti_Data) = "GATT server: services offered to connecting LE devices";
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
         LIBBASETYPEPTR, nh, 6, btgatt)
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
         LIBBASETYPEPTR, nh, 7, btgatt)
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
                bApplyPolicy(nh, BluetoothBase);
                CloseLibrary(BluetoothBase);
            }
            return(TRUE);

        case BCM_ServiceRecordEvent:
            /* somebody registered a service (it starts out disabled) or is
               taking one away */
            if(methoddata[1] && (BluetoothBase = OpenLibrary("bluetooth.library", 45)))
            {
                bApplyRecord(nh, BluetoothBase, (APTR) methoddata[0]);
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
static BOOL bOpenCfgWindow(struct BTGattBase *nh)
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
          AROS_UFHA(struct BTGattEntry *, ge, A1))
{
    AROS_USERFUNC_INIT

    if(ge)
    {
        strarr[0] = ge->ge_Mandatory ? "Always" : (ge->ge_Offered ? "Yes" : "No");
        strarr[1] = ge->ge_Name;
        strarr[2] = ge->ge_UUID;
        strarr[3] = ge->ge_Chars;
        strarr[4] = ge->ge_Owner;
    } else {
        strarr[0] = "\33bOffered";
        strarr[1] = "\33bService";
        strarr[2] = "\33bUUID";
        strarr[3] = "\33bValues";
        strarr[4] = "\33bProvided by";
    }
    return(0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bUUIDString()" */
static void bUUIDString(struct BTGattBase *nh, char *buf, ULONG uuid16, const UBYTE *key)
{
    static const char hex[] = "0123456789ABCDEF";
    LONG n;

    if(uuid16)
    {
        *buf++ = '0';
        *buf++ = 'x';
        for(n = 12; n >= 0; n -= 4)
        {
            *buf++ = hex[(uuid16 >> n) & 15];
        }
        *buf = 0;
        return;
    }
    /* the key is little-endian; a UUID is written the other way round */
    for(n = 15; n >= 0; n--)
    {
        *buf++ = hex[key[n] >> 4];
        *buf++ = hex[key[n] & 15];
        if((n == 12) || (n == 10) || (n == 8) || (n == 6))
        {
            *buf++ = '-';
        }
    }
    *buf = 0;
}
/* \\\ */

/* /// "bFillList()" */
/* The GATT services registered right now, and what the edited
   configuration says about each. */
static void bFillList(struct BTGattBase *nh)
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
        for(rec = list->lh_Head; rec->ln_Succ && (num < BTGATT_MAXENTRIES); rec = rec->ln_Succ)
        {
            struct BTGattEntry *ge = &nh->nh_Entries[num];
            IPTR proto = 0, numchars = 0;
            STRPTR name = NULL, owner = NULL;
            ULONG uuid16;

            btGetAttrs(BGA_SERVICERECORD, rec, BSRA_Protocol, &proto, BSRA_Name, &name, BSRA_Owner, &owner,
                       BSRA_NumCharacteristics, &numchars, TAG_END);
            if(proto != BSVP_ATT)
            {
                continue;
            }
            uuid16 = bRecordKey(BluetoothBase, rec, ge->ge_Key);
            ge->ge_Mandatory = bMandatory(uuid16);
            ge->ge_Offered = ge->ge_Mandatory || (bDisabledIndex(&nh->nh_GUICfg, ge->ge_Key) < 0);
            strncpy(ge->ge_Name, name ? (char *) name : "Unnamed service", sizeof(ge->ge_Name) - 1);
            ge->ge_Name[sizeof(ge->ge_Name) - 1] = 0;
            strncpy(ge->ge_Owner, owner ? (char *) owner : "", sizeof(ge->ge_Owner) - 1);
            ge->ge_Owner[sizeof(ge->ge_Owner) - 1] = 0;
            btSafeRawDoFmt((STRPTR) ge->ge_Chars, sizeof(ge->ge_Chars), "%ld", (ULONG) numchars);
            bUUIDString(nh, ge->ge_UUID, uuid16, ge->ge_Key);
            DoMethod(nh->nh_ListObj, MUIM_List_InsertSingle, ge, MUIV_List_Insert_Bottom);
            num++;
        }
    }
    btUnlockBase();
    set(nh->nh_ListObj, MUIA_List_Quiet, FALSE);
}
/* \\\ */

/* /// "bShowSelected()" */
/* The checkmark under the list follows the selected service. */
static struct BTGattEntry * bShowSelected(struct BTGattBase *nh)
{
    struct BTGattEntry *ge = NULL;

    DoMethod(nh->nh_ListObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ge);
    nnset(nh->nh_OfferObj, MUIA_Selected, ge ? ge->ge_Offered : FALSE);
    set(nh->nh_OfferObj, MUIA_Disabled, !ge || ge->ge_Mandatory);
    return(ge);
}
/* \\\ */

/* /// "bGUITaskCleanup()" */
static void bGUITaskCleanup(struct BTGattBase *nh)
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
    struct BTGattBase *nh;
    Object *offerlabel, *advlabel;

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
    CopyMem(&nh->nh_Cfg, &nh->nh_GUICfg, sizeof(struct BTGattCfg));
    nh->nh_DispHook.h_Entry = (HOOKFUNC) bDispHook;
    nh->nh_DispHook.h_Data = nh;

    nh->nh_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)libname,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)"Copyright 2026 The AROS Development Team",
        MUIA_Application_Author     , (IPTR)"The AROS Development Team",
        MUIA_Application_Description, (IPTR)"Settings for the Bluetooth GATT server",
        MUIA_Application_Base       , (IPTR)"BTGATT",
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
            MUIA_Window_Title, (IPTR)"Bluetooth LE services (GATT server)",
            MUIA_HelpNode, (IPTR)libname,

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)VGroup, GroupFrameT("GATT services"),
                    Child, (IPTR)ListviewObject,
                        MUIA_CycleChain, 1,
                        MUIA_Listview_List, (IPTR)(nh->nh_ListObj = ListObject,
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, (IPTR)"BAR,BAR,BAR,BAR P=\33r,",
                            MUIA_List_DisplayHook, (IPTR)&nh->nh_DispHook,
                            MUIA_ShortHelp, (IPTR)"The GATT services registered with the Bluetooth stack.\n"
                                                  "A device that connects only finds the ones offered.",
                            End),
                        End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(nh->nh_OfferObj = MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)(offerlabel = Label1((IPTR)"Offer the selected service to connecting devices")),
                        Child, (IPTR)HSpace(0),
                        End,
                    End,
                Child, (IPTR)HGroup, GroupFrameT("Visibility"),
                    Child, (IPTR)(nh->nh_AdvObj = MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)(advlabel = Label1((IPTR)"Let Bluetooth LE devices find and connect to this machine")),
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
    (void) offerlabel;
    (void) advlabel;

    set(nh->nh_OfferObj, MUIA_CycleChain, 1);
    set(nh->nh_AdvObj, MUIA_CycleChain, 1);
    set(nh->nh_AdvObj, MUIA_ShortHelp, (IPTR)"The radio advertises: phones and computers nearby can see this\n"
                                             "machine, connect to it without asking and use the offered services.");
    nnset(nh->nh_AdvObj, MUIA_Selected, nh->nh_GUICfg.gc_Advertise ? TRUE : FALSE);

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
                    struct BTGattEntry *ge = NULL;
                    IPTR sel = 0;

                    DoMethod(nh->nh_ListObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ge);
                    if(ge && !ge->ge_Mandatory)
                    {
                        if(retid == ID_OFFER)
                        {
                            get(nh->nh_OfferObj, MUIA_Selected, &sel);
                        } else {
                            sel = !ge->ge_Offered;
                        }
                        if(bSetDisabled(&nh->nh_GUICfg, ge->ge_Key, sel ? FALSE : TRUE))
                        {
                            ge->ge_Offered = sel ? TRUE : FALSE;
                        }
                        DoMethod(nh->nh_ListObj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
                    }
                    bShowSelected(nh);
                    break;
                }

                case ID_DEF_CONFIG:
                case ID_STORE_CONFIG:
                {
                    IPTR adv = 0;

                    get(nh->nh_AdvObj, MUIA_Selected, &adv);
                    nh->nh_GUICfg.gc_Advertise = adv ? TRUE : FALSE;
                    Forbid();
                    CopyMem(&nh->nh_GUICfg, &nh->nh_Cfg, sizeof(struct BTGattCfg));
                    Permit();
                    bStoreClassConfig(nh, BluetoothBase, retid == ID_DEF_CONFIG);
                    bApplyPolicy(nh, BluetoothBase);
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
