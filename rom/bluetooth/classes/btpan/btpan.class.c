/*
 *----------------------------------------------------------------------------
 *                         ethwrap class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <aros/isoascii.h>
#include "debug.h"

#include "btpan.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static
const APTR DevFuncTable[] =
{
    &AROS_SLIB_ENTRY(devOpen, dev, 1),
    &AROS_SLIB_ENTRY(devClose, dev, 2),
    &AROS_SLIB_ENTRY(devExpunge, dev, 3),
    &AROS_SLIB_ENTRY(devReserved, dev, 4),
    &AROS_SLIB_ENTRY(devBeginIO, dev, 5),
    &AROS_SLIB_ENTRY(devAbortIO, dev, 6),
    (APTR) -1,
};

static int libInit(LIBBASETYPEPTR nh)
{
    struct BTPanUnit *ncp;
    struct BTPanBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct BTPanDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct BTPanDevBase), NULL)))
        {
            ncp = &nh->nh_DefaultUnit;
            ncp->ncp_ClsBase = nh;
            ncp->ncp_Interface = NULL;
            ncp->ncp_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
            if(ncp->ncp_CDC)
            {
                nh->nh_DevBase->np_ClsBase = nh;
                Forbid();
                AddDevice((struct Device *) nh->nh_DevBase);
                nh->nh_DevBase->np_Library.lib_OpenCnt++;
                Permit();
                ret = nh;
            }
        } else {
            KPRINTF(20, ("failed to create btpan.device\n"));
        }
        if(!ret)
        {
            CloseLibrary(UtilityBase);
        }
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return(ret ? TRUE : FALSE);
}

static int libOpen(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    bLoadClassConfig(nh);
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    struct BTPanUnit *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);

        ncp = (struct BTPanUnit *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp->ncp_CDC);
            FreeVec(ncp);
            ncp = (struct BTPanUnit *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! btpan.class expunged!\n\n"));
    } else {
        KPRINTF(5, ("libExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        return(FALSE);
    }

    return(TRUE);
}

ADD2INITLIB(libInit, 0)
ADD2OPENLIB(libOpen, 0)
ADD2EXPUNGELIB(libExpunge, 0)
/* \\\ */


/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

struct AutoBindData
{
    UWORD abd_VendID;
    UWORD abd_ProdID;
};

struct AutoBindData ClassBinds[] =
{
    { 0x0402, 0x5632 },   // ALi defaults
    { 0x050d, 0x0004 },   // Belkin
    { 0x050f, 0x0190 },   // KC-190
    { 0x0525, 0x2888 },   // EPSON USB client
    { 0x0525, 0x9901 },   // Advance USBNET (eTEK)
    { 0x0547, 0x2720 },   // AnchorChips defaults
    { 0x0547, 0x2727 },   // Xircom PGUNET
    { 0x056c, 0x8100 },   // eTEK
    { 0, 0 }
};

/* /// "bAttemptServiceBinding()" */
struct BTPanUnit * GM_UNIQUENAME(bAttemptServiceBinding)(struct BTPanBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    IPTR uuid16 = 0;
    IPTR proto = 0;
    BOOL ispan = FALSE;

    KPRINTF(1, ("bAttemptServiceBinding(%08lx)\n", bsv));
    if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        btGetAttrs(BGA_SERVICE, bsv,
                   BSVA_UUID16, &uuid16,
                   BSVA_Protocol, &proto,
                   TAG_END);
        CloseLibrary(BluetoothBase);
        /* 0x1116 NAP (internet access), 0x1117 GN (ad-hoc group) */
        ispan = (proto == BSVP_L2CAP) && ((uuid16 == 0x1116) || (uuid16 == 0x1117));
    }
    return(ispan ? GM_UNIQUENAME(bForceServiceBinding)(nh, bsv) : NULL);
}
/* \\\ */

