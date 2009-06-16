/*
 *----------------------------------------------------------------------------
 *                         rawwrap class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "rawwrap.class.h"

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
    struct NepClassRawWrap *ncp;
    struct NepRawWrapBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct NepRawDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct NepRawDevBase), NULL)))
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
    struct NepClassRawWrap *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);

        ncp = (struct NepClassRawWrap *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp->ncp_CDC);
            FreeVec(ncp);
            ncp = (struct NepClassRawWrap *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! rawwrap.class expunged!\n\n"));
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
struct NepClassRawWrap * usbAttemptInterfaceBinding(struct NepRawWrapBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct ClsGlobalCfg *cgc;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    KPRINTF(1, ("nepRawWrapAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        cgc = &nh->nh_CurrentCGC;

        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    TAG_DONE);
        CloseLibrary(ps);
        if(cgc->cgc_BindAll)
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
        if(cgc->cgc_BindVendor && ((ifclass == 0xff) || (ifclass == 0)))
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassRawWrap * usbForceInterfaceBinding(struct NepRawWrapBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassRawWrap *ncp;
    struct NepClassRawWrap *tmpncp;
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

    KPRINTF(1, ("nepRawWrapAttemptInterfaceBinding(%08lx)\n", pif));
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
        ncp = (struct NepClassRawWrap *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if((ncp->ncp_UnitAltIfNum == altifnum) && (ncp->ncp_UnitIfNum == ifnum) &&
               (ncp->ncp_UnitProdID == prodid) && (ncp->ncp_UnitVendorID == vendid))
            {
                unitno = ncp->ncp_UnitNo;
                unitfound = TRUE;
                break;
            }
            ncp = (struct NepClassRawWrap *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(ncp = AllocVec(sizeof(struct NepClassRawWrap), MEMF_PUBLIC|MEMF_CLEAR)))
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

        if(!nLoadBindingConfig(ncp))
        {
            /* felix' defaults for epson scanners */
            if(vendid == 0x04b8)
            {
                switch(prodid)
                {
                    case 0x0101: // EPSON Perfection 636U and 636 Photo
                    case 0x0103: // EPSON Perfection 610
                    case 0x0104: // EPSON Perfection 1200U and 1200 Photo
                    case 0x0106: // EPSON Stylus Scan 2500
                    case 0x0107: // EPSON Expression 1600
                    case 0x010a: // EPSON Perfection 1640 SU and 1640 SU Photo
                    case 0x010b: // EPSON Perfection 1240 U
                    case 0x010c: // EPSON Perfection 640 U
                    case 0x010e: // EPSON Perfection 1680
                    case 0x0110: // EPSON Perfection 1650
                    case 0x011e: // EPSON Perfection 1660 (NEW IN V1.2!!)
                    case 0x011b: // EPSON Perfection 2400 (NEW IN V1.2!!)
                    case 0x0112: // EPSON Perfection 2450 - GT-9700 for the Japanese market
                        cdc = ncp->ncp_CDC;
                        cdc->cdc_InNakTimeout = 200;
                        cdc->cdc_OutNakTimeout = 200;
                        cdc->cdc_InBufferMode = BUFMODE_NO;
                        cdc->cdc_InBufferSize = 1;
                        cdc->cdc_ShortReadTerm = TRUE;
                        cdc->cdc_UnitExclusive = FALSE;
                        break;
                }
            }
        }

        /* Find next free unit number */
        if(unitno == (ULONG) -1)
        {
            unitno = ncp->ncp_CDC->cdc_DefaultUnit;
            tmpncp = (struct NepClassRawWrap *) nh->nh_Units.lh_Head;
            while(tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                if(tmpncp->ncp_UnitNo == unitno)
                {
                    unitno++;
                    tmpncp = (struct NepClassRawWrap *) nh->nh_Units.lh_Head;
                } else {
                    tmpncp = (struct NepClassRawWrap *) tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
                }
            }
        }
        ncp->ncp_UnitNo = unitno;
        Permit();

        psdSafeRawDoFmt(buf, 64, "rawwrap.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nRawWrapTask, ncp)))
        {
            psdBorrowLocksWait(tmptask, 1UL<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                //FreeSignal(ncp->ncp_ReadySignal);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Bold and raw dude '%s' fixed to %s unit %ld!",
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
void usbReleaseInterfaceBinding(struct NepRawWrapBase *nh, struct NepClassRawWrap *ncp)
{
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("nepRawWrapReleaseInterfaceBinding(%08lx)\n", ncp));
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
                       "'%s' was wrapped into a bodybag.",
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

    KPRINTF(1, ("nepRawWrapGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case UGA_CLASS:
             if((ti = FindTagItem(UCCA_Priority, tags)))
             {
                 *((SIPTR *) ti->ti_Data) = -110;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Simple USB bulk IO wrapper via usbraw.device";
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
                 *((IPTR *) ti->ti_Data) = ((struct NepClassRawWrap *) usbstruct)->ncp_UsingDefaultCfg;
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

    struct NepClassRawWrap *ncp;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassRawWrap *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenBindingCfgWindow(nh, &nh->nh_DummyNCP));

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassRawWrap *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            Forbid();
            ncp = (struct NepClassRawWrap *) nh->nh_Units.lh_Head;
            while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                nLoadBindingConfig(ncp);
                ncp = (struct NepClassRawWrap *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
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
BOOL nLoadClassConfig(struct NepRawWrapBase *nh)
{
    struct NepClassRawWrap *ncp = &nh->nh_DummyNCP;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct ClsGlobalCfg *cgc;
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
    /* Create global config */
    nh->nh_CurrentCGC.cgc_ChunkID = AROS_LONG2BE(MAKE_ID('R','W','R','G'));
    nh->nh_CurrentCGC.cgc_Length = AROS_LONG2BE(sizeof(struct ClsGlobalCfg)-8);
    nh->nh_CurrentCGC.cgc_BindVendor = FALSE;
    nh->nh_CurrentCGC.cgc_BindAll = FALSE;
    /* Create default config */
    cdc = ncp->ncp_CDC;
    cdc->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('R','W','R','P'));
    cdc->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    cdc->cdc_InNakTimeout = 300;
    cdc->cdc_OutNakTimeout = 300;
    cdc->cdc_InBufferMode = BUFMODE_READONREQ;
    cdc->cdc_InBufferSize = 2;
    cdc->cdc_ShortReadTerm = TRUE;
    cdc->cdc_DefaultUnit = 0;
    cdc->cdc_UnitExclusive = FALSE;
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
        cgc = psdGetCfgChunk(pic, AROS_LONG2BE(nh->nh_CurrentCGC.cgc_ChunkID));
        if(cgc)
        {
            CopyMem(((UBYTE *) cgc) + 8, ((UBYTE *) &nh->nh_CurrentCGC) + 8, min(AROS_LONG2BE(cgc->cgc_Length), AROS_LONG2BE(nh->nh_CurrentCGC.cgc_Length)));
            psdFreeVec(cgc);
            ncp->ncp_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nLoadBindingConfig()" */
BOOL nLoadBindingConfig(struct NepClassRawWrap *ncp)
{
    struct NepRawWrapBase *nh = ncp->ncp_ClsBase;
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
LONG nOpenBindingCfgWindow(struct NepRawWrapBase *nh, struct NepClassRawWrap *ncp)
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

/* /// "nRawWrapTask()" */
AROS_UFH0(void, nRawWrapTask)
{
    AROS_USERFUNC_INIT

    struct NepClassRawWrap *ncp;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    ULONG len;
    struct IOStdReq *ioreq;
    struct IOStdReq *ioreq2;

    if((ncp = nAllocRawWrap()))
    {
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
        }
        Permit();

        /* Main task */
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
                    psdAbortPipe(ncp->ncp_EPOutPipe);
                }
            }

            while((pp = (struct PsdPipe *) GetMsg(ncp->ncp_TaskMsgPort)))
            {
                KPRINTF(1, ("Pipe back %08lx\n", pp));
                if(pp == ncp->ncp_EPOutPipe)
                {
                    if((ioreq = ncp->ncp_WritePending))
                    {
                        ioerr = psdGetPipeError(pp);
                        ioreq->io_Actual = psdGetPipeActual(pp);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "RawWrap transmit failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            ioreq->io_Error = (ioerr > 0) ? IOERR_BADLENGTH : ioerr;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        ncp->ncp_WritePending = NULL;
                    }
                }
                else if(pp == ncp->ncp_EPInPipe)
                {
                    ncp->ncp_ShortPktRead = FALSE;
                    switch(ncp->ncp_CDC->cdc_InBufferMode)
                    {
                        case BUFMODE_NO:
                            if((ioreq = ncp->ncp_ReadPending))
                            {
                                ioerr = psdGetPipeError(pp);
                                ioreq->io_Actual = psdGetPipeActual(pp);
                                if(ioerr)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                   "RawWrap receive failed: %s (%ld)",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    ioreq->io_Error = (ioerr > 0) ? IOERR_BADLENGTH : ioerr;
                                }
                                ReplyMsg((struct Message *) ioreq);
                            }
                            break;

                        case BUFMODE_READAHEAD:
                        case BUFMODE_READONREQ:
                            ioerr = psdGetPipeError(pp);
                            len = psdGetPipeActual(pp);
                            ioreq = ncp->ncp_ReadPending;
                            KPRINTF(1, ("IO=%08lx, err=%ld, len=%ld\n", ioreq, ioerr, len));
                            if(ioerr)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "RawWrap receive failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                if(ioreq)
                                {
                                    if((ULONG) ioreq == (ULONG) -1)
                                    {
                                        ncp->ncp_ShortPktRead = TRUE;
                                    } else {
                                        ioreq->io_Actual += len;
                                        ioreq->io_Error = (ioerr > 0) ? IOERR_BADLENGTH : ioerr;
                                        Remove((struct Node *) ioreq);
                                        ReplyMsg((struct Message *) ioreq);
                                    }
                                }
                                break;
                            }
                            ncp->ncp_ShortPktRead = len & (ncp->ncp_EPInMaxPktSize-1);
                            if(!len)
                            {
                                ncp->ncp_ShortPktRead = TRUE;
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "RawWrap received null bytes packet.");
                            }
                            
                            if(ioreq)
                            {
                                if((ULONG) ioreq == (ULONG) -1)
                                {
                                    if(len)
                                    {
                                        ncp->ncp_ReadBuffer[ncp->ncp_RBufWOffset+len] = 0;
                                        KPRINTF(1, ("Received %ld bytes...('%s')\n", len, &ncp->ncp_ReadBuffer[ncp->ncp_RBufWOffset]));
                                        ncp->ncp_RBufFull += len;
                                        ncp->ncp_RBufWOffset += len;

                                        if(ncp->ncp_RBufWOffset + READBUFCHUNK >= ncp->ncp_RBufSize)
                                        {
                                            KPRINTF(5, ("Wrapping read buffer\n"));
                                            ncp->ncp_RBufWrap = ncp->ncp_RBufWOffset;
                                            ncp->ncp_RBufWOffset = 0;
                                        }
                                        if(ncp->ncp_RBufFull > ncp->ncp_RBufWrap)
                                        {
                                            KPRINTF(5, ("Read buffer overflow by %ld bytes\n",
                                                        ncp->ncp_RBufFull - ncp->ncp_RBufWrap));
                                            ncp->ncp_RBufFull = ncp->ncp_RBufWrap;
                                        }
                                    }
                                } else {
                                    ioreq->io_Actual += len;
                                }
                            }
                            break;
                    }
                    ncp->ncp_ReadPending = NULL;
                }
            }
            if((ncp->ncp_CDC->cdc_InBufferMode == BUFMODE_READAHEAD) &&
               (!(ncp->ncp_DenyRequests || ncp->ncp_ReadPending)))
            {
                psdSendPipe(ncp->ncp_EPInPipe, &ncp->ncp_ReadBuffer[ncp->ncp_RBufWOffset], READBUFCHUNK);
                ncp->ncp_ReadPending = (struct IOStdReq *) -1;
            }

            while((ioreq = (struct IOStdReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->io_Command, ioreq->io_Length));
                switch(ioreq->io_Command)
                {
                    case CMD_READ:
                        ioreq->io_Actual = 0;
                        Forbid();
                        AddTail(&ncp->ncp_ReadQueue, &ioreq->io_Message.mn_Node);
                        Permit();
                        break;

                    case CMD_WRITE:
                        ioreq->io_Actual = 0;
                        if(ioreq->io_Length == -1)
                        {
                            ioreq->io_Length = strlen(ioreq->io_Data);
                        }
                        Forbid();
                        AddTail(&ncp->ncp_WriteQueue, &ioreq->io_Message.mn_Node);
                        Permit();
                        break;

                    case CMD_CLEAR:
                        ncp->ncp_RBufWrap = ncp->ncp_RBufSize;
                        ncp->ncp_RBufWOffset = 0;
                        ncp->ncp_RBufROffset = 0;
                        ncp->ncp_RBufFull = 0;
                        if(ncp->ncp_ReadPending)
                        {
                            psdAbortPipe(ncp->ncp_EPInPipe);
                        }
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                        if((ioreq2 = ncp->ncp_WritePending))
                        {
                            psdAbortPipe(ncp->ncp_EPOutPipe);
                            psdWaitPipe(ncp->ncp_EPOutPipe);
                            ncp->ncp_WritePending = NULL;
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
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
                        if((ioreq2 = ncp->ncp_ReadPending))
                        {
                            psdAbortPipe(ncp->ncp_EPInPipe);
                            psdWaitPipe(ncp->ncp_EPInPipe);
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ncp->ncp_ReadPending = NULL;
                        }
                        //ncp->ncp_WritePending = NULL;
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            switch(ncp->ncp_CDC->cdc_InBufferMode)
            {
                case BUFMODE_NO:
                    ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
                    if((!ncp->ncp_ReadPending) && ioreq->io_Message.mn_Node.ln_Succ)
                    {
                        Remove((struct Node *) ioreq);
                        ncp->ncp_ReadPending = ioreq;
                        psdSendPipe(ncp->ncp_EPInPipe, ioreq->io_Data, ioreq->io_Length);
                    }
                    break;

                case BUFMODE_READAHEAD:
                case BUFMODE_READONREQ:
                    ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
                    while(ioreq->io_Message.mn_Node.ln_Succ)
                    {
                        if(ncp->ncp_RBufFull)
                        {
                            len = ioreq->io_Length - ioreq->io_Actual;
                            if(len > ncp->ncp_RBufFull)
                            {
                                len = ncp->ncp_RBufFull;
                            }
                            if(ncp->ncp_RBufROffset+len < ncp->ncp_RBufWrap)
                            {
                                /* No wraparound */
                                KPRINTF(1, ("NoWrapCopy %ld (%ld/%ld)\n", len, ncp->ncp_RBufROffset, ncp->ncp_RBufWrap));
                                CopyMem(&ncp->ncp_ReadBuffer[ncp->ncp_RBufROffset],
                                        &((UBYTE *)ioreq->io_Data)[ioreq->io_Actual],
                                        len);
                                ncp->ncp_RBufROffset += len;
                                ncp->ncp_RBufFull -= len;
                                ioreq->io_Actual += len;
                            } else {
                                /* Wrap around */
                                ULONG chunk1 = ncp->ncp_RBufWrap - ncp->ncp_RBufROffset;
                                KPRINTF(1, ("WrapCopy1 %ld (%ld/%ld)\n", chunk1, ncp->ncp_RBufROffset, ncp->ncp_RBufWrap));
                                ncp->ncp_RBufFull -= len;
                                CopyMem(&ncp->ncp_ReadBuffer[ncp->ncp_RBufROffset],
                                        &((UBYTE *)ioreq->io_Data)[ioreq->io_Actual],
                                        chunk1);
                                ioreq->io_Actual += chunk1;

                                len -= chunk1;
                                if(len)
                                {
                                    KPRINTF(1, ("WrapCopy2 %ld\n", len));
                                    CopyMem(ncp->ncp_ReadBuffer,
                                            &((UBYTE *)ioreq->io_Data)[ioreq->io_Actual],
                                             len);
                                    ioreq->io_Actual += len;
                                }
                                ncp->ncp_RBufROffset = len;
                            }
                        }
                        len = ioreq->io_Length - ioreq->io_Actual;
                        if((!len) || (ncp->ncp_ShortPktRead && ncp->ncp_CDC->cdc_ShortReadTerm))
                        {
                            if(len)
                            {
                                KPRINTF(1, ("Short Packet reply, %ld less bytes\n", len));
                            } else {
                                KPRINTF(1, ("IO ready reply\n"));
                            }
                            Remove((struct Node *) ioreq);
                            ReplyMsg((struct Message *) ioreq);
                            ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
                            ncp->ncp_ShortPktRead = FALSE;
                        } else {
                            if(ncp->ncp_CDC->cdc_InBufferMode == BUFMODE_READONREQ)
                            {
                                if(len >= ncp->ncp_EPInMaxPktSize)
                                {
                                    if(!ncp->ncp_ReadPending)
                                    {
                                        //Remove((struct Node *) ioreq);
                                        ncp->ncp_ReadPending = ioreq;
                                        psdSendPipe(ncp->ncp_EPInPipe, &((UBYTE *)ioreq->io_Data)[ioreq->io_Actual], len & (~(ncp->ncp_EPInMaxPktSize-1)));
                                    }
                                } else {
                                    psdSendPipe(ncp->ncp_EPInPipe, &ncp->ncp_ReadBuffer[ncp->ncp_RBufWOffset], ncp->ncp_EPInMaxPktSize);
                                    ncp->ncp_ReadPending = (struct IOStdReq *) -1;
                                }
                            }
                            break;
                        }
                    }
                    break;
            }
            ioreq = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
            if((!ncp->ncp_WritePending) && ioreq->io_Message.mn_Node.ln_Succ)
            {
                Remove((struct Node *) ioreq);
                ncp->ncp_WritePending = ioreq;
                psdSendPipe(ncp->ncp_EPOutPipe, ioreq->io_Data, ioreq->io_Length);
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
            psdAbortPipe(ncp->ncp_EPOutPipe);
            psdWaitPipe(ncp->ncp_EPOutPipe);
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ncp->ncp_WritePending = NULL;
        }
        if((ioreq = ncp->ncp_ReadPending))
        {
            KPRINTF(1, ("Aborting pending read...\n"));
            psdAbortPipe(ncp->ncp_EPInPipe);
            psdWaitPipe(ncp->ncp_EPInPipe);
            if((ULONG) ioreq != (ULONG) -1)
            {
                ioreq->io_Error = IOERR_ABORTED;
                ReplyMsg((struct Message *) ioreq);
            }
            ncp->ncp_ReadPending = NULL;
        }
        /* Read/Write queues */
        ioreq = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
        while(ioreq->io_Message.mn_Node.ln_Succ)
        {
            KPRINTF(1, ("Removing write request...\n"));
            Remove((struct Node *) ioreq);
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
        }
        ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
        while(ioreq->io_Message.mn_Node.ln_Succ)
        {
            KPRINTF(1, ("Removing read request...\n"));
            Remove((struct Node *) ioreq);
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
        }
        /* Command queue */
        while((ioreq = (struct IOStdReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
        {
            KPRINTF(1, ("Aborting pending requests...\n"));
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
        }
        Permit();
        KPRINTF(20, ("Going down the river!\n"));
        nFreeRawWrap(ncp);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocRawWrap()" */
struct NepClassRawWrap * nAllocRawWrap(void)
{
    struct Task *thistask;
    struct NepClassRawWrap *ncp;

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
                    EA_MaxPktSize, &ncp->ncp_EPInMaxPktSize,
                    TAG_END);

        ncp->ncp_ReadPending = NULL;
        ncp->ncp_WritePending = NULL;
        ncp->ncp_RBufWrap = ncp->ncp_RBufSize = ncp->ncp_CDC->cdc_InBufferSize*1024;
        if(!(ncp->ncp_ReadBuffer = AllocVec(ncp->ncp_RBufSize, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            KPRINTF(1, ("Out of memory for read buffer\n"));
            break;
        }
        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;
        ncp->ncp_RBufWOffset = 0;
        ncp->ncp_RBufROffset = 0;
        ncp->ncp_RBufFull = 0;

        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_EP0Pipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL)))
            {
                if((ncp->ncp_EPOutPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPOut)))
                {
                    /* Turn off short packets */
                    psdSetAttrs(PGA_PIPE, ncp->ncp_EPOutPipe,
                                PPA_NoShortPackets, TRUE,
                                PPA_NakTimeout, ncp->ncp_CDC->cdc_OutNakTimeout ? TRUE : FALSE,
                                PPA_NakTimeoutTime, ncp->ncp_CDC->cdc_OutNakTimeout*100,
                                TAG_END);
                    if((ncp->ncp_EPInPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPIn)))
                    {
                        /* Turn off short packets */
                        psdSetAttrs(PGA_PIPE, ncp->ncp_EPInPipe,
                                    PPA_NakTimeout, ncp->ncp_CDC->cdc_InNakTimeout ? TRUE : FALSE,
                                    PPA_NakTimeoutTime, ncp->ncp_CDC->cdc_InNakTimeout*100,
                                    PPA_AllowRuntPackets, TRUE,
                                    TAG_END);
                        ncp->ncp_Task = thistask;
                        return(ncp);
                    }
                    psdFreePipe(ncp->ncp_EPOutPipe);
                }
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

/* /// "nFreeRawWrap()" */
void nFreeRawWrap(struct NepClassRawWrap *ncp)
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

    psdFreePipe(ncp->ncp_EPInPipe);
    psdFreePipe(ncp->ncp_EPOutPipe);
    psdFreePipe(ncp->ncp_EP0Pipe);

    if(ncp->ncp_ReadBuffer)
    {
        FreeVec(ncp->ncp_ReadBuffer);
        ncp->ncp_ReadBuffer = NULL;
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

/**************************************************************************/

static char *BufferModeStrings[] = { "No Buffering", "Readahead", "Read on request", NULL };
static char *MainGUIPages[] = { "Global", "Interface", NULL };
static char *MainGUIPagesDefault[] = { "Global", "Default Interface", NULL };

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepRawWrapBase *nh;
    struct NepClassRawWrap *ncp;
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
        MUIA_Application_Copyright  , "©2002-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the rawwrap.class",
        MUIA_Application_Base       , "RAWWRAP",
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
                    MUIA_Menuitem_Title, "Set as Default",
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
                Child, RegisterGroup(ncp->ncp_Interface ? MainGUIPages : MainGUIPagesDefault),
                    MUIA_CycleChain, 1,
                    MUIA_Register_Frame, ncp->ncp_Interface ? FALSE : TRUE,
                    MUIA_Group_ActivePage, ncp->ncp_Interface ? 1 : 0,
                    Child, HGroup, GroupFrameT("Global Settings"),
                        MUIA_Disabled, ncp->ncp_Interface ? TRUE : FALSE,
                        Child, HSpace(0),
                        Child, ColGroup(2),
                            Child, Label((ULONG) "Bind to Vendor/Unknown Interfaces:"),
                            Child, HGroup,
                                Child, ncp->ncp_BindVendorObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nh->nh_CurrentCGC.cgc_BindVendor,
                                    MUIA_ShowSelState, FALSE,
                                    End,
                                Child, HSpace(0),
                                End,
                            Child, Label((ULONG) "Bind to all interfaces:"),
                            Child, HGroup,
                                Child, ncp->ncp_BindAllObj = ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, nh->nh_CurrentCGC.cgc_BindAll,
                                    MUIA_ShowSelState, FALSE,
                                    End,
                                Child, HSpace(0),
                                End,
                            End,
                        Child, HSpace(0),
                        End,
                    Child, ColGroup(2), GroupFrameT(ncp->ncp_Interface ? "Device Settings" : "Default Device Settings"),
                        //Child, HSpace(0),
                        Child, Label((ULONG) "Default " DEVNAME " Unit:"),
                        Child, HGroup,
                            Child, ncp->ncp_UnitObj = StringObject,
                                StringFrame,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Integer, ncp->ncp_CDC->cdc_DefaultUnit,
                                MUIA_String_Accept, "0123456789",
                                End,
                            Child, HSpace(0),
                            Child, Label((ULONG) "Exclusive access:"),
                            Child, ncp->ncp_UnitExclObj = ImageObject, ImageButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_Toggle,
                                MUIA_Image_Spec, MUII_CheckMark,
                                MUIA_Image_FreeVert, TRUE,
                                MUIA_Selected, ncp->ncp_CDC->cdc_UnitExclusive,
                                MUIA_ShowSelState, FALSE,
                                End,
                            End,
                        Child, Label((ULONG) "Out NAK Timeout:"),
                        Child, ncp->ncp_OutNakTimeoutObj = SliderObject, SliderFrame,
                            MUIA_CycleChain, 1,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 600,
                            MUIA_Numeric_Value, ncp->ncp_CDC->cdc_OutNakTimeout,
                            MUIA_Numeric_Format, "%ld00ms",
                            End,
                        Child, Label((ULONG) "In NAK Timeout:"),
                        Child, ncp->ncp_InNakTimeoutObj = SliderObject, SliderFrame,
                            MUIA_CycleChain, 1,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 600,
                            MUIA_Numeric_Value, ncp->ncp_CDC->cdc_InNakTimeout,
                            MUIA_Numeric_Format, "%ld00ms",
                            End,
                        Child, Label((ULONG) "In Buffer Mode:"),
                        Child, HGroup,
                            Child, ncp->ncp_InBufferModeObj = CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, BufferModeStrings,
                                MUIA_Cycle_Active, ncp->ncp_CDC->cdc_InBufferMode,
                                End,
                            Child, Label((ULONG) "Buffer Size:"),
                            Child, ncp->ncp_InBufferSizeObj = SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 1,
                                MUIA_Numeric_Max, 512,
                                MUIA_Numeric_Value, ncp->ncp_CDC->cdc_InBufferSize,
                                MUIA_Numeric_Format, "%ldKB",
                                End,
                            End,
                        Child, Label((ULONG) "Short Reads Terminate:"),
                        Child, HGroup,
                            Child, ncp->ncp_ShortReadTermObj = ImageObject, ImageButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_Toggle,
                                MUIA_Image_Spec, MUII_CheckMark,
                                MUIA_Image_FreeVert, TRUE,
                                MUIA_Selected, ncp->ncp_CDC->cdc_ShortReadTerm,
                                MUIA_ShowSelState, FALSE,
                                End,
                            Child, HSpace(0),
                            End,
                        End,
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
                {
                    if(!ncp->ncp_Interface)
                    {
                        get(ncp->ncp_BindVendorObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_BindVendor);
                        get(ncp->ncp_BindAllObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_BindAll);
                    }
                    get(ncp->ncp_UnitObj, MUIA_String_Integer, &ncp->ncp_CDC->cdc_DefaultUnit);
                    get(ncp->ncp_UnitExclObj, MUIA_Selected, &ncp->ncp_CDC->cdc_UnitExclusive);
                    get(ncp->ncp_OutNakTimeoutObj, MUIA_Numeric_Value, &ncp->ncp_CDC->cdc_OutNakTimeout);
                    get(ncp->ncp_InNakTimeoutObj, MUIA_Numeric_Value, &ncp->ncp_CDC->cdc_InNakTimeout);
                    get(ncp->ncp_InBufferSizeObj, MUIA_Numeric_Value, &ncp->ncp_CDC->cdc_InBufferSize);
                    get(ncp->ncp_InBufferModeObj, MUIA_Cycle_Active, &ncp->ncp_CDC->cdc_InBufferMode);
                    get(ncp->ncp_ShortReadTermObj, MUIA_Selected, &ncp->ncp_CDC->cdc_ShortReadTerm);

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
                            psdAddCfgEntry(pic, &nh->nh_CurrentCGC);
                            psdAddCfgEntry(pic, ncp->ncp_CDC);
                            psdSaveCfgToDisk(NULL, FALSE);
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
                }

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
void nGUITaskCleanup(struct NepClassRawWrap *ncp)
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
