/*
 * $Id$
 */

#include "debug.h"

#include "rndis.class.h"

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

#define UtilityBase nh->nh_UtilityBase

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
            KPRINTF(20, ("failed to create usbrndis.device\n"));
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

        KPRINTF(5, ("libExpunge: Unloading done! rndis.class expunged!\n\n"));
    } else {
        KPRINTF(5, ("libExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        return(FALSE);
    }

    return(TRUE);
}

ADD2INITLIB(libInit, 0)
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
    { 0x0bb4, 0x0ffe },      // HTC , Android phone
    //{ 0x12d1, 0x1039 },   // Huawei u8800
    { 0, 0 }
};

/* /// "usbAttemptDeviceBinding()" */
struct NepClassEth * usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct AutoBindData *abd = ClassBinds;
    struct PsdInterface *pif;
    IPTR prodid;
    IPTR vendid;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    KPRINTF(1, ("nepEthAttemptDeviceBinding(%08lx)\n", pd));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_VendorID, &vendid,
                    DA_ProductID, &prodid,
                    TAG_END);

        if( (pif = psdFindInterface(pd, NULL,TAG_END)) ){
            psdGetAttrs(PGA_INTERFACE, pif,
                        IFA_Class, &ifclass,
                        IFA_SubClass, &subclass,
                        IFA_Protocol, &proto,
                        TAG_DONE);

            if (ifclass == 224 &&  // WIRELESS
                subclass == 1 &&   // RF
                proto == 3)        // RNDIS
            {
                CloseLibrary(ps);
                return(usbForceDeviceBinding(nh, pd));
            }
        }

        while(abd->abd_VendID)
        {
            if((vendid == abd->abd_VendID) && (prodid == abd->abd_ProdID))
            {
                CloseLibrary(ps);
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
    STRPTR devname;
    STRPTR devidstr;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    BOOL  unitfound;
    UBYTE buf[64];

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
        }
        ncp->ncp_ClsBase = nh;
        ncp->ncp_Device = pd;
        ncp->ncp_UnitProdID = prodid;
        ncp->ncp_UnitVendorID = vendid;

        //nLoadBindingConfig(ncp);

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

        psdSafeRawDoFmt(buf, 64, "rndis.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if(psdSpawnSubTask(buf, nEthTask, ncp))
        {
            Wait(1L<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                //FreeSignal(ncp->ncp_ReadySignal);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Mr. Data linked '%s' to %s unit %ld!",
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
                       "Shrinkwrapped and wasted '%s'.",
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
                 *((SIPTR *) ti->ti_Data) = -100;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Ethernet SANA wrapper for RNDIS devices via usbrndis.device";
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

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/**************************************************************************/

#undef  ps
#define ps ncp->ncp_Base

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

        urndis_attach(ncp);

        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
        }
        Permit();

        {
            /* Record start time_of_day */
            //GetSysTime(&ncp->ncp_DeviceStats.LastStart);
            /* Now online */
            ncp->ncp_StateFlags |= DDF_ONLINE;
            ncp->ncp_StateFlags &= ~DDF_OFFLINE;
            /* Trigger any ONLINE events */
            nDoEvent(ncp, S2EVENT_ONLINE);
        }

        /* Main task */
        sigmask = (1L<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1L<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        do
        {
            // start transmitting read request if online...
            if((ncp->ncp_StateFlags & DDF_ONLINE) && (ncp->ncp_ReadPending == NULL))
            {
                ncp->ncp_ReadPending = ncp->ncp_ReadBuffer[ncp->ncp_ReadBufNum];
                psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_ReadPending, RNDIS_BUFSZ );
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
                        KPRINTF(1, ("ReadBack with %ld bytes.\n", pktlen));
                        // interleave next packet reading ASAP.
                        if(ncp->ncp_StateFlags & DDF_ONLINE)
                        {
                            ncp->ncp_ReadPending = ncp->ncp_ReadBuffer[ncp->ncp_ReadBufNum];
                            psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_ReadPending, RNDIS_BUFSZ );
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
                            urndis_decap(ncp, pktptr, pktlen);
                        }
                    }
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

        ncp->ncp_Interface = NULL;
        do
        {
            ncp->ncp_Interface = psdFindInterface(ncp->ncp_Device, ncp->ncp_Interface,
                                               TAG_END);
            if(!ncp->ncp_Interface)
            {
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

        } while(!(ncp->ncp_EPOut && ncp->ncp_EPIn));


        if(!ncp->ncp_Interface)
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "No interface?");
            break;
        }

        if(!(ncp->ncp_EPIn && ncp->ncp_EPOut))
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "IN or OUT endpoint missing!");
            break;
        }

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

                            return(ncp);
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
  //  UBYTE *packetdata;
    UBYTE *copydest;
    UWORD writelen;
    struct BufMan *bufman;
    struct Sana2PacketTypeStats *stats;
    UBYTE *buf = ncp->ncp_WriteBuffer[ncp->ncp_WriteBufNum];
    LONG encaplen;

    packettype = ioreq->ios2_PacketType;
    copydest   = buf;
    writelen   = ioreq->ios2_DataLength;
    bufman     = ioreq->ios2_BufferManagement;

    // remove RNDIS header
    encaplen = urndis_encap(ncp, buf ,writelen +
                    (!(ioreq->ios2_Req.io_Flags & SANA2IOF_RAW) ? sizeof(struct EtherPacketHeader) : 0)
                    );
    copydest += encaplen;
    writelen += encaplen;
    eph = (struct EtherPacketHeader *)copydest;

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
        nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_TX|S2EVENT_BUFF);

        /* Set error code and terminate the iorequest.
           NOTE: Can't use RC_* or deverror() this is not
           called from devBeginIO()! */
        ioreq->ios2_DataLength   = 0;
        ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        ioreq->ios2_WireError    = S2WERR_BUFF_ERROR;
        return FALSE;
    }

    //bug("out %d\n",writelen);
    KPRINTF(20, ("PktOut[%ld] %ld\n", ncp->ncp_WriteBufNum, writelen));
    //dumpmem(buf, writelen);

    ncp->ncp_WritePending[ncp->ncp_WriteBufNum] = ioreq;
    psdSendPipe(ncp->ncp_EPOutPipe[ncp->ncp_WriteBufNum], buf, (ULONG) writelen);
    ncp->ncp_WriteBufNum ^= 1;

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
        static const UBYTE bcast[ETHER_ADDR_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        if(memcmp(bcast, ioreq->ios2_DstAddr, ETHER_ADDR_SIZE)==0)
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
BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG pktlen)
{
    struct EtherPacketHeader *eph;
    struct BufMan *bufman;
    struct IOSana2Req *worknode, *nextnode;
    struct Sana2PacketTypeStats *stats;
    UWORD flags;
    UWORD datasize;

    KPRINTF(20, ("PktIn [%ld] %ld\n", ncp->ncp_ReadBufNum, pktlen));
    //bug("in %d\n",pktlen);

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
    nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_RX);
    return FALSE;
}
/* \\\ */

/**************************************************************************/




