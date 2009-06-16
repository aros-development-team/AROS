/*
 *----------------------------------------------------------------------------
 *                         bluetooth class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "bluetooth.class.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static
const APTR DevFuncTable[] =
{
    &AROS_SLIB_ENTRY(devOpen, dev),
    &AROS_SLIB_ENTRY(devClose, dev),
    &AROS_SLIB_ENTRY(devExpunge, dev),
    &AROS_SLIB_ENTRY(devReserved, dev),
    &AROS_SLIB_ENTRY(devBeginIO, dev),
    &AROS_SLIB_ENTRY(devAbortIO, dev),
    (APTR) -1,
};

static int libInit(LIBBASETYPEPTR nh)
{
    struct NepClassBT *ncp;
    struct NepBTBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);
        nh->nh_MemPool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR, 32*1024, sizeof(struct BTHCIEventMsg));
        if(nh->nh_MemPool)
        {
            if((nh->nh_DevBase = (struct NepBTDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
               sizeof(struct NepBTDevBase), NULL)))
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
                DeletePool(nh->nh_MemPool);
                KPRINTF(20, ("failed to create usbbluetooth.device\n"));
            }
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
    struct NepClassBT *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);

        ncp = (struct NepClassBT *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp->ncp_CDC);
            FreeVec(ncp);
            ncp = (struct NepClassBT *) nh->nh_Units.lh_Head;
        }
        if(nh->nh_MemPool)
        {
            DeletePool(nh->nh_MemPool);
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);
        KPRINTF(5, ("libExpunge: Unloading done! bluetooth.class expunged!\n\n"));
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

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassBT * usbAttemptInterfaceBinding(struct NepBTBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    KPRINTF(1, ("nepBTAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    TAG_DONE);
        CloseLibrary(ps);
        if((ifclass == BLUETOOTH_CLASSCODE) &&
           (subclass == BLUETOOTH_RF_SUBCLASS) &&
           (proto == BLUETOOTH_PROTO_PRG))
        {
             APTR *aclin;
             APTR *aclout;
             APTR *eventint;
             aclin = psdFindEndpoint(pif, NULL,
                                     EA_IsIn, TRUE,
                                     EA_TransferType, USEAF_BULK,
                                     TAG_END);
             aclout = psdFindEndpoint(pif, NULL,
                                      EA_IsIn, FALSE,
                                      EA_TransferType, USEAF_BULK,
                                      TAG_END);
             eventint = psdFindEndpoint(pif, NULL,
                                        EA_IsIn, TRUE,
                                        EA_TransferType, USEAF_INTERRUPT,
                                        TAG_END);
            if(aclin && aclout && eventint)
            {
                return(usbForceInterfaceBinding(nh, pif));
            }
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassBT * usbForceInterfaceBinding(struct NepBTBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassBT *ncp;
    struct NepClassBT *tmpncp;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    struct ClsDevCfg *cdc;
    STRPTR devname;
    STRPTR ifidstr;
    STRPTR devidstr;
    IPTR altifnum;
    IPTR ifnum;
    IPTR cfgnum;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    BOOL  unitfound;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepBTAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_InterfaceNum, &ifnum,
                    IFA_AlternateNum, &altifnum,
                    IFA_IDString, &ifidstr,
                    IFA_Config, &pc,
                    TAG_DONE);
        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    CA_ConfigNum, &cfgnum,
                    TAG_END);
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductID, &prodid,
                    DA_VendorID, &vendid,
                    DA_ProductName, &devname,
                    DA_IDString, &devidstr,
                    TAG_END);
        Forbid();
        unitfound = FALSE;
        unitno = (ULONG) -1;
        ncp = (struct NepClassBT *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if((ncp->ncp_UnitAltIfNum == altifnum) && (ncp->ncp_UnitIfNum == ifnum) &&
               (ncp->ncp_UnitProdID == prodid) && (ncp->ncp_UnitVendorID == vendid))
            {
                unitno = ncp->ncp_UnitNo;
                unitfound = TRUE;
                break;
            }
            ncp = (struct NepClassBT *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(ncp = AllocVec(sizeof(struct NepClassBT), MEMF_PUBLIC|MEMF_CLEAR)))
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
            NewList(&ncp->ncp_ReadQueue);
            NewList(&ncp->ncp_WriteQueue);
            AddTail(&nh->nh_Units, &ncp->ncp_Unit.unit_MsgPort.mp_Node);
        }
        ncp->ncp_ClsBase = nh;
        ncp->ncp_Interface = pif;
        ncp->ncp_Device = pd;
        ncp->ncp_Config = pc;
        ncp->ncp_UnitAltIfNum = altifnum;
        ncp->ncp_UnitIfNum = ifnum;
        ncp->ncp_UnitCfgNum = cfgnum;
        ncp->ncp_UnitProdID = prodid;
        ncp->ncp_UnitVendorID = vendid;
        ncp->ncp_DevIDString = devidstr;
        ncp->ncp_IfIDString = ifidstr;

        /* try to load default config */
        nLoadBindingConfig(ncp);

        /* Find next free unit number */
        if(unitno == (ULONG) -1)
        {
            unitno = ncp->ncp_CDC->cdc_DefaultUnit;
            tmpncp = (struct NepClassBT *) nh->nh_Units.lh_Head;
            while(tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                if(tmpncp->ncp_UnitNo == unitno)
                {
                    unitno++;
                    tmpncp = (struct NepClassBT *) nh->nh_Units.lh_Head;
                } else {
                    tmpncp = (struct NepClassBT *) tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
                }
            }
        }
        ncp->ncp_UnitNo = unitno;
        Permit();

        psdSafeRawDoFmt(buf, 64, "bluetooth.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nBTTask, ncp)))
        {
            psdBorrowLocksWait(tmptask, 1UL<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                //FreeSignal(ncp->ncp_ReadySignal);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "I've got blue teeth with '%s' through %s unit %ld!",
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

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepBTBase *nh, struct NepClassBT *ncp)
{
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("nepBTReleaseInterfaceBinding(%08lx)\n", ncp));
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
            Wait(1UL<<ncp->ncp_ReadySignal);
        }
        //FreeSignal(ncp->ncp_ReadySignal);
        psdGetAttrs(PGA_DEVICE, ncp->ncp_Device, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "'%s' lost all its teeth.",
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

    KPRINTF(1, ("nepBTGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "Bluetooth HCI via usbbluetooth.device";
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
                 *((IPTR *) ti->ti_Data) = ((struct NepClassBT *) usbstruct)->ncp_UsingDefaultCfg;
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

    struct NepClassBT *ncp;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassBT *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenBindingCfgWindow(nh, &nh->nh_DummyNCP));

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassBT *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            Forbid();
            ncp = (struct NepClassBT *) nh->nh_Units.lh_Head;
            while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                nLoadBindingConfig(ncp);
                ncp = (struct NepClassBT *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
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
BOOL nLoadClassConfig(struct NepBTBase *nh)
{
    struct NepClassBT *ncp = &nh->nh_DummyNCP;
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
    ncp->ncp_CDC->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('B','T','H','P'));
    ncp->ncp_CDC->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    ncp->ncp_CDC->cdc_DefaultUnit = 0;
    ncp->ncp_CDC->cdc_StackAuto = TRUE;
    ncp->ncp_UsingDefaultCfg = TRUE;
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
BOOL nLoadBindingConfig(struct NepClassBT *ncp)
{
    struct NepBTBase *nh = ncp->ncp_ClsBase;
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
    pic = psdGetUsbDevCfg(libname, ncp->ncp_DevIDString, ncp->ncp_IfIDString);
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
LONG nOpenBindingCfgWindow(struct NepBTBase *nh, struct NepClassBT *ncp)
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

/* /// "nIssueEventReq()" */
void nIssueEventReq(struct NepClassBT *ncp)
{
     struct BTHCIEventMsg *bem;
     bem = (struct BTHCIEventMsg *) GetMsg(ncp->ncp_EventReplyPort);
     if(!bem)
     {
         bem = psdAllocVec(sizeof(struct BTHCIEventMsg));
         if(!bem)
         {
             return;
         }
         bem->bem_Msg.mn_ReplyPort = ncp->ncp_EventReplyPort;
     } else {
         ncp->ncp_EventsPending--;
     }
     ncp->ncp_CurrEventMsg = bem;
     bem->bem_Msg.mn_Length = 0;
     psdSendPipe(ncp->ncp_EPEventIntPipe, &bem->bem_Event, ncp->ncp_EPEventIntMaxPktSize);
}
/* \\\ */

/* /// "nBTTask()" */
AROS_UFH0(void, nBTTask)
{
    AROS_USERFUNC_INIT

    struct NepClassBT *ncp;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    struct IOBTHCIReq *ioreq;
    struct IOBTHCIReq *ioreq2;
    struct BTHCIEventMsg *bem;
    ULONG len;

    if((ncp = nAllocBT()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1UL<<ncp->ncp_ReadySignal);
        }
        Permit();

        /* Main task */
        sigmask = (1UL<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1UL<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        KPRINTF(10, ("Unit Sigbit %ld\n", ncp->ncp_Unit.unit_MsgPort.mp_SigBit));
        KPRINTF(10, ("Task Sigbit %ld\n", ncp->ncp_TaskMsgPort->mp_SigBit));
        do
        {
            if(!ncp->ncp_CurrEventMsg)
            {
                nIssueEventReq(ncp);
            }
            if(ncp->ncp_AbortRead)
            {
                ncp->ncp_AbortRead = FALSE;
                if(ncp->ncp_ReadPending)
                {
                    psdAbortPipe(ncp->ncp_EPACLInPipe);
                }
            }
            if(ncp->ncp_AbortWrite)
            {
                ncp->ncp_AbortWrite = FALSE;
                if(ncp->ncp_WritePending)
                {
                    psdAbortPipe(ncp->ncp_EPACLOutPipe);
                }
            }
            while((pp = (struct PsdPipe *) GetMsg(ncp->ncp_TaskMsgPort)))
            {
                KPRINTF(1, ("Pipe back %08lx\n", pp));
                if(pp == ncp->ncp_EPEventIntPipe)
                {
                    ioerr = psdGetPipeError(pp);
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "BT Event reception failed: %s (%ld)",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    }
                    if((bem = ncp->ncp_CurrEventMsg))
                    {
                        len = psdGetPipeActual(pp);
                        bem->bem_Msg.mn_Length += len;
                        if(bem->bem_Msg.mn_Length >= 2)
                        {
                            if(bem->bem_Msg.mn_Length < bem->bem_Event.bhe_PayloadLength+2)
                            {
                                // actually, this should not have been a short packet.
                                if(len % ncp->ncp_EPEventIntMaxPktSize)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                   "BT Event short packet (%ld < %ld)",
                                                   bem->bem_Msg.mn_Length, bem->bem_Event.bhe_PayloadLength+2);
                                }
                                // read remaining packet
                                psdSendPipe(ncp->ncp_EPEventIntPipe,
                                            (((UBYTE *) &bem->bem_Event.bhe_EventType) + bem->bem_Msg.mn_Length),
                                            (ULONG) bem->bem_Event.bhe_PayloadLength+2 - bem->bem_Msg.mn_Length);
                            } else {
                                if(bem->bem_Msg.mn_Length > bem->bem_Event.bhe_PayloadLength+2)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                   "BT Event packet overflow (%ld > %ld)",
                                                   bem->bem_Msg.mn_Length, bem->bem_Event.bhe_PayloadLength+2);
                                }
                                /*psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                               "BT Event ready (%02lx: %ld)",
                                               bem->bem_Event.bhe_EventType, bem->bem_Event.bhe_PayloadLength);*/
                                if(ncp->ncp_EventMsgPort)
                                {
                                    PutMsg(ncp->ncp_EventMsgPort, &bem->bem_Msg);
                                } else {
                                    PutMsg(ncp->ncp_EventReplyPort, &bem->bem_Msg);
                                }
                                ncp->ncp_EventsPending++;
                                ncp->ncp_CurrEventMsg = NULL;
                                nIssueEventReq(ncp);
                            }
                        }
                    }
                }
                else if(pp == ncp->ncp_EPACLOutPipe)
                {
                    if((ioreq = ncp->ncp_WritePending))
                    {
                        ioerr = psdGetPipeError(pp);
                        ioreq->iobt_Actual = psdGetPipeActual(pp);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "BT ACL transmit failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            ioreq->iobt_Req.io_Error = (ioerr > 0) ? IOERR_BADLENGTH : ioerr;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        ncp->ncp_WritePending = NULL;
                    }
                }
                else if(pp == ncp->ncp_EPACLInPipe)
                {
                    if((ioreq = ncp->ncp_ReadPending))
                    {
                        ioerr = psdGetPipeError(pp);
                        ioreq->iobt_Actual = psdGetPipeActual(pp);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "BT ACL receive failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            ioreq->iobt_Req.io_Error = (ioerr > 0) ? IOERR_BADLENGTH : ioerr;
                        }
                        ReplyMsg((struct Message *) ioreq);
                    }
                    ncp->ncp_ReadPending = NULL;
                }
            }

            while((ioreq = (struct IOBTHCIReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->iobt_Req.io_Command, ioreq->iobt_Length));
                switch(ioreq->iobt_Req.io_Command)
                {
                    case BTCMD_READACL:
                        ioreq->iobt_Actual = 0;
                        Forbid();
                        AddTail(&ncp->ncp_ReadQueue, &ioreq->iobt_Req.io_Message.mn_Node);
                        Permit();
                        break;

                    case BTCMD_WRITEACL:
                        ioreq->iobt_Actual = 0;
                        Forbid();
                        AddTail(&ncp->ncp_WriteQueue, &ioreq->iobt_Req.io_Message.mn_Node);
                        Permit();
                        break;

                    case BTCMD_WRITEHCI:
                        KPRINTF(10, ("WriteHCI: %04lx (%ld)\n",
                                     *((UWORD *) ioreq->iobt_Data), ((UBYTE *) ioreq->iobt_Data)[2]));
                        psdPipeSetup(ncp->ncp_EPCmdPipe, URTF_CLASS|URTF_DEVICE,
                                     0, 0, 0);
                        ioreq->iobt_Req.io_Error = psdDoPipe(ncp->ncp_EPCmdPipe, ioreq->iobt_Data, ioreq->iobt_Length);
                        ioreq->iobt_Actual = psdGetPipeActual(ncp->ncp_EPCmdPipe);

                        KPRINTF(10, ("Actual/Length: %ld\n", ioreq->iobt_Actual, ioreq->iobt_Length));
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                        // nop
                        /* Reset does a flush too */
                    case CMD_FLUSH:
                        ioreq2 = (struct IOBTHCIReq *) ncp->ncp_WriteQueue.lh_Head;
                        while(ioreq2->iobt_Req.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->iobt_Req.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOBTHCIReq *) ncp->ncp_WriteQueue.lh_Head;
                        }
                        ioreq2 = (struct IOBTHCIReq *) ncp->ncp_ReadQueue.lh_Head;
                        while(ioreq2->iobt_Req.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->iobt_Req.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOBTHCIReq *) ncp->ncp_ReadQueue.lh_Head;
                        }
                        if((ioreq2 = ncp->ncp_ReadPending))
                        {
                            psdAbortPipe(ncp->ncp_EPACLInPipe);
                            psdWaitPipe(ncp->ncp_EPACLInPipe);
                            ioreq2->iobt_Req.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ncp->ncp_ReadPending = NULL;
                        }
                        if((ioreq2 = ncp->ncp_WritePending))
                        {
                            psdAbortPipe(ncp->ncp_EPACLOutPipe);
                            psdWaitPipe(ncp->ncp_EPACLOutPipe);
                            ioreq2->iobt_Req.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ncp->ncp_WritePending = NULL;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->iobt_Req.io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            ioreq = (struct IOBTHCIReq *) ncp->ncp_WriteQueue.lh_Head;
            if((!ncp->ncp_WritePending) && ioreq->iobt_Req.io_Message.mn_Node.ln_Succ)
            {
                Remove((struct Node *) ioreq);
                ncp->ncp_WritePending = ioreq;
                psdSendPipe(ncp->ncp_EPACLOutPipe, ioreq->iobt_Data, ioreq->iobt_Length);
            }
            ioreq = (struct IOBTHCIReq *) ncp->ncp_ReadQueue.lh_Head;
            if((!ncp->ncp_ReadPending) && ioreq->iobt_Req.io_Message.mn_Node.ln_Succ)
            {
                Remove((struct Node *) ioreq);
                ncp->ncp_ReadPending = ioreq;
                psdSendPipe(ncp->ncp_EPACLInPipe, ioreq->iobt_Data, ioreq->iobt_Length);
            }
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));
        Forbid();
        /* Now remove all requests still pending *anywhere* */
        ncp->ncp_DenyRequests = TRUE;
        /* Current transfers */
        if((ioreq = ncp->ncp_WritePending))
        {
            KPRINTF(1, ("Aborting pending write...\n"));
            psdAbortPipe(ncp->ncp_EPACLOutPipe);
            psdWaitPipe(ncp->ncp_EPACLOutPipe);
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ncp->ncp_WritePending = NULL;
        }
        if((ioreq = ncp->ncp_ReadPending))
        {
            KPRINTF(1, ("Aborting pending read...\n"));
            psdAbortPipe(ncp->ncp_EPACLInPipe);
            psdWaitPipe(ncp->ncp_EPACLInPipe);
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ncp->ncp_ReadPending = NULL;
        }
        if(ncp->ncp_CurrEventMsg)
        {
            KPRINTF(1, ("Aborting pending int...\n"));
            psdAbortPipe(ncp->ncp_EPEventIntPipe);
            psdWaitPipe(ncp->ncp_EPEventIntPipe);
            psdFreeVec(ncp->ncp_CurrEventMsg);
            ncp->ncp_CurrEventMsg = NULL;
        }
        /* Read/Write queues */
        ioreq = (struct IOBTHCIReq *) ncp->ncp_WriteQueue.lh_Head;
        while(ioreq->iobt_Req.io_Message.mn_Node.ln_Succ)
        {
            KPRINTF(1, ("Removing write request...\n"));
            Remove((struct Node *) ioreq);
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOBTHCIReq *) ncp->ncp_WriteQueue.lh_Head;
        }
        ioreq = (struct IOBTHCIReq *) ncp->ncp_ReadQueue.lh_Head;
        while(ioreq->iobt_Req.io_Message.mn_Node.ln_Succ)
        {
            KPRINTF(1, ("Removing read request...\n"));
            Remove((struct Node *) ioreq);
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOBTHCIReq *) ncp->ncp_ReadQueue.lh_Head;
        }
        /* Command queue */
        while((ioreq = (struct IOBTHCIReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
        {
            KPRINTF(1, ("Aborting pending requests...\n"));
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
        }
        Permit();
        KPRINTF(20, ("Going down the river!\n"));
        nFreeBT(ncp);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocBT()" */
struct NepClassBT * nAllocBT(void)
{
    struct Task *thistask;
    struct NepClassBT *ncp;

    thistask = FindTask(NULL);
    do
    {
        ncp = thistask->tc_UserData;
        if(!(ncp->ncp_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }

        ncp->ncp_EPACLIn = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                           EA_IsIn, TRUE,
                                           EA_TransferType, USEAF_BULK,
                                           TAG_END);
        ncp->ncp_EPACLOut = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                           EA_IsIn, FALSE,
                                           EA_TransferType, USEAF_BULK,
                                           TAG_END);
        ncp->ncp_EPEventInt = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                           EA_IsIn, TRUE,
                                           EA_TransferType, USEAF_INTERRUPT,
                                           TAG_END);

        if(!(ncp->ncp_EPACLIn && ncp->ncp_EPACLOut && ncp->ncp_EPEventInt))
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "IN or OUT endpoint missing!");
            break;
        }

        psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPACLIn,
                    EA_MaxPktSize, &ncp->ncp_EPACLInMaxPktSize,
                    TAG_END);

        psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPEventInt,
                    EA_MaxPktSize, &ncp->ncp_EPEventIntMaxPktSize,
                    TAG_END);
 
        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_EventReplyPort = CreateMsgPort()))
            {
                if((ncp->ncp_EPCmdPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL)))
                {
                    /* Enable nak timeout */
                    psdSetAttrs(PGA_PIPE, ncp->ncp_EPCmdPipe,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, 5000,
                                TAG_END);
                    if((ncp->ncp_EPACLOutPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPACLOut)))
                    {
                        /* Turn off short packets */
                        psdSetAttrs(PGA_PIPE, ncp->ncp_EPACLOutPipe,
                                    PPA_NoShortPackets, TRUE,
                                    PPA_NakTimeout, TRUE,
                                    PPA_NakTimeoutTime, 5000,
                                    TAG_END);
                        if((ncp->ncp_EPACLInPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPACLIn)))
                        {
                            /* Setup pipe */
                            psdSetAttrs(PGA_PIPE, ncp->ncp_EPACLInPipe,
                                        PPA_NakTimeout, FALSE,
                                        PPA_NakTimeoutTime, 5000,
                                        PPA_AllowRuntPackets, TRUE,
                                        TAG_END);
                            if((ncp->ncp_EPEventIntPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPEventInt)))
                            {
                                psdSetAttrs(PGA_PIPE, ncp->ncp_EPEventIntPipe,
                                            PPA_AllowRuntPackets, TRUE,
                                            TAG_END);
                                ncp->ncp_Task = thistask;
                                return(ncp);
                            }
                            psdFreePipe(ncp->ncp_EPACLInPipe);
                        }
                        psdFreePipe(ncp->ncp_EPACLOutPipe);
                    }
                    psdFreePipe(ncp->ncp_EPCmdPipe);
                }
                DeleteMsgPort(ncp->ncp_EventReplyPort);
            }
            DeleteMsgPort(ncp->ncp_TaskMsgPort);
        }
        FreeSignal((LONG) ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
    } while(FALSE);
    CloseLibrary(ncp->ncp_Base);
    Forbid();
    ncp->ncp_Task = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1UL<<ncp->ncp_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeBT()" */
void nFreeBT(struct NepClassBT *ncp)
{
    struct IOStdReq *ioreq;
    Forbid();
    /* Disable the message port, messages may still be queued */
    ncp->ncp_Unit.unit_MsgPort.mp_SigTask = NULL;
    ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
    FreeSignal((LONG) ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
    // get rid of all messages that still have appeared here
    while((ioreq = (struct IOStdReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
    {
        ioreq->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
    }
    Permit();

    psdFreePipe(ncp->ncp_EPCmdPipe);
    psdFreePipe(ncp->ncp_EPACLInPipe);
    psdFreePipe(ncp->ncp_EPACLOutPipe);
    psdFreePipe(ncp->ncp_EPEventIntPipe);
    while(ncp->ncp_EventsPending)
    {
        struct BTHCIEventMsg *bem;
        KPRINTF(10, ("%ld Events pending...", ncp->ncp_EventsPending));
        WaitPort(ncp->ncp_EventReplyPort);
        while((bem = (struct BTHCIEventMsg *) GetMsg(ncp->ncp_EventReplyPort)))
        {
            psdFreeVec(bem);
            ncp->ncp_EventsPending--;
        }
    }
    DeleteMsgPort(ncp->ncp_EventReplyPort);
    DeleteMsgPort(ncp->ncp_TaskMsgPort);
    CloseLibrary(ncp->ncp_Base);
    Forbid();
    ncp->ncp_Task = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1UL<<ncp->ncp_ReadySignal);
    }
}
/* \\\ */

/**************************************************************************/

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepBTBase *nh;
    struct NepClassBT *ncp;
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
        MUIA_Application_Title      , libname,
        MUIA_Application_Version    , VERSION_STRING,
        MUIA_Application_Copyright  , "©2005-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the bluetooth.class",
        MUIA_Application_Base       , "BLUETOOTH",
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, ncp->ncp_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                End,
            Child, MenuObjectT("Settings"),
                Child, ncp->ncp_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save",
                    MUIA_Menuitem_Shortcut, "S",
                    End,
                Child, ncp->ncp_SetDefaultMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save as Default",
                    MUIA_Menuitem_Shortcut, "D",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, ncp->ncp_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, "MUI Settings",
                    MUIA_Menuitem_Shortcut, "M",
                    End,
                End,
            End,

        SubWindow, ncp->ncp_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, libname,
            MUIA_HelpNode, libname,

            WindowContents, VGroup,
                Child, HGroup, GroupFrameT(ncp->ncp_Interface ? "Device Settings" : "Default Device Settings"),
                    Child, HSpace(0),
                    Child, ColGroup(2),
                        Child, Label((ULONG) "Autostart BT Stack:"),
                        Child, HGroup,
                            Child, ncp->ncp_StackAutoObj = ImageObject, ImageButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_Toggle,
                                MUIA_Image_Spec, MUII_CheckMark,
                                MUIA_Image_FreeVert, TRUE,
                                MUIA_Selected, ncp->ncp_CDC->cdc_StackAuto,
                                MUIA_ShowSelState, FALSE,
                                End,
                            Child, HSpace(0),
                            End,
                        Child, Label((ULONG) "Default " DEVNAME " Unit:"),
                        Child, ncp->ncp_UnitObj = StringObject,
                            StringFrame,
                            MUIA_CycleChain, 1,
                            MUIA_String_AdvanceOnCR, TRUE,
                            MUIA_String_Integer, ncp->ncp_CDC->cdc_DefaultUnit,
                            MUIA_String_Accept, "0123456789",
                            End,
                        End,
                    //Child, HSpace(0),
                    End,
                Child, VSpace(0),
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, ncp->ncp_UseObj = TextObject, ButtonFrame,
                        MUIA_ShowMe, ncp->ncp_Interface,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Save ",
                        End,
                    Child, ncp->ncp_SetDefaultObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, ncp->ncp_Interface ? "\33c Save as Default " : "\33c Save Defaults ",
                        End,
                    Child, ncp->ncp_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Use ",
                        End,
                    End,
                End,
            End,
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
        ULONG isopen;
        ULONG iconify;
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
                    get(ncp->ncp_UnitObj, MUIA_String_Integer, &ncp->ncp_CDC->cdc_DefaultUnit);
                    get(ncp->ncp_StackAutoObj, MUIA_Selected, &ncp->ncp_CDC->cdc_StackAuto);

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
                            if(psdAddCfgEntry(pic, ncp->ncp_CDC))
                            {
                                psdSaveCfgToDisk(NULL, FALSE);
                            }
                        }
                    }
                    if(ncp->ncp_Interface)
                    {
                        pic = psdGetUsbDevCfg(libname, ncp->ncp_DevIDString, ncp->ncp_IfIDString);
                        if(!pic)
                        {
                            psdSetUsbDevCfg(libname, ncp->ncp_DevIDString, ncp->ncp_IfIDString, NULL);
                            pic = psdGetUsbDevCfg(libname, ncp->ncp_DevIDString, ncp->ncp_IfIDString);
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

                case ID_ABOUT:
                    MUI_RequestA(ncp->ncp_App, ncp->ncp_MainWindow, 0, NULL, "Blimey!", VERSION_STRING, NULL);
                    break;
            }
            if(retid == MUIV_Application_ReturnID_Quit)
            {
                break;
            }
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
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
void nGUITaskCleanup(struct NepClassBT *ncp)
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
        Signal(ncp->ncp_ReadySigTask, 1UL<<ncp->ncp_ReadySignal);
    }
    --ncp->ncp_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */
