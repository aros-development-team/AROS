/*
 *----------------------------------------------------------------------------
 *                         asixeth class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "asixeth.class.h"

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
    struct NepClassEth *ncp;
    struct NepEthBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct NepEthDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct NepEthDevBase), NULL)))
        {
            ncp = &nh->nh_DummyNCP;
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
            KPRINTF(20, ("failed to create usbparallel.device\n"));
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
    nLoadClassConfig(nh);
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    struct NepClassEth *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);

        ncp = (struct NepClassEth *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp->ncp_CDC);
            FreeVec(ncp);
            ncp = (struct NepClassEth *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! asixeth.class expunged!\n\n"));
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
    UWORD abd_PatchFlags;
};

struct AutoBindData ClassBinds[] =
{
    { 0x0411, 0x003d, 0              }, // Buffalo LUA-U2-KTX
    { 0x04f1, 0x3008, 0              }, // JVC MP-PRX1 Port Replicator
    { 0x0557, 0x2009, 0              }, // ATEN UC210T
    { 0x077b, 0x2226, 0              }, // Linksys USB200M
    { 0x07aa, 0x0017, 0              }, // corega FEther USB2-TX
    { 0x07b8, 0x420a, PF_HAWKINGGPIO }, // Hawking UF200, TrendNet TU2-ET100
    { 0x0846, 0x1040, 0              }, // Netgear FA120
    { 0x08dd, 0x90ff, 0              }, // Billionton Systems, USB2AR
    { 0x0b95, 0x1720, 0              }, // Intellinet, ST Lab USB Ethernet
    { 0x1189, 0x0893, 0              }, // Surecom EP-1427X-2
    { 0x1631, 0x6200, 0              }, // goodway corp usb gwusb2e
    { 0x2001, 0x1a00, PF_DLINKGPIO   }, // DLink DUB-E100
    { 0x6189, 0x182d, 0              }, // Sitecom LN-029 "USB 2.0 10/100 Ethernet adapter"
    { 0x04bb, 0x0930, PF_AX88178     }, // IO-DATA ETG-US2
    { 0x0b95, 0x1780, PF_AX88178     }, // ASIX AX88178 10/100/1000
    { 0x1737, 0x0039, PF_AX88178     }, // Linksys USB1000
    { 0x050d, 0x5055, PF_AX88178     }, // Belkin F5D5055
    { 0x0411, 0x006e, PF_AX88178     }, // Buffalo LUA-U2-GT 10/100/1000
    { 0x07d1, 0x3c05, PF_AX88772     }, // DLink DUB-E100 H/W Ver B1
    { 0x2001, 0x3c05, PF_AX88772     }, // DLink DUB-E100 H/W Ver B1 Alternate
    { 0x0b95, 0x7720, PF_AX88772     }, // ASIX AX88772 10/100
    { 0x13b1, 0x0018, PF_AX88772     }, // Linksys USB200M Rev 2
    { 0x1557, 0x7720, PF_AX88772     }, // 0Q0 cable ethernet
    { 0x05ac, 0x1402, PF_AX88772     }, // Apple Adapter
    { 0, 0, 0 }
};

/* /// "usbAttemptDeviceBinding()" */
struct NepClassEth * usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct AutoBindData *abd = ClassBinds;
    IPTR prodid;
    IPTR vendid;

    KPRINTF(1, ("nepEthAttemptDeviceBinding(%08lx)\n", pd));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_VendorID, &vendid,
                    DA_ProductID, &prodid,
                    TAG_END);
        CloseLibrary(ps);
        while(abd->abd_VendID)
        {
            if((vendid == abd->abd_VendID) && (prodid == abd->abd_ProdID))
            {
                return(usbForceDeviceBinding(nh, pd));
            }
            abd++;
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceDeviceBinding()" */
struct NepClassEth * usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct NepClassEth *ncp;
    struct NepClassEth *tmpncp;
    struct ClsDevCfg *cdc;
    struct AutoBindData *abd = ClassBinds;
    STRPTR devname;
    STRPTR devidstr;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    BOOL  unitfound;
    UBYTE buf[64];
    UWORD patchflags = 0;
    struct Task *tmptask;

    KPRINTF(1, ("nepEthForceDeviceBinding(%08lx)\n", pd));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductID, &prodid,
                    DA_VendorID, &vendid,
                    DA_ProductName, &devname,
                    DA_IDString, &devidstr,
                    TAG_END);
        Forbid();

        while(abd->abd_VendID)
        {
            if((vendid == abd->abd_VendID) && (prodid == abd->abd_ProdID))
            {
                patchflags = abd->abd_PatchFlags;
                break;
            }
            abd++;
        }

        unitfound = FALSE;
        unitno = (ULONG) -1;
        ncp = (struct NepClassEth *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(!strcmp(ncp->ncp_DevIDString, devidstr))
            {
                unitno = ncp->ncp_UnitNo;
                unitfound = TRUE;
                break;
            }
            ncp = (struct NepClassEth *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(ncp = AllocVec(sizeof(struct NepClassEth), MEMF_PUBLIC|MEMF_CLEAR)))
            {
                Permit();
                CloseLibrary(ps);
                return(NULL);
            }
            ncp->ncp_CDC = cdc = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
            if(!cdc)
            {
                Permit();
                FreeVec(ncp);
                CloseLibrary(ps);
                return(NULL);
            }
            /* IORequests may be queued even if the task is gone. */
            ncp->ncp_UnitNo = (ULONG) -1;
            NewList(&ncp->ncp_Unit.unit_MsgPort.mp_MsgList);
            NewList(&ncp->ncp_OrphanQueue);
            NewList(&ncp->ncp_WriteQueue);
            NewList(&ncp->ncp_BufManList);
            NewList(&ncp->ncp_EventList);
            NewList(&ncp->ncp_TrackList);
            NewList(&ncp->ncp_Multicasts);
            strncpy(ncp->ncp_DevIDString, devidstr, 127);
            AddTail(&nh->nh_Units, &ncp->ncp_Unit.unit_MsgPort.mp_Node);
            ncp->ncp_PatchFlags = patchflags;
        }
        ncp->ncp_ClsBase = nh;
        ncp->ncp_Device = pd;
        ncp->ncp_UnitProdID = prodid;
        ncp->ncp_UnitVendorID = vendid;

        nLoadBindingConfig(ncp);

        /* Find next free unit number */
        if(unitno == (ULONG) -1)
        {
            unitno = ncp->ncp_CDC->cdc_DefaultUnit;
            tmpncp = (struct NepClassEth *) nh->nh_Units.lh_Head;
            while(tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                if(tmpncp->ncp_UnitNo == unitno)
                {
                    unitno++;
                    tmpncp = (struct NepClassEth *) nh->nh_Units.lh_Head;
                } else {
                    tmpncp = (struct NepClassEth *) tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
                }
            }
        }
        ncp->ncp_UnitNo = unitno;
        Permit();

        psdSafeRawDoFmt(buf, 64, "asixeth.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nEthTask, ncp)))
        {
            psdBorrowLocksWait(tmptask, 1L<<ncp->ncp_ReadySignal);
            //Wait(1L<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                //FreeSignal(ncp->ncp_ReadySignal);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "A six yards hit by '%s' on %s unit %ld!",
                               devname, nh->nh_DevBase->np_Library.lib_Node.ln_Name,
                               ncp->ncp_UnitNo);

                CloseLibrary(ps);
                return(ncp);
            }
        }
        ncp->ncp_ReadySigTask = NULL;
        //FreeSignal(ncp->ncp_ReadySignal);
        /* Get rid of unit structure */
        /*Forbid();
        Remove((struct Node *) ncp);
        FreeVec(ncp->ncp_CDC);
        FreeVec(ncp);
        Permit();*/
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseDeviceBinding()" */
void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp)
{
    struct Library *ps;
    STRPTR devname;
    KPRINTF(1, ("nepEthReleaseDeviceBinding(%08lx)\n", ncp));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        if(ncp->ncp_Task)
        {
            Signal(ncp->ncp_Task, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(ncp->ncp_Task)
        {
            Wait(1L<<ncp->ncp_ReadySignal);
        }
        //FreeSignal(ncp->ncp_ReadySignal);
        psdGetAttrs(PGA_DEVICE, ncp->ncp_Device, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "A '%s' sucks.",
                       devname);
        /*psdFreeVec(ncp);*/
        CloseLibrary(ps);
    }
}
/* \\\ */

/* /// "usbGetAttrsA()" */
AROS_LH3(LONG, usbGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, nep)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    KPRINTF(1, ("nepEthGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case UGA_CLASS:
             if((ti = FindTagItem(UCCA_Priority, tags)))
             {
                 *((SIPTR *) ti->ti_Data) = 0;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Ethernet SANA driver for ASIX chipsets via usbasixeth.device";
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasBindingCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_AfterDOSRestart, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nh->nh_DummyNCP.ncp_UsingDefaultCfg;
                 count++;
             }
             break;

         case UGA_BINDING:
             if((ti = FindTagItem(UCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = ((struct NepClassEth *) usbstruct)->ncp_UsingDefaultCfg;
                 count++;
             }
             break;
    }
    return(count);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbSetAttrsA()" */
AROS_LH3(LONG, usbSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, nep)
{
    AROS_LIBFUNC_INIT
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbDoMethodA()" */
AROS_LH2(IPTR, usbDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, nh, 7, nep)
{
    AROS_LIBFUNC_INIT

    struct NepClassEth *ncp;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptDeviceBinding:
            return((IPTR) usbAttemptDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ForceDeviceBinding:
            return((IPTR) usbForceDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ReleaseDeviceBinding:
            usbReleaseDeviceBinding(nh, (struct NepClassEth *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenBindingCfgWindow(nh, &nh->nh_DummyNCP));

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassEth *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            Forbid();
            ncp = (struct NepClassEth *) nh->nh_Units.lh_Head;
            while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                nLoadBindingConfig(ncp);
                ncp = (struct NepClassEth *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
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

/* /// "nLoadClassConfig()" */
BOOL nLoadClassConfig(struct NepEthBase *nh)
{
    struct NepClassEth *ncp = &nh->nh_DummyNCP;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(ncp->ncp_GUITask)
    {
        return(FALSE);
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Create default config */
    cdc = ncp->ncp_CDC;
    cdc->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('A','S','I','X'));
    cdc->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    cdc->cdc_DefaultUnit = 0;
    cdc->cdc_MediaType = 0;
    ncp->ncp_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = psdGetClsCfg(libname);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(ncp->ncp_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) ncp->ncp_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(ncp->ncp_CDC->cdc_Length)));
            psdFreeVec(cdc);
            ncp->ncp_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nLoadBindingConfig()" */
BOOL nLoadBindingConfig(struct NepClassEth *ncp)
{
    struct NepEthBase *nh = ncp->ncp_ClsBase;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Binding Config...\n"));
    if(ncp->ncp_GUITask)
    {
        return(FALSE);
    }
    //nLoadClassConfig(nh);
    *ncp->ncp_CDC = *nh->nh_DummyNCP.ncp_CDC;
    ncp->ncp_UsingDefaultCfg = TRUE;

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = psdGetUsbDevCfg(libname, ncp->ncp_DevIDString, NULL);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(ncp->ncp_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) ncp->ncp_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(ncp->ncp_CDC->cdc_Length)));
            psdFreeVec(cdc);
            ncp->ncp_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nOpenBindingCfgWindow()" */
LONG nOpenBindingCfgWindow(struct NepEthBase *nh, struct NepClassEth *ncp)
{
    struct Library *ps;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!ncp->ncp_GUITask)
    {
        if((ncp->ncp_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", nGUITask, ncp)))
        {
            Permit();
            CloseLibrary(ps);
            return(TRUE);
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/**************************************************************************/

#undef  ps
#define ps ncp->ncp_Base

static char *MediaTypeStrings[] =
{
    "Auto negotiation",
    "10Base-T Half Duplex",
    "10Base-T Full Duplex",
    "100Base-TX Half Duplex",
    "100Base-TX Full Duplex",
    "1000Base-TX Half Duplex",
    "1000Base-TX Full Duplex",
    NULL
};

/* /// "nEthTask()" */
AROS_UFH0(void, nEthTask)
{
    AROS_USERFUNC_INIT

    struct NepClassEth *ncp;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    UBYTE *pktptr;
    ULONG pktlen;
    UWORD cnt;
    LONG lastioerr = 0;
    ULONG errcount = 0;

    struct IOSana2Req *ioreq;

    if((ncp = nAllocEth()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
        }
        Permit();

        if(nReadEEPROMMAC(ncp, ncp->ncp_ROMAddress))
        {
            if(!(ncp->ncp_StateFlags & DDF_CONFIGURED))
            {
                // don't overwrite previously configured MAC Address
                CopyMem(ncp->ncp_ROMAddress, ncp->ncp_MacAddress, ETHER_ADDR_SIZE);
            }
        } else {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                           "Couldn't read EEPROM for MAC Address, using preset one.");
            if(!(ncp->ncp_StateFlags & DDF_CONFIGURED))
            {
                // don't overwrite previously configured MAC Address
                CopyMem(ncp->ncp_ROMAddress, ncp->ncp_MacAddress, ETHER_ADDR_SIZE);
            }
        }
        KPRINTF(10, ("MAC Address in EEPROM %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
                     ncp->ncp_MacAddress[0],
                     ncp->ncp_MacAddress[1],
                     ncp->ncp_MacAddress[2],
                     ncp->ncp_MacAddress[3],
                     ncp->ncp_MacAddress[4],
                     ncp->ncp_MacAddress[5]));

        //if((!(ncp->ncp_StateFlags & DDF_OFFLINE)))// && (ncp->ncp_StateFlags & DDF_CONFIGURED))
        {
            /* Record start time_of_day */
            //GetSysTime(&ncp->ncp_DeviceStats.LastStart);
            nSetOnline(ncp);
        }

        /* Main task */
        sigmask = (1L<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1L<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        do
        {
            // start transmitting read request if online...
            if((ncp->ncp_StateFlags & DDF_ONLINE) && (ncp->ncp_ReadPending == NULL))
            {
                ncp->ncp_ReadPending = ncp->ncp_ReadBuffer[ncp->ncp_ReadBufNum];
                psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_ReadPending, ETHER_MAX_LEN);
                ncp->ncp_ReadBufNum ^= 1;
            }
            while((pp = (struct PsdPipe *) GetMsg(ncp->ncp_TaskMsgPort)))
            {
                KPRINTF(1, ("Pipe back %08lx\n", pp));
                for(cnt = 0; cnt < 2; cnt++)
                {
                    if(pp == ncp->ncp_EPOutPipe[cnt])
                    {
                        if((ioreq = ncp->ncp_WritePending[cnt]))
                        {
                            ioerr = psdGetPipeError(pp);
                            if(ioerr)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "Eth transmit failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);

                                /* Trigger any tx or generic error events */
                                nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_TX);

                                /* Set error code and terminate the iorequest.
                                   NOTE: Can't use RC_* or deverror() this is not
                                   called from devBeginIO()!
                                */
                                ioreq->ios2_DataLength   = 0;
                                ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
                                ioreq->ios2_WireError    = S2WERR_GENERIC_ERROR;
                                psdDelayMS(50);
                            }
                            ReplyMsg((struct Message *) ioreq);
                            ncp->ncp_WritePending[cnt] = NULL;
                        }
                        break;
                    }
                }
                if(pp == ncp->ncp_EPInPipe)
                {
                    if((pktptr = ncp->ncp_ReadPending))
                    {
                        ioerr = psdGetPipeError(pp);
                        pktlen = psdGetPipeActual(pp);
                        KPRINTF(1, ("ReadBack with %ld bytes (%ld).\n", pktlen, ioerr));
                        // interleave next packet reading ASAP.
                        if(ncp->ncp_StateFlags & DDF_ONLINE)
                        {
                            ncp->ncp_ReadPending = ncp->ncp_ReadBuffer[ncp->ncp_ReadBufNum];
                            psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_ReadPending, ETHER_MAX_LEN);
                            ncp->ncp_ReadBufNum ^= 1;
                        } else {
                            ncp->ncp_ReadPending = NULL;
                        }
                        if(ioerr)
                        {
                            if(lastioerr != ioerr)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "Eth receive failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                errcount = 0;
                            } else {
                                errcount++;
                                if(errcount > 20)
                                {
                                    psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                                                   "That's it, that device pissed me off long enough!");
                                    Signal(ncp->ncp_Task, SIGBREAKF_CTRL_C);
                                }
                            }
                            lastioerr = ioerr;
                            psdDelayMS(50);
                            break;
                        } else {
                            KPRINTF(1, ("Pkt %ld received\n", pktlen));
                            DB(dumpmem(pktptr, pktlen));
                            nReadPacket(ncp, pktptr, pktlen);
                        }
                    }
                }
            }

            while((ioreq = (struct IOSana2Req *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->ios2_Req.io_Command, ioreq->ios2_DataLength));
                switch(ioreq->ios2_Req.io_Command)
                {
                    case S2_CONFIGINTERFACE:
                        //nWritePegRegs(ncp, ncp->ncp_MacAddress, ETHER_ADDR_SIZE, PEGREG_ETH_ID);
                        /* Now configured */
                        ncp->ncp_StateFlags |= DDF_CONFIGURED;
                        if(!(ncp->ncp_StateFlags & DDF_ONLINE))
                        {
                            nSetOnline(ncp);
                        }
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case S2_ADDMULTICASTADDRESS:
                    case S2_DELMULTICASTADDRESS:
                    case S2_ADDMULTICASTADDRESSES:
                    case S2_DELMULTICASTADDRESSES:
                        nUpdateRXMode(ncp);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case S2_ONLINE:
                        nSetOnline(ncp);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case S2_OFFLINE:
                        nWritePhyWord(ncp, ncp->ncp_PhyID, MII_BMCR, BMCR_PDOWN);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->ios2_Req.io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            Forbid();
            while((!ncp->ncp_WritePending[ncp->ncp_WriteBufNum]) && ncp->ncp_WriteQueue.lh_Head->ln_Succ)
            {
                ioreq = (struct IOSana2Req *) RemHead(&ncp->ncp_WriteQueue);
                Permit();
                nWritePacket(ncp, ioreq);
                Forbid();
            }
            Permit();
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));

        Forbid();
        /* Now remove all requests still pending *anywhere* */
        //ncp->ncp_DenyRequests = TRUE;
        /* Current transfers */
        for(cnt = 0; cnt < 2; cnt++)
        {
            if((ioreq = ncp->ncp_WritePending[cnt]))
            {
                KPRINTF(1, ("Aborting pending write...\n"));
                psdAbortPipe(ncp->ncp_EPOutPipe[cnt]);
                psdWaitPipe(ncp->ncp_EPOutPipe[cnt]);
                ioreq->ios2_Req.io_Error = IOERR_ABORTED;
                ReplyMsg((struct Message *) ioreq);
                ncp->ncp_WritePending[cnt] = NULL;
            }
        }
        if(ncp->ncp_ReadPending)
        {
            KPRINTF(1, ("Aborting pending read...\n"));
            psdAbortPipe(ncp->ncp_EPInPipe);
            psdWaitPipe(ncp->ncp_EPInPipe);
            ncp->ncp_ReadPending = NULL;
        }
        /* Command queue */
        /*while(ioreq = (struct IOSana2Req *) GetMsg(&ncp->ncp_Unit.unit_MsgPort))
        {
            KPRINTF(1, ("Aborting pending requests...\n"));
            ioreq->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
        }*/
        Permit();

        nDoEvent(ncp, S2EVENT_OFFLINE);

        KPRINTF(20, ("Going down the river!\n"));
        nFreeEth(ncp);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocEth()" */
struct NepClassEth * nAllocEth(void)
{
    struct Task *thistask;
    struct NepClassEth *ncp;

    thistask = FindTask(NULL);
    do
    {
        ncp = thistask->tc_UserData;
        if(!(ncp->ncp_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }

        ncp->ncp_Interface = psdFindInterface(ncp->ncp_Device, NULL,
                                              TAG_END);

        if(!ncp->ncp_Interface)
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "No interface?");
            break;
        }

        ncp->ncp_EPIn = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                        EA_IsIn, TRUE,
                                        EA_TransferType, USEAF_BULK,
                                        TAG_END);
        ncp->ncp_EPOut = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                         EA_IsIn, FALSE,
                                         EA_TransferType, USEAF_BULK,
                                         TAG_END);
        if(!(ncp->ncp_EPIn && ncp->ncp_EPOut))
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "IN or OUT endpoint missing!");
            break;
        }

        psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPOut,
                    EA_MaxPktSize, &ncp->ncp_EPOutMaxPktSize,
                    TAG_END);

        ncp->ncp_ReadPending = NULL;
        ncp->ncp_WritePending[0] = NULL;
        ncp->ncp_WritePending[1] = NULL;
        if(!(ncp->ncp_ReadBuffer[0] = AllocVec(ETHER_MAX_LEN * 4, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            KPRINTF(1, ("Out of memory for read buffer\n"));
            break;
        }
        ncp->ncp_ReadBuffer[1] = ncp->ncp_ReadBuffer[0] + ETHER_MAX_LEN;
        ncp->ncp_WriteBuffer[0] = ncp->ncp_ReadBuffer[1] + ETHER_MAX_LEN;
        ncp->ncp_WriteBuffer[1] = ncp->ncp_WriteBuffer[0] + ETHER_MAX_LEN;
        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_EP0Pipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL)))
            {
                if((ncp->ncp_EPOutPipe[0] = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPOut)))
                {
                    /* Turn off short packets */
                    psdSetAttrs(PGA_PIPE, ncp->ncp_EPOutPipe[0],
                                PPA_NoShortPackets, FALSE,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, 5000,
                                TAG_END);
                    if((ncp->ncp_EPOutPipe[1] = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPOut)))
                    {
                        /* Turn off short packets */
                        psdSetAttrs(PGA_PIPE, ncp->ncp_EPOutPipe[1],
                                    PPA_NoShortPackets, FALSE,
                                    PPA_NakTimeout, TRUE,
                                    PPA_NakTimeoutTime, 5000,
                                    TAG_END);
                        if((ncp->ncp_EPInPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPIn)))
                        {
                            /* Turn off short packets */
                            psdSetAttrs(PGA_PIPE, ncp->ncp_EPInPipe,
                                        PPA_NakTimeout, FALSE,
                                        PPA_NakTimeoutTime, 5000,
                                        PPA_AllowRuntPackets, TRUE,
                                        TAG_END);
                            ncp->ncp_Task = thistask;

                            if(nInitASIX(ncp))
                            {
                                return(ncp);
                            }
                            psdFreePipe(ncp->ncp_EPInPipe);
                        }
                        psdFreePipe(ncp->ncp_EPOutPipe[1]);
                    }
                    psdFreePipe(ncp->ncp_EPOutPipe[0]);
                }
                psdFreePipe(ncp->ncp_EP0Pipe);
            }
            DeleteMsgPort(ncp->ncp_TaskMsgPort);
        }
        FreeSignal((LONG) ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
    } while(FALSE);
    if(ncp->ncp_ReadBuffer[0])
    {
        FreeVec(ncp->ncp_ReadBuffer[0]);
        ncp->ncp_ReadBuffer[0] = NULL;
    }
    CloseLibrary(ncp->ncp_Base);
    Forbid();
    ncp->ncp_Task = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeEth()" */
