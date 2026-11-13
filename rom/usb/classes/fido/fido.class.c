/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fido.class - a Poseidon class for the FIDO (CTAPHID) interface of
          USB security keys: the HID interface whose report descriptor uses
          the FIDO usage page (0xF1D0). It binds that interface ahead of
          hid.class (priority 50) and exposes it as fido.device, one unit
          per key, carrying 64-byte reports in and out (<devices/fido.h>).
          The CTAPHID protocol itself is the user's business.

          After rawwrap.class by Chris Hodges.
*/

#include <aros/isoascii.h>

#include "debug.h"
#include "fido.class.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

#define FIDO_USAGEPAGE      0xf1d0
#define FIDO_HIDCLASSCODE   3
#define FIDO_MAXREPORTDESC  1024

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
    struct NepFidoBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);
#define UtilityBase nh->nh_UtilityBase
    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct NepFidoDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct NepFidoDevBase), NULL)))
        {
            nh->nh_DevBase->np_ClsBase = nh;
            Forbid();
            AddDevice((struct Device *) nh->nh_DevBase);
            nh->nh_DevBase->np_Library.lib_OpenCnt++;
            Permit();
            ret = nh;
        } else {
            KPRINTF(20, ("failed to create fido.device\n"));
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
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    struct NepClassFido *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        CloseLibrary((struct Library *) UtilityBase);

        ncp = (struct NepClassFido *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp);
            ncp = (struct NepClassFido *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! fido.class expunged!\n\n"));
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

/* /// "nReportHasUsagePage()" */
/* Does a HID report descriptor use the given usage page? Items are a
 * prefix byte (tag, type, size) and up to 4 data bytes; a global item
 * with tag 0 is a usage page. */
static BOOL nReportHasUsagePage(const UBYTE *rep, ULONG len, ULONG page)
{
    ULONG i = 0;

    while(i < len)
    {
        UBYTE prefix = rep[i++];
        ULONG size, k, value = 0;

        if(prefix == 0xfe)
        {
            /* long item: size byte, tag byte, data */
            if(i + 1 >= len)
                break;
            i += 2 + rep[i];
            continue;
        }
        size = prefix & 3;
        if(size == 3)
            size = 4;
        if((prefix & 0xfc) == 0x04)
        {
            for(k = 0; k < size && i + k < len; k++)
                value |= (ULONG) rep[i + k] << (8 * k);
            if(value == page)
                return(TRUE);
        }
        i += size;
    }
    return(FALSE);
}
/* \\\ */

/* /// "nIsFidoInterface()" */
/* Fetch the interface's HID descriptor and its report descriptor over
 * endpoint 0, and look for the FIDO usage page. Runs in the class
 * scanner's task, with a port of its own for the control transfers. */
static BOOL nIsFidoInterface(struct NepFidoBase *nh, struct Library *ps, struct PsdInterface *pif)
{
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    struct MsgPort *mp;
    struct PsdPipe *pp;
    IPTR ifnum;
    UBYTE hiddesc[9];
    UBYTE *rep;
    ULONG replen;
    LONG ioerr;
    BOOL res = FALSE;

    psdGetAttrs(PGA_INTERFACE, pif,
                IFA_InterfaceNum, &ifnum,
                IFA_Config, &pc,
                TAG_END);
    psdGetAttrs(PGA_CONFIG, pc,
                CA_Device, &pd,
                TAG_END);

    if(!(mp = CreateMsgPort()))
        return(FALSE);
    if((pp = psdAllocPipe(pd, mp, NULL)))
    {
        psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_INTERFACE,
                     USR_GET_DESCRIPTOR, UDT_HID<<8, ifnum);
        ioerr = psdDoPipe(pp, hiddesc, sizeof(hiddesc));
        if(ioerr == UHIOERR_RUNTPACKET)
            ioerr = 0;
        if(!ioerr && hiddesc[1] == UDT_HID && hiddesc[6] == UDT_REPORT)
        {
            replen = hiddesc[7] | (hiddesc[8] << 8);
            if(replen && replen <= FIDO_MAXREPORTDESC && (rep = AllocVec(replen, MEMF_PUBLIC|MEMF_CLEAR)))
            {
                psdPipeSetup(pp, URTF_IN|URTF_STANDARD|URTF_INTERFACE,
                             USR_GET_DESCRIPTOR, UDT_REPORT<<8, ifnum);
                ioerr = psdDoPipe(pp, rep, replen);
                if(!ioerr || ioerr == UHIOERR_RUNTPACKET)
                    res = nReportHasUsagePage(rep, psdGetPipeActual(pp), FIDO_USAGEPAGE);
                FreeVec(rep);
            }
        }
        psdFreePipe(pp);
    }
    DeleteMsgPort(mp);
    KPRINTF(1, ("nIsFidoInterface(%08lx): %ld\n", pif, res));
    return(res);
}
/* \\\ */

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassFido * usbAttemptInterfaceBinding(struct NepFidoBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    BOOL fido = FALSE;

    KPRINTF(1, ("nepFidoAttemptInterfaceBinding(%08lx)\n", pif));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    TAG_DONE);
        if(ifclass == FIDO_HIDCLASSCODE)
        {
            fido = nIsFidoInterface(nh, ps, pif);
        }
        CloseLibrary(ps);
        if(fido)
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassFido * usbForceInterfaceBinding(struct NepFidoBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassFido *ncp;
    struct NepClassFido *tmpncp;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
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

    KPRINTF(1, ("nepFidoForceInterfaceBinding(%08lx)\n", pif));

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
        /* the same key plugged in again gets its unit number back */
        unitfound = FALSE;
        unitno = (ULONG) -1;
        ncp = (struct NepClassFido *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if((ncp->ncp_UnitAltIfNum == altifnum) && (ncp->ncp_UnitIfNum == ifnum) &&
               (ncp->ncp_UnitProdID == prodid) && (ncp->ncp_UnitVendorID == vendid) && !ncp->ncp_Task)
            {
                unitno = ncp->ncp_UnitNo;
                unitfound = TRUE;
                break;
            }
            ncp = (struct NepClassFido *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(ncp = AllocVec(sizeof(struct NepClassFido), MEMF_PUBLIC|MEMF_CLEAR)))
            {
                Permit();
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
        ncp->ncp_ProductName = devname;

        /* Find next free unit number, from 0 */
        if(unitno == (ULONG) -1)
        {
            unitno = 0;
            tmpncp = (struct NepClassFido *) nh->nh_Units.lh_Head;
            while(tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                if(tmpncp->ncp_UnitNo == unitno)
                {
                    unitno++;
                    tmpncp = (struct NepClassFido *) nh->nh_Units.lh_Head;
                } else {
                    tmpncp = (struct NepClassFido *) tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
                }
            }
        }
        ncp->ncp_UnitNo = unitno;
        Permit();

        psdSafeRawDoFmt(buf, 64, "fido.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nFidoTask, ncp)))
        {
            psdBorrowLocksWait(tmptask, 1UL<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Security key '%s' is %s unit %ld.",
                               devname, nh->nh_DevBase->np_Library.lib_Node.ln_Name,
                               ncp->ncp_UnitNo);
                CloseLibrary(ps);
                return(ncp);
            }
        }
        ncp->ncp_ReadySigTask = NULL;
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepFidoBase *nh, struct NepClassFido *ncp)
{
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("nepFidoReleaseInterfaceBinding(%08lx)\n", ncp));

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
        psdGetAttrs(PGA_DEVICE, ncp->ncp_Device, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "Security key '%s' removed.",
                       devname);
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

    KPRINTF(1, ("nepFidoGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case UGA_CLASS:
             if((ti = FindTagItem(UCCA_Priority, tags)))
             {
                 /* ahead of hid.class, which would claim the interface too */
                 *((SIPTR *) ti->ti_Data) = 50;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "FIDO security keys (CTAPHID) via fido.device";
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
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
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             break;

         case UGA_BINDING:
             if((ti = FindTagItem(UCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
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
            usbReleaseInterfaceBinding(nh, (struct NepClassFido *) methoddata[0]);
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

/* /// "nFidoTask()" */
AROS_UFH0(void, nFidoTask)
{
    AROS_USERFUNC_INIT

    struct NepClassFido *ncp;
    struct PsdPipe *pp;
    struct PsdPipe *outpipe;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    ULONG len;
    struct IOStdReq *ioreq;
    struct IOStdReq *ioreq2;

    if((ncp = nAllocFido()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
        }
        Permit();

        /* reports go out through the OUT endpoint, or SET_REPORT on endpoint 0 */
        outpipe = ncp->ncp_EPOutPipe ? ncp->ncp_EPOutPipe : ncp->ncp_EP0Pipe;

        sigmask = (1L<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1L<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        do
        {
            if(ncp->ncp_AbortRead)
            {
                ncp->ncp_AbortRead = FALSE;
                if(ncp->ncp_ReadPending)
                {
                    psdAbortPipe(ncp->ncp_EPInPipe);
                }
            }
            if(ncp->ncp_AbortWrite)
            {
                ncp->ncp_AbortWrite = FALSE;
                if(ncp->ncp_WritePending)
                {
                    psdAbortPipe(outpipe);
                }
            }

            while((pp = (struct PsdPipe *) GetMsg(ncp->ncp_TaskMsgPort)))
            {
                KPRINTF(1, ("Pipe back %08lx\n", pp));
                if(pp == outpipe)
                {
                    if((ioreq = ncp->ncp_WritePending))
                    {
                        ioerr = psdGetPipeError(pp);
                        if(ioerr)
                        {
                            if(ioerr != UHIOERR_TIMEOUT)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "Report to the key failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            }
                            ioreq->io_Error = (ioerr > 0) ? IOERR_BADLENGTH : ioerr;
                            ioreq->io_Actual = 0;
                        } else {
                            ioreq->io_Actual = ioreq->io_Length;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        ncp->ncp_WritePending = NULL;
                    }
                }
                else if(pp == ncp->ncp_EPInPipe)
                {
                    if((ioreq = ncp->ncp_ReadPending))
                    {
                        ioerr = psdGetPipeError(pp);
                        len = psdGetPipeActual(pp);
                        if(ioerr && ioerr != UHIOERR_RUNTPACKET)
                        {
                            if(ioerr != UHIOERR_TIMEOUT)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "Report from the key failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            }
                            ioreq->io_Error = (ioerr > 0) ? IOERR_BADLENGTH : ioerr;
                            ioreq->io_Actual = 0;
                        } else {
                            ULONG n = (len < ioreq->io_Length) ? len : ioreq->io_Length;

                            CopyMem(ncp->ncp_InBuf, ioreq->io_Data, n);
                            ioreq->io_Actual = len;
                            ioreq->io_Error = 0;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        ncp->ncp_ReadPending = NULL;
                    }
                }
            }

            while((ioreq = (struct IOStdReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->io_Command, ioreq->io_Length));
                switch(ioreq->io_Command)
                {
                    case CMD_READ:
                        ioreq->io_Actual = 0;
                        if(!ioreq->io_Data || !ioreq->io_Length)
                        {
                            ioreq->io_Error = IOERR_BADLENGTH;
                            ReplyMsg((struct Message *) ioreq);
                            break;
                        }
                        Forbid();
                        AddTail(&ncp->ncp_ReadQueue, &ioreq->io_Message.mn_Node);
                        Permit();
                        break;

                    case CMD_WRITE:
                        ioreq->io_Actual = 0;
                        if(!ioreq->io_Data || !ioreq->io_Length || ioreq->io_Length > ncp->ncp_ReportSize)
                        {
                            ioreq->io_Error = IOERR_BADLENGTH;
                            ReplyMsg((struct Message *) ioreq);
                            break;
                        }
                        Forbid();
                        AddTail(&ncp->ncp_WriteQueue, &ioreq->io_Message.mn_Node);
                        Permit();
                        break;

                    case CMD_CLEAR:
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                        if((ioreq2 = ncp->ncp_WritePending))
                        {
                            psdAbortPipe(outpipe);
                            psdWaitPipe(outpipe);
                            ncp->ncp_WritePending = NULL;
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                        }
                        if((ioreq2 = ncp->ncp_ReadPending))
                        {
                            psdAbortPipe(ncp->ncp_EPInPipe);
                            psdWaitPipe(ncp->ncp_EPInPipe);
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ncp->ncp_ReadPending = NULL;
                        }
                        /* Reset does a flush too */
                    case CMD_FLUSH:
                        ioreq2 = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
                        while(ioreq2->io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
                        }
                        ioreq2 = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
                        while(ioreq2->io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }

            /* the next report in */
            ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
            if((!ncp->ncp_ReadPending) && ioreq->io_Message.mn_Node.ln_Succ)
            {
                Remove((struct Node *) ioreq);
                ncp->ncp_ReadPending = ioreq;
                psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_InBuf, ncp->ncp_ReportSize);
            }

            /* the next report out, zero padded to the report size */
            ioreq = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
            if((!ncp->ncp_WritePending) && ioreq->io_Message.mn_Node.ln_Succ)
            {
                Remove((struct Node *) ioreq);
                ncp->ncp_WritePending = ioreq;
                memset(ncp->ncp_OutBuf, 0, ncp->ncp_ReportSize);
                CopyMem(ioreq->io_Data, ncp->ncp_OutBuf, ioreq->io_Length);
                if(ncp->ncp_EPOutPipe)
                {
                    psdSendPipe(ncp->ncp_EPOutPipe, ncp->ncp_OutBuf, ncp->ncp_ReportSize);
                } else {
                    /* no OUT endpoint: SET_REPORT (output report, id 0) on endpoint 0 */
                    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT|URTF_CLASS|URTF_INTERFACE,
                                 UHR_SET_REPORT, 0x0200, ncp->ncp_UnitIfNum);
                    psdSendPipe(ncp->ncp_EP0Pipe, ncp->ncp_OutBuf, ncp->ncp_ReportSize);
                }
            }

            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));

        Forbid();
        /* Now remove all requests still pending *anywhere* */
        ncp->ncp_DenyRequests = TRUE;
        if((ioreq = ncp->ncp_WritePending))
        {
            psdAbortPipe(outpipe);
            psdWaitPipe(outpipe);
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ncp->ncp_WritePending = NULL;
        }
        if((ioreq = ncp->ncp_ReadPending))
        {
            psdAbortPipe(ncp->ncp_EPInPipe);
            psdWaitPipe(ncp->ncp_EPInPipe);
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ncp->ncp_ReadPending = NULL;
        }
        ioreq = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
        while(ioreq->io_Message.mn_Node.ln_Succ)
        {
            Remove((struct Node *) ioreq);
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
        }
        ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
        while(ioreq->io_Message.mn_Node.ln_Succ)
        {
            Remove((struct Node *) ioreq);
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
        }
        Permit();
        nFreeFido(ncp);
    }

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocFido()" */
struct NepClassFido * nAllocFido(void)
{
    struct Task *thistask;
    struct NepClassFido *ncp;
    ULONG bufsize;

    thistask = FindTask(NULL);
    do
    {
        ncp = thistask->tc_UserData;
        if(!(ncp->ncp_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        ncp->ncp_EPIn = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                        EA_IsIn, TRUE,
                                        EA_TransferType, USEAF_INTERRUPT,
                                        TAG_END);
        ncp->ncp_EPOut = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                         EA_IsIn, FALSE,
                                         EA_TransferType, USEAF_INTERRUPT,
                                         TAG_END);
        if(!ncp->ncp_EPIn)
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Interrupt IN endpoint missing!");
            break;
        }
        psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPIn,
                    EA_MaxPktSize, &ncp->ncp_EPInMaxPktSize,
                    TAG_END);
        ncp->ncp_EPOutMaxPktSize = 0;
        if(ncp->ncp_EPOut)
        {
            psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPOut,
                        EA_MaxPktSize, &ncp->ncp_EPOutMaxPktSize,
                        TAG_END);
        }
        /* CTAPHID reports are 64 bytes; the buffers take a full packet */
        ncp->ncp_ReportSize = FIDO_REPORTSIZE;
        bufsize = FIDO_REPORTSIZE;
        if(ncp->ncp_EPInMaxPktSize > bufsize)
            bufsize = ncp->ncp_EPInMaxPktSize;
        if(ncp->ncp_EPOutMaxPktSize > bufsize)
            bufsize = ncp->ncp_EPOutMaxPktSize;
        if(!(ncp->ncp_InBuf = AllocVec(bufsize * 2, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            KPRINTF(1, ("Out of memory for report buffers\n"));
            break;
        }
        ncp->ncp_OutBuf = ncp->ncp_InBuf + bufsize;

        ncp->ncp_ReadPending = NULL;
        ncp->ncp_WritePending = NULL;

        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_EP0Pipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL)))
            {
                ncp->ncp_EPOutPipe = NULL;
                if(!ncp->ncp_EPOut || (ncp->ncp_EPOutPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPOut)))
                {
                    if(ncp->ncp_EPOutPipe)
                    {
                        psdSetAttrs(PGA_PIPE, ncp->ncp_EPOutPipe,
                                    PPA_NoShortPackets, TRUE,
                                    PPA_NakTimeout, TRUE,
                                    PPA_NakTimeoutTime, 2000,
                                    TAG_END);
                    }
                    if((ncp->ncp_EPInPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPIn)))
                    {
                        /* a read waits for the key; short reports are fine */
                        psdSetAttrs(PGA_PIPE, ncp->ncp_EPInPipe,
                                    PPA_NakTimeout, FALSE,
                                    PPA_AllowRuntPackets, TRUE,
                                    TAG_END);
                        ncp->ncp_Task = thistask;
                        return(ncp);
                    }
                    if(ncp->ncp_EPOutPipe)
                        psdFreePipe(ncp->ncp_EPOutPipe);
                }
                psdFreePipe(ncp->ncp_EP0Pipe);
            }
            DeleteMsgPort(ncp->ncp_TaskMsgPort);
        }
        FreeSignal((LONG) ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
    } while(FALSE);
    if(ncp->ncp_InBuf)
    {
        FreeVec(ncp->ncp_InBuf);
        ncp->ncp_InBuf = ncp->ncp_OutBuf = NULL;
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

/* /// "nFreeFido()" */
void nFreeFido(struct NepClassFido *ncp)
{
    struct IOStdReq *ioreq;

    Forbid();
    /* Disable the message port, messages may still be queued */
    ncp->ncp_Unit.unit_MsgPort.mp_SigTask = NULL;
    ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
    FreeSignal((LONG) ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
    while((ioreq = (struct IOStdReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
    {
        ioreq->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
    }
    Permit();

    psdFreePipe(ncp->ncp_EPInPipe);
    if(ncp->ncp_EPOutPipe)
        psdFreePipe(ncp->ncp_EPOutPipe);
    psdFreePipe(ncp->ncp_EP0Pipe);
    if(ncp->ncp_InBuf)
    {
        FreeVec(ncp->ncp_InBuf);
        ncp->ncp_InBuf = ncp->ncp_OutBuf = NULL;
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
