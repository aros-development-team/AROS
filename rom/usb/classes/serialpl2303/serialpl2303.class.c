/*
 *----------------------------------------------------------------------------
 *                         serialpl2303 class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "serialpl2303.class.h"

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
            KPRINTF(20, ("failed to create serialpl2303.device\n"));
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

        KPRINTF(5, ("libExpunge: Unloading done! serialpl2303.class expunged!\n\n"));
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
    { PL2303_VENDOR_ID      , PL2303_PRODUCT_ID },
    { PL2303_VENDOR_ID      , PL2303_PRODUCT_ID_RSAQ2 },
    { PL2303_VENDOR_ID      , PL2303_PRODUCT_ID_RSAQ3 },
    { PL2303_VENDOR_ID      , PL2303_PRODUCT_ID_DCU11 },
    { PL2303_VENDOR_ID      , PL2303_PRODUCT_ID_PHAROS },
    { PL2303_VENDOR_ID      , PL2303_PRODUCT_ID_ALDIGA },
    { ATEN_VENDOR_ID        , ATEN_PRODUCT_ID },
    { ATEN_VENDOR_ID2       , ATEN_PRODUCT_ID },
    { IODATA_VENDOR_ID      , IODATA_PRODUCT_ID },
    { IODATA_VENDOR_ID      , IODATA_PRODUCT_ID_RSAQ5 },
    { ELCOM_VENDOR_ID       , ELCOM_PRODUCT_ID },
    { ELCOM_VENDOR_ID       , ELCOM_PRODUCT_ID_UCSGT },
    { ITEGNO_VENDOR_ID      , ITEGNO_PRODUCT_ID },
    { ITEGNO_VENDOR_ID      , ITEGNO_PRODUCT_ID_2 },
    { MA620_VENDOR_ID       , MA620_PRODUCT_ID },
    { RATOC_VENDOR_ID       , RATOC_PRODUCT_ID },
    { RATOC_VENDOR_ID       , RATOC_PRODUCT_ID_USB60F },
    { TRIPP_VENDOR_ID       , TRIPP_PRODUCT_ID },
    { RADIOSHACK_VENDOR_ID  , RADIOSHACK_PRODUCT_ID },
    { DCU10_VENDOR_ID       , DCU10_PRODUCT_ID },
    { ALCATEL_VENDOR_ID     , ALCATEL_PRODUCT_ID },
    { SITECOM_VENDOR_ID     , SITECOM_PRODUCT_ID },
    { SAMSUNG_VENDOR_ID     , SAMSUNG_PRODUCT_ID },
    { SIEMENS_VENDOR_ID     , SIEMENS_PRODUCT_ID_SX1 },
    { SIEMENS_VENDOR_ID     , SIEMENS_PRODUCT_ID_X65 },
    { SIEMENS_VENDOR_ID     , SIEMENS_PRODUCT_ID_X75 },
    { SIEMENS_VENDOR_ID     , SIEMENS_PRODUCT_ID_EF81 },
    { SYNTECH_VENDOR_ID     , SYNTECH_PRODUCT_ID },
    { NOKIA_CA42_VENDOR_ID  , NOKIA_CA42_PRODUCT_ID },
    { CA_42_CA42_VENDOR_ID  , CA_42_CA42_PRODUCT_ID },
    { SAGEM_VENDOR_ID       , SAGEM_PRODUCT_ID },
    { LEADTEK_VENDOR_ID     , LEADTEK_9531_PRODUCT_ID },
    { SPEEDDRAGON_VENDOR_ID , SPEEDDRAGON_PRODUCT_ID },
    { OTI_VENDOR_ID         , OTI_PRODUCT_ID },
    { DATAPILOT_U2_VENDOR_ID, DATAPILOT_U2_PRODUCT_ID },
    { BELKIN_VENDOR_ID      , BELKIN_PRODUCT_ID },
    { ALCOR_VENDOR_ID       , ALCOR_PRODUCT_ID },
    { WS002IN_VENDOR_ID     , WS002IN_PRODUCT_ID },
    { COREGA_VENDOR_ID      , COREGA_PRODUCT_ID },
    { HL340_VENDOR_ID       , HL340_PRODUCT_ID },
    { YCCABLE_VENDOR_ID     , YCCABLE_PRODUCT_ID },
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

        psdSafeRawDoFmt(buf, 64, "serialpl2303.class<%08lx>", ncp);
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
                 *((STRPTR *) ti->ti_Data) = "PL2303 serial adapter via serialpl2303.device";
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

/* /// "nSetSerialMode()" */
void nSetSerialMode(struct NepClassSerial *ncp, struct IOExtSer *ioreq)
{
    struct UsbCDCLineCoding ulc;
    LONG ioerr;
    ULONG bufsize = ioreq->io_RBufLen / NUMREADPIPES;

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
    ulc.dwDTERate = AROS_LONG2LE(ioreq->io_Baud);
    if(ioreq->io_StopBits == 1)
    {
        ulc.bCharFormat = 0;
    }
    else if(ioreq->io_StopBits == 2)
    {
        ulc.bCharFormat = 2;
    } else {
        ioreq->IOSer.io_Error = SerErr_InvParam;
        ulc.bCharFormat = 0;
    }
    ulc.bParityType = 0; /* no parity */
    if(ioreq->io_SerFlags & SERF_PARTY_ON)
    {
        if(ioreq->io_SerFlags & SERF_PARTY_ODD)
        {
            ulc.bParityType = 1; /* odd parity */
        } else {
            ulc.bParityType = 2; /* even parity */
        }
    }
    if(ioreq->io_ExtFlags & SEXTF_MSPON)
    {
        ioreq->io_SerFlags |= SERF_PARTY_ON;
        if(ioreq->io_ExtFlags & SEXTF_MARK)
        {
            ulc.bParityType = 3; /* mark parity */
        } else {
            ulc.bParityType = 4; /* space parity */
        }
    }
    ulc.bDataBits = ioreq->io_ReadLen;
    if(ioreq->io_SerFlags & SERF_RAD_BOOGIE)
    {
        ulc.bCharFormat = 0; /* 1 stop bit */
        ulc.bParityType = 0; /* no parity */
        ulc.bDataBits = 8; /* 8 data bits */
    }

    if(ncp->ncp_SiemensCrap)
    {
        // the siemens thing supports none of these...
        return;
    }

    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                 UCDCR_SET_LINE_CODING, 0, 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &ulc, 7);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "SET_LINE_CODING failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        ioreq->IOSer.io_Error = SerErr_LineErr;
    }
    /* Activate RTS/CTS */
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                 UCDCR_SET_CONTROL_LINE_STATE, (ULONG) ((ioreq->io_SerFlags & SERF_7WIRE) ? 0 : UPLF_CONTROL_RTS|UPLF_CONTROL_DTR), 0);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "SET_CONTROL_LINE_STATE failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        ioreq->IOSer.io_Error = SerErr_LineErr;
    }
    if(ioreq->io_SerFlags & SERF_7WIRE) /* Activate RTS/CTS */
    {
        psdPipeSetup(ncp->ncp_EP0Pipe, URTF_VENDOR|URTF_DEVICE,
                     UPLR_WRITE, 0, (ULONG) (ncp->ncp_HXChipset ? 0x61 : 0x41));
        ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                           "SET_RTS/CTS failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            ioreq->IOSer.io_Error = SerErr_LineErr;
        }
    }
}
/* \\\ */

/* /// "nSerialTask()" */
AROS_UFH0(void, nSerialTask)
{
    AROS_USERFUNC_INIT

    struct NepClassSerial *ncp;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    LONG len;
    struct IOExtSer *ioreq;
    struct IOExtSer *ioreq2;
    UWORD serstate;
    IPTR pending;
    UBYTE buf[8];
    ULONG seqnr;
    UWORD *seqptr;
    UWORD initseq[] = { URTF_IN , 0x8484, 0,
                        URTF_OUT, 0x0404, 0,
                        URTF_IN , 0x8484, 0,
                        URTF_IN , 0x8383, 0,
                        URTF_IN , 0x8484, 0,
                        URTF_OUT, 0x0404, 1,
                        URTF_IN , 0x8484, 0,
                        URTF_IN , 0x8383, 0,
                        URTF_OUT, 0x0000, 1,
                        URTF_OUT, 0x0001, 0,
                        //URTF_OUT, 0x0001, 0xc0,
                        URTF_OUT, 0x0002, 0x24,
                        0xffff };

    UWORD inithxs[] = { URTF_IN , 0x8484, 0,
                        URTF_OUT, 0x0404, 0,
                        URTF_IN , 0x8484, 0,
                        URTF_IN , 0x8383, 0,
                        URTF_IN , 0x8484, 0,
                        URTF_OUT, 0x0404, 1,
                        URTF_IN , 0x8484, 0,
                        URTF_IN , 0x8383, 0,
                        URTF_OUT, 0x0000, 1,
                        URTF_OUT, 0x0002, 0x44,
                        URTF_OUT, 0x0008, 0,
                        URTF_OUT, 0x0009, 0,
                        0xffff };

    if((ncp = nAllocSerial()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
        }
        Permit();

        /* HX Chipset speciality: clear endpoint stalls */
        if(!ncp->ncp_HXChipset)
        {
            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncp->ncp_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncp->ncp_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            psdPipeSetup(ncp->ncp_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncp->ncp_EPOutNum);
            ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncp->ncp_EPOutNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            seqptr = initseq;
        } else {
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                           "Using HX chipset init sequence...");
            seqptr = inithxs;
        }

        pp = ncp->ncp_EP0Pipe;
        if(ncp->ncp_SiemensCrap)
        {
            /* The siemens thing is probably not even a PL2303 chipset, it needs
               very different initalisation! */
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                           "Using Siemens chipset init sequence...");
            psdPipeSetup(pp, URTF_OUT|URTF_VENDOR|URTF_INTERFACE, 0x02, 0, 0);
            ioerr = psdDoPipe(pp, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                               "Error during Siemens sequence cmd %ld: %s (%ld)",
                               1,
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            psdPipeSetup(pp, URTF_IN|URTF_VENDOR|URTF_INTERFACE, 0x01, 0, 0);
            ioerr = psdDoPipe(pp, buf, 1);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                               "Error during Siemens sequence cmd %ld: %s (%ld)",
                               2,
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }

            psdPipeSetup(pp, URTF_IN|URTF_VENDOR|URTF_INTERFACE, 0x04, 0x0800, 0);
            ioerr = psdDoPipe(pp, buf, 2);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                               "Error during Siemens sequence cmd %ld: %s (%ld)",
                               3,
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            psdSendPipe(ncp->ncp_EPIntPipe, ncp->ncp_SerialStateReq, 10);
        } else {
            /* Output init sequence */
            seqnr = 1;
            while(*seqptr != 0xffff)
            {
                UWORD dir, idx, val;
                dir = *seqptr++;
                val = *seqptr++;
                idx = *seqptr++;
                if(dir == URTF_IN)
                {
                    psdPipeSetup(pp, URTF_IN|URTF_VENDOR|URTF_DEVICE, UPLR_READ,
                                 (ULONG) val, (ULONG) idx);
                    ioerr = psdDoPipe(pp, buf, 1);
                } else {
                    psdPipeSetup(pp, URTF_OUT|URTF_VENDOR|URTF_DEVICE, UPLR_WRITE,
                                 (ULONG) val, (ULONG) idx);
                    ioerr = psdDoPipe(pp, NULL, 0);
                }
                if(ioerr)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                   "Error during init sequence cmd %ld: %s (%ld)",
                                   seqnr,
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
                seqnr++;
            }
            psdSendPipe(ncp->ncp_EPIntPipe, &ncp->ncp_SerialStateReq[UART_STATE], 1);
        }

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
            while((pp = (struct PsdPipe *) GetMsg(ncp->ncp_TaskMsgPort)))
            {
                KPRINTF(1, ("Pipe back %08lx\n", pp));
                if(pp == ncp->ncp_EPIntPipe)
                {
                    KPRINTF(5, ("Int serial state back %04lx\n", ncp->ncp_SerialStateReq[UART_STATE]));
                    ioerr = psdGetPipeError(pp);
                    len = psdGetPipeActual(pp);
                    if(ioerr && (ioerr != UHIOERR_RUNTPACKET))
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "Serial state failed: %s (%ld)",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        psdDelayMS(100);
                    }
                    if(ncp->ncp_SiemensCrap)
                    {
                        psdSendPipe(ncp->ncp_EPIntPipe, &ncp->ncp_SerialStateReq[UART_STATE], 1);
                    } else {
                        psdSendPipe(ncp->ncp_EPIntPipe, ncp->ncp_SerialStateReq, 10);
                    }
                    break;
                }
            }
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
                        if(!(ncp->ncp_SerialStateReq[UART_STATE] & UART_DCD)) /* Carrier Detect */
                            serstate |= (1<<5);
                        if(!(ncp->ncp_SerialStateReq[UART_STATE] & UART_DSR)) /* DSR */
                            serstate |= (1<<3);
                        if(!(ncp->ncp_SerialStateReq[UART_STATE] & UART_CTS)) /* ClearToSend */
                            serstate |= (1<<4);
                        if(ncp->ncp_SerialStateReq[UART_STATE] & UART_BREAK) /* Break */
                            serstate |= (1<<10);
                        if(ncp->ncp_SerialStateReq[UART_STATE] & UART_RING) /* Ring */
                            serstate |= (1<<2);
                        if(ncp->ncp_SerialStateReq[UART_STATE] & (UART_FRAME_ERROR|UART_PARITY_ERROR|UART_OVERRUN_ERROR)) /* Framing, Parity, Hardware error */
                            serstate |= (1<<8);

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

                    case SDCMD_BREAK:
                        psdPipeSetup(ncp->ncp_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                                     UCDCR_SEND_BREAK, ioreq->io_BrkTime, (ULONG) ncp->ncp_UnitIfNum);
                        ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "SEND_BREAK failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            ioreq->IOSer.io_Error = SerErr_LineErr;
                        }
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

        psdAbortPipe(ncp->ncp_EPIntPipe);
        psdWaitPipe(ncp->ncp_EPIntPipe);

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
    IPTR maxpkt0 = 0;
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

        ncp->ncp_EPInt = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                         EA_IsIn, TRUE,
                                         EA_TransferType, USEAF_INTERRUPT,
                                         TAG_END);
        if(!ncp->ncp_EPInt)
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "No notify endpoint?!?");
            KPRINTF(1, ("Ooops!?! No notify endpoint defined?\n"));
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

                /* Check for HX Chipset */
                psdGetAttrs(PGA_PIPE, ncp->ncp_EP0Pipe, PPA_MaxPktSize, &maxpkt0, TAG_END);
                if(maxpkt0 == 64)
                {
                    ncp->ncp_HXChipset = TRUE;
                }
                if(ncp->ncp_UnitVendorID == SIEMENS_VENDOR_ID)
                {
                    ncp->ncp_SiemensCrap = TRUE;
                }

                ncp->ncp_AbortSignal = AllocSignal(-1);
                if((ncp->ncp_EPOutStream = psdOpenStream(ncp->ncp_EPOut,
                                                        PSA_BufferedWrite, FALSE,
                                                        PSA_NoZeroPktTerm, FALSE,
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
                        if((ncp->ncp_EPIntPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPInt)))
                        {
                            ncp->ncp_Task = thistask;
                            return(ncp);
                        }
                        psdCloseStream(ncp->ncp_EPInStream);
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
    psdFreePipe(ncp->ncp_EPIntPipe);
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