void nFreeEth(struct NepClassEth *ncp)
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

    psdFreePipe(ncp->ncp_EPInPipe);
    psdFreePipe(ncp->ncp_EPOutPipe[0]);
    psdFreePipe(ncp->ncp_EPOutPipe[1]);
    psdFreePipe(ncp->ncp_EP0Pipe);

    if(ncp->ncp_ReadBuffer[0])
    {
        FreeVec(ncp->ncp_ReadBuffer[0]);
        ncp->ncp_ReadBuffer[0] = NULL;
    }

    DeleteMsgPort(ncp->ncp_TaskMsgPort);
    CloseLibrary(ncp->ncp_Base);
    Forbid();
    ncp->ncp_Task = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
    }
}
/* \\\ */

/* /// "nReadEEPROMMAC()" */
BOOL nReadEEPROMMAC(struct NepClassEth *ncp, UBYTE *macptr)
{
    LONG ioerr;

    switch(ncp->ncp_PatchFlags & 0xf)
    {
        case PF_AX88178:
        case PF_AX88772:
            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_DEVICE|URTF_VENDOR, UAXR_READ_NODE_ID, 0, 0);
            break;

        default:
            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_DEVICE|URTF_VENDOR, UAXR_READ_NODE_ID_2, 0, 0);
            break;
    }
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, macptr, 6);
    if(ioerr)
    {
        KPRINTF(10, ("Couldn't read EEPROM %ld\n", ioerr));
        return(FALSE);
    }
    return(TRUE);
}
/* \\\ */

/* /// "nReadPhyWord()" */
LONG nReadPhyWord(struct NepClassEth *ncp, ULONG phyid, ULONG phyreg)
{
    UBYTE phyword[2];
    LONG ioerr;

    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_SET_SW_MII, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error obtaining PHY control %ld\n", ioerr));
        return(-1);
    }
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_DEVICE|URTF_VENDOR, UAXR_READ_MII_REG, phyid, phyreg);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, phyword, 2);
    if(ioerr)
    {
        KPRINTF(10, ("Could not read PHY %ld reg %ld, due to %ld)!\n", phyid, phyreg, ioerr));
        return(-1);
    }
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_SET_HW_MII, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error releasing PHY control %ld\n", ioerr));
        return(-1);
    }

    return(phyword[0]|(phyword[1]<<8));
}
/* \\\ */

/* /// "nWritePhyWord()" */
BOOL nWritePhyWord(struct NepClassEth *ncp, ULONG phyid, ULONG phyreg, ULONG value)
{
    UBYTE phyword[2];
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_SET_SW_MII, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error obtaining PHY control %ld\n", ioerr));
        return(FALSE);
    }

    phyword[0] = value;
    phyword[1] = value>>8;

    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_MII_REG, phyid, phyreg);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, phyword, 2);
    if(ioerr)
    {
        KPRINTF(10, ("Could not write PHY %ld reg %ld, due to %ld)!\n", phyid, phyreg, ioerr));
        return(FALSE);
    }
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_SET_HW_MII, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error releasing PHY control %ld\n", ioerr));
        return(FALSE);
    }
    return(TRUE);
}
/* \\\ */

/* /// "nWriteRXCtrl()" */
BOOL nWriteRXCtrl(struct NepClassEth *ncp, ULONG mode)
{
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_RX_CTL, mode, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error writing RX Ctrl %ld\n", ioerr));
        return(FALSE);
    }
    return(TRUE);
}
/* \\\ */

/* /// "nWriteMediumMode()" */
BOOL nWriteMediumMode(struct NepClassEth *ncp, ULONG mode)
{
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_MEDIUM_MODE, mode, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error writing Medium Mode %ld\n", ioerr));
        return(FALSE);
    }
    return(TRUE);
}
/* \\\ */

/* /// "nSWReset()" */
BOOL nSWReset(struct NepClassEth *ncp, ULONG mode)
{
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_SW_RESET, mode, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error writing SW RESET %ld\n", ioerr));
        return(FALSE);
    }
    psdDelayMS(150);
    return(TRUE);
}
/* \\\ */

/* /// "nWriteGPIOs()" */
BOOL nWriteGPIOs(struct NepClassEth *ncp, ULONG value, ULONG delay)
{
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_GPIOS, value, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Error writing GPIO %ld\n", ioerr));
        return(FALSE);
    }
    psdDelayMS(delay);
    return(TRUE);
}
/* \\\ */

/* /// "nGetPhyAddr()" */
UWORD nGetPhyAddr(struct NepClassEth *ncp)
{
    UBYTE phyid[2];
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_DEVICE|URTF_VENDOR, UAXR_READ_PHY_ID, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, phyid, 2);
    if(ioerr)
    {
        KPRINTF(10, ("Error obtaining PHY ADDR %ld\n", ioerr));
        return(0);
    }
    return(phyid[1]);
}
/* \\\ */

