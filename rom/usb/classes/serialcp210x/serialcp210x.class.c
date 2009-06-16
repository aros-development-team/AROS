/*
 *----------------------------------------------------------------------------
 *                         serialcp210x class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "serialcp210x.class.h"

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
    struct NepSerialBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct NepSerDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct NepSerDevBase), NULL)))
        {
            nh->nh_DevBase->np_ClsBase = nh;
            Forbid();
            AddDevice((struct Device *) nh->nh_DevBase);
            nh->nh_DevBase->np_Library.lib_OpenCnt++;
            Permit();
            ret = nh;
        } else {
            KPRINTF(20, ("failed to create serialcp210x.device\n"));
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
    struct NepClassSerial *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);
        
        ncp = (struct NepClassSerial *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp);
            ncp = (struct NepClassSerial *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! serialcp210x.class expunged!\n\n"));
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
    { 0x08e6, 0x5501 }, /* Gemalto Prox-PU/CU contactless smartcard reader */
    { 0x0fcf, 0x1003 }, /* Dynastream ANT development board */
    { 0x0fcf, 0x1004 }, /* Dynastream ANT2USB */
    { 0x10a6, 0xaa26 }, /* Knock-off DCU-11 cable */
    { 0x10ab, 0x10c5 }, /* Siemens MC60 Cable */
    { 0x10b5, 0xac70 }, /* Nokia CA-42 USB */
    { 0x10c4, 0x800a }, /* SPORTident BSM7-D-USB main station */
    { 0x10c4, 0x803b }, /* Pololu USB-serial converter */
    { 0x10c4, 0x8053 }, /* Enfora EDG1228 */
    { 0x10c4, 0x8066 }, /* Argussoft In-System Programmer */
    { 0x10c4, 0x807a }, /* Crumb128 board */
    { 0x10c4, 0x80ca }, /* Degree Controls Inc */
    { 0x10c4, 0x80dd }, /* Tracient RFID */
    { 0x10c4, 0x80f6 }, /* Suunto sports instrument */
    { 0x10c4, 0x813d }, /* Burnside Telecom Deskmobile */
    { 0x10c4, 0x814a }, /* West Mountain Radio RIGblaster P&P */
    { 0x10c4, 0x814b }, /* West Mountain Radio RIGtalk */
    { 0x10c4, 0x815e }, /* Helicomm IP-Link 1220-DVM */
    { 0x10c4, 0x81c8 }, /* Lipowsky Industrie Elektronik GmbH, Baby-JTAG */
    { 0x10c4, 0x81e2 }, /* Lipowsky Industrie Elektronik GmbH, Baby-LIN */
    { 0x10c4, 0x81e7 }, /* Aerocomm Radio */
    { 0x10c4, 0x8218 }, /* Lipowsky Industrie Elektronik GmbH, HARP-1 */
    { 0x10c4, 0x8293 }, /* Telegesys ETRX2USB */
    { 0x10c4, 0xea60 }, /* Silicon Labs factory default */
    { 0x10c4, 0xea61 }, /* Silicon Labs factory default */
    { 0x10c4, 0xf001 }, /* Elan Digital Systems USBscope50 */
    { 0x10c4, 0xf002 }, /* Elan Digital Systems USBwave12 */
    { 0x10c4, 0xf003 }, /* Elan Digital Systems USBpulse100 */
    { 0x10c4, 0xf004 }, /* Elan Digital Systems USBcount50 */
    { 0x10c5, 0xea61 }, /* Silicon Labs MobiData GPRS USB Modem */
    { 0x13ad, 0x9999 }, /* Baltech card reader */
    { 0x166a, 0x0303 }, /* Clipsal 5500PCU C-Bus USB interface */
    { 0x16d6, 0x0001 }, /* Jablotron serial interface */
    { 0, 0 }
};

/* /// "usbAttemptDeviceBinding()" */
struct NepClassSerial * usbAttemptDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct AutoBindData *abd = ClassBinds;
    IPTR prodid;
    IPTR vendid;

    KPRINTF(1, ("nepSerialAttemptDeviceBinding(%08lx)\n", pd));
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
struct NepClassSerial * usbForceDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct NepClassSerial *ncp;
    STRPTR devname;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    BOOL  unitfound;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepSerialForceDeviceBinding(%08lx)\n", pd));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductID, &prodid,
                    DA_VendorID, &vendid,
                    DA_ProductName, &devname,
                    TAG_END);
        Forbid();
        /* Find next free unit number */
        unitno = 0;
        ncp = (struct NepClassSerial *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncp->ncp_UnitNo == unitno)
            {
                unitno++;
                ncp = (struct NepClassSerial *) nh->nh_Units.lh_Head;
            } else {
                ncp = (struct NepClassSerial *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
            }
        }
        unitfound = FALSE;
        ncp = (struct NepClassSerial *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if((ncp->ncp_UnitProdID == prodid) && (ncp->ncp_UnitVendorID == vendid))
            {
                unitno = ncp->ncp_UnitNo;
                unitfound = TRUE;
                break;
            }
            ncp = (struct NepClassSerial *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(ncp = AllocVec(sizeof(struct NepClassSerial), MEMF_PUBLIC|MEMF_CLEAR)))
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
            ncp->ncp_DenyRequests = TRUE;
        }
        ncp->ncp_UnitNo = unitno;
        ncp->ncp_Device = pd;
        ncp->ncp_UnitProdID = prodid;
        ncp->ncp_UnitVendorID = vendid;
        Permit();

        psdSafeRawDoFmt(buf, 64, "serialcp210x.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nSerialTask, ncp)))
        {
            psdBorrowLocksWait(tmptask, 1UL<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                //FreeSignal(ncp->ncp_ReadySignal);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Another serial killer '%s' at %s unit %ld!",
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

/* /// "usbReleaseDeviceBinding()" */
void usbReleaseDeviceBinding(struct NepSerialBase *nh, struct NepClassSerial *ncp)
{
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("nepSerialReleaseDeviceBinding(%08lx)\n", ncp));
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
                       "Lost the wire on '%s'.",
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
                 *((STRPTR *) ti->ti_Data) = "CP210x serial adapter via serialcp210x.device";
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
        case UCM_AttemptDeviceBinding:
            return((IPTR) usbAttemptDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ForceDeviceBinding:
            return((IPTR) usbForceDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ReleaseDeviceBinding:
            usbReleaseDeviceBinding(nh, (struct NepClassSerial *) methoddata[0]);
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

/* /// "nSetRegister()" */
void nSetRegister(struct NepClassSerial *ncp, ULONG reg, ULONG value)
{
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_VENDOR|URTF_INTERFACE,
                 reg, value, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Could not set register %ld to %08lx: %s (%ld)",
                       reg, value,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
}
/* \\\ */

/* /// "nGetRegister()" */
ULONG nGetRegister(struct NepClassSerial *ncp, ULONG reg)
{
    LONG ioerr;
    UBYTE buf[1];
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_INTERFACE,
                 reg, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, buf, 1);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Could not get register %ld: %s (%ld)",
                       reg,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return 0;
    } else {
        return buf[0]; // although returned in UWORDs, fields are bytes!
    }
}
/* \\\ */

/* /// "nSetRegisters()" */
void nSetRegisters(struct NepClassSerial *ncp, ULONG regstart, UBYTE *values, ULONG len)
{
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_VENDOR|URTF_INTERFACE,
                 regstart, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, values, len);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Could not set registers %ld-%ld: %s (%ld)",
                       regstart, regstart + len - 1,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
}
/* \\\ */

/* /// "nSetSerialMode()" */
void nSetSerialMode(struct NepClassSerial *ncp, struct IOExtSer *ioreq)
{
    ULONG bufsize = ioreq->io_RBufLen / NUMREADPIPES;
    ULONG cfg = 0;
    UBYTE modemdata[16];
    LONG ioerr;

    if(bufsize < 1024)
    {
        bufsize = 1024;
    }
    bufsize = (bufsize + 127) & (~127);

    psdSetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream,
                PSA_BufferSize, bufsize,
                TAG_END);
    if(ioreq->io_SerFlags & SERF_EOFMODE)
    {
        psdSetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream,
                    PSA_TermArray, &ioreq->io_TermArray,
                    TAG_END);
        psdSetAttrs(PGA_PIPESTREAM, ncp->ncp_EPOutStream,
                    PSA_TermArray, &ioreq->io_TermArray,
                    TAG_END);
    } else {
        psdSetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream,
                    PSA_TermArray, NULL,
                    TAG_END);
        psdSetAttrs(PGA_PIPESTREAM, ncp->ncp_EPOutStream,
                    PSA_TermArray, NULL,
                    TAG_END);
    }
    nSetRegister(ncp, UCPR_SET_BAUDRATE, BAUD_RATE_GEN_FREQ / ioreq->io_Baud);

    if(ioreq->io_StopBits == 1)
    {
        cfg = BITS_STOP_1;
    }
    else if(ioreq->io_StopBits == 2)
    {
        cfg = BITS_STOP_2;
    } else {
        ioreq->IOSer.io_Error = SerErr_InvParam;
    }
    /* no parity by default */
    if(ioreq->io_SerFlags & SERF_PARTY_ON)
    {
        if(ioreq->io_SerFlags & SERF_PARTY_ODD)
        {
            cfg |= BITS_PARITY_ODD; /* odd parity */
        } else {
            cfg |= BITS_PARITY_EVEN; /* even parity */
        }
    }
    if(ioreq->io_ExtFlags & SEXTF_MSPON)
    {
        ioreq->io_SerFlags |= SERF_PARTY_ON;
        if(ioreq->io_ExtFlags & SEXTF_MARK)
        {
            cfg |= BITS_PARITY_MARK; /* mark parity */
        } else {
            cfg |= BITS_PARITY_SPACE; /* space parity */
        }
    }
    cfg |= ioreq->io_ReadLen<<8;

    if(ioreq->io_SerFlags & SERF_RAD_BOOGIE)
    {
        cfg = 0x0800;
    }

    nSetRegister(ncp, UCPR_SET_BITS, cfg);

    /* Activate RTS/CTS */

    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_INTERFACE,
                 UCPR_GET_MODEMCTL, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, modemdata, 16);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Could not get modem data: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return;
    }

    if(ioreq->io_SerFlags & SERF_7WIRE)
    {
        nSetRegister(ncp, UCPR_SET_CONTROL, UART_RTS|UART_WRITE_RTS|UART_DTR|UART_WRITE_DTR);
        modemdata[0] &= ~0x7b;
        modemdata[0] |= 0x09;
        modemdata[4] = 0x80;
        nSetRegisters(ncp, UCPR_SET_MODEMCTL, modemdata, 16);
    } else {
        nSetRegister(ncp, UCPR_SET_CONTROL, UART_WRITE_RTS|UART_WRITE_DTR);
        modemdata[0] &= ~0x7b;
        modemdata[0] |= 0x01;
        modemdata[4] = 0x40;
        nSetRegisters(ncp, UCPR_SET_MODEMCTL, modemdata, 16);
    }
}
/* \\\ */

/* /// "nSerialTask()" */
AROS_UFH0(void, nSerialTask)
{
    AROS_USERFUNC_INIT

    struct NepClassSerial *ncp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    struct IOExtSer *ioreq;
    struct IOExtSer *ioreq2;
    ULONG control;
    UWORD serstate;
    IPTR pending;

    if((ncp = nAllocSerial()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
        }
        Permit();

        nSetRegister(ncp, UCPR_SET_STATE, UART_ENABLE);

        sigmask = (1UL<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1UL<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
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
            psdStreamRead(ncp->ncp_EPInStream, NULL, 0);
            while((ioreq = (struct IOExtSer *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->IOSer.io_Command, ioreq->IOSer.io_Length));
                switch(ioreq->IOSer.io_Command)
                {
                    case CMD_CLEAR:
                        psdStreamFlush(ncp->ncp_EPOutStream);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                        /* Reset does a flush too */
                    case CMD_FLUSH:
                        Forbid();
                        ioreq2 = (struct IOExtSer *) ncp->ncp_WriteQueue.lh_Head;
                        while(ioreq2->IOSer.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->IOSer.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOExtSer *) ncp->ncp_WriteQueue.lh_Head;
                        }
                        ioreq2 = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
                        while(ioreq2->IOSer.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->IOSer.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        Permit();
                        break;

                    case SDCMD_QUERY:
                        serstate = 0;
                        control = nGetRegister(ncp, UCPR_GET_CONTROL);

                        if(!(control & UART_DCD)) /* Carrier Detect */
                            serstate |= (1<<5);
                        if(!(control & UART_DSR)) /* DSR */
                            serstate |= (1<<3);
                        if(!(control & UART_CTS)) /* ClearToSend */
                            serstate |= (1<<4);
                        if(!(control & UART_RTS)) /* ReadyToSend */
                            serstate |= (1<<6);
                        if(!(control & UART_DTR)) /* DTR */
                            serstate |= (1<<7);
                        if(control & UART_RING) /* Ring */
                            serstate |= (1<<2);

                        ioreq->io_Status = serstate;
                        pending = 0;
                        psdGetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream,
                                    PSA_BytesPending, &pending,
                                    TAG_END);
                        ioreq->IOSer.io_Actual = pending;
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case SDCMD_SETPARAMS:
                        nSetSerialMode(ncp, ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->IOSer.io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            if(!ncp->ncp_DevSuspend)
            {
                pending = 0;
                psdGetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream,
                            PSA_BytesPending, &pending,
                            TAG_END);
                Forbid();
                ioreq = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
                while(ioreq->IOSer.io_Message.mn_Node.ln_Succ && pending)
                {
                    Remove((struct Node *) ioreq);
                    Permit();
                    ioreq->IOSer.io_Actual = psdStreamRead(ncp->ncp_EPInStream, ioreq->IOSer.io_Data, ioreq->IOSer.io_Length);
                    ioerr = psdGetStreamError(ncp->ncp_EPInStream);
                    if(ioerr == UHIOERR_NAKTIMEOUT)
                    {
                        Forbid();
                        AddHead(&ncp->ncp_ReadQueue, &ioreq->IOSer.io_Message.mn_Node);
                        break;
                    } else {
                        if(ioerr > 0)
                        {
                            ioreq->IOSer.io_Error = SerErr_LineErr;
                        } else {
                            ioreq->IOSer.io_Error = ioerr;
                        }
                        ReplyMsg((struct Message *) ioreq);
                    }
                    psdGetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream,
                                PSA_BytesPending, &pending,
                                TAG_END);
                    Forbid();
                    ioreq = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
                }
                ioreq = (struct IOExtSer *) ncp->ncp_WriteQueue.lh_Head;
                while(ioreq->IOSer.io_Message.mn_Node.ln_Succ)
                {
                    Remove((struct Node *) ioreq);
                    ncp->ncp_WritePending = ioreq;
                    Permit();
                    ioreq->IOSer.io_Actual = psdStreamWrite(ncp->ncp_EPOutStream, ioreq->IOSer.io_Data, ioreq->IOSer.io_Length);
                    ncp->ncp_WritePending = NULL;
                    ioerr = psdGetStreamError(ncp->ncp_EPInStream);
                    if(ioerr > 0)
                    {
                        ioreq->IOSer.io_Error = SerErr_LineErr;
                    } else {
                        ioreq->IOSer.io_Error = ioerr;
                    }
                    ReplyMsg((struct Message *) ioreq);
                    Forbid();
                    ioreq = (struct IOExtSer *) ncp->ncp_WriteQueue.lh_Head;
                }
                Permit();
            }
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));
        KPRINTF(1, ("Going down...\n"));
        /* Now remove all requests still pending *anywhere* */
        ncp->ncp_DenyRequests = TRUE;

        nSetRegister(ncp, UCPR_SET_STATE, UART_DISABLE);

        /* Read/Write queues */
        Forbid();
        ioreq = (struct IOExtSer *) ncp->ncp_WriteQueue.lh_Head;
        while(ioreq->IOSer.io_Message.mn_Node.ln_Succ)
        {
            Remove((struct Node *) ioreq);
            ioreq->IOSer.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOExtSer *) ncp->ncp_WriteQueue.lh_Head;
        }
        ioreq = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
        while(ioreq->IOSer.io_Message.mn_Node.ln_Succ)
        {
            Remove((struct Node *) ioreq);
            ioreq->IOSer.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
        }
        /* Command queue */
        while((ioreq = (struct IOExtSer *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
        {
            ioreq->IOSer.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
        }
        Permit();
        KPRINTF(20, ("Going down the river!\n"));
        nFreeSerial(ncp);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocSerial()" */
struct NepClassSerial * nAllocSerial(void)
{
    struct Task *thistask;
    struct NepClassSerial *ncp;
    IPTR epnum;

    thistask = FindTask(NULL);
    ncp = thistask->tc_UserData;
    do
    {
        if(!(ncp->ncp_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }

        ncp->ncp_Interface = psdFindInterface(ncp->ncp_Device, NULL, TAG_END);

        if(!ncp->ncp_Interface)
        {
            KPRINTF(1, ("Ooops!?! No interfaces defined?\n"));
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

        psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPIn,
                    EA_EndpointNum, &epnum,
                    TAG_END);
        ncp->ncp_EPInNum = epnum;
        psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPOut,
                    EA_EndpointNum, &epnum,
                    TAG_END);
        ncp->ncp_EPOutNum = epnum;

        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;
        ncp->ncp_WritePending = NULL;

        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_EP0Pipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL)))
            {

                ncp->ncp_AbortSignal = AllocSignal(-1);
                if((ncp->ncp_EPOutStream = psdOpenStream(ncp->ncp_EPOut,
                                                        PSA_BufferedWrite, FALSE,
                                                        PSA_NoZeroPktTerm, TRUE,
                                                        PSA_NakTimeout, TRUE,
                                                        PSA_NakTimeoutTime, 5000,
                                                        PSA_AbortSigMask, (1UL<<ncp->ncp_AbortSignal)|SIGBREAKF_CTRL_C,
                                                        TAG_END)))
                {
                    if((ncp->ncp_EPInStream = psdOpenStream(ncp->ncp_EPIn,
                                                           PSA_ReadAhead, TRUE,
                                                           PSA_BufferedRead, TRUE,
                                                           PSA_NumPipes, NUMREADPIPES,
                                                           PSA_BufferSize, DEFREADBUFLEN,
                                                           PSA_AllowRuntPackets, TRUE,
                                                           PSA_DoNotWait, TRUE,
                                                           PSA_AbortSigMask, SIGBREAKF_CTRL_C,
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
        Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeSerial()" */
void nFreeSerial(struct NepClassSerial *ncp)
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

    psdCloseStream(ncp->ncp_EPInStream);
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