/* /// "bForceServiceBinding()" */
struct BTPanUnit * GM_UNIQUENAME(bForceServiceBinding)(struct BTPanBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    struct BTPanUnit *ncp;
    struct BtDevice *bd = NULL;
    struct BtEndpoint *bep;
    UBYTE *addr = NULL;
    STRPTR devname = NULL;
    STRPTR devidstr = NULL;
    IPTR uuid16 = 0;
    ULONG unitno;
    BOOL unitfound;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("bForceServiceBinding(%08lx)\n", bsv));
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(NULL);
    }
    btGetAttrs(BGA_SERVICE, bsv, BSVA_Device, &bd, BSVA_UUID16, &uuid16, TAG_END);
    btGetAttrs(BGA_DEVICE, bd, BDA_Name, &devname, BDA_Address, &addr, BDA_IDString, &devidstr, TAG_END);
    bep = btFindEndpoint(bsv, NULL, BEA_Type, BEPT_L2CAP, BEA_PSM, BNEP_PSM, TAG_END);
    if(!bep)
    {
        bep = btFindEndpoint(bsv, NULL, BEA_Type, BEPT_L2CAP, TAG_END);
    }
    if(!bep)
    {
        CloseLibrary(BluetoothBase);
        return(NULL);
    }

    Forbid();
    /* Find next free unit number */
    unitno = nh->nh_DefaultUnit.ncp_CDC->cdc_DefaultUnit;
    ncp = (struct BTPanUnit *) nh->nh_Units.lh_Head;
    while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
    {
        if(ncp->ncp_UnitNo == unitno)
        {
            unitno++;
            ncp = (struct BTPanUnit *) nh->nh_Units.lh_Head;
        } else {
            ncp = (struct BTPanUnit *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
    }
    /* the same remote network keeps its unit number across rebinds */
    unitfound = FALSE;
    ncp = (struct BTPanUnit *) nh->nh_Units.lh_Head;
    while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
    {
        if(addr && !memcmp(ncp->ncp_UnitAddr, addr, 6) && (ncp->ncp_UnitUUID == uuid16))
        {
            if(ncp->ncp_Task)
            {
                /* already bound (a second record for the same network):
                   one unit, one task */
                Permit();
                KPRINTF(10, ("unit %ld already bound\n", ncp->ncp_UnitNo));
                CloseLibrary(BluetoothBase);
                return(NULL);
            }
            unitno = ncp->ncp_UnitNo;
            unitfound = TRUE;
            break;
        }
        ncp = (struct BTPanUnit *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
    }
    if(!unitfound)
    {
        if(!(ncp = AllocVec(sizeof(struct BTPanUnit), MEMF_PUBLIC|MEMF_CLEAR)))
        {
            Permit();
            CloseLibrary(BluetoothBase);
            return(NULL);
        }
        ncp->ncp_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
        if(!ncp->ncp_CDC)
        {
            Permit();
            FreeVec(ncp);
            CloseLibrary(BluetoothBase);
            return(NULL);
        }
        /* IORequests may be queued even if the task is gone. */
        NewList(&ncp->ncp_Unit.unit_MsgPort.mp_MsgList);
        NewList(&ncp->ncp_BufManList);
        NewList(&ncp->ncp_EventList);
        NewList(&ncp->ncp_TrackList);
        NewList(&ncp->ncp_Multicasts);
        NewList(&ncp->ncp_OrphanQueue);
        NewList(&ncp->ncp_WriteQueue);
        AddTail(&nh->nh_Units, &ncp->ncp_Unit.unit_MsgPort.mp_Node);
    }
    ncp->ncp_UnitNo = unitno;
    ncp->ncp_ClsBase = nh;
    ncp->ncp_DevBase = nh->nh_DevBase;
    ncp->ncp_Device = bd;
    ncp->ncp_Service = bsv;
    ncp->ncp_Endpoint = bep;
    ncp->ncp_Interface = (APTR) ncp;   /* marks a real binding (vs the defaults) */
    if(addr)
    {
        CopyMem(addr, ncp->ncp_UnitAddr, 6);
        CopyMem(addr, ncp->ncp_PeerAddr, 6);
    }
    ncp->ncp_UnitUUID = uuid16;
    if(devidstr)
    {
        strncpy((char *) ncp->ncp_DevIDString, (char *) devidstr, sizeof(ncp->ncp_DevIDString) - 1);
    }
    Permit();

    bLoadBindingConfig(ncp);

    btSafeRawDoFmt(buf, 64, "btpan.class<%08lx>", (IPTR) ncp);
    ncp->ncp_ReadySignal = SIGB_SINGLE;
    ncp->ncp_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    if((tmptask = btSpawnSubTask(buf, (APTR) bEthTask, ncp)))
    {
        btBorrowLocksWait(tmptask, 1UL<<ncp->ncp_ReadySignal);
        if(ncp->ncp_Task)
        {
            ncp->ncp_ReadySigTask = NULL;
            btAddErrorMsg(RETURN_OK, (STRPTR) libname,
                           "Network on '%s' (%s) at btpan.device unit %ld.",
                           devname ? devname : (STRPTR) "device",
                           (uuid16 == 0x1116) ? (STRPTR) "NAP" : (STRPTR) "GN",
                           ncp->ncp_UnitNo);
            CloseLibrary(BluetoothBase);
            return(ncp);
        }
    }
    ncp->ncp_ReadySigTask = NULL;
    /* the unit stays in the list for a later rebind */
    CloseLibrary(BluetoothBase);
    return(NULL);
}
/* \\\ */

/* /// "bReleaseServiceBinding()" */
void GM_UNIQUENAME(bReleaseServiceBinding)(struct BTPanBase *nh, struct BTPanUnit *ncp)
{
    struct Library *BluetoothBase;
    STRPTR devname = NULL;

    KPRINTF(1, ("bReleaseServiceBinding(%08lx)\n", ncp));
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return;
    }
    Forbid();
    ncp->ncp_ReadySignal = SIGB_SINGLE;
    ncp->ncp_ReadySigTask = FindTask(NULL);
    if(ncp->ncp_GUITask)
    {
        Signal(ncp->ncp_GUITask, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(ncp->ncp_GUITask)
    {
        Wait(1UL<<ncp->ncp_ReadySignal);
    }
    Forbid();
    if(ncp->ncp_Task)
    {
        Signal(ncp->ncp_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(ncp->ncp_Task)
    {
        Wait(1UL<<ncp->ncp_ReadySignal);
    }
    ncp->ncp_ReadySigTask = NULL;
    btGetAttrs(BGA_DEVICE, ncp->ncp_Device, BDA_Name, &devname, TAG_END);
    btAddErrorMsg(RETURN_OK, (STRPTR) libname,
                   "Network on '%s' released (unit %ld stays reserved).",
                   devname ? devname : (STRPTR) "device", ncp->ncp_UnitNo);
    CloseLibrary(BluetoothBase);
}
/* \\\ */



/* /// "btcGetAttrsA()" */
AROS_LH3(LONG, btcGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, btpan)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    KPRINTF(1, ("nepEthGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case BCGA_CLASS:
             if((ti = FindTagItem(BCCA_Priority, tags)))
             {
                 *((SIPTR *) ti->ti_Data) = -100;
                 count++;
             }
             if((ti = FindTagItem(BCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Networking (PAN/BNEP) via btpan.device";
                 count++;
             }
             if((ti = FindTagItem(BCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(BCCA_HasBindingCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(BCCA_AfterDOSRestart, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(BCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nh->nh_DefaultUnit.ncp_UsingDefaultCfg;
                 count++;
             }
             break;

         case BCGA_BINDING:
             if((ti = FindTagItem(BCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = ((struct BTPanUnit *) usbstruct)->ncp_UsingDefaultCfg;
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
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, btpan)
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
         LIBBASETYPEPTR, nh, 7, btpan)
{
    AROS_LIBFUNC_INIT

    struct BTPanUnit *ncp;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case BCM_AttemptServiceBinding:
            return((SIPTR) GM_UNIQUENAME(bAttemptServiceBinding)(nh, (struct BtService *) methoddata[0]));

        case BCM_ForceServiceBinding:
            return((SIPTR) GM_UNIQUENAME(bForceServiceBinding)(nh, (struct BtService *) methoddata[0]));

        case BCM_ReleaseServiceBinding:
            GM_UNIQUENAME(bReleaseServiceBinding)(nh, (struct BTPanUnit *) methoddata[0]);
            return(TRUE);

        case BCM_AttemptDeviceBinding:
        case BCM_ForceDeviceBinding:
            return(0); /* only service bindings */

        case BCM_DeviceDisconnected:
            /* the unit stays; its channels fail and are re-issued when the
               device reconnects (BCHA_AutoConnect) */
            return(TRUE);

        case BCM_OpenCfgWindow:
            return(bOpenBindingCfgWindow(nh, &nh->nh_DefaultUnit));

        case BCM_OpenBindingCfgWindow:
            return(bOpenBindingCfgWindow(nh, (struct BTPanUnit *) methoddata[0]));

        case BCM_ConfigChangedEvent:
            bLoadClassConfig(nh);
            Forbid();
            ncp = (struct BTPanUnit *) nh->nh_Units.lh_Head;
            while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                bLoadBindingConfig(ncp);
                ncp = (struct BTPanUnit *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
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

/* /// "bLoadClassConfig()" */
BOOL bLoadClassConfig(struct BTPanBase *nh)
{
    struct BTPanUnit *ncp = &nh->nh_DefaultUnit;
    struct Library *BluetoothBase;
    struct ClsDevCfg *cdc;
    struct BtIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(ncp->ncp_GUITask)
    {
        return(FALSE);
    }
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(FALSE);
    }

    Forbid();
    /* Create default config */
    cdc = ncp->ncp_CDC;
    cdc->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('B','P','A','N'));
    cdc->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    cdc->cdc_DefaultUnit = 0;
    cdc->cdc_MACAddress[0] = 0x00;
    cdc->cdc_MACAddress[1] = 0xff;
    cdc->cdc_MACAddress[2] = 0xff;
    cdc->cdc_MACAddress[3] = ((IPTR) cdc)>>20;
    cdc->cdc_MACAddress[4] = ((IPTR) cdc)>>12;
    cdc->cdc_MACAddress[5] = ((IPTR) cdc)>>4;
    ncp->ncp_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = btGetClsCfg(libname);
    if(pic)
    {
        cdc = btGetCfgChunk(pic, AROS_LONG2BE(ncp->ncp_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) ncp->ncp_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(ncp->ncp_CDC->cdc_Length)));
            btFreeVec(cdc);
            ncp->ncp_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(BluetoothBase);
    return(FALSE);
}
/* \\\ */

/* /// "bLoadBindingConfig()" */
BOOL bLoadBindingConfig(struct BTPanUnit *ncp)
{
    struct BTPanBase *nh = ncp->ncp_ClsBase;
    struct Library *BluetoothBase;
    struct ClsDevCfg *cdc;
    struct BtIFFContext *pic;

    KPRINTF(10, ("Loading Binding Config...\n"));
    if(ncp->ncp_GUITask)
    {
        return(FALSE);
    }
    //bLoadClassConfig(nh);
    *ncp->ncp_CDC = *nh->nh_DefaultUnit.ncp_CDC;
    ncp->ncp_UsingDefaultCfg = TRUE;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = btGetDevCfg(libname, ncp->ncp_DevIDString, NULL);
    if(pic)
    {
        cdc = btGetCfgChunk(pic, AROS_LONG2BE(ncp->ncp_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) ncp->ncp_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(ncp->ncp_CDC->cdc_Length)));
            btFreeVec(cdc);
            ncp->ncp_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(BluetoothBase);
    return(FALSE);
}
/* \\\ */

/* /// "bOpenBindingCfgWindow()" */
LONG bOpenBindingCfgWindow(struct BTPanBase *nh, struct BTPanUnit *ncp)
{
    struct Library *BluetoothBase;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(FALSE);
    }
    Forbid();
    if(!ncp->ncp_GUITask)
    {
        if((ncp->ncp_GUITask = btSpawnSubTask(MOD_NAME_STRING " GUI", bGUITask, ncp)))
        {
            Permit();
            CloseLibrary(BluetoothBase);
            return(TRUE);
        }
    }
    Permit();
    CloseLibrary(BluetoothBase);
    return(FALSE);
}
/* \\\ */

/**************************************************************************/

#undef  BluetoothBase
#define BluetoothBase ncp->ncp_Base

/* /// "bBNEPQueueCtl()" */
/* queue one control message; sent when the write channel is idle */
static void bBNEPQueueCtl(struct BTPanUnit *ncp, const UBYTE *msg, ULONG len)
{
    if(len > sizeof(ncp->ncp_CtlBuf))
    {
        return;
    }
    CopyMem((APTR) msg, ncp->ncp_CtlBuf, len);
    ncp->ncp_CtlLen = len;
}
/* \\\ */

/* /// "bBNEPServe()" */
/* keep the write channel busy: BNEP setup first, then queued control
   replies, then data packets */
static void bBNEPServe(struct BTPanUnit *ncp)
{
    struct IOSana2Req *ioreq;

    while(ncp->ncp_WriteKind == BPWK_NONE)
    {
        if(ncp->ncp_BNEPState == BPS_DOWN)
        {
            static const UBYTE setup[] = { BNEP_CONTROL, BNEP_CTL_SETUP_REQ, 2,
                                           0x11, 0x16,   /* destination: NAP */
                                           0x11, 0x15 }; /* source: PANU */
            UBYTE req[sizeof(setup)];
            CopyMem((APTR) setup, req, sizeof(setup));
            if(ncp->ncp_UnitUUID == 0x1117)
            {
                req[4] = 0x17;   /* destination: GN */
            }
            CopyMem(req, ncp->ncp_CtlBuf, sizeof(setup));
            btSendChannel(ncp->ncp_WriteCh, ncp->ncp_CtlBuf, sizeof(setup));
            ncp->ncp_WriteKind = BPWK_CTL;
            ncp->ncp_BNEPState = BPS_SETUP;
            return;
        }
        if(ncp->ncp_CtlLen)
        {
            btSendChannel(ncp->ncp_WriteCh, ncp->ncp_CtlBuf, ncp->ncp_CtlLen);
            ncp->ncp_CtlLen = 0;
            ncp->ncp_WriteKind = BPWK_CTL;
            return;
        }
        if(ncp->ncp_BNEPState != BPS_UP)
        {
            return;
        }
        Forbid();
        ioreq = (struct IOSana2Req *) RemHead(&ncp->ncp_WriteQueue);
        Permit();
        if(!ioreq)
        {
            return;
        }
        if(!bWritePacket(ncp, ioreq))
        {
            ReplyMsg((struct Message *) ioreq);
        }
    }
}
/* \\\ */

/* /// "bBNEPControl()" */
static void bBNEPControl(struct BTPanUnit *ncp, const UBYTE *p, ULONG len)
{
    UBYTE rsp[8];

    if(len < 1)
    {
        return;
    }
    switch(p[0])
    {
        case BNEP_CTL_SETUP_RSP:
            if(len >= 3)
            {
                UWORD code = (p[1] << 8) | p[2];
                if(ncp->ncp_BNEPState == BPS_SETUP)
                {
                    if(code == 0)
                    {
                        ncp->ncp_BNEPState = BPS_UP;
                        /* a peer with tethering disabled accepts the setup and
                           then drops the channel over and over: say it once,
                           and report loss/recovery instead of every cycle */
                        if(!ncp->ncp_EstLogged)
                        {
                            btAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                           "btpan.device unit %ld: BNEP connection established.",
                                           ncp->ncp_UnitNo);
                            ncp->ncp_EstLogged = TRUE;
                        }
                    } else {
                        ncp->ncp_BNEPState = BPS_DOWN;
                        btAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "btpan.device unit %ld: BNEP setup refused (0x%04lx).",
                                       ncp->ncp_UnitNo, (ULONG) code);
                        /* try again when the next write is served */
                        btDelayMS(2000);
                    }
                }
            }
            break;

        case BNEP_CTL_SETUP_REQ:
            /* we are the client end: not allowed */
            rsp[0] = BNEP_CONTROL; rsp[1] = BNEP_CTL_SETUP_RSP; rsp[2] = 0x00; rsp[3] = 0x04;
            bBNEPQueueCtl(ncp, rsp, 4);
            break;

        case BNEP_CTL_FILTER_NET_SET:
            rsp[0] = BNEP_CONTROL; rsp[1] = BNEP_CTL_FILTER_NET_RSP; rsp[2] = 0x00; rsp[3] = 0x00;
            bBNEPQueueCtl(ncp, rsp, 4);
            break;

        case BNEP_CTL_FILTER_MC_SET:
            rsp[0] = BNEP_CONTROL; rsp[1] = BNEP_CTL_FILTER_MC_RSP; rsp[2] = 0x00; rsp[3] = 0x00;
            bBNEPQueueCtl(ncp, rsp, 4);
            break;

        default:
            rsp[0] = BNEP_CONTROL; rsp[1] = BNEP_CTL_NOT_UNDERSTOOD; rsp[2] = p[0];
            bBNEPQueueCtl(ncp, rsp, 3);
            break;
    }
}
/* \\\ */

/* /// "bBNEPInput()" */
/* one BNEP packet from the read channel: reconstruct the ethernet frame
   (the compressed forms leave out the addresses L2CAP already implies)
   and hand it to the SANA-II side */
static void bBNEPInput(struct BTPanUnit *ncp, const UBYTE *p, ULONG len)
{
    UBYTE type;
    BOOL ext;
    ULONG pos = 1;
    UBYTE *eth = ncp->ncp_AsmBuf;
    ULONG plen;

    if(len < 1)
    {
        return;
    }
    type = p[0] & ~BNEP_EXT_FLAG;
    ext = (p[0] & BNEP_EXT_FLAG) ? TRUE : FALSE;

    switch(type)
    {
        case BNEP_GENERAL:
            if(len < pos + 14) return;
            CopyMem((APTR) &p[pos], eth, 14);
            pos += 14;
            break;
        case BNEP_COMPRESSED:
            if(len < pos + 2) return;
            CopyMem(ncp->ncp_MacAddress, eth, 6);
            CopyMem(ncp->ncp_PeerAddr, eth + 6, 6);
            eth[12] = p[pos]; eth[13] = p[pos + 1];
            pos += 2;
            break;
        case BNEP_COMPRESSED_SRC:
            if(len < pos + 8) return;
            CopyMem((APTR) &p[pos], eth, 6);
            CopyMem(ncp->ncp_PeerAddr, eth + 6, 6);
            eth[12] = p[pos + 6]; eth[13] = p[pos + 7];
            pos += 8;
            break;
        case BNEP_COMPRESSED_DST:
            if(len < pos + 8) return;
            CopyMem(ncp->ncp_MacAddress, eth, 6);
            CopyMem((APTR) &p[pos], eth + 6, 6);
            eth[12] = p[pos + 6]; eth[13] = p[pos + 7];
            pos += 8;
            break;
        case BNEP_CONTROL:
            /* control messages have no ethernet payload */
            if(len > pos)
            {
                bBNEPControl(ncp, &p[pos], len - pos);
            }
            return;
        default:
            return;
    }
    /* skip extension headers */
    while(ext)
    {
        if(len < pos + 2) return;
        ext = (p[pos] & BNEP_EXT_FLAG) ? TRUE : FALSE;
        if((p[pos] & ~BNEP_EXT_FLAG) == 0x00)
        {
            /* extension control: may carry piggybacked control messages */
            bBNEPControl(ncp, &p[pos + 2], p[pos + 1]);
        }
        pos += 2 + p[pos + 1];
        if(pos > len) return;
    }
    plen = len - pos;
    if(plen > ETHERPKT_SIZE)
    {
        return;
    }
    CopyMem((APTR) &p[pos], eth + 14, plen);
    if(ncp->ncp_LossLogged)
    {
        btAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "btpan.device unit %ld: BNEP connection recovered.",
                       ncp->ncp_UnitNo);
        ncp->ncp_LossLogged = FALSE;
    }
    ncp->ncp_RetryMS = 0;
    bReadPacket(ncp, eth, 14 + plen);
}
/* \\\ */

/* /// "bEthTask()" */
AROS_UFH0(void, bEthTask)
{
    AROS_USERFUNC_INIT

    struct BTPanUnit *ncp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    struct IOSana2Req *ioreq;

    if((ncp = bAllocEth()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
        }
        Permit();

        if(!(ncp->ncp_StateFlags & DDF_CONFIGURED))
        {
            /* don't overwrite a previously configured MAC address */
            CopyMem(ncp->ncp_CDC->cdc_MACAddress, ncp->ncp_MacAddress, ETHER_ADDR_SIZE);
        }

        ncp->ncp_StateFlags |= DDF_ONLINE;
        ncp->ncp_StateFlags &= ~DDF_OFFLINE;
        bDoEvent(ncp, S2EVENT_ONLINE);

        /* one read always pending; the BNEP setup goes out first */
        ncp->ncp_BNEPState = BPS_DOWN;
        btSendChannel(ncp->ncp_ReadCh, ncp->ncp_ReadBuffer[0], ETHER_MAX_LEN + BNEP_HDRMAX);
        ncp->ncp_ReadPending = ncp->ncp_ReadBuffer[0];
        bBNEPServe(ncp);

        sigmask = (1L<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1L<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        do
        {
            APTR ch;
            while((ch = (APTR) GetMsg(ncp->ncp_TaskMsgPort)))
            {
                if(ch == ncp->ncp_ReadCh)
                {
                    ioerr = btGetChannelError(ch);
                    if(!ioerr)
                    {
                        bBNEPInput(ncp, ncp->ncp_ReadBuffer[0], btGetChannelActual(ch));
                    }
                    else if((ioerr == IOERR_ABORTED) || (ioerr == BTIOERR_NOTCONNECTED) || (ioerr == BTIOERR_DISCONNECTED))
                    {
                        /* link down: a fresh L2CAP channel needs a fresh
                           BNEP setup once the device is back */
                        if((ncp->ncp_BNEPState == BPS_UP) && !ncp->ncp_LossLogged)
                        {
                            btAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "btpan.device unit %ld: BNEP connection lost - retrying.",
                                           ncp->ncp_UnitNo);
                            ncp->ncp_LossLogged = TRUE;
                        }
                        ncp->ncp_BNEPState = BPS_DOWN;
                        ncp->ncp_RetryMS = ncp->ncp_RetryMS ?
                            ((ncp->ncp_RetryMS >= 30000) ? 60000 : ncp->ncp_RetryMS * 2) : 2000;
                        btDelayMS(ncp->ncp_RetryMS);
                    } else {
                        bDoEvent(ncp, S2EVENT_ERROR|S2EVENT_RX);
                        btDelayMS(250);
                    }
                    if(!(SetSignal(0, 0) & SIGBREAKF_CTRL_C))
                    {
                        btSendChannel(ncp->ncp_ReadCh, ncp->ncp_ReadBuffer[0], ETHER_MAX_LEN + BNEP_HDRMAX);
                    }
                }
                else if(ch == ncp->ncp_WriteCh)
                {
                    ioerr = btGetChannelError(ch);
                    if(ncp->ncp_WriteKind == BPWK_DATA)
                    {
                        if((ioreq = ncp->ncp_WritePending[0]))
                        {
                            if(ioerr)
                            {
                                bDoEvent(ncp, S2EVENT_ERROR|S2EVENT_TX);
                                ioreq->ios2_DataLength   = 0;
                                ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
                                ioreq->ios2_WireError    = S2WERR_GENERIC_ERROR;
                            }
                            ReplyMsg((struct Message *) ioreq);
                            ncp->ncp_WritePending[0] = NULL;
                        }
                    }
                    ncp->ncp_WriteKind = BPWK_NONE;
                    if(ioerr && ((ioerr == BTIOERR_NOTCONNECTED) || (ioerr == BTIOERR_DISCONNECTED)))
                    {
                        if((ncp->ncp_BNEPState == BPS_UP) && !ncp->ncp_LossLogged)
                        {
                            btAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "btpan.device unit %ld: BNEP connection lost - retrying.",
                                           ncp->ncp_UnitNo);
                            ncp->ncp_LossLogged = TRUE;
                        }
                        ncp->ncp_BNEPState = BPS_DOWN;
                        ncp->ncp_RetryMS = ncp->ncp_RetryMS ?
                            ((ncp->ncp_RetryMS >= 30000) ? 60000 : ncp->ncp_RetryMS * 2) : 2000;
                        btDelayMS(ncp->ncp_RetryMS);
                    }
                }
            }
            bBNEPServe(ncp);
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));

        Forbid();
        if((ioreq = ncp->ncp_WritePending[0]))
        {
            ioreq->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ncp->ncp_WritePending[0] = NULL;
        }
        Permit();

        bDoEvent(ncp, S2EVENT_OFFLINE);

        KPRINTF(20, ("Going down the river!\n"));
        bFreeEth(ncp);
    }

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bAllocEth()" */
struct BTPanUnit * bAllocEth(void)
{
    struct Task *thistask;
    struct BTPanUnit *ncp;
    APTR bth = NULL;
    UBYTE *hwaddr = NULL;

    thistask = FindTask(NULL);
    ncp = thistask->tc_UserData;
    do
    {
        if(!(ncp->ncp_Base = OpenLibrary("bluetooth.library", 1)))
        {
            Alert(AG_OpenLib);
            break;
        }

        /* our station address defaults to the radio's BD address */
        btGetAttrs(BGA_DEVICE, ncp->ncp_Device, BDA_Hardware, &bth, TAG_END);
        if(bth)
        {
            btGetAttrs(BGA_HARDWARE, bth, BHA_Address, &hwaddr, TAG_END);
        }
        if(hwaddr)
        {
            CopyMem(hwaddr, ncp->ncp_ROMAddress, ETHER_ADDR_SIZE);
        }

        ncp->ncp_ReadPending = NULL;
        ncp->ncp_WritePending[0] = NULL;
        ncp->ncp_WritePending[1] = NULL;
        if(!(ncp->ncp_ReadBuffer[0] = AllocVec((ETHER_MAX_LEN + BNEP_HDRMAX) * 4, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            KPRINTF(1, ("Out of memory for read buffer\n"));
            break;
        }
        ncp->ncp_ReadBuffer[1] = ncp->ncp_ReadBuffer[0] + ETHER_MAX_LEN + BNEP_HDRMAX;
        ncp->ncp_WriteBuffer[0] = ncp->ncp_ReadBuffer[1] + ETHER_MAX_LEN + BNEP_HDRMAX;
        ncp->ncp_WriteBuffer[1] = ncp->ncp_WriteBuffer[0] + ETHER_MAX_LEN + BNEP_HDRMAX;
        ncp->ncp_AsmBuf = ncp->ncp_ReadBuffer[1];   /* reassembly area */
        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_ReadCh = btAllocChannel(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_Endpoint)))
            {
                if((ncp->ncp_WriteCh = btAllocChannel(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_Endpoint)))
                {
                    btSetAttrs(BGA_CHANNEL, ncp->ncp_ReadCh, BCHA_AutoConnect, TRUE, TAG_END);
                    btSetAttrs(BGA_CHANNEL, ncp->ncp_WriteCh, BCHA_AutoConnect, TRUE, TAG_END);
                    btChannelSetup(ncp->ncp_ReadCh, BTPR_READ, 0, 0);
                    btChannelSetup(ncp->ncp_WriteCh, BTPR_WRITE, 0, 0);
                    ncp->ncp_Task = thistask;
                    return(ncp);
                }
                btFreeChannel(ncp->ncp_ReadCh);
                ncp->ncp_ReadCh = NULL;
            }
            DeleteMsgPort(ncp->ncp_TaskMsgPort);
            ncp->ncp_TaskMsgPort = NULL;
        }
        FreeSignal((LONG) ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
    } while(FALSE);
    if(ncp->ncp_ReadBuffer[0])
    {
        FreeVec(ncp->ncp_ReadBuffer[0]);
        ncp->ncp_ReadBuffer[0] = NULL;
    }
    if(ncp->ncp_Base)
    {
        CloseLibrary(ncp->ncp_Base);
        ncp->ncp_Base = NULL;
    }
    Forbid();
    ncp->ncp_Task = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "bFreeEth()" */
void bFreeEth(struct BTPanUnit *ncp)
{
    struct IOSana2Req *ioreq;
    Forbid();
    /* Disable the message port, messages may still be queued */
    ncp->ncp_Unit.unit_MsgPort.mp_SigTask = NULL;
    ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
    FreeSignal((LONG) ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
    // get rid of all messages that still have appeared here
    while((ioreq = (struct IOSana2Req *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
    {
        ioreq->ios2_Req.io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
    }
    Permit();

    if(ncp->ncp_ReadCh)
    {
        btAbortChannel(ncp->ncp_ReadCh);
        btWaitChannel(ncp->ncp_ReadCh);
        btFreeChannel(ncp->ncp_ReadCh);
        ncp->ncp_ReadCh = NULL;
    }
    if(ncp->ncp_WriteCh)
    {
        btAbortChannel(ncp->ncp_WriteCh);
        btWaitChannel(ncp->ncp_WriteCh);
        btFreeChannel(ncp->ncp_WriteCh);
        ncp->ncp_WriteCh = NULL;
    }

    if(ncp->ncp_ReadBuffer[0])
    {
        FreeVec(ncp->ncp_ReadBuffer[0]);
        ncp->ncp_ReadBuffer[0] = NULL;
    }

    DeleteMsgPort(ncp->ncp_TaskMsgPort);
    ncp->ncp_TaskMsgPort = NULL;
    CloseLibrary(ncp->ncp_Base);
    ncp->ncp_Base = NULL;
    Forbid();
    ncp->ncp_Task = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
    }
}
/* \\\ */

/* /// "bDoEvent()" */
void bDoEvent(struct BTPanUnit *ncp, ULONG events)
{
    struct IOSana2Req *worknode, *nextnode;

    KPRINTF(1, ("DoEvent events: 0x%08lx\n", events));

    Forbid();
    /* Process pending S2_ONEVENT requests */
    worknode = (struct IOSana2Req *) ncp->ncp_EventList.lh_Head;
    while((nextnode = (struct IOSana2Req *) (((struct Node *) worknode)->ln_Succ)))
    {
        if(worknode->ios2_WireError & events)
        {
            Remove(&worknode->ios2_Req.io_Message.mn_Node);
            worknode->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            KPRINTF(1, ("DoEvent: returned eventreq 0x%08lx\n", worknode));
            ReplyMsg(&worknode->ios2_Req.io_Message);
        }
        worknode = nextnode;
    }
    Permit();
}
/* \\\ */

/* /// "support routines" */
static
inline void *callcopy(void *routine,
                      void *from,
                      void *to,
                      ULONG len)
{
  void * (*call) (APTR, APTR, ULONG) = routine;

  return (*call) (from, to, len);
}

#define callfilter CallHookPkt
/* \\\ */

/* /// "bWritePacket()" */
BOOL bWritePacket(struct BTPanUnit *ncp, struct IOSana2Req *ioreq)
{
    ULONG packettype;
    struct EtherPacketHeader *eph;
    UBYTE *copydest;
    UWORD writelen;
    struct BufMan *bufman;
    struct Sana2PacketTypeStats *stats;
    UBYTE *buf = ncp->ncp_WriteBuffer[ncp->ncp_WriteBufNum];

    packettype = ioreq->ios2_PacketType;
    /* BNEP general ethernet: type byte, then the ethernet frame */
    buf[0]     = BNEP_GENERAL;
    eph        = (struct EtherPacketHeader *) (buf + 1);
    copydest   = buf + 1;
    writelen   = ioreq->ios2_DataLength;
    bufman     = ioreq->ios2_BufferManagement;

    /* Not a raw packet? */
    if(!(ioreq->ios2_Req.io_Flags & SANA2IOF_RAW))
    {
        UWORD cnt;
        KPRINTF(10, ("RAW WRITE!\n"));
        /* The ethernet header isn't included in the data */
        /* Build ethernet packet header */
        for(cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++)
        {
            eph->eph_Dest[cnt] = ioreq->ios2_DstAddr[cnt];
            eph->eph_Src[cnt]  = ncp->ncp_MacAddress[cnt];
        }
        eph->eph_Type = AROS_BE2WORD(packettype);

        /* Packet data is at txbuffer */
        copydest += sizeof(struct EtherPacketHeader);
        writelen += sizeof(struct EtherPacketHeader);
    }

    /* Dma not available, fallback to regular copy */
    if(callcopy(bufman->bm_CopyFromBuf, copydest, ioreq->ios2_Data, ioreq->ios2_DataLength) == NULL)
    {
        KPRINTF(10, ("writepacket: copyfrom returned failure!\n"));

        /* Trigger any tx, buff or generic error events */
        bDoEvent(ncp, S2EVENT_ERROR|S2EVENT_TX|S2EVENT_BUFF);

        /* Set error code and terminate the iorequest.
           NOTE: Can't use RC_* or deverror() this is not
           called from devBeginIO()! */
        ioreq->ios2_DataLength   = 0;
        ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        ioreq->ios2_WireError    = S2WERR_BUFF_ERROR;
        return FALSE;
    }

    KPRINTF(20, ("PktOut[%ld] %ld\n", ncp->ncp_WriteBufNum, writelen));

    ncp->ncp_WritePending[0] = ioreq;
    btSendChannel(ncp->ncp_WriteCh, buf, (ULONG) writelen + 1);
    ncp->ncp_WriteKind = BPWK_DATA;

    DB(
        if(AROS_BE2WORD(eph->eph_Type) < 1500)
        {
            KPRINTF(5, ("writepacket: %04lx%08lx > %04lx%08lx (IEEE802.3) len %lu, %lu bytes\n",
                        *((UWORD *) eph->eph_Src), *((ULONG *) (eph->eph_Src + 2)),
                        *((UWORD *) eph->eph_Dest), *((ULONG *) (eph->eph_Dest + 2)),
                        AROS_BE2WORD(eph->eph_Type), writelen));
        } else {
            KPRINTF(5, ("writepacket: %04lx%08lx > %04lx%08lx type %lu, %lu bytes\n",
                        *((UWORD *) eph->eph_Src), *((ULONG *) (eph->eph_Src + 2)),
                        *((UWORD *) eph->eph_Dest), *((ULONG *) (eph->eph_Dest + 2)),
                        AROS_BE2WORD(eph->eph_Type), writelen));
        }
        //dumpmem(buf, (ULONG) writelen);
    )

    /* Update statistics */
    stats = FindPacketTypeStats(ncp, packettype);
    if(stats)
    {
        stats->PacketsSent++;
        stats->BytesSent += writelen;
    }
    ncp->ncp_DeviceStats.PacketsSent++;

    return TRUE;
}
/* \\\ */

/* /// "bReadIOReq()" */
UWORD bReadIOReq(struct BTPanUnit *ncp, struct EtherPacketHeader *eph, UWORD datasize, struct IOSana2Req *ioreq, UWORD flags)
{
    LIBBASETYPEPTR nh = ncp->ncp_ClsBase;
    UBYTE *copyfrom;
    UWORD cnt;

    /* Handle RAW read */
    if(ioreq->ios2_Req.io_Flags & SANA2IOF_RAW)
    {
        /* ShapeShifter won't work with `sizeof(struct etherpacket_hdr)'
           here. This is most likely because it want the RAW ethernet
           packet checksum size (4) added to the packet size. */
        copyfrom = (UBYTE *) eph;
        datasize += sizeof(struct EtherPacketHeader) + 4;
    } else {
        copyfrom = (UBYTE *) (eph + 1);
    }

    /* Build up the ios2 structure enough so we can call the packet filter. */
    ioreq->ios2_PacketType = AROS_BE2WORD(eph->eph_Type);
    for(cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++)
    {
        ioreq->ios2_SrcAddr[cnt] = eph->eph_Src[cnt];
        ioreq->ios2_DstAddr[cnt] = eph->eph_Dest[cnt];
    }
    ioreq->ios2_DataLength = datasize;
    /* Call the packet filter, if available. */
    if((flags & PACKETFILTER) &&
       (((struct BufMan *) ioreq->ios2_BufferManagement)->bm_PacketFilter) &&
       (!callfilter(((struct BufMan *) ioreq->ios2_BufferManagement)->bm_PacketFilter,
                   ioreq, copyfrom)))
    {
        /* This packet got dropped! */
        KPRINTF(7, ("readioreq: packet type %lu for ioreq 0x%08lx dropped\n",
                AROS_BE2WORD(eph->eph_Type), ioreq));
        return flags;
    }


    /* Ok, the packet didn't get dropped, set the BCAST and MCAST
       flags according to dstaddr. */

    /* Address == Multicast? */
    if(ioreq->ios2_DstAddr[0] & 1)
    {
        /* Address == Broadcast? */
        static const UBYTE bcast[ETHER_ADDR_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        if(memcmp(bcast, ioreq->ios2_DstAddr, ETHER_ADDR_SIZE) == 0)
        {
            ioreq->ios2_Req.io_Flags |= SANA2IOF_BCAST;
        } else {
            ioreq->ios2_Req.io_Flags |= SANA2IOF_MCAST;
        }
    }

    /* Finally copy the packet data! */
    if(callcopy(((struct BufMan *) ioreq->ios2_BufferManagement)->bm_CopyToBuf,
               ioreq->ios2_Data, copyfrom, ioreq->ios2_DataLength))
    {
        DB(
        KPRINTF(5, ("readioreq: copytobuffed packet ior 0x%08lx, %04lx%08lx < %04lx%08lx, type %lu, %lu bytes, %s%s%s\n",
                    ioreq,
                    *((UWORD *) ioreq->ios2_DstAddr), *((ULONG *) (ioreq->ios2_DstAddr + 2)),
                    *((UWORD *) ioreq->ios2_SrcAddr), *((ULONG *) (ioreq->ios2_SrcAddr + 2)),
                    ioreq->ios2_PacketType, ioreq->ios2_DataLength,
                    (ioreq->ios2_Req.io_Flags & SANA2IOF_RAW) ? "RAW " : "",
                    (ioreq->ios2_Req.io_Flags & SANA2IOF_BCAST) ? "BCAST " : "",
                    (ioreq->ios2_Req.io_Flags & SANA2IOF_MCAST) ? "MCAST " : ""));
        //dumpmem(copyfrom, ioreq->ios2_DataLength);
        )

        /* Clear the dropped flag */
        flags &= ~DROPPED;
    } else {
        KPRINTF(10, ("readioreq: copyto returned failure!\n"));

        /* Trigger any rx, buff or generic error events */
        bDoEvent(ncp, S2EVENT_ERROR|S2EVENT_RX|S2EVENT_BUFF);

        /* Set error code.
           NOTE: Can't use RC_* or deverror() this is not called from devBeginIO()!
        */
        ioreq->ios2_DataLength   = 0;
        ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        ioreq->ios2_WireError    = S2WERR_BUFF_ERROR;
    }

    /* Pull the ioreq off the list & terminate it */
    Forbid();
    Remove((struct Node *) ioreq);
    Permit();
    ReplyMsg((struct Message *) ioreq);
    return flags;
}
/* \\\ */

/* /// "bReadPacket()" */
BOOL bReadPacket(struct BTPanUnit *ncp, UBYTE *pktptr, ULONG pktlen)
{
    struct EtherPacketHeader *eph;
    struct BufMan *bufman;
    struct IOSana2Req *worknode, *nextnode;
    struct Sana2PacketTypeStats *stats;
    UWORD flags;
    UWORD datasize;

    KPRINTF(20, ("PktIn [%ld] %ld\n", ncp->ncp_ReadBufNum, pktlen));

    if(pktlen < 14)
    {
        ncp->ncp_DeviceStats.BadData++;
        return FALSE;
    }
    ncp->ncp_DeviceStats.PacketsReceived++;

    eph = (struct EtherPacketHeader *) pktptr;
    stats = FindPacketTypeStats(ncp, (ULONG) AROS_BE2WORD(eph->eph_Type));
    flags = DROPPED|PACKETFILTER;

    /* Calculate size of the actual data */
    datasize = pktlen - sizeof(struct EtherPacketHeader);

    /* Is the packet datasize valid? */
    if(pktlen <= ETHER_MAX_LEN)
    {
        /* Update the packet statistics */
        if(stats)
        {
            stats->PacketsReceived++;
            stats->BytesReceived += datasize;  /* NOTE: don't include headers */
        }

        /* For each device user (bufman)
           NOTE: We absolutely *MUST* try to offer the packet to *all*
           different device users (SANA-II V2 spec requirement). */
        Forbid();
        bufman = (struct BufMan *) ncp->ncp_BufManList.lh_Head;
        while(((struct Node *) bufman)->ln_Succ)
        {
            /* For each queued read request (ioreq) */
            worknode = (struct IOSana2Req *) bufman->bm_RXQueue.lh_Head;
            while((nextnode = (struct IOSana2Req *) (((struct Node *) worknode)->ln_Succ)))
            {
                /* Check the packet type. Also handles 802.3 packets. */
                if((worknode->ios2_PacketType == AROS_BE2WORD(eph->eph_Type)) ||
                   ((AROS_BE2WORD(eph->eph_Type) < 1500) && (worknode->ios2_PacketType < 1500)))
                {
                    flags = bReadIOReq(ncp, eph, datasize, worknode, flags);
                    /* Break out - let other callers get the packet too */
                    break;
                }
                worknode = nextnode;
            }
            bufman = (struct BufMan *) (((struct Node *) bufman)->ln_Succ);
        }
        Permit();
        /* Now we've tried to give the packet to every CMD_READ caller.
           If DROPPED is set at this point no-one wanted this packet. */
        if(flags & DROPPED)
        {
            /* So there were no outstanding CMD_READs or the packet wasn't
               accepted by any of them. Okay, check if we have any pending
               S2_READORPHAN ioreq in list and if we have return this packet
               with it. Note that packet filter must not be used for this
               time!

               NOTE: orphanlist is global, ie. only one caller will get the
               packet if multiple users have pending S2_READORPHANs.
            */

            /* Process pending orphanread iorequs */
            Forbid();
            worknode = (struct IOSana2Req *) ncp->ncp_OrphanQueue.lh_Head;
            while((nextnode = (struct IOSana2Req *) (((struct Node *) worknode)->ln_Succ)))
            {
                bReadIOReq(ncp, eph, datasize, worknode, 0);
                worknode = nextnode;
            }
            Permit();
        } else {
            /* Packet not dropped - return ok */
            return TRUE;
        }
    } else {
        KPRINTF(20, ("Pktlen %ld invalid!\n", pktlen));
        ncp->ncp_DeviceStats.BadData++;
    }
    /* Update global dropped packet counter. */
    ncp->ncp_DeviceStats.UnknownTypesReceived++;

    /* Update dropped packet statistics. */
    if(stats)
    {
        stats->PacketsDropped++;
    }
    KPRINTF(9, ("readpacket: packet type %lu dropped\n", AROS_BE2WORD(eph->eph_Type)));

    /* Trigger any rx or generic error events */
    bDoEvent(ncp, S2EVENT_ERROR|S2EVENT_RX);
    return FALSE;
}
/* \\\ */

/**************************************************************************/

/* /// "bGetMACAddress()" */
void bGetMACAddress(UBYTE *macaddr, CONST_STRPTR tmpstr)
{
    ULONG macbyte = 0;
    UBYTE ch;
    UWORD cnt = 0;

    while((ch = *tmpstr++))
    {
        if((ch >= '0') && (ch <= '9'))
        {
            macbyte <<= 4;
            macbyte += ch - '0';
        }
        else if((ch >= 'a') && (ch <= 'f'))
        {
            macbyte <<= 4;
            macbyte += ch - 'a' + 10;
        }
        else if((ch >= 'A') && (ch <= 'F'))
        {
            macbyte <<= 4;
            macbyte += ch - 'A' + 10;
        }
        else if(ch == ':')
        {
            if(cnt == 5)
            {
                break;
            }
            macaddr[cnt] = macbyte;
            macbyte = 0;
            cnt++;
        }
    }
    macaddr[cnt] = macbyte;
}
/* \\\ */

/* /// "bGUITask()" */
AROS_UFH0(void, bGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct BTPanBase *nh;
    struct BTPanUnit *ncp;
    struct BtIFFContext *pic;
    UBYTE macaddr[20];

    thistask = FindTask(NULL);
#undef BluetoothBase
#define BluetoothBase ncp->ncp_BtBase
#undef IntuitionBase
#define IntuitionBase ncp->ncp_IntBase
#undef MUIMasterBase
#define MUIMasterBase ncp->ncp_MUIBase

    ncp = thistask->tc_UserData;
    nh = ncp->ncp_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        bGUITaskCleanup(ncp);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        bGUITaskCleanup(ncp);
        return;
    }
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        bGUITaskCleanup(ncp);
        return;
    }

    btSafeRawDoFmt(macaddr, 20, "%02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
                    ncp->ncp_CDC->cdc_MACAddress[0],
                    ncp->ncp_CDC->cdc_MACAddress[1],
                    ncp->ncp_CDC->cdc_MACAddress[2],
                    ncp->ncp_CDC->cdc_MACAddress[3],
                    ncp->ncp_CDC->cdc_MACAddress[4],
                    ncp->ncp_CDC->cdc_MACAddress[5]);

    ncp->ncp_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)libname,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)ISOASCII_COPYRIGHT "2006-2007 Harry Sintonen & Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Harry Sintonen <sintonen@iki.fi> & Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the Bluetooth PAN class",
        MUIA_Application_Base       , (IPTR)"BTPAN",
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(ncp->ncp_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(ncp->ncp_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Save",
                    MUIA_Menuitem_Shortcut, (IPTR)"S",
                    End),
                Child, (IPTR)(ncp->ncp_SetDefaultMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Set as Default",
                    MUIA_Menuitem_Shortcut, (IPTR)"D",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(ncp->ncp_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            End,

        SubWindow, (IPTR)(ncp->ncp_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR)libname,
            MUIA_HelpNode, (IPTR)libname,

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)ColGroup(2), GroupFrameT(ncp->ncp_Interface ? "Device Settings" : "Default Device Settings"),
                    //Child, HSpace(0),
                    Child, (IPTR)Label((IPTR) "Default MAC Address:"),
                    Child, (IPTR)(ncp->ncp_MACAddressObj = StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_Contents, (IPTR)macaddr,
                        MUIA_String_MaxLen, 18,
                        MUIA_String_Accept, (IPTR)"0123456789abcdefABCDEF:",
                        End),
                    Child, (IPTR)Label((IPTR) "Default " DEVNAME " Unit:"),
                    Child, (IPTR)(ncp->ncp_UnitObj = StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_Integer, ncp->ncp_CDC->cdc_DefaultUnit,
                        MUIA_String_Accept, (IPTR)"0123456789",
                        End),
                    End,
                Child, (IPTR)VSpace(0),
                Child, (IPTR)HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(ncp->ncp_UseObj = TextObject, ButtonFrame,
                        MUIA_ShowMe, (IPTR)ncp->ncp_Interface,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Save ",
                        End),
                    Child, (IPTR)(ncp->ncp_SetDefaultObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)(ncp->ncp_Interface ? "\33c Save as Default " : "\33c Save Defaults "),
                        End),
                    Child, (IPTR)(ncp->ncp_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Use ",
                        End),
                    End,
                End,
            End),
        End;

    if(!ncp->ncp_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        bGUITaskCleanup(ncp);
        return;
    }

    DoMethod(ncp->ncp_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             ncp->ncp_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(ncp->ncp_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             ncp->ncp_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(ncp->ncp_SetDefaultObj, MUIM_Notify, MUIA_Pressed, FALSE,
             ncp->ncp_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(ncp->ncp_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             ncp->ncp_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(ncp->ncp_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncp->ncp_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(ncp->ncp_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncp->ncp_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(ncp->ncp_SetDefaultMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncp->ncp_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(ncp->ncp_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncp->ncp_App, 2, MUIM_Application_OpenConfigWindow, 0);
    {
        IPTR  isopen = 0;
        IPTR  iconify = 0;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(ncp->ncp_App, MUIA_Application_Iconified, &iconify);
        set(ncp->ncp_MainWindow, MUIA_Window_Open, TRUE);
        get(ncp->ncp_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            bGUITaskCleanup(ncp);
            return;
        }
        sigmask = 0;
        do
        {
            retid = DoMethod(ncp->ncp_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_DEF_CONFIG:
                case ID_STORE_CONFIG:
                case MUIV_Application_ReturnID_Quit:
                {
                    CONST_STRPTR tmpstr = "";
                    get(ncp->ncp_UnitObj, MUIA_String_Integer, &ncp->ncp_CDC->cdc_DefaultUnit);
                    get(ncp->ncp_MACAddressObj, MUIA_String_Contents, &tmpstr);

                    bGetMACAddress(ncp->ncp_CDC->cdc_MACAddress, tmpstr);

                    if(retid == ID_DEF_CONFIG)
                    {
                        pic = btGetClsCfg(libname);
                        if(!pic)
                        {
                            btSetClsCfg(libname, NULL);
                            pic = btGetClsCfg(libname);
                        }
                        if(pic)
                        {
                            btAddCfgEntry(pic, ncp->ncp_CDC);
                            btSaveCfgToDisk(NULL, FALSE);
                        }
                    }
                    if(ncp->ncp_Interface)
                    {
                        pic = btGetDevCfg(libname, ncp->ncp_DevIDString, NULL);
                        if(!pic)
                        {
                            btSetDevCfg(libname, ncp->ncp_DevIDString, NULL, NULL);
                            pic = btGetDevCfg(libname, ncp->ncp_DevIDString, NULL);
                        }
                        if(pic)
                        {
                            if(btAddCfgEntry(pic, ncp->ncp_CDC))
                            {
                                if(retid != MUIV_Application_ReturnID_Quit)
                                {
                                    btSaveCfgToDisk(NULL, FALSE);
                                }
                                retid = MUIV_Application_ReturnID_Quit;
                            }
                        }
                    } else {
                        retid = MUIV_Application_ReturnID_Quit;
                    }
                    break;
                }

                case ID_ABOUT:
                    MUI_RequestA(ncp->ncp_App, ncp->ncp_MainWindow, 0, NULL, "Phat!", VERSION_STRING, NULL);
                    break;
            }
            if(retid == MUIV_Application_ReturnID_Quit)
            {
                break;
            }
            if(sigs)
            {
                sigs = Wait(sigs|sigmask|SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
            }
        } while(TRUE);
        set(ncp->ncp_MainWindow, MUIA_Window_Open, FALSE);
    }
    bGUITaskCleanup(ncp);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bGUITaskCleanup()" */
void bGUITaskCleanup(struct BTPanUnit *ncp)
{
    if(ncp->ncp_App)
    {
        MUI_DisposeObject(ncp->ncp_App);
        ncp->ncp_App = NULL;
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
    ncp->ncp_GUIBinding = NULL;
    ncp->ncp_GUITask = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
    }
    --ncp->ncp_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */
