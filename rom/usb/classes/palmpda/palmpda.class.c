/*
 *----------------------------------------------------------------------------
 *                         palmpda class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "palmpda.class.h"

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
            /*NewList(&nh->nh_DummyUnit.unit_MsgPort.mp_MsgList);
            nh->nh_DummyUnit.unit_MsgPort.mp_SigBit = 0;
            nh->nh_DummyUnit.unit_MsgPort.mp_SigTask = NULL;
            nh->nh_DummyUnit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
            nh->nh_DummyUnit.unit_MsgPort.mp_Flags = PA_IGNORE;*/

            Forbid();
            AddDevice((struct Device *) nh->nh_DevBase);
            nh->nh_DevBase->np_Library.lib_OpenCnt++;
            Permit();
            ret = nh;
        } else {
            KPRINTF(20, ("failed to create palmpda.device\n"));
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

        KPRINTF(5, ("libExpunge: Unloading done! palmpda.class expunged!\n\n"));
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
    { HANDSPRING_VENDOR_ID, HANDSPRING_VISOR_ID },
    { HANDSPRING_VENDOR_ID, HANDSPRING_TREO_ID },
    { HANDSPRING_VENDOR_ID, HANDSPRING_TREO600_ID },
    { PALM_VENDOR_ID      , PALM_M500_ID },
    { PALM_VENDOR_ID      , PALM_M505_ID },
    { PALM_VENDOR_ID      , PALM_M515_ID },
    { PALM_VENDOR_ID      , PALM_I705_ID },
    { PALM_VENDOR_ID      , PALM_M100_ID },
    { PALM_VENDOR_ID      , PALM_M125_ID },
    { PALM_VENDOR_ID      , PALM_M130_ID },
    { PALM_VENDOR_ID      , PALM_TUNGSTEN_T_ID },
    { PALM_VENDOR_ID      , PALM_TUNGSTEN_Z_ID },
    { PALM_VENDOR_ID      , PALM_ZIRE31_ID },
    { PALM_VENDOR_ID      , PALM_ZIRE_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_3_5_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_4_0_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_S360_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_4_1_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_NX60_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_NZ90V_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_UX50_ID },
    { SONY_VENDOR_ID      , SONY_CLIE_TJ25_ID },
    { SAMSUNG_VENDOR_ID   , SAMSUNG_SCH_I330_ID },
    { SAMSUNG_VENDOR_ID   , SAMSUNG_SPH_I500_ID },
    { GARMIN_VENDOR_ID    , GARMIN_IQUE_3600_ID },
    { ACEECA_VENDOR_ID    , ACEECA_MEZ1000_ID },
    { KYOCERA_VENDOR_ID   , KYOCERA_7135_ID },
    { FOSSIL_VENDOR_ID    , FOSSIL_ABACUS_ID },
    { ZODIAC_VENDOR_ID    , ZODIAC_ZODIAC_ID },
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

        nLoadClassConfig(nh);
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
        ncp->ncp_DevBase = nh->nh_DevBase;
        ncp->ncp_UnitNo = unitno;
        ncp->ncp_Device = pd;
        ncp->ncp_UnitProdID = prodid;
        ncp->ncp_UnitVendorID = vendid;
        Permit();

        psdSafeRawDoFmt(buf, 64, "palmpda.class<%08lx>", ncp);
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
                               "This '%s' PDA is hot at %s unit %ld!",
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
                       "'%s' annealed and broke off.",
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

    KPRINTF(1, ("nepSerialGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "Palm PDA HotSync via usbpalm.device";
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
        case UCM_AttemptDeviceBinding:
            return((IPTR) usbAttemptDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ForceDeviceBinding:
            return((IPTR) usbForceDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ReleaseDeviceBinding:
            usbReleaseDeviceBinding(nh, (struct NepClassSerial *) methoddata[0]);
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
BOOL nLoadClassConfig(struct NepSerialBase *nh)
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
    nh->nh_CurrentCGC.cgc_ChunkID = AROS_LONG2BE(MAKE_ID('P','A','L','M'));
    nh->nh_CurrentCGC.cgc_Length = AROS_LONG2BE(sizeof(struct ClsGlobalCfg)-8);
    nh->nh_CurrentCGC.cgc_ShellStack = 8192;
    strcpy(nh->nh_CurrentCGC.cgc_ShellCon, "CON:///130/Palm HotSync Launcher/CLOSE/AUTO/WAIT");
    strcpy(nh->nh_CurrentCGC.cgc_InhibitTask, "SpitfireHSM");
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
LONG nOpenCfgWindow(struct NepSerialBase *nh)
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

/* /// "nSetSerialMode()" */
void nSetSerialMode(struct NepClassSerial *ncp, struct IOExtSer *ioreq)
{
    ncp->ncp_IsConfigured = TRUE;
}
/* \\\ */

/* /// "nSerialTask()" */
AROS_UFH0(void, nSerialTask)
{
    AROS_USERFUNC_INIT

    struct NepClassSerial *ncp;
    struct NepSerialBase *nh;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    IPTR pending;
    struct IOExtSer *ioreq;
    struct IOExtSer *ioreq2;

    if((ncp = nAllocSerial()))
    {
        KPRINTF(1, ("nSerialTask()\n"));
        Forbid();
        if(ncp->ncp_ReadySigTask)
        {
            Signal(ncp->ncp_ReadySigTask, 1UL<<ncp->ncp_ReadySignal);
        }
        Permit();
        nh = ncp->ncp_DevBase->np_ClsBase;

        /* Main task */
        sigmask = (1UL<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit)|(1UL<<ncp->ncp_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        {
            struct MsgPort *tmpmp = NULL;
            psdGetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream, PSA_MessagePort, &tmpmp, TAG_END);
            if(tmpmp)
            {
                sigmask |= (1UL<<tmpmp->mp_SigBit);
            }
        }
        /* check, if we need to execute a command? */
        if(*nh->nh_CurrentCGC.cgc_Command)
        {
            BOOL launch = TRUE;
            if(nh->nh_CurrentCGC.cgc_InhibitTask)
            {
                if(FindTask(nh->nh_CurrentCGC.cgc_InhibitTask))
                {
                    launch = FALSE;
                }
            }
            if(launch)
            {
                struct Library *dosbase;
                if((dosbase = OpenLibrary("dos.library", 39)))
                {
                    BPTR fhandle;
                    LONG ioerr;
#define DOSBase dosbase
                    if((fhandle = Open(nh->nh_CurrentCGC.cgc_ShellCon, MODE_READWRITE)))
                    {
                        ioerr = SystemTags(nh->nh_CurrentCGC.cgc_Command,
                                           SYS_Input, fhandle,
                                           SYS_Output, NULL,
                                           SYS_Asynch, TRUE,
                                           NP_StackSize, nh->nh_CurrentCGC.cgc_ShellStack,
                                           TAG_END);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                                           "Execution of '%s' failed (%ld).",
                                           nh->nh_CurrentCGC.cgc_Command, ioerr);
                            Close(fhandle);
                        } else {
                            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                           "Launching '%s'...",
                                           nh->nh_CurrentCGC.cgc_Command);
                        }
                    } else {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                                       "Console '%s' could not be opened!",
                                       nh->nh_CurrentCGC.cgc_ShellCon);
                    }
                    CloseLibrary(dosbase);
#undef DOSBase
                }
            }
        }
        //nServerRitual(ncp);
        do
        {
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

                    case SDCMD_SETPARAMS:
                        nSetSerialMode(ncp, ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case SDCMD_QUERY:
                        ioreq->io_Status = (1<<5);
                        pending = 0;
                        psdGetAttrs(PGA_PIPESTREAM, ncp->ncp_EPInStream,
                                    PSA_BytesPending, &pending,
                                    TAG_END);
                        ioreq->IOSer.io_Actual = pending;
                        if(!ioreq->IOSer.io_Actual)
                        {
                            ioreq->IOSer.io_Actual = 1;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_INVALID:
                        psdStreamFlush(ncp->ncp_EPInStream);
                        psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_ENDPOINT,
                                     UPR_CLOSE_NOTIFICATION, 0, 0);
                        ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "CLOSE_NOTIFICATION failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
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
                Forbid();
                ioreq = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
                while(ioreq->IOSer.io_Message.mn_Node.ln_Succ)
                {
                    if(!ncp->ncp_IsConfigured)
                    {
                        Permit();
                        nSetSerialMode(ncp, ioreq);
                        Forbid();
                    }
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
                    if(!ncp->ncp_IsConfigured)
                    {
                        Permit();
                        nSetSerialMode(ncp, ioreq);
                        Forbid();
                    }
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
    LONG ioerr;
    ULONG cnt;
    UWORD dummy;
    
    thistask = FindTask(NULL);
    ncp = thistask->tc_UserData;
    do
    {
        if(!(ncp->ncp_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        if(!(ncp->ncp_Interface = psdFindInterface(ncp->ncp_Device, NULL, TAG_END)))
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "No interface found?!?");
            break;
        }

        if((ncp->ncp_TaskMsgPort = CreateMsgPort()))
        {
            if((ncp->ncp_EP0Pipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL)))
            {
                psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_ENDPOINT,
                             UPR_GET_EXT_CONNECTION_INFORMATION, 0, 0);
                ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &ncp->ncp_ExtConnectInfo, sizeof(struct ExtConnectInfo));
                ncp->ncp_ExtConnectInfo.Streams[0].FctID = AROS_LONG2LE(ncp->ncp_ExtConnectInfo.Streams[0].FctID);
                ncp->ncp_ExtConnectInfo.Streams[1].FctID = AROS_LONG2LE(ncp->ncp_ExtConnectInfo.Streams[1].FctID);
                if(!ioerr)
                {
                    for(cnt = 0; cnt < ncp->ncp_ExtConnectInfo.NumPorts; cnt++)
                    {
                        switch(ncp->ncp_ExtConnectInfo.Streams[cnt].FctID)
                        {
                            case MAKE_ID('s','y','n','c'):
                                psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "There's that hotsync port (%ld) we've been looking for!", cnt+1);
                                ncp->ncp_HotsyncPort = cnt+1;
                                if(ncp->ncp_ExtConnectInfo.DiffEndPoints)
                                {
                                    ncp->ncp_EPInNum = (ncp->ncp_ExtConnectInfo.Streams[cnt].EPInfo >> 4) & 15;
                                    ncp->ncp_EPOutNum = ncp->ncp_ExtConnectInfo.Streams[cnt].EPInfo & 15;
                                } else {
                                    ncp->ncp_EPInNum = ncp->ncp_EPOutNum = ncp->ncp_ExtConnectInfo.Streams[cnt].Port;
                                }
                                break;

                            default:
                            {
                                char fctid[5];
                                memcpy(fctid, &ncp->ncp_ExtConnectInfo.Streams[cnt].FctID, 4);
                                fctid[4] = 0;
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Unknown function %s at PDA port %ld!", fctid, cnt+1);
                            }
                        }
                    }
                } else {
                    psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                                   "GET_EXT_CONNECTION_INFORMATION failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_ENDPOINT,
                                 UPR_GET_CONNECTION_INFORMATION, 0, 0);
                    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &ncp->ncp_ConnectInfo, sizeof(struct ConnectInfo));
                    ncp->ncp_ConnectInfo.NumPorts = AROS_WORD2LE(ncp->ncp_ConnectInfo.NumPorts);
                    if(!ioerr)
                    {
                        for(cnt = 0; cnt < ncp->ncp_ConnectInfo.NumPorts; cnt++)
                        {
                            switch(ncp->ncp_ConnectInfo.Streams[cnt].FctID)
                            {
                                case PALM_FUNCTION_GENERIC:
                                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "This PDA has a generic port (%ld)!", cnt+1);
                                    //ncp->ncp_HotsyncPort = cnt+1;
                                    break;

                                case PALM_FUNCTION_DEBUGGER:
                                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Uuuuh, your PDA has a debugging port (%ld) aswell!", cnt+1);
                                    break;

                                case PALM_FUNCTION_HOTSYNC:
                                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "There's that hotsync port (%ld) we've been looking for!", cnt+1);
                                    ncp->ncp_HotsyncPort = cnt+1;
                                    ncp->ncp_EPOutNum = cnt+1;
                                    ncp->ncp_EPInNum = cnt+1;
                                    break;

                                case PALM_FUNCTION_CONSOLE:
                                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Your PDA supplies a console port (%ld). Nice!", cnt+1);
                                    break;

                                case PALM_FUNCTION_REMOTE_FILE_SYS:
                                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Holy Cow! There's a remote file system port (%ld)!", cnt+1);
                                    break;

                                default:
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                   "Unknown function %02lx at PDA port %ld!",
                                                   ncp->ncp_ConnectInfo.Streams[cnt].FctID,
                                                   cnt+1);
                                    break;
                            }
                        }

                        psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_ENDPOINT,
                                     UPR_REQUEST_BYTES_AVAILABLE, 0, 5);
                        ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &dummy, 2);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                                           "REQUEST_BYTES_AVAILABLE failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        }
                    } else {
                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                                       "GET_CONNECTION_INFORMATION failed: %s (%ld)",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    }
                }
                if(ncp->ncp_HotsyncPort)
                {
                    ncp->ncp_EPIn = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                                    EA_EndpointNum, ncp->ncp_EPInNum,
                                                    EA_IsIn, TRUE,
                                                    EA_TransferType, USEAF_BULK,
                                                    TAG_END);
                    ncp->ncp_EPOut = psdFindEndpoint(ncp->ncp_Interface, NULL,
                                                     EA_EndpointNum, ncp->ncp_EPOutNum,
                                                     EA_IsIn, FALSE,
                                                     EA_TransferType, USEAF_BULK,
                                                     TAG_END);
                    if(ncp->ncp_EPIn && ncp->ncp_EPOut)
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
                                                                   PSA_ReadAhead, FALSE,
                                                                   PSA_BufferedRead, TRUE,
                                                                   PSA_NumPipes, NUMREADPIPES,
                                                                   PSA_BufferSize, DEFREADBUFLEN,
                                                                   PSA_AllowRuntPackets, TRUE,
                                                                   PSA_DoNotWait, FALSE,
                                                                   PSA_NakTimeout, TRUE,
                                                                   PSA_NakTimeoutTime, 5000,
                                                                   PSA_AbortSigMask, SIGBREAKF_CTRL_C,
                                                                   TAG_END)))
                            {
                                ncp->ncp_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
                                ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
                                ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
                                ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;
                                ncp->ncp_WritePending = NULL;
                                ncp->ncp_Task = thistask;
                                return(ncp);
                            }
                            psdCloseStream(ncp->ncp_EPOutStream);
                        }
                        FreeSignal(ncp->ncp_AbortSignal);
                    } else {
                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Unable to find endpoints for hotsync port!");
                    }
                } else {
                    psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Sorry, there's no hotsync port! Giving up!");
                }
                psdFreePipe(ncp->ncp_EP0Pipe);
            }
            DeleteMsgPort(ncp->ncp_TaskMsgPort);
        }
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