/* /// "nInitASIX()" */
BOOL nInitASIX(struct NepClassEth *ncp)
{
    LONG data;
    LONG ioerr;
    UWORD embeddedphy;
    BOOL result = TRUE;
    UBYTE buf[2];

    switch(ncp->ncp_PatchFlags & 0xf)
    {
        case PF_AX88178:
            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "This adapter uses the AX88178 chipset. This code is untested! Please report, if it works!");
            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_ENABLE, 0, 0);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                KPRINTF(10, ("Error writing enable %ld\n", ioerr));
                return(FALSE);
            }

            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_DEVICE|URTF_VENDOR, UAXR_READ_EEPROM, 0x0017, 0);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, buf, 2);
            if(ioerr)
            {
                KPRINTF(10, ("Error read eeprom %ld\n", ioerr));
                return(FALSE);
            }

            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_DISABLE, 0, 0);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                KPRINTF(10, ("Error writing disable %ld\n", ioerr));
                return(FALSE);
            }
            if((buf[0] != 0xff) || (buf[1] != 0xff))
            {
                ncp->ncp_PhyMode = buf[0] & 7;
                ncp->ncp_LedMode = buf[1];
            }
            result &= nWriteGPIOs(ncp, AX_GPIO_RSE|AX_GPIO_GPO_1|AX_GPIO_GPO1EN, 40);
            if(buf[1] != 0x01)
            {
                result &= nWriteGPIOs(ncp, AX_GPIO_GPO_1|AX_GPIO_GPO1EN|AX_GPIO_GPO_2|AX_GPIO_GPO2EN, 30);
                result &= nWriteGPIOs(ncp, AX_GPIO_GPO_1|AX_GPIO_GPO1EN|AX_GPIO_GPO2EN, 300);
                result &= nWriteGPIOs(ncp, AX_GPIO_GPO_1|AX_GPIO_GPO1EN|AX_GPIO_GPO_2|AX_GPIO_GPO2EN, 30);
            } else {
                result &= nWriteGPIOs(ncp, AX_GPIO_GPO1EN, 30);
                result &= nWriteGPIOs(ncp, AX_GPIO_GPO_1|AX_GPIO_GPO1EN, 30);
            }

            result &= nSWReset(ncp, AX_SWRESET_CLEAR);
            psdDelayMS(150);
            result &= nSWReset(ncp, AX_SWRESET_PRL|AX_SWRESET_IPPD);
            psdDelayMS(150);
            result &= nWriteRXCtrl(ncp, 0x0000);

            ncp->ncp_PhyID = nGetPhyAddr(ncp);
            if(ncp->ncp_PhyMode == PHY_MODE_MARVELL)
            {
                nWritePhyWord(ncp, ncp->ncp_PhyID, MII_MARVELL_CTRL, MARVELL_CTRL_RXDELAY|MARVELL_CTRL_TXDELAY);
                if(ncp->ncp_LedMode)
                {
                    ULONG reg;
                    reg = nReadPhyWord(ncp, ncp->ncp_PhyID, MII_MARVELL_LED_CTRL);
                    reg &= 0xf8ff;
                    reg |= 0x0101;
                    nWritePhyWord(ncp, ncp->ncp_PhyID, MII_MARVELL_LED_CTRL, reg);
                }

                psdDelayMS(60);
            }

            result &= nWritePhyWord(ncp, ncp->ncp_PhyID, MII_BMCR, BMCR_RESET|BMCR_ANENABLE);
            result &= nWritePhyWord(ncp, ncp->ncp_PhyID, MII_ADVERTISE, ADVERTISE_ALL|ADVERTISE_CSMA|ADVERTISE_PAUSE_CAP);
            result &= nWritePhyWord(ncp, ncp->ncp_PhyID, MII_CTRL1000, ADVERTISE_1000FULL|ADVERTISE_1000HALF);

            result &= nWriteMediumMode(ncp, AX88178_MEDIUM_DEFAULT);

            result &= nWriteRXCtrl(ncp, AX_DEFAULT_RX_CTL);
            // RX Size == 2048
            break;

        case PF_AX88772:
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "This adapter uses the AX88772 chipset.");
            result &= nWriteGPIOs(ncp, AX_GPIO_RSE|AX_GPIO_GPO_2|AX_GPIO_GPO2EN, 5);

            ncp->ncp_PhyID = nGetPhyAddr(ncp);
            embeddedphy = ((ncp->ncp_PhyID & 0x1f) == 0x10) ? 1 : 0;

            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_SW_PHY_SELECT, (ULONG) embeddedphy, 0);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                KPRINTF(10, ("Error selecting PHY %ld\n", ioerr));
                return(FALSE);
            }
            result &= nSWReset(ncp, AX_SWRESET_IPPD|AX_SWRESET_PRL);
            psdDelayMS(150);
            result &= nSWReset(ncp, AX_SWRESET_CLEAR);
            psdDelayMS(150);
            result &= nSWReset(ncp, (ULONG) (embeddedphy ? AX_SWRESET_IPRL : AX_SWRESET_PRTE));
            psdDelayMS(150);
            result &= nWriteRXCtrl(ncp, 0x0000);

            ncp->ncp_PhyID = nGetPhyAddr(ncp);

            result &= nSWReset(ncp, AX_SWRESET_PRL);
            psdDelayMS(150);
            result &= nSWReset(ncp, AX_SWRESET_IPRL|AX_SWRESET_PRL);
            psdDelayMS(150);

            result &= nWritePhyWord(ncp, ncp->ncp_PhyID, MII_BMCR, BMCR_RESET);
            result &= nWritePhyWord(ncp, ncp->ncp_PhyID, MII_ADVERTISE, ADVERTISE_ALL|ADVERTISE_CSMA|ADVERTISE_PAUSE_CAP);

            result &= nWriteMediumMode(ncp, AX88772_MEDIUM_DEFAULT);

            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_IPG0, AX88772_IPG0_DEFAULT|(AX88772_IPG1_DEFAULT<<8), AX88772_IPG2_DEFAULT);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                KPRINTF(10, ("Error writing IPG0-2 %ld\n", ioerr));
                return(FALSE);
            }

            result &= nWriteRXCtrl(ncp, AX_DEFAULT_RX_CTL);
            // RX Size == 2048

            break;

        default: // AX88172
            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "This adapter uses the AX88172 chipset.");
            data = 0x130103;
            if(ncp->ncp_PatchFlags & PF_DLINKGPIO)
            {
                data = 0x9f9d9f;
            }
            if(ncp->ncp_PatchFlags & PF_HAWKINGGPIO)
            {
                data = 0x1f1d1f;
            }

            result &= nWriteGPIOs(ncp, (data>>16) & 0xff, 5);
            result &= nWriteGPIOs(ncp, (data>>8) & 0xff, 5);
            result &= nWriteGPIOs(ncp, data & 0xff, 5);

            result &= nWriteRXCtrl(ncp, 0x80);
            ncp->ncp_PhyID = nGetPhyAddr(ncp);

            result &= nWritePhyWord(ncp, ncp->ncp_PhyID, MII_BMCR, BMCR_RESET);
            result &= nWritePhyWord(ncp, ncp->ncp_PhyID, MII_ADVERTISE, ADVERTISE_ALL|ADVERTISE_CSMA|ADVERTISE_PAUSE_CAP);
            break;
    }
    if(!result)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Failed to initialize adapter!");
    }

    return(result);
}
/* \\\ */

/* /// "nUpdateRXMode()" */
void nUpdateRXMode(struct NepClassEth *ncp)
{
    ULONG rxmode;
    LONG ioerr;
    switch(ncp->ncp_PatchFlags & 0xf)
    {
        case PF_AX88178:
        case PF_AX88772:
            rxmode = AX_DEFAULT_RX_CTL;
            if(ncp->ncp_OpenFlags & SANA2OPF_PROM)
            {
                rxmode |= AX_RX_CTL_PRO;
            }
            if(ncp->ncp_Multicasts.lh_Head->ln_Succ)
            {
                rxmode |= AX_RX_CTL_AM;
            }
            break;

        default:
            rxmode = 0x8c;
            if(ncp->ncp_OpenFlags & SANA2OPF_PROM)
            {
                rxmode |= 0x01;
            }
            if(ncp->ncp_Multicasts.lh_Head->ln_Succ)
            {
                rxmode |= 0x10;
            }
            break;
    }
    if(ncp->ncp_Multicasts.lh_Head->ln_Succ)
    {
        psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_DEVICE|URTF_VENDOR, UAXR_WRITE_MULTI_FILTER, 0, 0);
        ioerr = psdDoPipe(ncp->ncp_EP0Pipe, ncp->ncp_MulticastArray, 8);
        if(ioerr)
        {
            KPRINTF(10, ("Error writing multicast filter %ld\n", ioerr));
        }
    }
    nWriteRXCtrl(ncp, rxmode);
}
/* \\\ */

/* /// "MediaSpeeds" */
static UWORD nAX88178Speeds[7] =
{
    AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE,
                              AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE,
                 AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE,
    AX_MEDIUM_PS|             AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE,
    AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE,
    AX_MEDIUM_PS|             AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE|AX_MEDIUM_GM|AX_MEDIUM_ENCK,
    AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE|AX_MEDIUM_GM|AX_MEDIUM_ENCK
};

static UWORD nAX88772Speeds[7] =
{
    AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE,
                              AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE,
                 AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE,
    AX_MEDIUM_PS|             AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE,
    AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE,
    AX_MEDIUM_PS|             AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE,
    AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE,
};

static UWORD nAX88172Speeds[7] =
{
    AX88172_MEDIUM_FD|AX88172_MEDIUM_FC|AX88172_MEDIUM_TX,
                      AX88172_MEDIUM_FC|AX88172_MEDIUM_TX,
    AX88172_MEDIUM_FD|AX88172_MEDIUM_FC|AX88172_MEDIUM_TX,
                      AX88172_MEDIUM_FC|AX88172_MEDIUM_TX,
    AX88172_MEDIUM_FD|AX88172_MEDIUM_FC|AX88172_MEDIUM_TX,
                      AX88172_MEDIUM_FC|AX88172_MEDIUM_TX,
    AX88172_MEDIUM_FD|AX88172_MEDIUM_FC|AX88172_MEDIUM_TX,
};
/* \\\ */

/* /// "nSetOnline()" */
void nSetOnline(struct NepClassEth *ncp)
{
    LONG data;
    LONG data2;
    ULONG bmcr;
    BOOL autoneg = FALSE;
    BOOL linkgood = FALSE;
    UWORD timeout = 60;
    ULONG mediummode;
    UWORD *mediatable;

    switch(ncp->ncp_PatchFlags & 0xf)
    {
        case PF_AX88178:
            mediatable = nAX88178Speeds;
            break;

        case PF_AX88772:
            mediatable = nAX88772Speeds;
            break;

        default:
            mediatable = nAX88172Speeds;
            break;
    }
    mediummode = mediatable[ncp->ncp_CDC->cdc_MediaType];

    switch(ncp->ncp_CDC->cdc_MediaType)
    {
        case MT_AUTO:
            KPRINTF(10, ("Autonegotiaton!\n"));
            bmcr = BMCR_ANENABLE|BMCR_ANRESTART;
            autoneg = TRUE;
            break;

        case MT_10BASE_T_HALF_DUP:
            bmcr = 0;
            //mediummode = 0;
            break;

        case MT_10BASE_T_FULL_DUP:
            bmcr = BMCR_FULLDPLX;
            break;

        case MT_100BASE_TX_HALF_DUP:
            bmcr = BMCR_SPEED1000;
            break;

        case MT_1000BASE_TX_HALF_DUP:
            bmcr = BMCR_SPEED100;
            break;

        case MT_1000BASE_TX_FULL_DUP:
            bmcr = BMCR_SPEED1000|BMCR_FULLDPLX;
            break;

        case MT_100BASE_TX_FULL_DUP:
        default:
            bmcr = BMCR_SPEED100|BMCR_FULLDPLX;
            mediummode = mediatable[MT_100BASE_TX_FULL_DUP];
            break;
    }

    nWritePhyWord(ncp, ncp->ncp_PhyID, MII_BMCR, bmcr);
    while(--timeout)
    {
        psdDelayMS(50);
        data = nReadPhyWord(ncp, ncp->ncp_PhyID, MII_BMSR);
        KPRINTF(10, ("Status: %04lx\n", data));
        if(data < 0)
        {
            timeout = 0;
            break;
        }

        linkgood = (data & BMSR_LSTATUS);
        if((!autoneg) && linkgood)
        {
            // no need to wait for autonegotiation
            break;
        }

        // complete?
        if(data & BMSR_ANEGCOMPLETE)
        {
            break;
        }
    }

    if(autoneg)
    {
        STRPTR negstr = NULL;
        if(timeout)
        {
            KPRINTF(10, ("Auto neg successful!\n"));
            data = nReadPhyWord(ncp, ncp->ncp_PhyID, MII_LPA);
            KPRINTF(10, ("Auto neg state %04lx\n", data));
            if(data < 0)
            {
                timeout = 0;
            }
            if(ncp->ncp_PatchFlags & PF_AX88178)
            {
                data2 = nReadPhyWord(ncp, ncp->ncp_PhyID, MII_STAT1000);
                if(data2 & ADVERTISE_1000FULL)
                {
                    negstr = MediaTypeStrings[MT_1000BASE_TX_FULL_DUP];
                    mediummode = mediatable[MT_1000BASE_TX_FULL_DUP];
                }
                else if(data2 & ADVERTISE_1000HALF)
                {
                    negstr = MediaTypeStrings[MT_1000BASE_TX_HALF_DUP];
                    mediummode = mediatable[MT_1000BASE_TX_HALF_DUP];
                }
            }
            if(!negstr)
            {
                if(data & ADVERTISE_100FULL)
                {
                    // 100Base-TX Full Duplex
                    negstr = MediaTypeStrings[MT_100BASE_TX_FULL_DUP];
                    mediummode = mediatable[MT_100BASE_TX_FULL_DUP];
                }
                else if(data & ADVERTISE_100HALF)
                {
                    // 100Base-TX Half Duplex
                    negstr = MediaTypeStrings[MT_100BASE_TX_HALF_DUP];
                    mediummode = mediatable[MT_100BASE_TX_HALF_DUP];
                }
                else if(data & ADVERTISE_10FULL)
                {
                    // 10Base-T Full Duplex
                    negstr = MediaTypeStrings[MT_10BASE_T_FULL_DUP];
                    mediummode = mediatable[MT_10BASE_T_FULL_DUP];
                }
                else if(data & ADVERTISE_10HALF)
                {
                    // 10Base-T Half Duplex
                    negstr = MediaTypeStrings[MT_10BASE_T_HALF_DUP];
                    mediummode = mediatable[MT_10BASE_T_HALF_DUP];
                } else {
                    timeout = 0;
                }
            }
        }
        if(!timeout)
        {
            negstr = MediaTypeStrings[MT_100BASE_TX_FULL_DUP];
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Autonegotiation failed! Using %s instead.", negstr);
            nWritePhyWord(ncp, ncp->ncp_PhyID, MII_BMCR, BMCR_SPEED100|BMCR_FULLDPLX);
            mediummode = mediatable[MT_100BASE_TX_FULL_DUP];
        } else {
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Autonegotiation: Using %s.", negstr);
        }
    }
    if(!linkgood)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "No link, check the plug and cable.");
    }

    nWriteMediumMode(ncp, mediummode);
    nUpdateRXMode(ncp);

    /* Now online */
    ncp->ncp_StateFlags |= DDF_ONLINE;
    ncp->ncp_StateFlags &= ~DDF_OFFLINE;
    /* Trigger any ONLINE events */
    nDoEvent(ncp, S2EVENT_ONLINE);
}
/* \\\ */

/* /// "nDoEvent()" */
void nDoEvent(struct NepClassEth *ncp, ULONG events)
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

