/*
 *----------------------------------------------------------------------------
 *                         printer class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "printer.class.h"

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
    struct NepPrinterBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct NepPrtDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct NepPrtDevBase), NULL)))
        {
            nh->nh_DevBase->np_ClsBase = nh;
            Forbid();
            AddDevice((struct Device *) nh->nh_DevBase);
            nh->nh_DevBase->np_Library.lib_OpenCnt++;
            Permit();
            ret = nh;
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
    struct NepClassPrinter *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);

        ncp = (struct NepClassPrinter *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp);
            ncp = (struct NepClassPrinter *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! printer.class expunged!\n\n"));
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
struct NepClassPrinter * usbAttemptInterfaceBinding(struct NepPrinterBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    KPRINTF(1, ("nepPrinterAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    TAG_DONE);
        CloseLibrary(ps);
        if((ifclass == PRINTER_CLASSCODE) && (subclass == 0x01) &&
           ((proto == PRT_PROTO_BIDIR) || (proto == PRT_PROTO_UNIDIR)))// || (proto == PRT_PROTO_IEEE1284)))
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassPrinter * usbForceInterfaceBinding(struct NepPrinterBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassPrinter *ncp;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;
    IPTR altifnum;
    IPTR ifnum;
    IPTR cfgnum;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    BOOL  unitfound;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepPrinterAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_InterfaceNum, &ifnum,
                    IFA_AlternateNum, &altifnum,
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
                    TAG_END);
        Forbid();
        /* Find next free unit number */
        unitno = 0;
        ncp = (struct NepClassPrinter *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncp->ncp_UnitNo == unitno)
            {
                unitno++;
                ncp = (struct NepClassPrinter *) nh->nh_Units.lh_Head;
            } else {
                ncp = (struct NepClassPrinter *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
            }
        }
        unitfound = FALSE;
        ncp = (struct NepClassPrinter *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if((ncp->ncp_UnitAltIfNum == altifnum) && (ncp->ncp_UnitIfNum == ifnum) &&
               (ncp->ncp_UnitProdID == prodid) && (ncp->ncp_UnitVendorID == vendid))
            {
                unitno = ncp->ncp_UnitNo;
                unitfound = TRUE;
                break;
            }
            ncp = (struct NepClassPrinter *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(ncp = AllocVec(sizeof(struct NepClassPrinter), MEMF_PUBLIC|MEMF_CLEAR)))
            {
                Permit();
                CloseLibrary(ps);
                return(NULL);
            }
            /* IORequests may be queued even if the task is gone. */
            NewList(&ncp->ncp_Unit.unit_MsgPort.mp_MsgList);
            NewList(&ncp->ncp_ReadQueue);
            NewList(&ncp->ncp_WriteQueue);
            AddTail(&nh->nh_Units, &ncp->ncp_Unit.unit_MsgPort.mp_Node);
        }
        ncp->ncp_ClsBase = nh;
        ncp->ncp_UnitNo = unitno;
        ncp->ncp_Interface = pif;
        ncp->ncp_Device = pd;
        ncp->ncp_Config = pc;
        ncp->ncp_UnitAltIfNum = altifnum;
        ncp->ncp_UnitIfNum = ifnum;
        ncp->ncp_UnitCfgNum = cfgnum;
        ncp->ncp_UnitProdID = prodid;
        ncp->ncp_UnitVendorID = vendid;

        Permit();

        nLoadClassConfig(nh);

        psdSafeRawDoFmt(buf, 64, "printer.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nPrinterTask, ncp)))
        {
            psdBorrowLocksWait(tmptask, 1UL<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                //FreeSignal(ncp->ncp_ReadySignal);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Please welcome '%s' to the family at %s unit %ld!",
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
        FreeVec(ncp);
        Permit();*/
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepPrinterBase *nh, struct NepClassPrinter *ncp)
{
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("nepPrinterReleaseInterfaceBinding(%08lx)\n", ncp));
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
                       "'%s' died of boredom.",
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

    KPRINTF(1, ("nepHidGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "Bidirectional printer driver via usbparallel.device";
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasBindingCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_AfterDOSRestart, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nh->nh_UsingDefaultCfg;
                 count++;
             }
             break;

         case UGA_BINDING:
             if((ti = FindTagItem(UCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
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
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassPrinter *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenCfgWindow(nh));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "nLoadClassConfig()" */
BOOL nLoadClassConfig(struct NepPrinterBase *nh)
{
    struct Library *ps;
    struct ClsGlobalCfg *cgc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(nh->nh_GUITask)
    {
        return(FALSE);
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    /* Create default config */
    nh->nh_CurrentCGC.cgc_ChunkID = AROS_LONG2BE(MAKE_ID('U','P','R','T'));
    nh->nh_CurrentCGC.cgc_Length = AROS_LONG2BE(sizeof(struct ClsGlobalCfg)-8);
    nh->nh_CurrentCGC.cgc_EpsonInit = TRUE;
    nh->nh_CurrentCGC.cgc_SoftReset = TRUE;
    nh->nh_UsingDefaultCfg = TRUE;
    pic = psdGetClsCfg(libname);
    if(pic)
    {
        if((cgc = psdGetCfgChunk(pic, AROS_LONG2BE(nh->nh_CurrentCGC.cgc_ChunkID))))
        {
            CopyMem(((UBYTE *) cgc) + 8, ((UBYTE *) &nh->nh_CurrentCGC) + 8, min(AROS_LONG2BE(cgc->cgc_Length), AROS_LONG2BE(nh->nh_CurrentCGC.cgc_Length)));
            psdFreeVec(cgc);
            nh->nh_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nOpenCfgWindow()" */
LONG nOpenCfgWindow(struct NepPrinterBase *nh)
{
    struct Library *ps;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!nh->nh_GUITask)
    {
        if((nh->nh_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", nGUITask, nh)))
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

/* /// "nPrinterTask()" */
AROS_UFH0(void, nPrinterTask)
{
    AROS_USERFUNC_INIT

    struct NepClassPrinter *ncp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    struct PsdConfig *pc;
    IPTR confignum;
    UWORD len;
    BOOL  epsonmode = FALSE;
    UBYTE *tmpbuf;
    UBYTE portstatus;
    char *epsonid = "MFG:EPSON";//;CMD:ESCPL2,BDC,D4;MDL:Stylus Photo 870;CLS:PRINTER;DES:EPSON Stylus Photo 870;";
    struct IOExtPar *ioreq;
    struct IOExtPar *ioreq2;
    char epsonusbinit[] = { 0x00, 0x00, 0x00, 0x1b, 0x01, 0x40, 0x45, 0x4a,
                            0x4c, 0x20, 0x31, 0x32, 0x38, 0x34, 0x2e, 0x34,
                            0x0a, 0x40, 0x45, 0x4a, 0x4c, 0x20, 0x20, 0x20,
                            0x20, 0x20, 0x0a };

    if((ncp = nAllocPrinter()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1UL<<ncp->ncp_ReadySignal);
        }
        Permit();

        psdGetAttrs(PGA_INTERFACE, ncp->ncp_Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_ConfigNum, &confignum, TAG_END);

        psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                     UPR_GET_DEVICE_ID, confignum-1, (ULONG) ((ncp->ncp_UnitIfNum<<8)|ncp->ncp_UnitAltIfNum));

        ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &len, 2);
        if(ioerr)
        {
            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                         UPR_GET_DEVICE_ID, 0, (ULONG) (ncp->ncp_UnitIfNum<<8));

            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &len, 2);
        }
        if(!ioerr)
        {
            if((tmpbuf = psdAllocVec((ULONG) len+1)))
            {
                ioerr = psdDoPipe(ncp->ncp_EP0Pipe, tmpbuf, (ULONG) len);
                if(!ioerr)
                {
                    tmpbuf[len] = 0;
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "Look at this: '%s'", &tmpbuf[2]);
                    if(!strncmp(&tmpbuf[2], epsonid, 9))
                    {
                        epsonmode = TRUE;
                    }
                } else {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                   "GET_DEVICE_ID (len %ld) failed: %s (%ld)",
                                   len, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
                psdFreeVec(tmpbuf);
            }
        } else {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                           "GET_DEVICE_ID (len %ld) failed: %s (%ld)",
                           2, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }

        if(ncp->ncp_ClsBase->nh_CurrentCGC.cgc_SoftReset)
        {
            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                         UPR_SOFT_RESET, 0, ((ULONG) ncp->ncp_UnitIfNum)<<8);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                               "SOFT_RESET(USB1.1) failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_OTHER,
                             UPR_SOFT_RESET, 0, ((ULONG) ncp->ncp_UnitIfNum)<<8);
                ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
                if(ioerr)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                   "SOFT_RESET(USB1.0) failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
            }
        }
        if(epsonmode && ncp->ncp_ClsBase->nh_CurrentCGC.cgc_EpsonInit)
        {
            psdStreamWrite(ncp->ncp_EPOutStream, epsonusbinit, 27);
            psdStreamFlush(ncp->ncp_EPOutStream);
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Inserted special Epson init sequence...");
        }

        /* Main task */
        sigmask = (1UL<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1UL<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        if(ncp->ncp_EPInStream)
        {
            struct MsgPort *tmpmp = NULL;
            psdGetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream, PSA_MessagePort, &tmpmp, TAG_END);
            if(tmpmp)
            {
                sigmask |= (1UL<<tmpmp->mp_SigBit);
            }
        }
        do
        {
            while((ioreq = (struct IOExtPar *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->IOPar.io_Command, ioreq->IOPar.io_Length));
                switch(ioreq->IOPar.io_Command)
                {
                    case CMD_CLEAR:
                        psdStreamFlush(ncp->ncp_EPOutStream);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                        psdStreamFlush(ncp->ncp_EPOutStream);
                        if(ncp->ncp_ClsBase->nh_CurrentCGC.cgc_SoftReset)
                        {
                            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                                         UPR_SOFT_RESET, 0, ((ULONG) ncp->ncp_UnitIfNum)<<8);
                            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
                            if(ioerr)
                            {
                                psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                             UPR_SOFT_RESET, 0, ((ULONG) ncp->ncp_UnitIfNum)<<8);
                                ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
                            }
                            if(ioerr)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "SOFT_RESET failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                ioreq->IOPar.io_Error = ParErr_LineErr;
                            }
                        }
                        /* Reset does a flush too */

                    case CMD_FLUSH:
                        Forbid();
                        ioreq2 = (struct IOExtPar *) ncp->ncp_WriteQueue.lh_Head;
                        while(ioreq2->IOPar.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->IOPar.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOExtPar *) ncp->ncp_WriteQueue.lh_Head;
                        }
                        ioreq2 = (struct IOExtPar *) ncp->ncp_ReadQueue.lh_Head;
                        while(ioreq2->IOPar.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->IOPar.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOExtPar *) ncp->ncp_ReadQueue.lh_Head;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        Permit();
                        break;

                    case PDCMD_QUERY:
                        ioreq->io_Status = 0;
                        psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                                     UPR_GET_PORT_STATUS, 0, ((ULONG) ncp->ncp_UnitIfNum)<<8);
                        ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &portstatus, 1);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "GET_PORT_STATUS failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            ioreq->IOPar.io_Error = ParErr_LineErr;
                        }
                        if(portstatus & 0x10) ioreq->io_Status |= IOPTF_PARSEL;
                        if(portstatus & 0x20) ioreq->io_Status |= IOPTF_PAPEROUT;
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    /*case PDCMD_SETPARAMS:
                        ReplyMsg((struct Message *) ioreq);
                        break;*/

                    default:
                        ioreq->IOPar.io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            if(!ncp->ncp_DevSuspend)
            {
                Forbid();
                ioreq = (struct IOExtPar *) ncp->ncp_ReadQueue.lh_Head;
                while(ioreq->IOPar.io_Message.mn_Node.ln_Succ)
                {
                    Remove((struct Node *) ioreq);
                    Permit();
                    ioreq->IOPar.io_Actual = psdStreamRead(ncp->ncp_EPInStream, ioreq->IOPar.io_Data, ioreq->IOPar.io_Length);
                    ioerr = psdGetStreamError(ncp->ncp_EPInStream);
                    if(ioerr == UHIOERR_NAKTIMEOUT)
                    {
                        Forbid();
                        AddHead(&ncp->ncp_ReadQueue, &ioreq->IOPar.io_Message.mn_Node);
                        break;
                    } else {
                        if(ioerr > 0)
                        {
                            ioreq->IOPar.io_Error = ParErr_LineErr;
                        } else {
                            ioreq->IOPar.io_Error = ioerr;
                        }
                        ReplyMsg((struct Message *) ioreq);
                    }
                    Forbid();
                    ioreq = (struct IOExtPar *) ncp->ncp_ReadQueue.lh_Head;
                } 
                ioreq = (struct IOExtPar *) ncp->ncp_WriteQueue.lh_Head;
                while(ioreq->IOPar.io_Message.mn_Node.ln_Succ)
                {
                    Remove((struct Node *) ioreq);
                    ncp->ncp_WritePending = ioreq;
                    Permit();
                    ioreq->IOPar.io_Actual = psdStreamWrite(ncp->ncp_EPOutStream, ioreq->IOPar.io_Data, ioreq->IOPar.io_Length);
                    ncp->ncp_WritePending = NULL;
                    ioerr = psdGetStreamError(ncp->ncp_EPInStream);
                    if(ioerr > 0)
                    {
                        ioreq->IOPar.io_Error = ParErr_LineErr;
                    } else {
                        ioreq->IOPar.io_Error = ioerr;
                    }
                    ReplyMsg((struct Message *) ioreq);
                    Forbid();
                    ioreq = (struct IOExtPar *) ncp->ncp_WriteQueue.lh_Head;
                }
                Permit();
            }
            if(ncp->ncp_FlushBuffer)
            {
                psdStreamFlush(ncp->ncp_EPOutStream);
                ncp->ncp_FlushBuffer = FALSE;
            }
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));
        /* Now remove all requests still pending *anywhere* */
        ncp->ncp_DenyRequests = TRUE;
        /* Current transfers */
        psdStreamFlush(ncp->ncp_EPOutStream);
        /* Read/Write queues */
        Forbid();
        ioreq = (struct IOExtPar *) ncp->ncp_WriteQueue.lh_Head;
        while(ioreq->IOPar.io_Message.mn_Node.ln_Succ)
        {
            Remove((struct Node *) ioreq);
            ioreq->IOPar.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOExtPar *) ncp->ncp_WriteQueue.lh_Head;
        }
        ioreq = (struct IOExtPar *) ncp->ncp_ReadQueue.lh_Head;
        while(ioreq->IOPar.io_Message.mn_Node.ln_Succ)
        {
            Remove((struct Node *) ioreq);
            ioreq->IOPar.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOExtPar *) ncp->ncp_ReadQueue.lh_Head;
        }
        /* Command queue */
        while((ioreq = (struct IOExtPar *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
        {
            ioreq->IOPar.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
        }
        Permit();
        KPRINTF(20, ("Going down the river!\n"));
        nFreePrinter(ncp);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocPrinter()" */
struct NepClassPrinter * nAllocPrinter(void)
{
    struct Task *thistask;
    struct NepClassPrinter *ncp;

    thistask = FindTask(NULL);
    ncp = thistask->tc_UserData;
    do
    {
        if(!(ncp->ncp_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
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
        if(!ncp->ncp_EPOut)
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "OUT endpoint missing!");
            break;
        }

        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;
        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_EP0Pipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL)))
            {
                psdSetAttrs(PGA_PIPE, ncp->ncp_EP0Pipe,
                            PPA_NakTimeout, TRUE,
                            PPA_NakTimeoutTime, 5000,
                            TAG_END);
                ncp->ncp_AbortSignal = AllocSignal(-1);
                if((ncp->ncp_EPOutStream = psdOpenStream(ncp->ncp_EPOut,
                                                        PSA_BufferSize, 1024,
                                                        PSA_BufferedWrite, TRUE,
                                                        PSA_AbortSigMask, 1UL<<ncp->ncp_AbortSignal,
                                                        TAG_END)))
                {
                    /* EPIn is only optional! */
                    if(!ncp->ncp_EPIn)
                    {
                        ncp->ncp_Task = thistask;
                        return(ncp);
                    }
                    if((ncp->ncp_EPInStream = psdOpenStream(ncp->ncp_EPIn,
                                                           PSA_BufferSize, 1024,
                                                           PSA_ReadAhead, FALSE,
                                                           PSA_BufferedRead, TRUE,
                                                           PSA_ShortPktTerm, TRUE,
                                                           PSA_NakTimeout, TRUE,
                                                           PSA_NakTimeoutTime, 1000,
                                                           TAG_END)))
                    {
                        ncp->ncp_Task = thistask;
                        return(ncp);
                    }
                    psdCloseStream(ncp->ncp_EPOutStream);
                }
                FreeSignal(ncp->ncp_AbortSignal);
                psdFreePipe(ncp->ncp_EP0Pipe);
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

/* /// "nFreePrinter()" */
void nFreePrinter(struct NepClassPrinter *ncp)
{
    struct IOStdReq *ioreq;
    Forbid();
    FreeSignal(ncp->ncp_AbortSignal);
    ncp->ncp_AbortSignal = -1;
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

    if(ncp->ncp_EPInStream)
    {
        psdCloseStream(ncp->ncp_EPInStream);
        ncp->ncp_EPInStream = NULL;
    }
    psdCloseStream(ncp->ncp_EPOutStream);
    psdFreePipe(ncp->ncp_EP0Pipe);
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
    struct NepPrinterBase *nh;
    APTR pic;

    thistask = FindTask(NULL);
#undef ps
#define ps nh->nh_PsdBase
#undef IntuitionBase
#define IntuitionBase nh->nh_IntBase
#undef MUIMasterBase
#define MUIMasterBase nh->nh_MUIBase

    nh = thistask->tc_UserData;
    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        nGUITaskCleanup(nh);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        nGUITaskCleanup(nh);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        nGUITaskCleanup(nh);
        return;
    }

    nh->nh_App = ApplicationObject,
        MUIA_Application_Title      , libname,
        MUIA_Application_Version    , VERSION_STRING,
        MUIA_Application_Copyright  , "©2002-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the printer.class",
        MUIA_Application_Base       , "PRINTER",
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, nh->nh_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                End,
            Child, MenuObjectT("Settings"),
                Child, nh->nh_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save",
                    MUIA_Menuitem_Shortcut, "S",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, nh->nh_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, "MUI Settings",
                    MUIA_Menuitem_Shortcut, "M",
                    End,
                End,
            End,

        SubWindow, nh->nh_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, libname,
            MUIA_HelpNode, libname,

            WindowContents, VGroup,
/*                Child, actionobj = NewObject(ActionClass->mcc_Class, 0,
                    MUIA_ShowMe, FALSE,
                    End, */
                Child, ColGroup(2), GroupFrameT("Global Settings"),
                    Child, Label((ULONG) "Epson Init Sequence:"),
                    Child, HGroup,
                        Child, nh->nh_EpsonInitObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nh->nh_CurrentCGC.cgc_EpsonInit,
                            MUIA_ShowSelState, FALSE,
                            End,
                        Child, HSpace(0),
                        End,
                    Child, Label((ULONG) "Enable SoftReset:"),
                    Child, HGroup,
                        Child, nh->nh_SoftResetObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nh->nh_CurrentCGC.cgc_SoftReset,
                            MUIA_ShowSelState, FALSE,
                            End,
                        Child, HSpace(0),
                        End,
                    End,
                Child, VSpace(0),
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, nh->nh_UseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Save ",
                        End,
                    Child, nh->nh_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Use ",
                        End,
                    End,
                End,
            End,
        End;

    if(!nh->nh_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        nGUITaskCleanup(nh);
        return;
    }
    DoMethod(nh->nh_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nh->nh_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(nh->nh_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nh->nh_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nh->nh_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(nh->nh_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(nh->nh_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nh->nh_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_OpenConfigWindow, 0);

    {
        ULONG isopen;
        ULONG iconify;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(nh->nh_App, MUIA_Application_Iconified, &iconify);
        set(nh->nh_MainWindow, MUIA_Window_Open, TRUE);
        get(nh->nh_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            nGUITaskCleanup(nh);
            return;
        }
        sigmask = 0;
        do
        {
            retid = DoMethod(nh->nh_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_STORE_CONFIG:
                case MUIV_Application_ReturnID_Quit:
                    get(nh->nh_EpsonInitObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_EpsonInit);
                    get(nh->nh_SoftResetObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_SoftReset);
                    pic = psdGetClsCfg(libname);
                    if(!pic)
                    {
                        psdSetClsCfg(libname, NULL);
                        pic = psdGetClsCfg(libname);
                    }
                    if(pic)
                    {
                        if(psdAddCfgEntry(pic, &nh->nh_CurrentCGC))
                        {
                            if(retid != MUIV_Application_ReturnID_Quit)
                            {
                                psdSaveCfgToDisk(NULL, FALSE);
                            }
                            retid = MUIV_Application_ReturnID_Quit;
                        }
                    }
                    break;

               case ID_ABOUT:
                    MUI_RequestA(nh->nh_App, nh->nh_MainWindow, 0, NULL, "I'm utterly squished!", VERSION_STRING, NULL);
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
        set(nh->nh_MainWindow, MUIA_Window_Open, FALSE);
    }
    nGUITaskCleanup(nh);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void nGUITaskCleanup(struct NepPrinterBase *nh)
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
    if(ps)
    {
        CloseLibrary(ps);
        ps = NULL;
    }
    Forbid();
    nh->nh_GUITask = NULL;
    --nh->nh_Library.lib_OpenCnt;
}
/* \\\ */