/**************************************************************************/

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepSerialBase *nh;
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
        MUIA_Application_Title      , (IPTR)libname,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)"©2004-2009 Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the palmpda.class",
        MUIA_Application_Base       , (IPTR)"PALMPDA",
        MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(nh->nh_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(nh->nh_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Save",
                    MUIA_Menuitem_Shortcut, (IPTR)"S",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(nh->nh_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            End,

        SubWindow, (IPTR)(nh->nh_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR)libname,
            MUIA_HelpNode, (IPTR)libname,

            WindowContents, (IPTR)VGroup,
/*                Child, actionobj = NewObject(ActionClass->mcc_Class, 0,
                    MUIA_ShowMe, FALSE,
                    End, */
                Child, (IPTR)ColGroup(2), GroupFrameT((IPTR)"Global Settings"),
                    Child, (IPTR)Label((IPTR) "Shell console window:"),
                    Child, (IPTR)(nh->nh_ConWindowObj = StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_Contents, (IPTR)nh->nh_CurrentCGC.cgc_ShellCon,
                        MUIA_String_MaxLen, 128,
                        End),
                    Child, (IPTR)Label((IPTR) "Shell default stack:"),
                    Child, (IPTR)(nh->nh_ShellStackObj = StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_Integer, nh->nh_CurrentCGC.cgc_ShellStack,
                        MUIA_String_Accept, (IPTR)"0123456789",
                        End),
                    Child, (IPTR)Label((IPTR) "Command:"),
                    Child, (IPTR)PopaslObject,
                        MUIA_Popstring_String, (IPTR)(nh->nh_ShellComObj = StringObject,
                            StringFrame,
                            MUIA_CycleChain, 1,
                            MUIA_String_AdvanceOnCR, TRUE,
                            MUIA_String_Contents, (IPTR)nh->nh_CurrentCGC.cgc_Command,
                            MUIA_String_MaxLen, 256,
                            End),
                        MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                        ASLFR_TitleText, (IPTR)"Select an executable...",
                        End,
                    Child, (IPTR)Label((IPTR) "Don't launch, when this task found:"),
                    Child, (IPTR)(nh->nh_InhibitTaskObj = StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                        MUIA_String_AdvanceOnCR, TRUE,
                        MUIA_String_Contents, (IPTR)nh->nh_CurrentCGC.cgc_InhibitTask,
                        MUIA_String_MaxLen, 64,
                        End),
                    End,
                Child, (IPTR)VSpace(0),
                Child, (IPTR)HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(nh->nh_UseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Save ",
                        End),
                    Child, (IPTR)(nh->nh_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Use ",
                        End),
                    End,
                End,
            End),
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
        IPTR  isopen = 0;
        IPTR  iconify = 0;
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
                {
                    STRPTR tmpstr = NULL;

                    get(nh->nh_ShellStackObj, MUIA_String_Integer, &nh->nh_CurrentCGC.cgc_ShellStack);
                    get(nh->nh_ConWindowObj, MUIA_String_Contents, &tmpstr);
                    strncpy(nh->nh_CurrentCGC.cgc_ShellCon, tmpstr, 127);
                    get(nh->nh_ShellComObj, MUIA_String_Contents, &tmpstr);
                    strncpy(nh->nh_CurrentCGC.cgc_Command, tmpstr, 255);
                    get(nh->nh_InhibitTaskObj, MUIA_String_Contents, &tmpstr);
                    strncpy(nh->nh_CurrentCGC.cgc_InhibitTask, tmpstr, 63);
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
               }
               case ID_ABOUT:
                    MUI_RequestA(nh->nh_App, nh->nh_MainWindow, 0, NULL, "Hot stuff!", VERSION_STRING, NULL);
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
void nGUITaskCleanup(struct NepSerialBase *nh)
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