/* /// "nWritePacket()" */
BOOL nWritePacket(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    ULONG packettype;
    struct EtherPacketHeader *eph;
    UBYTE *copydest;
    UWORD writelen;
    struct BufMan *bufman;
    struct Sana2PacketTypeStats *stats;
    UBYTE *buf = ncp->ncp_WriteBuffer[ncp->ncp_WriteBufNum];

    packettype = ioreq->ios2_PacketType;
    writelen   = ioreq->ios2_DataLength;
    bufman     = ioreq->ios2_BufferManagement;

    if(ncp->ncp_PatchFlags & (PF_AX88178|PF_AX88772))
    {
        // the first four bytes are the length
        eph        = (struct EtherPacketHeader *) &buf[4];
        copydest   = buf + 4;
    } else {
        eph        = (struct EtherPacketHeader *) buf;
        copydest   = buf;
    }

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
        eph->eph_Type = AROS_WORD2BE(packettype);

        /* Packet data is at txbuffer */
        copydest += sizeof(struct EtherPacketHeader);
        writelen += sizeof(struct EtherPacketHeader);
    }

    //memset(buf + 2, 0x55, writelen);

    /* Dma not available, fallback to regular copy */
    if(callcopy(bufman->bm_CopyFromBuf, copydest, ioreq->ios2_Data, ioreq->ios2_DataLength) == NULL)
    {
        KPRINTF(10, ("writepacket: copyfrom returned failure!\n"));

        /* Trigger any tx, buff or generic error events */
        nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_TX|S2EVENT_BUFF);

        /* Set error code and terminate the iorequest.
           NOTE: Can't use RC_* or deverror() this is not
           called from devBeginIO()! */
        ioreq->ios2_DataLength   = 0;
        ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        ioreq->ios2_WireError    = S2WERR_BUFF_ERROR;
        return FALSE;
    }

    if(ncp->ncp_PatchFlags & (PF_AX88178|PF_AX88772))
    {
        buf[0] = writelen;
        buf[1] = writelen>>8;
        buf[2] = ~buf[0];
        buf[3] = ~buf[1];
        /* Adjust writelen to legal packet size. */
        if(writelen < ETHER_MIN_LEN)
        {
            memset(buf + writelen + 4, 0, ETHER_MIN_LEN - writelen);
            writelen = ETHER_MIN_LEN;
        }
        writelen += 4;
    } else {
        /* Adjust writelen to legal packet size. */
        if(writelen < ETHER_MIN_LEN)
        {
            memset(buf + writelen, 0, ETHER_MIN_LEN - writelen);
            writelen = ETHER_MIN_LEN;
        }
        if(!(writelen & (ncp->ncp_EPOutMaxPktSize-1)))
        {
            // adjust size to send one extra byte as zero packet termination doesn't seem to work with this adapter
            buf[writelen++] = 0;
            KPRINTF(10, ("Zero Packet adjustment\n"));
        }
    }

    KPRINTF(20, ("PktOut[%ld] %ld\n", ncp->ncp_WriteBufNum, writelen));

    ncp->ncp_WritePending[ncp->ncp_WriteBufNum] = ioreq;
    psdSendPipe(ncp->ncp_EPOutPipe[ncp->ncp_WriteBufNum], buf, (ULONG) writelen);
    ncp->ncp_WriteBufNum ^= 1;

    DB(
        if(AROS_BE2WORD(eph->eph_Type) < ETHERPKT_SIZE)
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

/* /// "nReadIOReq()" */
UWORD nReadIOReq(struct NepClassEth *ncp, struct EtherPacketHeader *eph, UWORD datasize, struct IOSana2Req *ioreq, UWORD flags)
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
        const UBYTE broadcast[ETHER_ADDR_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        if (memcmp(ioreq->ios2_DstAddr, broadcast, ETHER_ADDR_SIZE) == 0)
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
        nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_RX|S2EVENT_BUFF);

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

/* /// "nReadPacket()" */
BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG len)
{
    struct EtherPacketHeader *eph;
    struct BufMan *bufman;
    struct IOSana2Req *worknode, *nextnode;
    struct Sana2PacketTypeStats *stats;
    UWORD flags;
    UWORD datasize;
    UWORD pktlen;
    BOOL ret = FALSE;

    KPRINTF(20, ("PktIn [%ld] %ld\n", ncp->ncp_ReadBufNum, len));

    // loop over packet buffer for multiple packets on AX88178 and AX88772
    for(; len; len -= pktlen, pktptr += pktlen)
    {
        if(ncp->ncp_PatchFlags & (PF_AX88178|PF_AX88772))
        {
            if(len < 4)
            {
                ncp->ncp_DeviceStats.BadData++;
                return FALSE;
            }
            if((pktptr[0] + pktptr[2] != 0xff) || (pktptr[1] + pktptr[3] != 0xff))
            {
                ncp->ncp_DeviceStats.BadData++;
                KPRINTF(20, ("InverseLength mismatch\n"));
                return FALSE;
            }
            pktlen = *pktptr++;
            pktlen |= (*pktptr++)<<8;
            pktptr += 2;
            len -= 4;
            if(len < ((pktlen + 1) & ~1))
            {
                KPRINTF(20, ("Pktlen %ld < len %ld\n", pktlen, len));
                return FALSE;
            }
        } else {
            pktlen = len;
        }
        ncp->ncp_DeviceStats.PacketsReceived++;

        eph = (struct EtherPacketHeader *) pktptr;
        stats = FindPacketTypeStats(ncp, (ULONG) AROS_BE2WORD(eph->eph_Type));
        flags = DROPPED|PACKETFILTER;

        /* Calculate size of the actual data */
        datasize = pktlen - sizeof(struct EtherPacketHeader);

        /* Is the packet datasize valid? */
        if((pktlen >= ETHER_MIN_LEN) && (pktlen <= ETHER_MAX_LEN))
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
                       ((AROS_BE2WORD(eph->eph_Type) < ETHERPKT_SIZE) && (worknode->ios2_PacketType < ETHERPKT_SIZE)))
                    {
                        flags = nReadIOReq(ncp, eph, datasize, worknode, flags);
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
                    nReadIOReq(ncp, eph, datasize, worknode, 0);
                    worknode = nextnode;
                }
                Permit();
            } else {
                /* Packet not dropped - return ok */
                ret = TRUE;
                continue;
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
        nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_RX);

        if(!(ncp->ncp_PatchFlags & (PF_AX88178|PF_AX88772)))
        {
            // other chipsets don't support multiple packets in one transfer
            break;
        }
    }
    return ret;
}
/* \\\ */

/**************************************************************************/

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepEthBase *nh;
    struct NepClassEth *ncp;
    struct PsdIFFContext *pic;

    thistask = FindTask(NULL);
#undef ps
#define ps ncp->ncp_PsdBase
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
        nGUITaskCleanup(ncp);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        nGUITaskCleanup(ncp);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        nGUITaskCleanup(ncp);
        return;
    }

    ncp->ncp_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)libname,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)"©2007-2009 Harry Sintonen & Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Harry Sintonen <sintonen@iki.fi> & Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the asixeth.class",
        MUIA_Application_Base       , (IPTR)"ASIXETH",
        MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
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
                Child, (IPTR)(MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End),
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
                Child, (IPTR)ColGroup(2), GroupFrameT((IPTR)(ncp->ncp_Interface ? "Device Settings" : "Default Device Settings")),
                    //Child, HSpace(0),
                    Child, (IPTR)Label((IPTR) "Media Type:"),
                    Child, (IPTR)(ncp->ncp_MediaTypeObj = CycleObject,
                        MUIA_CycleChain, 1,
                        MUIA_Cycle_Entries, (IPTR)MediaTypeStrings,
                        MUIA_Cycle_Active, ncp->ncp_CDC->cdc_MediaType,
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
        nGUITaskCleanup(ncp);
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
            nGUITaskCleanup(ncp);
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
                    get(ncp->ncp_UnitObj, MUIA_String_Integer, &ncp->ncp_CDC->cdc_DefaultUnit);
                    get(ncp->ncp_MediaTypeObj, MUIA_Cycle_Active, &ncp->ncp_CDC->cdc_MediaType);

                    if(retid == ID_DEF_CONFIG)
                    {
                        pic = psdGetClsCfg(libname);
                        if(!pic)
                        {
                            psdSetClsCfg(libname, NULL);
                            pic = psdGetClsCfg(libname);
                        }
                        if(pic)
                        {
                            psdAddCfgEntry(pic, ncp->ncp_CDC);
                            psdSaveCfgToDisk(NULL, FALSE);
                        }
                    }
                    if(ncp->ncp_Interface)
                    {
                        pic = psdGetUsbDevCfg(libname, ncp->ncp_DevIDString, NULL);
                        if(!pic)
                        {
                            psdSetUsbDevCfg(libname, ncp->ncp_DevIDString, NULL, NULL);
                            pic = psdGetUsbDevCfg(libname, ncp->ncp_DevIDString, NULL);
                        }
                        if(pic)
                        {
                            if(psdAddCfgEntry(pic, ncp->ncp_CDC))
                            {
                                if(retid != MUIV_Application_ReturnID_Quit)
                                {
                                    psdSaveCfgToDisk(NULL, FALSE);
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
    nGUITaskCleanup(ncp);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void nGUITaskCleanup(struct NepClassEth *ncp)
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
    if(ps)
    {
        CloseLibrary(ps);
        ps = NULL;
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
