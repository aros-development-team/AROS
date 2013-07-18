/*
 *----------------------------------------------------------------------------
 *                         massstorage class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "massstorage.class.h"

#define DEF_NAKTIMEOUT  (100)

/* /// "Lib Stuff" */
static const STRPTR GM_UNIQUENAME(libname) = MOD_NAME_STRING;

static
const APTR GM_UNIQUENAME(DevFuncTable)[] =
{
    &AROS_SLIB_ENTRY(devOpen, usbscsidev, 1),
    &AROS_SLIB_ENTRY(devClose, usbscsidev, 2),
    &AROS_SLIB_ENTRY(devExpunge, usbscsidev, 3),
    &AROS_SLIB_ENTRY(devReserved, usbscsidev, 4),
    &AROS_SLIB_ENTRY(devBeginIO, usbscsidev, 5),
    &AROS_SLIB_ENTRY(devAbortIO, usbscsidev, 6),
    (APTR) -1,
};

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh)
{
    struct ClsDevCfg *cdc = NULL;
    struct ClsUnitCfg *cuc = NULL;
    struct NepClassMS *ncm;
    struct NepMSBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

#define	UtilityBase nh->nh_UtilityBase
    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);
    if(UtilityBase)
    {
        /* Initialize device node & library struct */
        KPRINTF(1, ("UtilityOkay\n"));
        NewList(&nh->nh_Units);
        InitSemaphore(&nh->nh_TaskLock);

        ncm = &nh->nh_DummyNCM;
        ncm->ncm_ClsBase = nh;
        ncm->ncm_Interface = NULL;
        strcpy(ncm->ncm_LUNIDStr, "Default");
        strcpy(ncm->ncm_LUNNumStr, "All");
        ncm->ncm_CDC = cdc = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
        ncm->ncm_CUC = cuc = AllocVec(sizeof(struct ClsUnitCfg), MEMF_PUBLIC|MEMF_CLEAR);
        if(cdc && cuc)
        {
            KPRINTF(1, ("MakeLibrary\n"));
            if((nh->nh_DevBase = (struct NepMSDevBase *) MakeLibrary((APTR) GM_UNIQUENAME(DevFuncTable), NULL, (APTR) GM_UNIQUENAME(devInit),
               sizeof(struct NepMSDevBase), NULL)))
            {
                KPRINTF(1,("AddDevice\n"));
                nh->nh_DevBase->np_ClsBase = nh;
                Forbid();
                AddDevice((struct Device *) nh->nh_DevBase);
                nh->nh_DevBase->np_Library.lib_OpenCnt++;
                Permit();
                ret = nh;
            } else {
                KPRINTF(20, ("failed to create usbscsi.device\n"));
            }
        }
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }
    if(!ret)
    {
        FreeVec(cdc);
        FreeVec(cuc);
        CloseLibrary(UtilityBase);
    }
    KPRINTF(10, ("libInit: Ok\n"));
    return(ret ? TRUE : FALSE);
}

static int GM_UNIQUENAME(libOpen)(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    GM_UNIQUENAME(nLoadClassConfig)(nh);

    return(TRUE);
}

static int GM_UNIQUENAME(libClose)(LIBBASETYPEPTR nh)
{
    if(nh->nh_Library.lib_OpenCnt == 0) // FIXME is this 0 or 1? Does AROS decrease it before calling libClose?
    {
        ObtainSemaphore(&nh->nh_TaskLock);
        Forbid();
        nh->nh_ReadySignal = SIGB_SINGLE;
        nh->nh_ReadySigTask = FindTask(NULL);
        if(nh->nh_RemovableTask)
        {
            Signal(nh->nh_RemovableTask, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nh->nh_RemovableTask)
        {
            Wait(1L<<nh->nh_ReadySignal);
        }
        //FreeSignal(nh->nh_ReadySignal);
        ReleaseSemaphore(&nh->nh_TaskLock);
    }
    KPRINTF(5, ("libClose: lib_OpenCnt = %ld\n", nh->nh_Library.lib_OpenCnt));
    return(TRUE);
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR nh)
{
    struct NepClassMS *ncm;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if((nh->nh_DevBase->np_Library.lib_OpenCnt == 1))
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));

        Remove(&nh->nh_Library.lib_Node);
        ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
        while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncm);
            FreeVec(ncm->ncm_CDC);
            FreeVec(ncm->ncm_CUC);
            FreeVec(ncm->ncm_DevIDString);
            FreeVec(ncm->ncm_IfIDString);
            FreeVec(ncm);
            ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
        }

        ncm = &nh->nh_DummyNCM;
        FreeVec(ncm->ncm_CDC);
        FreeVec(ncm->ncm_CUC);

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);
        KPRINTF(5, ("libExpunge: Unloading done! massstorage.class expunged!\n\n"));
    } else {
        KPRINTF(5, ("libExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        return(FALSE);
    }

    return(TRUE);
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2OPENLIB(GM_UNIQUENAME(libOpen), 0)
ADD2CLOSELIB(GM_UNIQUENAME(libClose), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
/* \\\ */

/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassMS * GM_UNIQUENAME(usbAttemptInterfaceBinding)(struct NepMSBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;
	struct PsdConfig *pc;
    struct PsdDevice *pd;
	IPTR prodid;
    IPTR vendid;
	
    KPRINTF(1, ("nepMSAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
					IFA_Config, &pc,
                    TAG_END);
		psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
					TAG_END);			
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductID, &prodid,
                    DA_VendorID, &vendid,
                    TAG_END);

        CloseLibrary(ps);
		
		// Huawei modem, massstorage is useless.		
		if( (vendid == 0x12d1 ) && (prodid == 0x1001 || prodid == 0x1003 ) ) return(NULL);		
		
        if((ifclass == MASSSTORE_CLASSCODE) &&
           ((subclass == MS_SCSI_SUBCLASS) ||
            (subclass == MS_RBC_SUBCLASS) ||
            (subclass == MS_ATAPI_SUBCLASS) ||
            (subclass == MS_FDDATAPI_SUBCLASS) ||
            (subclass == MS_UFI_SUBCLASS)) &&
           ((proto == MS_PROTO_BULK) ||
            (proto == MS_PROTO_CB) ||
            (proto == MS_PROTO_CBI)))
        {
            return(GM_UNIQUENAME(usbForceInterfaceBinding)(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassMS * GM_UNIQUENAME(usbForceInterfaceBinding)(struct NepMSBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct Task *subtask;
    struct NepClassMS *ncm;
    struct NepClassMS *firstncm = NULL;
    struct NepClassMS *tmpncm;
    struct MsgPort *mp;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    struct PsdPipe *pp;
    struct ClsDevCfg *cdc;
    struct ClsUnitCfg *cuc;
    STRPTR devname;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;
    IPTR ifnum;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    STRPTR devidstr;
    STRPTR ifidstr;
    BOOL  unitfound;
    UBYTE buf[64];
    UBYTE maxlun;
    UWORD lunnum;
    LONG ioerr;
    LONG retry;
    ULONG patchflags = 0;
    BOOL delayedstore = FALSE;

    KPRINTF(1, ("nepMSAttemptInterfaceBinding(%08lx)\n", pif));
    if(!(mp = CreateMsgPort()))
    {
        return(NULL);
    }
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    IFA_InterfaceNum, &ifnum,
                    IFA_Config, &pc,
                    IFA_IDString, &ifidstr,
                    TAG_DONE);
        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    TAG_END);
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductID, &prodid,
                    DA_VendorID, &vendid,
                    DA_ProductName, &devname,
                    DA_IDString, &devidstr,
                    TAG_END);
        maxlun = 0;
        /* Patches and fixes */
        if((proto != MS_PROTO_BULK) && (proto != MS_PROTO_CB) && (proto != MS_PROTO_CBI))
        {
            proto = MS_PROTO_BULK;
        }
        if((subclass != MS_SCSI_SUBCLASS) &&
           (subclass != MS_RBC_SUBCLASS) &&
           (subclass != MS_ATAPI_SUBCLASS) &&
           (subclass != MS_FDDATAPI_SUBCLASS) &&
           (subclass != MS_UFI_SUBCLASS))
        {
            subclass = MS_SCSI_SUBCLASS;
        }

        if(proto == MS_PROTO_BULK)
        {
            if(vendid == 0x05e3) /* 2.5 HD Wrapper by Eagle Tec */
            {
                patchflags |= PFF_FIX_INQ36|PFF_SIMPLE_SCSI|PFF_DELAY_DATA;
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Broken Genesys firmware data phase delay activated. Performance loss!");
            }
            if((vendid == 0x0d7d) && (prodid == 0x1600)) /* HAMA Memory stick */
            {
                patchflags |= PFF_SIMPLE_SCSI;
            }
            if(((vendid == 0x04cb) && (prodid == 0x0100)) || /* Fujifilm FinePix 1400Zoom */
               ((vendid == 0x0204) && (prodid == 0x6025)) || /* Brock's EXIGO Flashstick */
               ((vendid == 0x0aec) && (prodid == 0x5010))) /* SOYO Multislot Reader */
            {
                patchflags |= PFF_FIX_INQ36;
            }
            if(((vendid == 0x0c76) && (prodid == 0x0005)) || /* JetFlash */
               ((vendid == 0x066f) && (prodid == 0x8000))) /* Aiptek_mp3-310_128MB.txt */
            {
                patchflags |= PFF_NO_RESET;
            }

            if(((vendid == 0x059b) && (prodid == 0x0031)) || /* ZIP 100 */
               //((vendid == 0x0aec) && (prodid == 0x5010)) || /* Neodio CF-Reader */
               ((vendid == 0x058f) && (prodid == 0x9380)) || /* guido's stick */
               ((vendid == 0x3579) && (prodid == 0x6901)))
               //((vendid == 0x07c4) && (prodid == 0xb00b)))   /* USB Memory Stick */
            {
                patchflags |= PFF_SINGLE_LUN;
            }
            if(patchflags)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Preconfig patchflags 0x%04lx", patchflags);
            }
        }
        lunnum = 0;
        while(lunnum <= maxlun)
        {
            /*if(firstncm)
            {
                unitno = firstncm->ncm_UnitNo + lunnum;
            } else {
                unitno = lunnum;
            }*/
            Forbid();
            unitfound = FALSE;
            ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
            while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                if((strcmp(devidstr, ncm->ncm_DevIDString) == 0) &&
                   (strcmp(ifidstr, ncm->ncm_IfIDString) == 0) &&
                   (ncm->ncm_UnitLUN == lunnum))
                {
                    unitno = ncm->ncm_UnitNo;
                    unitfound = TRUE;
                    break;
                }
                ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
            }
            if(!unitfound)
            {
                /* as units are freed in the expunge-vector, the memory is
                   outside the scope of the poseidon library */
                if(!(ncm = AllocVec(sizeof(struct NepClassMS), MEMF_PUBLIC|MEMF_CLEAR)))
                {
                    Permit();
                    DeleteMsgPort(mp);
                    CloseLibrary(ps);
                    return(NULL);
                }
                ncm->ncm_ClsBase = nh;
                ncm->ncm_UnitNo = (ULONG) ~0;
                ncm->ncm_CDC = cdc = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
                ncm->ncm_CUC = cuc = AllocVec(sizeof(struct ClsUnitCfg), MEMF_PUBLIC|MEMF_CLEAR);
                ncm->ncm_DevIDString = AllocVec((ULONG) strlen(devidstr)+1, MEMF_PUBLIC|MEMF_CLEAR);
                ncm->ncm_IfIDString = AllocVec((ULONG) strlen(ifidstr)+1, MEMF_PUBLIC|MEMF_CLEAR);
                if(!(cdc && cuc && ncm->ncm_DevIDString && ncm->ncm_IfIDString))
                {
                    FreeVec(cdc);
                    FreeVec(cuc);
                    FreeVec(ncm->ncm_DevIDString);
                    FreeVec(ncm->ncm_IfIDString);
                    FreeVec(ncm);
                    Permit();
                    DeleteMsgPort(mp);
                    CloseLibrary(ps);
                    return(NULL);
                }
                /* IORequests may be queued even if the task is gone. */
                NewList(&ncm->ncm_Unit.unit_MsgPort.mp_MsgList);
                NewList(&ncm->ncm_XFerQueue);
                NewList(&ncm->ncm_DCInts);
                InitSemaphore(&ncm->ncm_XFerLock);
                AddTail(&nh->nh_Units, &ncm->ncm_Unit.unit_MsgPort.mp_Node);
                strcpy(ncm->ncm_DevIDString, devidstr);
                strcpy(ncm->ncm_IfIDString, ifidstr);

            }
            ncm->ncm_Interface = pif;
            ncm->ncm_Device = pd;
            ncm->ncm_Config = pc;
            ncm->ncm_UnitLUN = lunnum;
            ncm->ncm_UnitIfNum = ifnum;
            ncm->ncm_UnitProdID = prodid;
            ncm->ncm_UnitVendorID = vendid;
            ncm->ncm_TPType = proto;
            ncm->ncm_CSType = subclass;
            ncm->ncm_BlockSize = 0;
            ncm->ncm_BlockShift = 9;
            psdSafeRawDoFmt(ncm->ncm_LUNNumStr, 4, "%ld", ncm->ncm_UnitLUN);

            if(!firstncm)
            {
                firstncm = ncm;
            }
            ncm->ncm_UnitLUN0 = firstncm;

            GM_UNIQUENAME(nLoadBindingConfig)(ncm);
            if(ncm->ncm_UsingDefaultCfg)
            {
                ncm->ncm_CDC->cdc_PatchFlags |= nh->nh_DummyNCM.ncm_CDC->cdc_PatchFlags;
            } else {
                patchflags = 0; // specific flags override defaults, ALL defaults.
            }

            patchflags |= ncm->ncm_CDC->cdc_PatchFlags;
            if((vendid == 0x090a) && (prodid == 0x1100))
            {
                patchflags |= PFF_CLEAR_EP;
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Enabling clear endpoint halt mode for this device!");
            }
            if(vendid == 0x07b4) /* Olympus C-xx */
            {
                patchflags |= PFF_CSS_BROKEN;
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Workaround for broken Olympus cameras enabled.");
            }
            if(vendid == 0x067b) /* Prolific */
            {
                patchflags |= PFF_CSS_BROKEN;
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Workaround for broken Prolific signature enabled.");
            }
            if((vendid == 0x0c76) && (prodid == 0x0005))
            {
                patchflags |= PFF_FIX_INQ36|PFF_FAKE_INQUIRY|PFF_MODE_XLATE;
            }

            // do this for the first LUN only
            if(lunnum == 0)
            {
                if((pp = psdAllocPipe(pd, mp, NULL)))
                {
                    psdSetAttrs(PGA_PIPE, pp,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, 1000,
                                TAG_END);
                    if(((vendid == 0x04e6) && ((prodid == 0x0002) || (prodid == 0x000b) || (prodid == 0x000c))) ||
                       ((vendid == 0x050d) && (prodid == 0x0115)) ||
                       ((vendid == 0x07af) && ((prodid == 0x0004) || (prodid == 0x0005))))
                    {
                        UBYTE databyte = 0x01;
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Attempting to do special eUSCSI init...");

                        psdPipeSetup(pp, URTF_VENDOR|URTF_INTERFACE, 0x0c, 0x01, ifnum);
                        ioerr = psdDoPipe(pp, &databyte, 1);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                           "eUSCSI_init failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        }
                    }
                    if((vendid == 0x1019) && (prodid == 0x0C55))
                    {
                        struct UsbMSCmdBlkWrapper umscbw;
                        struct UsbMSCmdStatusWrapper umscsw;
                        struct PsdPipe *outep, *inep;
                        struct PsdPipe *outpp, *inpp;

                        inep = psdFindEndpoint(pif, NULL,
                                               EA_IsIn, TRUE,
                                               EA_TransferType, USEAF_BULK,
                                               TAG_END);
                        outep = psdFindEndpoint(pif, NULL,
                                                EA_IsIn, FALSE,
                                                EA_TransferType, USEAF_BULK,
                                                TAG_END);
                        if(inep && outep)
                        {
                            inpp = psdAllocPipe(pd, mp, inep);
                            outpp = psdAllocPipe(pd, mp, outep);
                            if(inpp && outpp)
                            {
                                psdSetAttrs(PGA_PIPE, inpp,
                                            PPA_NakTimeout, TRUE,
                                            PPA_NakTimeoutTime, 5000,
                                            TAG_END);
                                psdSetAttrs(PGA_PIPE, outpp,
                                            PPA_NakTimeout, TRUE,
                                            PPA_NakTimeoutTime, 5000,
                                            PPA_NoShortPackets, TRUE,
                                            TAG_END);

                                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                               "Attempting to do special UCR-61S2B init...");
                                umscbw.dCBWSignature = AROS_LONG2LE(0x43425355);
                                umscbw.dCBWTag = 0;
                                umscbw.dCBWDataTransferLength = 0;
                                umscbw.bmCBWFlags = 0;
                                umscbw.bCBWLUN = 0;
                                umscbw.bCBWCBLength = 12;
                                memset(umscbw.CBWCB, 0, 16);
                                CopyMem("\xec\x0a\x06\x00$PCCHIPS", umscbw.CBWCB, (ULONG) umscbw.bCBWCBLength);
                                ioerr = psdDoPipe(outpp, &umscbw, UMSCBW_SIZEOF);
                                if(ioerr)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                   "UCR-61S2B init command failed: %s (%ld)",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                }
                                ioerr = psdDoPipe(inpp, &umscsw, UMSCSW_SIZEOF);
                                if(ioerr)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                   "UCR-61S2B init status failed: %s (%ld)",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                }
                            }
                            if(inpp)
                            {
                                psdFreePipe(inpp);
                            }
                            if(outpp)
                            {
                                psdFreePipe(outpp);
                            }
                        }
                    }

                    if(!(patchflags & PFF_SINGLE_LUN))
                    {
                        retry = 3;
                        maxlun = 0;
                        do
                        {
                            psdPipeSetup(pp, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                                         UMSR_GET_MAX_LUN, 0, ifnum);
                            ioerr = psdDoPipe(pp, &maxlun, 1);
                            if(ioerr)
                            {
                                maxlun = 0;
                                if((retry > 1) && (ioerr != UHIOERR_NAKTIMEOUT) && (ioerr != UHIOERR_TIMEOUT))
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                   "GET_MAX_LUN failed: %s (%ld), retrying in 0.5secs.",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    psdDelayMS(500);
                                } else {
                                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "GET_MAX_LUN failed: %s (%ld)",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                }
                            } else {
                                /*psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                               "Number of logical units: %ld", (ULONG) maxlun+1);*/
                                break;
                            }
                        } while(--retry);
                        if(ioerr)
                        {
                            if((!(patchflags & PFF_NO_FALLBACK)) && (!(patchflags & PFF_SINGLE_LUN)))
                            {
                                patchflags |= PFF_SINGLE_LUN;
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                               "Fallback: Enabling SingleLUN.");
                                delayedstore = TRUE;
                            }
                        }
                        if(maxlun > 7)
                        {
                             psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                            "MaxLUN value %ld does not seem reasonable. Reducing to %ld.", maxlun, 3);
                             maxlun = 3;
                        }
                    } else {
                        maxlun = 0;
                    }
                    psdFreePipe(pp);
                }
            }
            ncm->ncm_MaxLUN = maxlun;

            /* Find next free unit number */
            if(ncm->ncm_UnitNo == (ULONG) ~0)
            {
                unitno = ncm->ncm_CUC->cuc_DefaultUnit;
                tmpncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
                while(tmpncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
                {
                    if(tmpncm->ncm_UnitNo == unitno)
                    {
                        unitno++;
                        tmpncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
                    } else {
                        tmpncm = (struct NepClassMS *) tmpncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
                    }
                }
                ncm->ncm_UnitNo = unitno;
            }
            ncm->ncm_CDC->cdc_PatchFlags = patchflags;
            Permit();

            if(delayedstore)
            {
                ncm->ncm_Base = ps;
                GM_UNIQUENAME(nStoreConfig)(ncm);
                delayedstore = FALSE;
            }
            psdSafeRawDoFmt(buf, 64, "massstorage.class<%08lx,%ld>", ncm, lunnum);
            ncm->ncm_ReadySignal = SIGB_SINGLE;
            ncm->ncm_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            subtask = psdSpawnSubTask(buf, (APTR) GM_UNIQUENAME(nMSTask), ncm);
            if(subtask)
            {
                psdBorrowLocksWait(subtask, 1UL<<ncm->ncm_ReadySignal);
                if(ncm->ncm_Task)
                {
                    //ncm->ncm_ReadySigTask = NULL;
                    //FreeSignal(ncm->ncm_ReadySignal);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "MSD '%s' LUN %ld available through %s unit %ld!",
                                   devname, lunnum, nh->nh_DevBase->np_Library.lib_Node.ln_Name,
                                   ncm->ncm_UnitNo);
                    //lunnum++;
                    //continue;
                }
            }
            ncm->ncm_ReadySigTask = NULL;
            //FreeSignal(ncm->ncm_ReadySignal);
            /* Get rid of unit structure */
            /* Well, actually don't. maybe we will be able to get back to it later */
            /*Forbid();
            Remove(ncm);
            Permit();
            FreeVec(ncm);
            if(firstncm == ncm)
            {
                firstncm = NULL;
            }*/
            lunnum++;
        }
        CloseLibrary(ps);
    }
    DeleteMsgPort(mp);
    return(firstncm);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void GM_UNIQUENAME(usbReleaseInterfaceBinding)(struct NepMSBase *nh, struct NepClassMS *ncm)
{
    struct Library *ps;
    STRPTR devname;
    struct NepClassMS *ncmhead;

    KPRINTF(1, ("nepMSReleaseInterfaceBinding(%08lx)\n", ncm));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        if(ncm->ncm_GUITask)
        {
            Signal(ncm->ncm_GUITask, SIGBREAKF_CTRL_C);
        }
        Permit();
        psdGetAttrs(PGA_DEVICE, ncm->ncm_Device, DA_ProductName, &devname, TAG_END);
        ncmhead = ncm;
        ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
        while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            KPRINTF(10, ("ncm = %08lx, ncmhead = %08lx, unit0 = %08lx\n", ncm, ncmhead, ncm->ncm_UnitLUN0));
            if((ncm->ncm_UnitLUN0 == ncmhead) && (ncm->ncm_Task))
            {
                /*ncm->ncm_UnitLUN0 = NULL;*/
                Forbid();
                ncm->ncm_ReadySignal = SIGB_SINGLE;
                ncm->ncm_ReadySigTask = FindTask(NULL);
                ncm->ncm_DenyRequests = TRUE;
                if(ncm->ncm_Task)
                {
                    Signal(ncm->ncm_Task, SIGBREAKF_CTRL_C);
                }
                Permit();
                while(ncm->ncm_Task)
                {
                    psdBorrowLocksWait(ncm->ncm_Task, 1UL<<ncm->ncm_ReadySignal);
                }
                //FreeSignal(ncm->ncm_ReadySignal);
            }
            ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "'%s' retreated, pitiful coward.",
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
         LIBBASETYPEPTR, nh, 5, massstorage)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    KPRINTF(1, ("nepMSGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "Supports storage devices via usbscsi.device";
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
                 *((IPTR *) ti->ti_Data) = nh->nh_DummyNCM.ncm_UsingDefaultCfg;
                 count++;
             }
             if((ti = FindTagItem(UCCA_SupportsSuspend, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             break;

         case UGA_BINDING:
             if((ti = FindTagItem(UCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = ((struct NepClassMS *) usbstruct)->ncm_UsingDefaultCfg;
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
         LIBBASETYPEPTR, nh, 6, massstorage)
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
         LIBBASETYPEPTR, nh, 7, massstorage)
{
    AROS_LIBFUNC_INIT

    struct NepClassMS *ncm;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) GM_UNIQUENAME(usbAttemptInterfaceBinding)(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) GM_UNIQUENAME(usbForceInterfaceBinding)(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            GM_UNIQUENAME(usbReleaseInterfaceBinding)(nh, (struct NepClassMS *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(GM_UNIQUENAME(nOpenBindingCfgWindow)(nh, &nh->nh_DummyNCM));

        case UCM_OpenBindingCfgWindow:
            return(GM_UNIQUENAME(nOpenBindingCfgWindow)(nh, (struct NepClassMS *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            GM_UNIQUENAME(nLoadClassConfig)(nh);
            Forbid();
            ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
            while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                GM_UNIQUENAME(nLoadBindingConfig)(ncm);
                ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
            }
            Permit();
            return(TRUE);

        case UCM_AttemptSuspendDevice:
            ncm = (struct NepClassMS *) methoddata[0];
            ncm->ncm_Running = FALSE;
            return(TRUE);

        case UCM_AttemptResumeDevice:
            ncm = (struct NepClassMS *) methoddata[0];
            ncm->ncm_Running = TRUE;
            Signal(ncm->ncm_Task, (1L<<ncm->ncm_TaskMsgPort->mp_SigBit));
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "nLoadClassConfig()" */
BOOL GM_UNIQUENAME(nLoadClassConfig)(struct NepMSBase *nh)
{
    struct NepClassMS *ncm = &nh->nh_DummyNCM;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct ClsUnitCfg *cuc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(ncm->ncm_GUITask)
    {
        return(FALSE);
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Create default config */
    cdc = ncm->ncm_CDC;
    cdc->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('M','S','D','C'));
    cdc->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    cdc->cdc_NakTimeout = DEF_NAKTIMEOUT;
    cdc->cdc_PatchFlags = PFF_MODE_XLATE|PFF_NO_RESET|PFF_FIX_INQ36|PFF_SIMPLE_SCSI;
    cdc->cdc_FATDosType = 0x46415400;
    cdc->cdc_StartupDelay = 0;
    cdc->cdc_MaxTransfer = 5;
    strcpy(cdc->cdc_FATFSName, "fat-handler");
    strcpy(cdc->cdc_FATControl, ""); // FIXME

    cdc->cdc_CDDosType = 0x43444653; // FIXME
    strcpy(cdc->cdc_CDFSName, "cdrom-handler"); // FIXME
    strcpy(cdc->cdc_CDControl, ""); // FIXME
    cdc->cdc_NTFSDosType = 0x4e544653; // FIXME
    strcpy(cdc->cdc_NTFSName, "ntfs-handler"); // FIXME

    cuc = ncm->ncm_CUC;
    cuc->cuc_ChunkID = AROS_LONG2BE(MAKE_ID('L','U','N','0'));
    cuc->cuc_Length = AROS_LONG2BE(sizeof(struct ClsUnitCfg)-8);
    cuc->cuc_AutoMountFAT = TRUE;
    strcpy(cuc->cuc_FATDOSName, "UMSD");
    cuc->cuc_FATBuffers = 100;
    cuc->cuc_AutoMountRDB = TRUE;
    cuc->cuc_BootRDB = TRUE;
    cuc->cuc_DefaultUnit = 0;
    cuc->cuc_AutoUnmount = TRUE;
    cuc->cuc_MountAllFAT = TRUE;
    cuc->cuc_AutoMountCD = TRUE;

    ncm->ncm_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = psdGetClsCfg(GM_UNIQUENAME(libname));
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(ncm->ncm_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) ncm->ncm_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(ncm->ncm_CDC->cdc_Length)));
            psdFreeVec(cdc);
            ncm->ncm_UsingDefaultCfg = FALSE;
        }
        cuc = psdGetCfgChunk(pic, AROS_LONG2BE(ncm->ncm_CUC->cuc_ChunkID));
        if(cuc)
        {
            CopyMem(((UBYTE *) cuc) + 8, ((UBYTE *) ncm->ncm_CUC) + 8, min(AROS_LONG2BE(cuc->cuc_Length), AROS_LONG2BE(ncm->ncm_CUC->cuc_Length)));
            psdFreeVec(cuc);
            ncm->ncm_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nLoadBindingConfig()" */
BOOL GM_UNIQUENAME(nLoadBindingConfig)(struct NepClassMS *ncm)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct ClsUnitCfg *cuc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Binding Config...\n"));
    if(ncm->ncm_GUITask)
    {
        return(FALSE);
    }
    //GM_UNIQUENAME(nLoadClassConfig)(nh);
    *ncm->ncm_CDC = *nh->nh_DummyNCM.ncm_CDC;
    *ncm->ncm_CUC = *nh->nh_DummyNCM.ncm_CUC;
    ncm->ncm_CUC->cuc_ChunkID = AROS_LONG2BE(MAKE_ID('L','U','N','0')+ncm->ncm_UnitLUN);
    ncm->ncm_UsingDefaultCfg = TRUE;

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), ncm->ncm_DevIDString, ncm->ncm_IfIDString);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(ncm->ncm_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) ncm->ncm_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(ncm->ncm_CDC->cdc_Length)));
            psdFreeVec(cdc);
            ncm->ncm_UsingDefaultCfg = FALSE;
        }
        cuc = psdGetCfgChunk(pic, AROS_LONG2BE(ncm->ncm_CUC->cuc_ChunkID));
        if(cuc)
        {
            CopyMem(((UBYTE *) cuc) + 8, ((UBYTE *) ncm->ncm_CUC) + 8, min(AROS_LONG2BE(cuc->cuc_Length), AROS_LONG2BE(ncm->ncm_CUC->cuc_Length)));
            psdFreeVec(cuc);
            ncm->ncm_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nOpenBindingCfgWindow()" */
LONG GM_UNIQUENAME(nOpenBindingCfgWindow)(struct NepMSBase *nh, struct NepClassMS *ncm)
{
    struct Library *ps;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!ncm->ncm_GUITask)
    {
        if((ncm->ncm_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", GM_UNIQUENAME(nGUITask), ncm)))
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

/* /// "nStartRemovableTask()" */
BOOL GM_UNIQUENAME(nStartRemovableTask)(struct Library *ps, struct NepMSBase *nh)
{
    struct Task *tmptask;
    ObtainSemaphore(&nh->nh_TaskLock);
    if(nh->nh_RemovableTask)
    {
        ReleaseSemaphore(&nh->nh_TaskLock);
        return(TRUE);
    }

    nh->nh_ReadySignal = SIGB_SINGLE;
    nh->nh_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    if((tmptask = psdSpawnSubTask(MOD_NAME_STRING " Removable Task", GM_UNIQUENAME(nRemovableTask), nh)))
    {
        psdBorrowLocksWait(tmptask, 1UL<<nh->nh_ReadySignal);
    }
    nh->nh_ReadySigTask = NULL;
    //FreeSignal(nh->nh_ReadySignal);
    if(nh->nh_RemovableTask)
    {
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Removable Task started.");
        ReleaseSemaphore(&nh->nh_TaskLock);
        return(TRUE);
    }
    ReleaseSemaphore(&nh->nh_TaskLock);
    return(FALSE);
}
/* \\\ */

/**************************************************************************/

#undef  ps
#define ps ncm->ncm_Base

const STRPTR DeviceTypeStrings[] =
{
    "Direct Access",
    "Sequential Access",
    "Printer",
    "Processor",
    "Worm",
    "CD/DVD ROM",
    "Scanner",
    "Optical",
    "Medium Changer",
    "Communications",
    "Arts 1",
    "Arts 2",
    "RAID",
    "Enclosure",
    "Simple Direct Access",
    "Optical Card",
    "Reserved",
    "Object Based"
};

UWORD PrimeTable[] =
{
    5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
    151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
    229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307,
    311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389,
    397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467,
    479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571,
    577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653,
    659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751,
    757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853,
    857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947,
    953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021, 1031, 1033,
    1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097, 1103, 1109,
    1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213,
    1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291,
    1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373, 1381, 1399,
    1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481,
    1483, 1487, 1489, 1493, 1499, 1511, 1523, 1531, 1543, 1549, 1553, 1559,
    1567, 1571, 1579, 1583, 1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627,
    1637, 1657, 1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733,
    1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811, 1823, 1831,
    1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931,
    1933, 1949, 1951, 1973, 1979, 1987, 1993, 1997, 1999, 0
};

/* /// "nHexString()" */
void GM_UNIQUENAME(nHexString)(UBYTE *src, ULONG len, UBYTE *buf)
{
    static char *hexchars = "0123456789ABCDEF";
    UWORD cnt = 0;

    while(cnt < len)
    {
        if(cnt++)
        {
            *buf++ = ' ';
        }
        *buf++ = hexchars[*src>>4];
        *buf++ = hexchars[*src++ & 0xf];
    }
    *buf = 0;
}
/* \\\ */

/* /// "nMSTask()" */
AROS_UFH0(void, GM_UNIQUENAME(nMSTask))
{
    AROS_USERFUNC_INIT

    struct NepClassMS *ncm;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    UWORD cnt;

    struct IOStdReq *ioreq;
    struct IOStdReq *ioreq2;

    struct SCSICmd scsicmd;
    UBYTE inquirydata[36];
    //UBYTE buf[256];
    UBYTE cmd6[6];
    UBYTE sensedata[18];

    if((ncm = GM_UNIQUENAME(nAllocMS())))
    {
        Forbid();
        if(ncm->ncm_ReadySigTask)
        {
            Signal(ncm->ncm_ReadySigTask, 1L<<ncm->ncm_ReadySignal);
        }
        Permit();

        if(ncm->ncm_CDC->cdc_PatchFlags)
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "Postconfig patchflags 0x%04lx%s%s%s%s%s%s%s%s%s%s%s%s%s%s.",
                           ncm->ncm_CDC->cdc_PatchFlags,
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_SINGLE_LUN) ? " SingleLun" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_MODE_XLATE) ? " ModeXLate" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_EMUL_LARGE_BLK) ? " EmulLarge" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT) ? " RemSupport" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_INQ36) ? " FixInq36" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA) ? " DelayData" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_SIMPLE_SCSI) ? " SimpleSCSI" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_RESET) ? " NoReset" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_FAKE_INQUIRY) ? " FakeInq" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_CAPACITY) ? " FixCapacity" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK) ? " NoFallback" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_CSS_BROKEN) ? " CSSBroken" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_CLEAR_EP) ? " ClearEP" : "",
                           (ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG) ? " Debug" : "");
        }

        if(ncm->ncm_CDC->cdc_StartupDelay)
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "Delaying init sequence by %ld00ms.",
                           ncm->ncm_CDC->cdc_StartupDelay);

            psdDelayMS(ncm->ncm_CDC->cdc_StartupDelay*100);
        }
        if(!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_RESET))
        {
            nLockXFer(ncm);
            if(nBulkReset(ncm))
            {
                if((!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_RESET)))
                {
                    ncm->ncm_CDC->cdc_PatchFlags |= PFF_NO_RESET;
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Fallback: Enabling No Reset.");
                    GM_UNIQUENAME(nStoreConfig)(ncm);
                }
            }
            nUnlockXFer(ncm);
        }

        ncm->ncm_UnitReady = FALSE;
        ncm->ncm_Removable = TRUE;
        ncm->ncm_DenyRequests = FALSE;

        scsicmd.scsi_Data = (UWORD *) inquirydata;
        scsicmd.scsi_Length = 36;
        scsicmd.scsi_Command = cmd6;
        scsicmd.scsi_CmdLength = 6;
        scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
        scsicmd.scsi_SenseData = sensedata;
        scsicmd.scsi_SenseLength = 18;
        cmd6[0] = SCSI_INQUIRY;
        cmd6[1] = 0;
        cmd6[2] = 0;
        cmd6[3] = 0;
        cmd6[4] = 36;
        cmd6[5] = 0;
        if((ioerr = nScsiDirect(ncm, &scsicmd)))
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "SCSI_INQUIRY failed: %ld",
                           ioerr);
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Try increasing the startup delay value or fake inquiry.");
        } else {
            strncpy(ncm->ncm_LUNIDStr, &inquirydata[16], 16);
            // trim spaces
            cnt = strlen(ncm->ncm_LUNIDStr);
            while(cnt--)
            {
                if(ncm->ncm_LUNIDStr[cnt] == ' ')
                {
                    ncm->ncm_LUNIDStr[cnt] = 0;
                } else {
                    break;
                }
            }

            ncm->ncm_DeviceType = inquirydata[0] & PDT_MASK;
            if(ncm->ncm_DeviceType > 0x11)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Illegal Device Type %02lx", ncm->ncm_DeviceType);
                ncm->ncm_DeviceType = 0;
            } else {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Device '%s' is of %s type.", ncm->ncm_LUNIDStr, DeviceTypeStrings[ncm->ncm_DeviceType]);
            }
            if((ncm->ncm_DeviceType == PDT_WORM) ||
               (ncm->ncm_DeviceType == PDT_CDROM))
            {
                // assume 2048 byte blocks
                ncm->ncm_BlockSize = 2048;
                ncm->ncm_BlockShift = 11;
                if(ncm->ncm_CDC->cdc_NakTimeout == DEF_NAKTIMEOUT)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Silently increasing NAK Timeout value to 15 seconds for CD/DVD drives...");
                    ncm->ncm_CDC->cdc_NakTimeout = 150;
                    GM_UNIQUENAME(nStoreConfig)(ncm);

                    psdSetAttrs(PGA_PIPE, ncm->ncm_EP0Pipe,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, (ncm->ncm_CDC->cdc_NakTimeout+1)*100,
                                TAG_END);
                    psdSetAttrs(PGA_PIPE, ncm->ncm_EPInPipe,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, ncm->ncm_CDC->cdc_NakTimeout*100,
                                TAG_END);
                    psdSetAttrs(PGA_PIPE, ncm->ncm_EPOutPipe,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, ncm->ncm_CDC->cdc_NakTimeout*100,
                                TAG_END);
                }
                else if(!ncm->ncm_CDC->cdc_NakTimeout)
                {
                    // that's okay, nak timeout disabled
                }
                else if(ncm->ncm_CDC->cdc_NakTimeout < 150)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "NAK Timeout should be at least 15 seconds for CD/DVD drives!");
                }
            }

            if(!(inquirydata[1] & 0x80))
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Device does not seem to use removable media.");
                ncm->ncm_Removable = FALSE;
                ncm->ncm_UnitReady = TRUE;
                ncm->ncm_ChangeCount++;
            }
        }

        /* Main task */
        sigmask = (1L<<ncm->ncm_Unit.unit_MsgPort.mp_SigBit)|
                  (1L<<ncm->ncm_TaskMsgPort->mp_SigBit)|
                  SIGBREAKF_CTRL_C;
        if(ncm->ncm_CSType == MS_UFI_SUBCLASS)
        {
            struct IOStdReq tmpio;
            tmpio.io_Command = CMD_START;
            nStartStop(ncm, &tmpio);
        }

        // if not removable, call it once to trigger mounting...
        if(!ncm->ncm_Removable)
        {
            ncm->ncm_ForceRTCheck = TRUE;
        }
        ncm->ncm_Running = TRUE;

        do
        {
            if(ncm->ncm_Removable || ncm->ncm_ForceRTCheck)
            {
                GM_UNIQUENAME(nStartRemovableTask)(ps, ncm->ncm_ClsBase);
                ncm->ncm_ForceRTCheck = FALSE;
            }
            while((ioreq = (struct IOStdReq *) GetMsg(&ncm->ncm_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->io_Command, ioreq->io_Length));

                switch(ioreq->io_Command)
                {
                    case TD_GETGEOMETRY:
                        nGetGeometry(ncm, ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case TD_EJECT:
                    case CMD_START:
                    case CMD_STOP:
                        nStartStop(ncm, ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_READ:
                        ioreq->io_Actual = 0;
                    case NSCMD_TD_READ64:
                    case TD_READ64:
                        nRead64(ncm, ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case TD_SEEK:
                        ioreq->io_Actual = 0;
                    case NSCMD_TD_SEEK64:
                    case TD_SEEK64:
                        nSeek64(ncm, ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case TD_FORMAT:
                    case CMD_WRITE:
                        ioreq->io_Actual = 0;
                    case NSCMD_TD_FORMAT64:
                    case NSCMD_TD_WRITE64:
                    case TD_FORMAT64:
                    case TD_WRITE64:
                        nWrite64(ncm, ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case HD_SCSICMD:
                        ioreq->io_Error = nScsiDirect(ncm, ioreq->io_Data);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                        if((ioreq2 = ncm->ncm_XFerPending))
                        {
                            ncm->ncm_XFerPending = NULL;
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                        }
                        /* Reset does a flush too */
                    case CMD_FLUSH:
                        ioreq2 = (struct IOStdReq *) ncm->ncm_XFerQueue.lh_Head;
                        while(ioreq2->io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOStdReq *) ncm->ncm_XFerQueue.lh_Head;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));
        ncm->ncm_DenyRequests = TRUE;
        /* Device ejected */
        ncm->ncm_UnitReady = FALSE;
        ncm->ncm_ChangeCount++;
        ioreq = (struct IOStdReq *) ncm->ncm_DCInts.lh_Head;
        while(((struct Node *) ioreq)->ln_Succ)
        {
            Cause(ioreq->io_Data);
            ioreq = (struct IOStdReq *) ((struct Node *) ioreq)->ln_Succ;
        }
        if(!ncm->ncm_Removable)
        {
            GM_UNIQUENAME(nStartRemovableTask)(ps, ncm->ncm_ClsBase);
        }
        KPRINTF(20, ("Going down the river!\n"));
        GM_UNIQUENAME(nFreeMS)(ncm);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocMS()" */
struct NepClassMS * GM_UNIQUENAME(nAllocMS)(void)
{
    struct Task *thistask;
    struct NepClassMS *ncm;
    IPTR epnum;

    thistask = FindTask(NULL);
    ncm = thistask->tc_UserData;
    do
    {
        if(!(ncm->ncm_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib | AO_Unknown);
            break;
        }
        ncm->ncm_EPInt = psdFindEndpoint(ncm->ncm_Interface, NULL,
                                         EA_IsIn, TRUE,
                                         EA_TransferType, USEAF_INTERRUPT,
                                         TAG_END);
        ncm->ncm_EPIn = psdFindEndpoint(ncm->ncm_Interface, NULL,
                                        EA_IsIn, TRUE,
                                        EA_TransferType, USEAF_BULK,
                                        TAG_END);
        ncm->ncm_EPOut = psdFindEndpoint(ncm->ncm_Interface, NULL,
                                         EA_IsIn, FALSE,
                                         EA_TransferType, USEAF_BULK,
                                         TAG_END);
        if(!(ncm->ncm_EPIn && ncm->ncm_EPOut))
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "IN or OUT endpoint missing!");
            break;
        }
        if((!ncm->ncm_EPInt) && (ncm->ncm_TPType == MS_PROTO_CBI))
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "INT endpoint missing!");
            break;
        } else {
            psdGetAttrs(PGA_ENDPOINT, ncm->ncm_EPOut,
                        EA_EndpointNum, &epnum,
                        TAG_END);
            ncm->ncm_EPIntNum = epnum;
        }
        psdGetAttrs(PGA_ENDPOINT, ncm->ncm_EPIn,
                    EA_EndpointNum, &epnum,
                    TAG_END);
        ncm->ncm_EPInNum = epnum;
        psdGetAttrs(PGA_ENDPOINT, ncm->ncm_EPOut,
                    EA_EndpointNum, &epnum,
                    TAG_END);
        ncm->ncm_EPOutNum = epnum;

        ncm->ncm_BulkResetBorks = FALSE;
        ncm->ncm_GeoChangeCount = 0xffffffff;

        ncm->ncm_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        ncm->ncm_Unit.unit_MsgPort.mp_SigTask = thistask;
        ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        ncm->ncm_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;
        ncm->ncm_XFerPending = NULL;
        if((ncm->ncm_TaskMsgPort = CreateMsgPort()))
        {
            if((ncm->ncm_EP0Pipe = psdAllocPipe(ncm->ncm_Device, ncm->ncm_TaskMsgPort, NULL)))
            {
                if((ncm->ncm_EPOutPipe = psdAllocPipe(ncm->ncm_Device, ncm->ncm_TaskMsgPort, ncm->ncm_EPOut)))
                {
                    if((ncm->ncm_EPInPipe = psdAllocPipe(ncm->ncm_Device, ncm->ncm_TaskMsgPort, ncm->ncm_EPIn)))
                    {
                        if(ncm->ncm_CDC->cdc_NakTimeout)
                        {
                            psdSetAttrs(PGA_PIPE, ncm->ncm_EP0Pipe,
                                        PPA_NakTimeout, TRUE,
                                        PPA_NakTimeoutTime, (ncm->ncm_CDC->cdc_NakTimeout+1)*100,
                                        TAG_END);
                            psdSetAttrs(PGA_PIPE, ncm->ncm_EPInPipe,
                                        PPA_NakTimeout, TRUE,
                                        PPA_NakTimeoutTime, ncm->ncm_CDC->cdc_NakTimeout*100,
                                        TAG_END);
                            psdSetAttrs(PGA_PIPE, ncm->ncm_EPOutPipe,
                                        PPA_NakTimeout, TRUE,
                                        PPA_NakTimeoutTime, ncm->ncm_CDC->cdc_NakTimeout*100,
                                        TAG_END);
                        }
                        psdSetAttrs(PGA_PIPE, ncm->ncm_EPOutPipe,
                                    PPA_NoShortPackets, TRUE,
                                    TAG_END);
                        if(ncm->ncm_EPInt)
                        {
                            if((ncm->ncm_EPIntPipe = psdAllocPipe(ncm->ncm_Device, ncm->ncm_TaskMsgPort, ncm->ncm_EPInt)))
                            {
                                if(ncm->ncm_CDC->cdc_NakTimeout)
                                {
                                    psdSetAttrs(PGA_PIPE, ncm->ncm_EPIntPipe,
                                                PPA_NakTimeout, TRUE,
                                                PPA_NakTimeoutTime, ncm->ncm_CDC->cdc_NakTimeout*100,
                                                TAG_END);
                                }
                                ncm->ncm_Task = thistask;
                                return(ncm);
                            }
                        } else {
                            ncm->ncm_Task = thistask;
                            return(ncm);
                        }
                        psdFreePipe(ncm->ncm_EPInPipe);
                    }
                    psdFreePipe(ncm->ncm_EPOutPipe);
                }
                psdFreePipe(ncm->ncm_EP0Pipe);
            }
            DeleteMsgPort(ncm->ncm_TaskMsgPort);
        }
        FreeSignal((LONG) ncm->ncm_Unit.unit_MsgPort.mp_SigBit);
    } while(FALSE);
    CloseLibrary(ncm->ncm_Base);
    Forbid();
    ncm->ncm_Task = NULL;
    if(ncm->ncm_ReadySigTask)
    {
        Signal(ncm->ncm_ReadySigTask, 1L<<ncm->ncm_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeMS()" */
void GM_UNIQUENAME(nFreeMS)(struct NepClassMS *ncm)
{
    struct IOStdReq *ioreq;
    /* Disable the message port, messages may still be queued */
    Forbid();
    ncm->ncm_Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
    ncm->ncm_Unit.unit_MsgPort.mp_SigTask = NULL;
    FreeSignal((LONG) ncm->ncm_Unit.unit_MsgPort.mp_SigBit);
    // get rid of all messages that still have appeared here
    while((ioreq = (struct IOStdReq *) GetMsg(&ncm->ncm_Unit.unit_MsgPort)))
    {
        ioreq->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
    }
    Permit();

    psdFreePipe(ncm->ncm_EPIntPipe);
    psdFreePipe(ncm->ncm_EPInPipe);
    psdFreePipe(ncm->ncm_EPOutPipe);
    psdFreePipe(ncm->ncm_EP0Pipe);
    DeleteMsgPort(ncm->ncm_TaskMsgPort);

    psdFreeVec(ncm->ncm_OneBlock);
    ncm->ncm_OneBlock = NULL;
    ncm->ncm_OneBlockSize = 0;

    CloseLibrary(ncm->ncm_Base);
    Forbid();
    ncm->ncm_Task = NULL;
    if(ncm->ncm_ReadySigTask)
    {
        Signal(ncm->ncm_ReadySigTask, 1L<<ncm->ncm_ReadySignal);
    }
}
/* \\\ */

/* /// "nGetModePage()" */
UBYTE * nGetModePage(struct NepClassMS *ncm, UBYTE page)
{
    UBYTE cmd6[6];
    struct SCSICmd scsicmd;
    UBYTE sensedata[18];
    LONG ioerr;
    UBYTE *res;
    ULONG pf;

    KPRINTF(10, ("page %ld\n", page));

    memset(ncm->ncm_ModePageBuf, 0x00, 255);
    scsicmd.scsi_Data = (UWORD *) ncm->ncm_ModePageBuf;
    scsicmd.scsi_Length = 255;
    scsicmd.scsi_Command = cmd6;
    scsicmd.scsi_CmdLength = 6;
    scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE;
    scsicmd.scsi_SenseData = sensedata;
    scsicmd.scsi_SenseLength = 18;
    cmd6[0] = SCSI_MODE_SENSE_6;
    cmd6[1] = 0x00; /* no block descriptors */
    cmd6[2] = page;
    cmd6[3] = 0;
    cmd6[4] = 255;
    cmd6[5] = 0;
    if((ioerr = nScsiDirect(ncm, &scsicmd)))
    {
        KPRINTF(10, ("ioerr %ld\n", ioerr));
        pf = ncm->ncm_CDC->cdc_PatchFlags;
        if(pf & PFF_DEBUG)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "SCSI_MODE_SENSE(0x%02lx) failed: %ld",
                           page, ioerr);
        }
        if((!(pf & PFF_NO_FALLBACK)) && (!(pf & PFF_MODE_XLATE)) && (ioerr == HFERR_Phase))
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_MODE_XLATE;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling CMD6->CMD10.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
        }
        else if((!(pf & PFF_NO_FALLBACK)) && (pf & PFF_MODE_XLATE) && (!(pf & PFF_SIMPLE_SCSI)) && (ioerr == HFERR_Phase))
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_SIMPLE_SCSI;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling Simple SCSI.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
        }
        return(NULL);
    }
    pf = ncm->ncm_CDC->cdc_PatchFlags;
    res = ncm->ncm_ModePageBuf;
    if((scsicmd.scsi_Actual < 6) || (scsicmd.scsi_Actual < res[3]+6))
    {
        if(pf & PFF_DEBUG)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "SCSI_MODE_SENSE(0x%02lx) failed: only %ld returned",
                           page, scsicmd.scsi_Actual);
        }
        return(NULL);
    }
    if(pf & PFF_DEBUG)
    {
        UBYTE hexbuf[12*3+2];
        GM_UNIQUENAME(nHexString)(res, 12, hexbuf);
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "ModePage (%ld) header: %s...", scsicmd.scsi_Actual, hexbuf);
    }
    if(res[3])
    {
        res += res[3];
        scsicmd.scsi_Actual -= res[3];
    }
    res += 4; // skip mode header */
    scsicmd.scsi_Actual -= 4;
    if((*res & 0x3f) != (page & 0x3f))
    {
        if(pf & PFF_DEBUG)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "SCSI_MODE_SENSE(0x%02lx) failed: wrong page 0x%02lx returned",
                           page, *res);
        }
        return(NULL);
    }
    if(scsicmd.scsi_Actual < res[1])
    {
        if(pf & PFF_DEBUG)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "SCSI_MODE_SENSE(0x%02lx) failed: page incomplete",
                           page);
        }
        return(NULL);
    }

#if 0
    if(*res & 0x40) /* subpage mode? */
    {
        res += 2;
    }
#endif
    return(res);
}
/* \\\ */

/* /// "nGetBlockSize()" */
LONG nGetBlockSize(struct NepClassMS *ncm)
{
    UBYTE cmd10[10];
    struct SCSICmd scsicmd;
    UBYTE sensedata[18];
    ULONG capacity[2];
    LONG ioerr;

    scsicmd.scsi_Data = (UWORD *) capacity;
    scsicmd.scsi_Length = 8;
    scsicmd.scsi_Command = cmd10;
    scsicmd.scsi_CmdLength = 10;
    scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE;
    scsicmd.scsi_SenseData = sensedata;
    scsicmd.scsi_SenseLength = 18;
    cmd10[0] = SCSI_DA_READ_CAPACITY;
    cmd10[1] = 0;
    cmd10[2] = 0;
    cmd10[3] = 0;
    cmd10[4] = 0;
    cmd10[5] = 0;
    cmd10[6] = 0;
    cmd10[7] = 0;
    cmd10[8] = 0;
    cmd10[9] = 0;
    if((ioerr = nScsiDirect(ncm, &scsicmd)))
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "SCSI_READ_CAPACITY failed: %ld",
                       ioerr);
        if(!ncm->ncm_BlockSize)
        {
            ncm->ncm_Geometry.dg_SectorSize = ncm->ncm_BlockSize = 512;
            ncm->ncm_BlockShift = 9;
        }
    } else {
        ncm->ncm_Geometry.dg_SectorSize = ncm->ncm_BlockSize = AROS_BE2LONG(capacity[1]);
        ncm->ncm_BlockShift = 0;
        while((1<<ncm->ncm_BlockShift) < ncm->ncm_BlockSize)
        {
            ncm->ncm_BlockShift++;
        }
        ncm->ncm_Geometry.dg_TotalSectors = AROS_BE2LONG(capacity[0])+1;
    }
    return(ioerr);
}
/* \\\ */

/* /// "nFakeGeometry()" */
void nFakeGeometry(struct NepClassMS *ncm, struct DriveGeometry *tddg)
{
    UWORD cnt;
    ULONG primes[32];
    UWORD count[32];
    UWORD curpos;
    UWORD curprime;
    ULONG curprimesq;
    ULONG remblks = tddg->dg_TotalSectors;
    UWORD *primetblptr = PrimeTable;
    UWORD totfactors;
    UWORD factor;
    ULONG remfact;
    UWORD tries;
    BOOL taken;

    if(!ncm->ncm_Geometry.dg_TotalSectors)
    {
        return;
    }
    KPRINTF(10, ("Faking geometry for %ld sectors\n", tddg->dg_TotalSectors));
    //psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Prime factorisation for %ld", remblks);
    tddg->dg_DeviceType = ncm->ncm_DeviceType;
    tddg->dg_Flags = ncm->ncm_Removable ? DGF_REMOVABLE : 0;
    tddg->dg_BufMemType = MEMF_PUBLIC;
    tddg->dg_Reserved = 0;
    do
    {
        KPRINTF(10, ("Loop\n"));
        // first prime is counted manually
        *primes = 2;
        *count = 0;
        curpos = 0;
        while(!(remblks & 1))
        {
            (*count)++;
            remblks >>= 1;
        }
        if(*count)
        {
            curpos++;
        }

        primes[curpos] = 3;
        count[curpos] = 0;
        primetblptr = PrimeTable;
        curprime = 3;
        curprimesq = 9;
        // abort if no primes can be found anymore
        do
        {
            KPRINTF(10, ("remblks = %ld, curprime=%ld\n", remblks, curprime));
            if(remblks % curprime)
            {
                if(remblks >= curprimesq)
                {
                    // it's a prime!
                    break;
                }
                if(count[curpos]) // prime at least once
                {
                    // next prime
                    count[++curpos] = 0;
                    if(curpos == 31) // end of buffer reached
                    {
                        break;
                    }
                }
                // try next "prime"
                if(*primetblptr)
                {
                    // use lookup table for the first primes to 2000
                    curprime = *primetblptr++;
                } else {
                    // do it the hard way :-(
                    curprime += 2;
                }
                primes[curpos] = curprime;
                curprimesq = curprime * curprime;
            } else {
                // factor found
                count[curpos]++;
                remblks /= curprime;
            }
        } while((remblks > 1) && (remblks >= curprime));
        if(count[curpos])
        {
            curpos++;
        }
        if((remblks > 1) && curpos)
        {
            count[curpos] = 1;
            primes[curpos++] = remblks;
        }
        KPRINTF(10, ("Priming done, %ld different primes\n", curpos));
        if(curpos)
        {
            break;
        }
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "FakeGeometry: Total number of blocks is a prime number!");
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_CAPACITY)
        {
            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK)
            {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "This is probably due to the Fix Capacity switch being enabled incorrectly. Please check this.");
                return;
            } else {
                ncm->ncm_CDC->cdc_PatchFlags &= ~PFF_FIX_CAPACITY;
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "Fallback: Disabling Fix Capacity.");
                GM_UNIQUENAME(nStoreConfig)(ncm);
                remblks = ++tddg->dg_TotalSectors;
            }
        } else {
            if(!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK))
            {
                ncm->ncm_CDC->cdc_PatchFlags |= PFF_FIX_CAPACITY;
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                              "Fallback: Enabling Fix Capacity.");
                GM_UNIQUENAME(nStoreConfig)(ncm);
            } else {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                              "Assuming Fix Capacity bug (total blocks instead of last block)!");
            }
            remblks = --tddg->dg_TotalSectors;
        }
    } while(TRUE);

    totfactors = 0;
    for(cnt = 0; cnt < curpos; cnt++)
    {
        totfactors += count[cnt];
    }

    // Rules:
    // - We want at least blocks per track * heads >= 64
    // - We want heads * heads < blocks per track
    // - We want blocks per track < cylinders
    // - We want heads < 8
    // - We want blocks per track < 256
    // - The rest goes into cylinders
    tddg->dg_TrackSectors = 1;
    tddg->dg_Heads = 1;
    remfact = tddg->dg_TotalSectors;
    tries = 0;
    while(totfactors)
    {
        KPRINTF(10, ("Totfactors %ld\n", totfactors));
        cnt = 0;
        while(!count[cnt])
        {
            cnt++;
            if(cnt >= curpos)
            {
                cnt = 0;
            }
        }
        taken = TRUE;
        factor = primes[cnt];
        if((tddg->dg_TrackSectors * factor < 256) &&
           (tddg->dg_TrackSectors * factor < remfact))
        {
            tddg->dg_TrackSectors *= factor;
            remfact /= factor;
        }
        else if((tddg->dg_Heads * factor <= 16) &&
                (tddg->dg_Heads * tddg->dg_Heads * factor * factor < tddg->dg_TrackSectors))
        {
            tddg->dg_Heads *= factor;
            remfact /= factor;
        }
        else if((tddg->dg_Heads * tddg->dg_TrackSectors >= 64) ||
                (tries > curpos))
        {
            // do nothing
        } else {
            taken = FALSE;
        }
        if(taken)
        {
            totfactors--;
            count[cnt]--;
            tries = 0;
        } else {
            tries++;
        }
        KPRINTF(10, ("Factor=%ld: Cylinders = %ld, Heads=%ld, TrackSectors=%ld, Blocks=%ld\n",
                     factor, remfact, tddg->dg_Heads, tddg->dg_TrackSectors,
                     tddg->dg_TotalSectors));

    }
    KPRINTF(10, ("Final: Cylinders = %ld, Heads=%ld, TrackSectors=%ld, Blocks=%ld\n",
                 remfact, tddg->dg_Heads, tddg->dg_TrackSectors,
                 tddg->dg_TotalSectors));

    tddg->dg_Cylinders = remfact;
    tddg->dg_CylSectors = tddg->dg_TrackSectors * tddg->dg_Heads;
}
/* \\\ */

struct	CapacityData
{
	ULONG	SectorCount;
	ULONG	SectorSize;
};

#define SERVICEACTION_CAPACITY16 0x10

/* /// "nGetGeometry()" */
LONG nGetGeometry(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    struct DriveGeometry *tddg;
    ULONG length;
    UBYTE cmd10[10];
    struct SCSICmd scsicmd;
    struct CapacityData capacitydata;
    UBYTE sensedata[18];
    LONG ioerr;
    ULONG tmpval;
    UBYTE *mpdata;
    BOOL gotblks = FALSE;
    BOOL gotcyl = FALSE;
    BOOL gotheads = FALSE;
    BOOL gotsect = FALSE;
    BOOL gotcylsect = FALSE;
    KPRINTF(10, ("\n"));

    ioreq->io_Error = 0;
    ioreq->io_Actual = 0;
    tddg = (struct DriveGeometry *) ioreq->io_Data;
    if(!tddg)
    {
        ioreq->io_Error = TDERR_NotSpecified;
        return(ioreq->io_Error);
    }
    length = min(ioreq->io_Length,sizeof(struct DriveGeometry));
    if((ncm->ncm_GeoChangeCount == ncm->ncm_ChangeCount) && ncm->ncm_Geometry.dg_TotalSectors)
    {
        // cached
        memcpy(tddg,&ncm->ncm_Geometry, (size_t) length);
        ioreq->io_Actual = length;
        return(0);
    }

    ncm->ncm_Geometry.dg_DeviceType = ncm->ncm_DeviceType;
    ncm->ncm_Geometry.dg_Flags = ncm->ncm_Removable ? DGF_REMOVABLE : 0;

    /*
     * Capacity10, 32bit Sectorcount
     */
    scsicmd.scsi_Data = (UWORD *) &capacitydata;
    scsicmd.scsi_Length = sizeof(capacitydata);
    scsicmd.scsi_Command = cmd10;
    scsicmd.scsi_CmdLength = 10;
    scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE;
    scsicmd.scsi_SenseData = sensedata;
    scsicmd.scsi_SenseLength = 18;
    cmd10[0] = SCSI_DA_READ_CAPACITY;
    cmd10[1] = 0;
    cmd10[2] = 0;
    cmd10[3] = 0;
    cmd10[4] = 0;
    cmd10[5] = 0;
    cmd10[6] = 0;
    cmd10[7] = 0;
    cmd10[8] = 0;
    cmd10[9] = 0;

    if((ioerr = nScsiDirect(ncm, &scsicmd)))
    {
        KPRINTF(10, ("ioerr %ld\n",ioerr));
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "SCSI_READ_CAPACITY failed: %ld",
                       ioerr);
        /*
         * memset(tddg,0,Length);
         * The error should be enough
         */
        return(ioreq->io_Error = ioerr);
    } else {
        /*
         * 32Bit Totalsectors
         * R.S.
         */
        ncm->ncm_BlockSize = ncm->ncm_Geometry.dg_SectorSize = AROS_BE2LONG(capacitydata.SectorSize);
        ncm->ncm_Geometry.dg_TotalSectors = AROS_BE2LONG(capacitydata.SectorCount) + 1;
        if(capacitydata.SectorCount == 0xffffffff)
        {
            ncm->ncm_Geometry.dg_TotalSectors--; // set to maximum
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Capacity exceeds the maximum supported for 32 bit sector counts (usually >2TB)!");
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "I/O operations will still work, but geometry and partitioning will be unreliable!");
        }
        KPRINTF(10, ("blocksize %ld totalsectors %ld\n", ncm->ncm_BlockSize, ncm->ncm_Geometry.dg_TotalSectors));
        gotblks = TRUE;
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "Capacity: %ld blocks of %ld bytes", ncm->ncm_Geometry.dg_TotalSectors, ncm->ncm_Geometry.dg_SectorSize);
        }
        ncm->ncm_BlockShift = 0;
        while((1<<ncm->ncm_BlockShift) < ncm->ncm_BlockSize)
        {
            ncm->ncm_BlockShift++;
        }
    }
    KPRINTF(10, ("PatchFlags 0x%lx DeviceType %ld\n",ncm->ncm_CDC->cdc_PatchFlags,ncm->ncm_DeviceType));

    if(!((ncm->ncm_CDC->cdc_PatchFlags & PFF_SIMPLE_SCSI) ||
         (ncm->ncm_DeviceType == PDT_WORM) ||
         (ncm->ncm_DeviceType == PDT_CDROM)))
    {
        KPRINTF(10, ("SIMPLE_SCSI or PDT_WORM/CDROM\n"));
        // cd roms don't have valid or sensible capacity mode pages
        if((mpdata = nGetModePage(ncm, 0x03)))
        {
            if((tmpval = (mpdata[10]<<8)+mpdata[11]))
            {
                ncm->ncm_Geometry.dg_TrackSectors = tmpval;
                gotsect = TRUE;
                if(!ncm->ncm_Geometry.dg_Cylinders)
                {
                    ncm->ncm_Geometry.dg_Cylinders = ncm->ncm_Geometry.dg_TotalSectors;
                }
            }
            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Capacity: TrackSectors=%ld",
                               ncm->ncm_Geometry.dg_TrackSectors);
            }
        }
    }
    /*
     * Cylinder and co are only defined for old 32Bit Totalsectors.
     * >2TB they have no meaning anyway, so they are calculated based
     * on 32Bit INT_MAX.
     */
    // recheck, could have simple scsi enabled in the meanwhile
    if(!((ncm->ncm_CDC->cdc_PatchFlags & PFF_SIMPLE_SCSI) ||
         (ncm->ncm_DeviceType == PDT_WORM) ||
         (ncm->ncm_DeviceType == PDT_CDROM)))
    {
        KPRINTF(10, ("recheck1 SIMPLE_SCSI or PDT_WORM/CDROM\n"));
        if((mpdata = nGetModePage(ncm, 0x04)))
        {
            if((tmpval = (mpdata[2]<<16)+(mpdata[3]<<8)+mpdata[4]))
            {
                ncm->ncm_Geometry.dg_Cylinders = tmpval;
                ncm->ncm_Geometry.dg_Heads = mpdata[5];
                gotcyl = gotheads = TRUE;
            }
            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Capacity: Cylinders=%ld, Heads=%ld",
                               ncm->ncm_Geometry.dg_Cylinders, ncm->ncm_Geometry.dg_Heads);
            }
        }
    }
    // recheck, could have simple scsi enabled in the meanwhile
    if(!((ncm->ncm_CDC->cdc_PatchFlags & PFF_SIMPLE_SCSI) ||
         (ncm->ncm_DeviceType == PDT_WORM) ||
         (ncm->ncm_DeviceType == PDT_CDROM)))
    {
        KPRINTF(10, ("recheck2 SIMPLE_SCSI or PDT_WORM/CDROM\n"));
        if((mpdata = nGetModePage(ncm, 0x05)))
        {
            if((tmpval = (mpdata[8]<<8)+mpdata[9]))
            {
                ncm->ncm_Geometry.dg_Cylinders = tmpval;
                ncm->ncm_Geometry.dg_Heads = mpdata[4];
                ncm->ncm_Geometry.dg_TrackSectors = mpdata[5];

                gotcyl = gotheads = gotsect = TRUE;

                if(!gotblks)
                {
                    ncm->ncm_BlockSize = ncm->ncm_Geometry.dg_SectorSize = (mpdata[6]<<8)+mpdata[7];
                    ncm->ncm_BlockShift = 0;
                    while((1<<ncm->ncm_BlockShift) < ncm->ncm_BlockSize)
                    {
                        ncm->ncm_BlockShift++;
                    }
                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                    {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Capacity: %ld blocks of %ld bytes", ncm->ncm_Geometry.dg_TotalSectors, ncm->ncm_BlockSize);
                    }
                }
                else if(ncm->ncm_BlockSize != (mpdata[6]<<8)+mpdata[7])
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Inconsistent block size information %ld != %ld!",
                                   ncm->ncm_BlockSize, (mpdata[6]<<8)+mpdata[7]);
                }
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Capacity: Cylinders=%ld, Heads=%ld, TrackSectors=%ld",
                                   ncm->ncm_Geometry.dg_Cylinders, ncm->ncm_Geometry.dg_Heads, ncm->ncm_Geometry.dg_TrackSectors);
                }
            }
        }
    }
    // missing total blocks?
    if((!gotblks) && gotcyl && gotheads && gotsect)
    {
        ncm->ncm_Geometry.dg_TotalSectors = ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_Heads * ncm->ncm_Geometry.dg_TrackSectors;
        gotblks = TRUE;
    }
    // missing cylinders?
    if(gotblks && (!gotcyl) && gotheads && gotsect)
    {
        ncm->ncm_Geometry.dg_Cylinders = ncm->ncm_Geometry.dg_TotalSectors / (ncm->ncm_Geometry.dg_Heads * ncm->ncm_Geometry.dg_TrackSectors);
        gotcyl = TRUE;
    }
    // missing heads?
    if(gotblks && gotcyl && (!gotheads) && gotsect)
    {
        ncm->ncm_Geometry.dg_Heads = ncm->ncm_Geometry.dg_TotalSectors / (ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_TrackSectors);
        gotheads = TRUE;
    }
    // missing tracks per sector
    if(gotblks && gotcyl && gotheads && (!gotsect))
    {
        ncm->ncm_Geometry.dg_TrackSectors = ncm->ncm_Geometry.dg_TotalSectors / (ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_Heads);
        gotsect = TRUE;
    }

    if(gotblks && gotcyl && gotheads && gotsect &&
       (ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_TrackSectors * ncm->ncm_Geometry.dg_Heads == ncm->ncm_Geometry.dg_TotalSectors - 1) &&
       (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_CAPACITY)))
    {
        if(!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK))
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_FIX_CAPACITY;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling Fix Capacity.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
            ncm->ncm_Geometry.dg_TotalSectors--;
        } else {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Fix Capacity is probably needed for this device. Please check this.");
        }
    }
    else if(gotblks && gotcyl && gotheads && gotsect &&
            (ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_TrackSectors * ncm->ncm_Geometry.dg_Heads == ncm->ncm_Geometry.dg_TotalSectors + 1) &&
            ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_CAPACITY)
    {
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Fix Capacity is probably enabled incorrectly. Please check this.");
        } else {
            ncm->ncm_CDC->cdc_PatchFlags &= ~PFF_FIX_CAPACITY;
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Disabling Fix Capacity.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
            ncm->ncm_Geometry.dg_TotalSectors++;
        }
    }

    // some devices report these bogus values regardless of actual device capacity, though the total number of blocks is correct.
    if(((ncm->ncm_Geometry.dg_Cylinders == 500) && (ncm->ncm_Geometry.dg_TrackSectors == 32) && (ncm->ncm_Geometry.dg_Heads == 8)) ||
       ((ncm->ncm_Geometry.dg_Cylinders == 16383) && (ncm->ncm_Geometry.dg_TrackSectors == 63) && (ncm->ncm_Geometry.dg_Heads == 16)))
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Firmware returns known bogus geometry, will fall back to faked geometry!");
        gotheads = gotcyl = gotsect = FALSE;
        if((ncm->ncm_CDC->cdc_PatchFlags & (PFF_SIMPLE_SCSI|PFF_NO_FALLBACK)) == PFF_SIMPLE_SCSI)
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_SIMPLE_SCSI;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling Simple SCSI.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
        }
    }

    // missing more than one?
    if(gotblks && (!(gotheads && gotcyl && gotsect)))
    {
        nFakeGeometry(ncm, &ncm->ncm_Geometry);
    }
    if(!gotcylsect)
    {
        ncm->ncm_Geometry.dg_CylSectors = ncm->ncm_Geometry.dg_TrackSectors * ncm->ncm_Geometry.dg_Heads;
    }

    if(ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_CylSectors != ncm->ncm_Geometry.dg_TotalSectors)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Estimated Geometry yields %ld less total blocks %ld: Cylinders=%ld, CylSectors=%ld, Heads=%ld, TrackSectors=%ld, Blocks=%ld",
                       ncm->ncm_Geometry.dg_TotalSectors - ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_CylSectors,
                       ncm->ncm_Geometry.dg_Cylinders * ncm->ncm_Geometry.dg_CylSectors,
                       ncm->ncm_Geometry.dg_Cylinders, ncm->ncm_Geometry.dg_CylSectors, ncm->ncm_Geometry.dg_Heads, ncm->ncm_Geometry.dg_TrackSectors,
                       ncm->ncm_Geometry.dg_TotalSectors);
    }
    else if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
    {
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Capacity: Cylinders=%ld, CylSectors=%ld, Heads=%ld, TrackSectors=%ld, Blocks=%ld, SectorSize=%ld",
                       ncm->ncm_Geometry.dg_Cylinders, ncm->ncm_Geometry.dg_CylSectors, ncm->ncm_Geometry.dg_Heads, ncm->ncm_Geometry.dg_TrackSectors,
                       ncm->ncm_Geometry.dg_TotalSectors, ncm->ncm_Geometry.dg_SectorSize);
    }

    ncm->ncm_Geometry.dg_BufMemType = MEMF_PUBLIC;
    ncm->ncm_Geometry.dg_Reserved = 0;
    memcpy(tddg, &ncm->ncm_Geometry, (size_t) length);
    ioreq->io_Actual = length;
    ncm->ncm_GeoChangeCount = ncm->ncm_ChangeCount;
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nStartStop()" */
LONG nStartStop(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    UBYTE cmd6[6];
    struct SCSICmd scsicmd;
    UBYTE sensedata[18];
    LONG ioerr;

    if((ncm->ncm_CDC->cdc_PatchFlags & PFF_SIMPLE_SCSI)
        && ioreq->io_Command != TD_EJECT)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Simple SCSI: Ignoring START_STOP_UNIT command");
        return(0);
    }
    scsicmd.scsi_Data = NULL;
    scsicmd.scsi_Length = 0;
    scsicmd.scsi_Command = cmd6;
    scsicmd.scsi_CmdLength = 6;
    scsicmd.scsi_Flags = SCSIF_AUTOSENSE|0x80;
    scsicmd.scsi_SenseData = sensedata;
    scsicmd.scsi_SenseLength = 18;
    cmd6[0] = SCSI_DA_START_STOP_UNIT;
    cmd6[1] = 0;
    cmd6[2] = 0;
    cmd6[3] = 0;
    switch(ioreq->io_Command)
    {
        case CMD_START:
            cmd6[4] = 0x01;
            break;

        case CMD_STOP:
            cmd6[4] = 0x00;
            break;

        case TD_EJECT:
            cmd6[4] = ioreq->io_Length ? 0x02 : 0x03;
            break;
    }
    cmd6[5] = 0;
    if((ioerr = nScsiDirect(ncm, &scsicmd)))
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "START_STOP_UNIT failed: %ld",
                       ioerr);
        ioreq->io_Error = TDERR_NotSpecified;
    }
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nRead64Emul()" */
LONG nRead64Emul(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    UBYTE cmd10[10];
    UBYTE sensedata[18];
    ULONG dataoffset = 0;
    ULONG dataremain = ioreq->io_Length;
    ULONG datalen;
    ULONG startblock;
    ULONG maxtrans = 1UL<<(ncm->ncm_CDC->cdc_MaxTransfer+16);
    ULONG insideblockoffset;
    struct SCSICmd scsicmd;

    if(dataremain & 511)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Attempt to read partial block (%ld %% %ld != 0)!",
                       dataremain, ncm->ncm_BlockSize);
        ioreq->io_Actual = 0;
        return(ioreq->io_Error = IOERR_BADLENGTH);
    }
    if(ioreq->io_Offset & 511)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Attempt to read unaligned block (%ld %% %ld != 0)!",
                       ioreq->io_Offset, ncm->ncm_BlockSize);
        ioreq->io_Actual = 0;
        return(ioreq->io_Error = IOERR_BADADDRESS);
    }

    if(!ncm->ncm_OneBlock || (ncm->ncm_OneBlockSize < ncm->ncm_BlockSize))
    {
        psdFreeVec(ncm->ncm_OneBlock);
        if(!(ncm->ncm_OneBlock = psdAllocVec(ncm->ncm_BlockSize)))
        {
            return(IOERR_BADADDRESS);
        }
        ncm->ncm_OneBlockSize = ncm->ncm_BlockSize;
    }

    startblock = (ioreq->io_Offset>>ncm->ncm_BlockShift)|(ioreq->io_Actual<<(32-ncm->ncm_BlockShift));
    insideblockoffset = (ioreq->io_Offset & ((1<<ncm->ncm_BlockShift)-1));
    while(dataremain)
    {
        KPRINTF(10, ("Reading from block %ld, %ld bytes left...\n", startblock, dataremain));
        datalen = dataremain;
        if(datalen > maxtrans)
        {
            datalen = maxtrans;
        }
        if(insideblockoffset || (datalen < ncm->ncm_BlockSize))
        {
            if(datalen > ncm->ncm_BlockSize-insideblockoffset)
            {
                datalen = ncm->ncm_BlockSize-insideblockoffset;
            }
            scsicmd.scsi_Data = (UWORD *) ncm->ncm_OneBlock;
            scsicmd.scsi_Length = ncm->ncm_BlockSize;
            scsicmd.scsi_Command = cmd10;
            scsicmd.scsi_CmdLength = 10;
            scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
            scsicmd.scsi_SenseData = sensedata;
            scsicmd.scsi_SenseLength = 18;
            cmd10[0] = SCSI_DA_READ_10;
            cmd10[1] = 0;
            *((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
            cmd10[6] = 0;
            cmd10[7] = 0;
            cmd10[8] = 1;
            cmd10[9] = 0;
            if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
            {
                KPRINTF(10, ("Read error!\n"));
                break;
            }
            CopyMemQuick(&ncm->ncm_OneBlock[insideblockoffset], &(((UBYTE *) ioreq->io_Data)[dataoffset]), datalen);
            insideblockoffset = 0;
            startblock++;
        } else {
            scsicmd.scsi_Data = (UWORD *) &(((UBYTE *) ioreq->io_Data)[dataoffset]);
            scsicmd.scsi_Length = datalen;
            scsicmd.scsi_Command = cmd10;
            scsicmd.scsi_CmdLength = 10;
            scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
            scsicmd.scsi_SenseData = sensedata;
            scsicmd.scsi_SenseLength = 18;
            cmd10[0] = SCSI_DA_READ_10;
            cmd10[1] = 0;
            *((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
            cmd10[6] = 0;
            cmd10[7] = datalen>>(ncm->ncm_BlockShift+8);
            cmd10[8] = datalen>>ncm->ncm_BlockShift;
            cmd10[9] = 0;
            if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
            {
                KPRINTF(10, ("Read error!\n"));
                break;
            }
            startblock += (insideblockoffset+datalen)>>ncm->ncm_BlockShift;
        }
        dataoffset += datalen;
        dataremain -= datalen;
    }
    ioreq->io_Actual = dataoffset;
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nWrite64Emul()" */
LONG nWrite64Emul(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    UBYTE cmd10[10];
    UBYTE sensedata[18];
    ULONG dataoffset = 0;
    ULONG dataremain = ioreq->io_Length;
    ULONG datalen;
    ULONG startblock;
    ULONG maxtrans = 1UL<<(ncm->ncm_CDC->cdc_MaxTransfer+16);
    ULONG insideblockoffset;
    struct SCSICmd scsicmd;

    if(dataremain & 511)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Attempt to write partial block (%ld %% %ld != 0)!",
                       dataremain, ncm->ncm_BlockSize);
        ioreq->io_Actual = 0;
        return(ioreq->io_Error = IOERR_BADLENGTH);
    }
    if(ioreq->io_Offset & 511)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Attempt to write unaligned block (%ld %% %ld != 0)!",
                       ioreq->io_Offset, ncm->ncm_BlockSize);
        ioreq->io_Actual = 0;
        return(ioreq->io_Error = IOERR_BADADDRESS);
    }

    if(!ncm->ncm_OneBlock || (ncm->ncm_OneBlockSize < ncm->ncm_BlockSize))
    {
        psdFreeVec(ncm->ncm_OneBlock);
        if(!(ncm->ncm_OneBlock = psdAllocVec(ncm->ncm_BlockSize)))
        {
            return(IOERR_BADADDRESS);
        }
        ncm->ncm_OneBlockSize = ncm->ncm_BlockSize;
    }

    startblock = (ioreq->io_Offset>>ncm->ncm_BlockShift)|(ioreq->io_Actual<<(32-ncm->ncm_BlockShift));
    insideblockoffset = (ioreq->io_Offset & ((1<<ncm->ncm_BlockShift)-1));
    while(dataremain)
    {
        KPRINTF(10, ("Writing from block %ld, %ld bytes left...\n", startblock, dataremain));
        datalen = dataremain;
        if(datalen > maxtrans)
        {
            datalen = maxtrans;
        }
        if(insideblockoffset || (datalen < ncm->ncm_BlockSize))
        {
            if(datalen > ncm->ncm_BlockSize-insideblockoffset)
            {
                datalen = ncm->ncm_BlockSize-insideblockoffset;
            }
            scsicmd.scsi_Data = (UWORD *) ncm->ncm_OneBlock;
            scsicmd.scsi_Length = ncm->ncm_BlockSize;
            scsicmd.scsi_Command = cmd10;
            scsicmd.scsi_CmdLength = 10;
            scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
            scsicmd.scsi_SenseData = sensedata;
            scsicmd.scsi_SenseLength = 18;
            cmd10[0] = SCSI_DA_READ_10;
            cmd10[1] = 0;
            *((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
            cmd10[6] = 0;
            cmd10[7] = 0;
            cmd10[8] = 1;
            cmd10[9] = 0;
            if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
            {
                KPRINTF(10, ("Read error!\n"));
                break;
            }
            CopyMemQuick(&(((UBYTE *) ioreq->io_Data)[dataoffset]), &ncm->ncm_OneBlock[insideblockoffset], datalen);

            //scsicmd.scsi_Data = (UWORD *) ncm->ncm_OneBlock;
            //scsicmd.scsi_Length = ncm->ncm_BlockSize;
            //scsicmd.scsi_Command = cmd10;
            //scsicmd.scsi_CmdLength = 10;
            scsicmd.scsi_Flags = SCSIF_WRITE|SCSIF_AUTOSENSE|0x80;
            //scsicmd.scsi_SenseData = sensedata;
            //scsicmd.scsi_SenseLength = 18;
            cmd10[0] = SCSI_DA_WRITE_10;
            //cmd10[1] = 0;
            //*((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
            //cmd10[6] = 0;
            //cmd10[7] = 0;
            //cmd10[8] = 1;
            //cmd10[9] = 0;
            if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
            {
                KPRINTF(10, ("Write error!\n"));
                break;
            }
            insideblockoffset = 0;
            startblock++;
        } else {
            scsicmd.scsi_Data = (UWORD *) &(((UBYTE *) ioreq->io_Data)[dataoffset]);
            scsicmd.scsi_Length = datalen;
            scsicmd.scsi_Command = cmd10;
            scsicmd.scsi_CmdLength = 10;
            scsicmd.scsi_Flags = SCSIF_WRITE|SCSIF_AUTOSENSE|0x80;
            scsicmd.scsi_SenseData = sensedata;
            scsicmd.scsi_SenseLength = 18;
            cmd10[0] = SCSI_DA_WRITE_10;
            cmd10[1] = 0;
            *((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
            cmd10[6] = 0;
            cmd10[7] = datalen>>(ncm->ncm_BlockShift+8);
            cmd10[8] = datalen>>ncm->ncm_BlockShift;
            cmd10[9] = 0;
            if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
            {
                KPRINTF(10, ("Write error!\n"));
                break;
            }
            startblock += (insideblockoffset+datalen)>>ncm->ncm_BlockShift;
        }
        dataoffset += datalen;
        dataremain -= datalen;
    }
    ioreq->io_Actual = dataoffset;
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nRead64()" */
LONG nRead64(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    UBYTE cmd10[10];
    UBYTE cmd16[16];
    UBYTE sensedata[18];
    ULONG dataoffset = 0;
    ULONG dataremain = ioreq->io_Length;
    ULONG datalen;
    ULONG startblockhigh;
    ULONG startblock;
    ULONG oldstartblock;
    ULONG maxtrans = 1UL<<(ncm->ncm_CDC->cdc_MaxTransfer+16);
    struct SCSICmd scsicmd;

    if(!ncm->ncm_BlockSize)
    {
        nGetBlockSize(ncm);
    }
    if((((dataremain >> ncm->ncm_BlockShift)<<ncm->ncm_BlockShift) != dataremain) ||
       (((ioreq->io_Offset >> ncm->ncm_BlockShift)<<ncm->ncm_BlockShift) != ioreq->io_Offset))
    {
        KPRINTF(20, ("unaligned read access offset %ld, length %ld...\n", ioreq->io_Offset, dataremain));
        if((!(ncm->ncm_CDC->cdc_PatchFlags & PFF_EMUL_LARGE_BLK)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK)))
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_EMUL_LARGE_BLK;
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling emulation for large block devices.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
        }
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_EMUL_LARGE_BLK)
        {
            return(nRead64Emul(ncm, ioreq));
        }
        if((dataremain >> ncm->ncm_BlockShift)<<ncm->ncm_BlockShift != dataremain)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to read partial block (%ld %% %ld != 0)!",
                           dataremain, ncm->ncm_BlockSize);
            ioreq->io_Error = IOERR_BADLENGTH;
        } else {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to read unaligned block (%ld %% %ld != 0)!",
                           ioreq->io_Offset, ncm->ncm_BlockSize);
            ioreq->io_Error = IOERR_BADADDRESS;
        }
        if(ncm->ncm_BlockSize != 512)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "The used FileSystem or other software must support %ld byte blocks!",
                           ncm->ncm_BlockSize);
        }
        ioreq->io_Actual = 0;
        return(ioreq->io_Error);
    }

    startblockhigh = ioreq->io_Actual>>ncm->ncm_BlockShift;
    startblock = (ioreq->io_Offset>>ncm->ncm_BlockShift)|(ioreq->io_Actual<<(32-ncm->ncm_BlockShift));
    while(dataremain)
    {
        KPRINTF(10, ("Reading from block %ld, %ld bytes left...\n", startblock, dataremain));
        datalen = dataremain;
        if(datalen > maxtrans)
        {
            datalen = maxtrans;
        }
        scsicmd.scsi_Data = (UWORD *) &(((UBYTE *) ioreq->io_Data)[dataoffset]);
        scsicmd.scsi_Length = datalen;
        scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
        scsicmd.scsi_SenseData = sensedata;
        scsicmd.scsi_SenseLength = 18;
        if(startblockhigh)
        {
            // Arithmetics for >2 TB needed
            scsicmd.scsi_Command = cmd16;
            scsicmd.scsi_CmdLength = 16;
            cmd16[0] = SCSI_DA_READ_16;
            cmd16[1] = 0;
            *((ULONG *) (&cmd16[2])) = AROS_LONG2BE(startblockhigh);
            *((ULONG *) (&cmd16[6])) = AROS_LONG2BE(startblock);
            cmd16[10] = datalen>>(ncm->ncm_BlockShift+24);
            cmd16[11] = datalen>>(ncm->ncm_BlockShift+16);
            cmd16[12] = datalen>>(ncm->ncm_BlockShift+8);
            cmd16[13] = datalen>>ncm->ncm_BlockShift;
            cmd16[14] = 0;
            cmd16[15] = 0;
        } else {
            scsicmd.scsi_Command = cmd10;
            scsicmd.scsi_CmdLength = 10;
            cmd10[0] = SCSI_DA_READ_10;
            cmd10[1] = 0;
            *((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
            cmd10[6] = 0;
            cmd10[7] = datalen>>(ncm->ncm_BlockShift+8);
            cmd10[8] = datalen>>ncm->ncm_BlockShift;
            cmd10[9] = 0;
        }
        if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
        {
            KPRINTF(10, ("Read error!\n"));
            break;
        }
        dataoffset += datalen;
        dataremain -= datalen;
        oldstartblock = startblock;
        startblock += datalen>>ncm->ncm_BlockShift;
        if(startblock < oldstartblock)
        {
            // wrap around occurred
            startblockhigh++;
        }
    }
    ioreq->io_Actual = dataoffset;
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nSeek64()" */
LONG nSeek64(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    UBYTE cmd10[10];
    UBYTE sensedata[18];
    struct SCSICmd scsicmd;
    ULONG startblock;

    if(!ncm->ncm_BlockSize)
    {
        nGetBlockSize(ncm);
    }
    if(((ioreq->io_Offset >> ncm->ncm_BlockShift)<<ncm->ncm_BlockShift) != ioreq->io_Offset)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Attempt to to seek to unaligned block (%ld %% %ld != 0)!",
                       ioreq->io_Offset, ncm->ncm_BlockSize);
        if(ncm->ncm_BlockSize != 512)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "The used FileSystem or other software must support %ld byte blocks!",
                           ncm->ncm_BlockSize);
        }
        ioreq->io_Actual = 0;
        return(ioreq->io_Error = IOERR_BADADDRESS);
    }

    startblock = (ioreq->io_Offset>>ncm->ncm_BlockShift)|(ioreq->io_Actual<<(32-ncm->ncm_BlockShift));
    scsicmd.scsi_Data = NULL;
    scsicmd.scsi_Length = 0;
    scsicmd.scsi_CmdLength = 10;
    scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
    scsicmd.scsi_SenseData = sensedata;
    scsicmd.scsi_SenseLength = 18;
    cmd10[0] = SCSI_DA_SEEK_10;
    cmd10[1] = 0;
    *((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
    cmd10[6] = 0;
    cmd10[7] = 0;
    cmd10[8] = 0;
    cmd10[9] = 0;
    if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
    {
        KPRINTF(10, ("Seek error!\n"));
    }
    ioreq->io_Actual = 0;
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nWrite64()" */
LONG nWrite64(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    UBYTE cmd10[10];
    UBYTE cmd16[16];
    UBYTE sensedata[18];
    ULONG dataoffset = 0;
    ULONG dataremain = ioreq->io_Length;
    ULONG datalen;
    ULONG startblockhigh;
    ULONG startblock;
    ULONG oldstartblock;
    ULONG maxtrans = 1UL<<(ncm->ncm_CDC->cdc_MaxTransfer+16);
    struct SCSICmd scsicmd;

    if(!ncm->ncm_BlockSize)
    {
        nGetBlockSize(ncm);
    }
    if(((dataremain >> ncm->ncm_BlockShift)<<ncm->ncm_BlockShift != dataremain) ||
       ((ioreq->io_Offset >> ncm->ncm_BlockShift)<<ncm->ncm_BlockShift != ioreq->io_Offset))
    {
        KPRINTF(20, ("unaligned write access offset %ld, length %ld...\n", ioreq->io_Offset, dataremain));
        if((!(ncm->ncm_CDC->cdc_PatchFlags & PFF_EMUL_LARGE_BLK)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK)))
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_EMUL_LARGE_BLK;
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling emulation for large block devices.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
        }
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_EMUL_LARGE_BLK)
        {
            return(nWrite64Emul(ncm, ioreq));
        }
        if((dataremain >> ncm->ncm_BlockShift)<<ncm->ncm_BlockShift != dataremain)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to write partial block (%ld %% %ld != 0)!",
                           dataremain, ncm->ncm_BlockSize);
            ioreq->io_Error = IOERR_BADLENGTH;
        } else {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Attempt to write unaligned block (%ld %% %ld != 0)!",
                           ioreq->io_Offset, ncm->ncm_BlockSize);
            ioreq->io_Error = IOERR_BADADDRESS;
        }
        if(ncm->ncm_BlockSize != 512)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "The used FileSystem or other software must support %ld byte blocks!",
                           ncm->ncm_BlockSize);
        }
        ioreq->io_Actual = 0;
        return(ioreq->io_Error);
    }

    startblockhigh = ioreq->io_Actual>>ncm->ncm_BlockShift;
    startblock = (ioreq->io_Offset>>ncm->ncm_BlockShift)|(ioreq->io_Actual<<(32-ncm->ncm_BlockShift));
    while(dataremain)
    {
        datalen = dataremain;
        if(datalen > maxtrans)
        {
            datalen = maxtrans;
        }
        scsicmd.scsi_Data = (UWORD *) &(((UBYTE *) ioreq->io_Data)[dataoffset]);
        scsicmd.scsi_Length = datalen;
        scsicmd.scsi_Flags = SCSIF_WRITE|SCSIF_AUTOSENSE|0x80;
        scsicmd.scsi_SenseData = sensedata;
        scsicmd.scsi_SenseLength = 18;
        if(startblockhigh)
        {
            // Arithmetics for >2 TB needed
            scsicmd.scsi_Command = cmd16;
            scsicmd.scsi_CmdLength = 16;
            cmd16[0] = SCSI_DA_WRITE_16;
            cmd16[1] = 0;
            *((ULONG *) (&cmd16[2])) = AROS_LONG2BE(startblockhigh);
            *((ULONG *) (&cmd16[6])) = AROS_LONG2BE(startblock);
            cmd16[10] = datalen>>(ncm->ncm_BlockShift+24);
            cmd16[11] = datalen>>(ncm->ncm_BlockShift+16);
            cmd16[12] = datalen>>(ncm->ncm_BlockShift+8);
            cmd16[13] = datalen>>ncm->ncm_BlockShift;
            cmd16[14] = 0;
            cmd16[15] = 0;
        } else {
            scsicmd.scsi_Command = cmd10;
            scsicmd.scsi_CmdLength = 10;
            cmd10[0] = SCSI_DA_WRITE_10;
            cmd10[1] = 0;
            *((ULONG *) (&cmd10[2])) = AROS_LONG2BE(startblock);
            cmd10[6] = 0;
            cmd10[7] = datalen>>(ncm->ncm_BlockShift+8);
            cmd10[8] = datalen>>ncm->ncm_BlockShift;
            cmd10[9] = 0;
        }
        if((ioreq->io_Error = nScsiDirect(ncm, &scsicmd)))
        {
            break;
        }
        dataoffset += datalen;
        dataremain -= datalen;
        oldstartblock = startblock;
        startblock += datalen>>ncm->ncm_BlockShift;
        if(startblock < oldstartblock)
        {
            // wrap around occurred
            startblockhigh++;
        }
    }
    ioreq->io_Actual = dataoffset;
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nCBIRequestSense()" */
LONG nCBIRequestSense(struct NepClassMS *ncm, UBYTE *senseptr, ULONG datalen)
{
    LONG ioerr;
    UBYTE sensecmd[12];
    LONG actual = 0;
    struct UsbMSCBIStatusWrapper umscsw;

    memset(sensecmd, 0, 12);
    senseptr[2] = SK_ILLEGAL_REQUEST;
    sensecmd[0] = SCSI_REQUEST_SENSE;
    sensecmd[1] = 0x00;
    sensecmd[2] = 0x00;
    sensecmd[3] = 0x00;
    sensecmd[4] = datalen;
    sensecmd[5] = 0;
    KPRINTF(2, ("sense command block phase...\n"));

    /*psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);*/
    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                 UMSR_ADSC, 0, (ULONG) ncm->ncm_UnitIfNum);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, sensecmd, ((ncm->ncm_CSType == MS_ATAPI_SUBCLASS) ||
                      (ncm->ncm_CSType == MS_FDDATAPI_SUBCLASS) ||
                      (ncm->ncm_CSType == MS_UFI_SUBCLASS)) ? (ULONG) 12 : (ULONG) 6);
    if(!ioerr)
    {
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_CLEAR_EP)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }

        KPRINTF(2, ("sense data phase %ld bytes...\n", datalen));
        ioerr = psdDoPipe(ncm->ncm_EPInPipe, senseptr, datalen);
        actual = psdGetPipeActual(ncm->ncm_EPInPipe);
        if(ioerr == UHIOERR_STALL)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }
        if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
        {
            KPRINTF(2, ("sense command status phase...\n"));
            if(ncm->ncm_TPType == MS_PROTO_CBI)
            {
                umscsw.bType = 0;
                umscsw.bValue = USMF_CSW_PHASEERR;
                ioerr = psdDoPipe(ncm->ncm_EPIntPipe, &umscsw, sizeof(struct UsbMSCBIStatusWrapper));
                if(ioerr && (ioerr != UHIOERR_RUNTPACKET))
                {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "Status interrupt failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    return(0);
                }
                umscsw.bValue &= USMF_CSW_PERSIST; /* mask out other bits */
            } else {
                umscsw.bType = 0;
                umscsw.bValue = USMF_CSW_PASS;
                ioerr = 0;
            }
            if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
            {
                KPRINTF(2, ("sense Status:\n"
                            "  Status   : %02lx\n",
                            umscsw.bValue));
                if(umscsw.bValue)
                {
                    /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Sense failed: %ld",
                                   umscsw.bValue);*/
                    if(umscsw.bValue == USMF_CSW_PHASEERR)
                    {
                        return(0);
                    }
                } else {
                    switch(senseptr[2] & SK_MASK)
                    {
                        case SK_UNIT_ATTENTION:
                            if((senseptr[12] == 0x28) ||
                               (senseptr[12] == 0x3A))
                            {
                                ncm->ncm_ChangeCount++;
                            }
                            break;
                    }

                    if((ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG) && (senseptr[2] & SK_MASK))
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "Request Sense Key %lx/%02lx/%02lx",
                                       senseptr[2] & SK_MASK,
                                       senseptr[12],
                                       senseptr[13]);
                    }
                }
            } else {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                              "Sense status failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
        } else {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Sense data failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }
    } else {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Sense block failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    return(actual);
}
/* \\\ */

/* /// "nBulkReset()" */
LONG nBulkReset(struct NepClassMS *ncm)
{
    LONG ioerr;
    LONG ioerr2 = 0;
    static UBYTE cbiresetcmd12[12] = { 0x1D, 0x04, 0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    //struct UsbMSCBIStatusWrapper umscsw;
    //UBYTE sensedata[18];
    if(ncm->ncm_DenyRequests)
    {
        return UHIOERR_TIMEOUT;
    }
    KPRINTF(1, ("Bulk Reset\n"));
    //psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Bulk Reset...");
    switch(ncm->ncm_TPType)
    {
        case MS_PROTO_BULK:
            if(!ncm->ncm_BulkResetBorks)
            {
                 psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                              UMSR_BULK_ONLY_RESET, 0, (ULONG) ncm->ncm_UnitIfNum);
                 ioerr2 = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                 if(ioerr2 == UHIOERR_TIMEOUT)
                 {
                     return(ioerr2);
                 }
                 if(ioerr2)
                 {
                     psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                    "BULK_ONLY_RESET failed: %s (%ld)",
                                    psdNumToStr(NTS_IOERR, ioerr2, "unknown"), ioerr2);
                     ncm->ncm_BulkResetBorks = TRUE;
                 }
                 if(ncm->ncm_DenyRequests)
                 {
                     return ioerr2;
                 }
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(ncm->ncm_DenyRequests)
            {
                return ioerr;
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPOutNum);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPOutNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            return(ioerr2 ? ioerr2 : ioerr);

        case MS_PROTO_CBI:
        case MS_PROTO_CB:
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                         UMSR_ADSC, 0, (ULONG) ncm->ncm_UnitIfNum);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, cbiresetcmd12, 12);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CBI_RESET failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(ncm->ncm_DenyRequests)
            {
                return ioerr;
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(ncm->ncm_DenyRequests)
            {
                return ioerr;
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPOutNum);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPOutNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            //nCBIRequestSense(ncm, sensedata, 18);
            return(ioerr);
    }
    return(0);
}
/* \\\ */

/* /// "nBulkClear()" */
LONG nBulkClear(struct NepClassMS *ncm)
{
    LONG ioerr;
    if(ncm->ncm_DenyRequests)
    {
        return UHIOERR_TIMEOUT;
    }
    KPRINTF(1, ("Bulk Clear\n"));
    //psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Bulk Clear...");
    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
    if(ioerr == UHIOERR_TIMEOUT)
    {
        return(ioerr);
    }
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                       ncm->ncm_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    if(ncm->ncm_DenyRequests)
    {
        return ioerr;
    }
    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPOutNum);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                       ncm->ncm_EPOutNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    return(ioerr);
}
/* \\\ */

/* /// "nScsiDirect()" */
LONG nScsiDirect(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    STRPTR prodname;
    STRPTR vendname;
    UBYTE cmd12[12];
    struct SCSICmd scsicmd10;
    BOOL usecmd10 = FALSE;
    LONG res;
    UBYTE *sensedata = NULL;
    UBYTE *buf;
    BOOL xlate = FALSE;
    ULONG pf = ncm->ncm_CDC->cdc_PatchFlags;

    scsicmd->scsi_Actual = 0;
    scsicmd->scsi_CmdActual = 0;
    scsicmd->scsi_SenseActual = 0;

    if(((pf & PFF_MODE_XLATE) && (scsicmd->scsi_CmdLength == 6)) ||
       (ncm->ncm_CSType == MS_FDDATAPI_SUBCLASS) ||
       (ncm->ncm_CSType == MS_ATAPI_SUBCLASS) ||
       (ncm->ncm_CSType == MS_UFI_SUBCLASS))
    {
        xlate = TRUE;
    }

    if(pf & PFF_SIMPLE_SCSI)
    {
        if(scsicmd->scsi_Flags & SCSIF_AUTOSENSE)
        {
            if(scsicmd->scsi_SenseLength > 18)
            {
                KPRINTF(10, ("Fixing Sense Length to 18!\n"));
                scsicmd->scsi_SenseLength = 18;
            }
        }
        switch(scsicmd->scsi_Command[0])
        {
            case SCSI_TEST_UNIT_READY:
            case SCSI_INQUIRY:
            //case SCSI_LOG_SELECT:
            //case SCSI_LOG_SENSE:
            case SCSI_REQUEST_SENSE:
            //case SCSI_MODE_SELECT_6:
            //case SCSI_MODE_SELECT_10:
            //case SCSI_MODE_SENSE_6:
            //case SCSI_MODE_SENSE_10:
            case SCSI_DA_READ_6:
            case SCSI_DA_READ_10:
            case SCSI_DA_READ_CAPACITY:
            case SCSI_DA_SEEK_6:
            case SCSI_DA_SEEK_10:
            case SCSI_DA_WRITE_6:
            case SCSI_DA_WRITE_10:
                break;

            case SCSI_MODE_SENSE_6:
            {
                UWORD modepage = scsicmd->scsi_Command[2] & 0x3f;
                UBYTE *data = (UBYTE *) scsicmd->scsi_Data;
                if((modepage == 0x3f) ||
                   (modepage == 0x03) ||
                   (modepage == 0x04) ||
                   (modepage == 0x05))
                {
                    if(!(ncm->ncm_BlockSize && ncm->ncm_Geometry.dg_TotalSectors))
                    {
                        nGetBlockSize(ncm);
                    }
                    if(ncm->ncm_Geometry.dg_TotalSectors)
                    {
                        nFakeGeometry(ncm, &ncm->ncm_Geometry);
                        ncm->ncm_GeoChangeCount = ncm->ncm_ChangeCount;
                    }
                    memset(data, 0, (size_t) scsicmd->scsi_Length);
                    scsicmd->scsi_Status = SCSI_GOOD;
                }
                if(modepage == 0x3f) // all available mode pages
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Simple SCSI: Faking All Mode Pages 0x03-0x05");
                    data[0] = 3+2+22+2+22+2+30;
                    data += 4;
                    scsicmd->scsi_Actual = 4;
                    if(scsicmd->scsi_Length >= 4+2+22)
                    {
                        data[0] = 0x03; // mode page
                        data[1] = 22; // page length
                        data[10] = ncm->ncm_Geometry.dg_TrackSectors>>8;
                        data[11] = ncm->ncm_Geometry.dg_TrackSectors;
                        data[12] = ncm->ncm_Geometry.dg_SectorSize>>8;
                        data[13] = ncm->ncm_Geometry.dg_SectorSize;
                        data += 2+22;
                        scsicmd->scsi_Actual += 2+22;
                    }
                    if(scsicmd->scsi_Length >= 4+2+22+2+22)
                    {
                        data[0] = 0x04; // mode page
                        data[1] = 22; // page length
                        data[2] = ncm->ncm_Geometry.dg_Cylinders>>16;
                        data[3] = ncm->ncm_Geometry.dg_Cylinders>>8;
                        data[4] = ncm->ncm_Geometry.dg_Cylinders;
                        data[5] = ncm->ncm_Geometry.dg_Heads;
                        data += 2+22;
                        scsicmd->scsi_Actual += 2+22;
                    }
                    if(scsicmd->scsi_Length >= 4+2+22+2+22+2+30)
                    {
                        data[0] = 0x05; // mode page
                        data[1] = 30; // page length
                        data[4] = ncm->ncm_Geometry.dg_Heads;
                        data[5] = ncm->ncm_Geometry.dg_TrackSectors;
                        data[6] = ncm->ncm_Geometry.dg_SectorSize>>8;
                        data[7] = ncm->ncm_Geometry.dg_SectorSize;
                        data[8] = ncm->ncm_Geometry.dg_Cylinders>>8;
                        data[9] = ncm->ncm_Geometry.dg_Cylinders;
                        //data += 2+30;
                        scsicmd->scsi_Actual += 2+30;
                    }
                    return(0);
                }
                else if((modepage == 0x03) && (scsicmd->scsi_Length >= 6+22)) // Format Device mode page
                {
                    // fake geometry request
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Simple SCSI: Faking Mode Page 0x03 (Format Device)");

                    data[0] = 5+22; // length
                    data += 4;
                    data[0] = 0x03; // mode page
                    data[1] = 22; // page length
                    data[10] = ncm->ncm_Geometry.dg_TrackSectors>>8;
                    data[11] = ncm->ncm_Geometry.dg_TrackSectors;
                    data[12] = ncm->ncm_Geometry.dg_SectorSize>>8;
                    data[13] = ncm->ncm_Geometry.dg_SectorSize;
                    scsicmd->scsi_Actual = 6+22;
                    return(0);
                }
                else if((modepage == 0x04) && (scsicmd->scsi_Length >= 6+22)) //  Rigid Disk Geometry mode page
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Simple SCSI: Faking Mode Page 0x04 (Rigid Disk Geometry)");
                    data[0] = 5+22; // length
                    data += 4;
                    data[0] = 0x04; // mode page
                    data[1] = 22; // page length
                    data[2] = ncm->ncm_Geometry.dg_Cylinders>>16;
                    data[3] = ncm->ncm_Geometry.dg_Cylinders>>8;
                    data[4] = ncm->ncm_Geometry.dg_Cylinders;
                    data[5] = ncm->ncm_Geometry.dg_Heads;
                    scsicmd->scsi_Actual = 6+22;
                    return(0);
                }
                else if((modepage == 0x05) && (scsicmd->scsi_Length >= 6+30)) // Flexible Disk mode page
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Simple SCSI: Faking Mode Page 0x05 (Flexible Disk)");
                    data[0] = 5+30; // length
                    data += 4;
                    data[0] = 0x05; // mode page
                    data[1] = 30; // page length
                    data[4] = ncm->ncm_Geometry.dg_Heads;
                    data[5] = ncm->ncm_Geometry.dg_TrackSectors;
                    data[6] = ncm->ncm_Geometry.dg_SectorSize>>8;
                    data[7] = ncm->ncm_Geometry.dg_SectorSize;
                    data[8] = ncm->ncm_Geometry.dg_Cylinders>>8;
                    data[9] = ncm->ncm_Geometry.dg_Cylinders;
                    scsicmd->scsi_Actual = 6+30;
                    return(0);
                }
                // fall through
            }

            case 0x51: // drive ready
            case SCSI_CD_READ_TOC:
            case SCSI_CD_PAUSE_RESUME:
            case SCSI_CD_PLAY_AUDIO_10:
            case SCSI_CD_PLAY_AUDIO_12:
            case SCSI_CD_PLAY_AUDIO_MSF:
            case SCSI_CD_PLAY_AUDIO_TRACK_INDEX:
            case SCSI_CD_PLAY_TRACK_RELATIVE_10:
            case SCSI_CD_PLAY_TRACK_RELATIVE_12:
            case SCSI_CD_BLANK:
            case SCSI_CD_CLOSE_TRACK:
            case SCSI_CD_GET_CONFIGURATION:
            case SCSI_CD_GET_EVENT_STATUS_NOTIFICATION:
            case SCSI_CD_GET_PERFORMANCE:
            case SCSI_CD_LOAD_UNLOAD_MEDIUM:
            case SCSI_DA_START_STOP_UNIT:
            case SCSI_CD_MECHANISM_STATUS:
            case SCSI_CD_PRE_FETCH:
            case SCSI_CD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            case SCSI_CD_READ_HEADER:
            case SCSI_CD_READ_SUB_CHANNEL:
                if((scsicmd->scsi_Command[0] != SCSI_MODE_SENSE_6) &&
                   ((ncm->ncm_DeviceType == PDT_WORM) || (ncm->ncm_DeviceType == PDT_CDROM)))
                {
                    // allows these CD rom commands even with SimpleSCSI enabled.
                    break;
                }

            default:
            {
                UBYTE cmdstrbuf[16*3+2];

                GM_UNIQUENAME(nHexString)(scsicmd->scsi_Command, (ULONG) (scsicmd->scsi_CmdLength < 16 ? scsicmd->scsi_CmdLength : 16), cmdstrbuf);
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Simple SCSI: Filtering SCSI command %s", cmdstrbuf);

                scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                scsicmd->scsi_SenseActual = 0;
                if(scsicmd->scsi_Flags & SCSIF_AUTOSENSE)
                {
                    if(scsicmd->scsi_SenseLength >= 18)
                    {
                        memset(scsicmd->scsi_SenseData, 0x00, 18);
                        scsicmd->scsi_SenseData[0] = 0x80|0x70;
                        scsicmd->scsi_SenseData[2] = SK_ILLEGAL_REQUEST;
                        scsicmd->scsi_SenseData[12] = 0x20; // unsupported command
                        scsicmd->scsi_SenseActual = 18;
                    }
                }
                return(-1);
            }
        }
    }

    if((scsicmd->scsi_Command[0] == SCSI_TEST_UNIT_READY) && scsicmd->scsi_Length)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Bogus TEST_UNIT_READY data length set to 0.");
        scsicmd->scsi_Length = 0;
    }

    if(pf & PFF_FAKE_INQUIRY)
    {
        if(scsicmd->scsi_Command[0] == SCSI_INQUIRY)
        {
            if(scsicmd->scsi_Length >= 36)
            {
                UBYTE *data = (UBYTE *) scsicmd->scsi_Data;
                if(pf & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Faking Inquiry.");
                }
                KPRINTF(10, ("Faking INQUIRY!\n"));
                psdGetAttrs(PGA_DEVICE, ncm->ncm_Device,
                            DA_ProductName, &prodname,
                            DA_Manufacturer, &vendname,
                            TAG_END);
                memset(data, 0x00, (size_t) scsicmd->scsi_Length);
                data[1] = 0x80; // removable media
                data[2] = 0x02; // 0x03 would be ANSI X3.301:1997 (SPC).
                data[3] = 0x02; // Response data format = 2
                data[4] = 32; // additional length n-4
                CopyMem(vendname, &data[8], (ULONG) ((strlen(vendname) < 8) ? strlen(vendname) : 8));
                CopyMem(prodname, &data[16], (ULONG) ((strlen(prodname) < 16) ? strlen(prodname) : 16));
                scsicmd->scsi_CmdActual = scsicmd->scsi_CmdLength;
                scsicmd->scsi_Status = SCSI_GOOD;
                scsicmd->scsi_SenseActual = 0;
                return(0);
            }
            return(-1);
        }
    }

    if(pf & PFF_FIX_INQ36)
    {
        if((scsicmd->scsi_Command)[0] == SCSI_INQUIRY)
        {
            if(scsicmd->scsi_Length > 36)
            {
                if(pf & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Fixing Inquiry.");
                }

                KPRINTF(10, ("Fixing INQUIRY!\n"));
                scsicmd->scsi_Length = 36;
            }
            else if(scsicmd->scsi_Length < 36)
            {
                if(pf & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Couldn't fix Inquiry < %ld.",
                                   scsicmd->scsi_Length);
                }
                KPRINTF(10, ("Couldn't fix INQUIRY!\n"));
            }
        }
    }

    if(xlate)
    {
        scsicmd10 = *scsicmd;
        scsicmd10.scsi_Command = cmd12;
        scsicmd10.scsi_CmdLength = 10;
        memset(cmd12, 0x00, 12);
        switch(scsicmd->scsi_Command[0])
        {
            case SCSI_DA_READ_6:
            case SCSI_DA_WRITE_6:
            case SCSI_DA_SEEK_6:
                cmd12[0] = scsicmd->scsi_Command[0] | 0x20;
                cmd12[1] = scsicmd->scsi_Command[1] & 0xe0;
                cmd12[3] = scsicmd->scsi_Command[1] & 0x1f;
                cmd12[4] = scsicmd->scsi_Command[2];
                cmd12[5] = scsicmd->scsi_Command[3];
                cmd12[8] = scsicmd->scsi_Command[4];
                cmd12[9] = scsicmd->scsi_Command[5];
                usecmd10 = TRUE;
                break;

            case SCSI_MODE_SELECT_6:
                cmd12[0] = SCSI_MODE_SELECT_10;
                cmd12[1] = scsicmd->scsi_Command[1];
                //cmd12[2] = scsicmd->scsi_Command[2]; // reserved
                //cmd12[3] = scsicmd->scsi_Command[3]; // reserved
                cmd12[7] = (scsicmd->scsi_Command[4]+4)>>8;
                cmd12[8] = scsicmd->scsi_Command[4]+4;
                cmd12[9] = scsicmd->scsi_Command[5];

                sensedata = psdAllocVec(scsicmd->scsi_Length+4);
                if(sensedata && (scsicmd->scsi_Length >= 4))
                {
                    buf = (UBYTE *) scsicmd->scsi_Data;
                    sensedata[1] = *buf++;
                    sensedata[2] = *buf++;
                    sensedata[3] = *buf++;
                    sensedata[7] = *buf++;

                    scsicmd10.scsi_Length = scsicmd->scsi_Length+4;
                    scsicmd10.scsi_Data = (UWORD *) sensedata;

                    CopyMem(buf, &sensedata[8], scsicmd->scsi_Length-4);
                }
                usecmd10 = TRUE;
                break;

            case SCSI_MODE_SENSE_6:
                cmd12[0] = SCSI_MODE_SENSE_10;
                cmd12[1] = scsicmd->scsi_Command[1] & 0xf7;
                cmd12[2] = scsicmd->scsi_Command[2];
                cmd12[3] = scsicmd->scsi_Command[3];
                // Workaround: Some devices are seriously broken and do not interpret
                // the upper byte of the allocation length field.
                // Hence they return 3 bytes instead of 259 bytes. For this special case,
                // we will simply truncate the size by four, to get back to a 255 byte
                // buffer.
                if((scsicmd->scsi_Command[4] > 251) && (scsicmd->scsi_Length == scsicmd->scsi_Command[4]))
                {
                    scsicmd->scsi_Command[4] -= 4;
                    scsicmd->scsi_Length -= 4;
                }
                cmd12[7] = (scsicmd->scsi_Command[4]+4)>>8;
                cmd12[8] = scsicmd->scsi_Command[4]+4;
                cmd12[9] = scsicmd->scsi_Command[5];
                sensedata = psdAllocVec(scsicmd->scsi_Length+4);
                if(sensedata)
                {
                    scsicmd10.scsi_Length = scsicmd->scsi_Length+4;
                    scsicmd10.scsi_Data = (UWORD *) sensedata;
                }
                usecmd10 = TRUE;
                break;
        }
    }

    if((ncm->ncm_CSType == MS_ATAPI_SUBCLASS) ||
       (ncm->ncm_CSType == MS_FDDATAPI_SUBCLASS) ||
       (ncm->ncm_CSType == MS_UFI_SUBCLASS))
    {
        if(!usecmd10)
        {
            scsicmd10 = *scsicmd;
            scsicmd10.scsi_Command = cmd12;
            memset(cmd12, 0x00, 12);
            CopyMem(scsicmd->scsi_Command, cmd12, (ULONG) scsicmd->scsi_CmdLength);
            usecmd10 = TRUE;
        }
        scsicmd10.scsi_CmdLength = 12;
        if(ncm->ncm_CSType == MS_UFI_SUBCLASS)
        {
            switch(cmd12[0])
            {
                case SCSI_REQUEST_SENSE:
                    cmd12[4] = 18; /* restrict UFI response to 18 bytes */
                    scsicmd10.scsi_Flags &= ~SCSIF_AUTOSENSE;
                    break;

                case SCSI_INQUIRY:
                    KPRINTF(10, ("Disabling autosense for UFI!\n"));
                    cmd12[4] = 36; /* restrict UFI response to 36 bytes */
                    scsicmd10.scsi_Flags &= ~SCSIF_AUTOSENSE;
                    break;

                case SCSI_MODE_SELECT_10:
                    cmd12[7] = 0;
                    cmd12[8] = 8;
                    break;

            }
        }
    }

    if(usecmd10)
    {
        if(pf & PFF_DEBUG)
        {
            UBYTE cmd1strbuf[16*3+2];
            UBYTE cmd2strbuf[16*3+2];

            GM_UNIQUENAME(nHexString)(scsicmd->scsi_Command, (ULONG) (scsicmd->scsi_CmdLength < 16 ? scsicmd->scsi_CmdLength : 16), cmd1strbuf);
            GM_UNIQUENAME(nHexString)(scsicmd10.scsi_Command, (ULONG) (scsicmd10.scsi_CmdLength < 16 ? scsicmd10.scsi_CmdLength : 16), cmd2strbuf);
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Mode XLATE for %s -> %s", cmd1strbuf, cmd2strbuf);
            if(scsicmd->scsi_Length && (scsicmd10.scsi_Command[0] == SCSI_MODE_SELECT_10))
            {
                GM_UNIQUENAME(nHexString)((UBYTE *) scsicmd->scsi_Data, (ULONG) (scsicmd->scsi_Length < 16 ? scsicmd->scsi_Length : 16), cmd1strbuf);
                GM_UNIQUENAME(nHexString)((UBYTE *) scsicmd10.scsi_Data, (ULONG) (scsicmd10.scsi_Length < 16 ? scsicmd10.scsi_Length : 16), cmd2strbuf);
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Request: %s (%ld) -> %s (%ld)",
                               cmd1strbuf, scsicmd->scsi_Length,
                               cmd2strbuf, scsicmd10.scsi_Length);
            }
        }
        KPRINTF(20, ("Mode XLATE for cmd %lx\n", scsicmd->scsi_Command[0]));
    }

    switch(ncm->ncm_TPType)
    {
        case MS_PROTO_BULK:
            res = nScsiDirectBulk(ncm, usecmd10 ? &scsicmd10 : scsicmd);
            break;

        case MS_PROTO_CB:
        case MS_PROTO_CBI:
            res = nScsiDirectCBI(ncm, usecmd10 ? &scsicmd10 : scsicmd);
            break;

        default:
            return(-1);
    }
    KPRINTF(20, ("Cmd done %ld\n", res));
    if(usecmd10)
    {
        if(sensedata)
        {
            scsicmd->scsi_Actual = 0;
            if(scsicmd10.scsi_Command[0] == SCSI_MODE_SENSE_10)
            {
                UBYTE cmd1strbuf[16*3+2];
                UBYTE cmd2strbuf[16*3+2];

                if(scsicmd10.scsi_Actual >= 8)
                {
                    scsicmd->scsi_Actual = scsicmd10.scsi_Actual - 4;
                    buf = (UBYTE *) scsicmd->scsi_Data;
                    *buf++ = sensedata[1];
                    *buf++ = sensedata[2];
                    *buf++ = sensedata[3];
                    *buf++ = sensedata[7];
                    CopyMem(&sensedata[8], buf, (ULONG) scsicmd10.scsi_Actual - 8);
                    if(pf & PFF_DEBUG)
                    {
                        GM_UNIQUENAME(nHexString)((UBYTE *) scsicmd10.scsi_Data, scsicmd10.scsi_Actual < 16 ? scsicmd10.scsi_Actual : 16, cmd1strbuf);
                        GM_UNIQUENAME(nHexString)((UBYTE *) scsicmd->scsi_Data, scsicmd->scsi_Actual < 16 ? scsicmd->scsi_Actual : 16, cmd2strbuf);
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "Response: %s (%ld) -> %s (%ld)",
                                       cmd1strbuf, scsicmd10.scsi_Actual,
                                       cmd2strbuf, scsicmd->scsi_Actual);
                    }
                }
            }
            psdFreeVec(sensedata);
        } else {
            scsicmd->scsi_Actual = scsicmd10.scsi_Actual;
        }
        scsicmd->scsi_CmdActual = scsicmd10.scsi_CmdActual;
        scsicmd->scsi_Status = scsicmd10.scsi_Status;
        scsicmd->scsi_SenseActual = scsicmd10.scsi_SenseActual;
    }

    pf = ncm->ncm_CDC->cdc_PatchFlags;
    if((res == HFERR_Phase) && (!(pf & PFF_NO_FALLBACK)) && (!(pf & PFF_SIMPLE_SCSI)))
    {
        switch(scsicmd->scsi_Command[0])
        {
            case SCSI_TEST_UNIT_READY:
            case SCSI_INQUIRY:
            //case SCSI_LOG_SELECT:
            //case SCSI_LOG_SENSE:
            case SCSI_REQUEST_SENSE:
            //case SCSI_MODE_SELECT_6:
            //case SCSI_MODE_SELECT_10:
            //case SCSI_MODE_SENSE_6:
            //case SCSI_MODE_SENSE_10:
            case SCSI_DA_READ_6:
            case SCSI_DA_READ_10:
            case SCSI_DA_READ_CAPACITY:
            case SCSI_DA_SEEK_6:
            case SCSI_DA_SEEK_10:
            case SCSI_DA_WRITE_6:
            case SCSI_DA_WRITE_10:
                break;

            default:
                ncm->ncm_CDC->cdc_PatchFlags |= PFF_SIMPLE_SCSI;
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Fallback: Enabling Simple SCSI.");
                GM_UNIQUENAME(nStoreConfig)(ncm);
                break;
        }
    }

    if((!res) && ((scsicmd->scsi_Command)[0] == SCSI_DA_READ_CAPACITY) && (pf & PFF_FIX_CAPACITY) && (scsicmd->scsi_Length >= 8))
    {
        ULONG *capacity = ((ULONG *) scsicmd->scsi_Data);
        *capacity = AROS_LONG2BE(AROS_BE2LONG(*capacity)-1);
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Fix Capacity: Correcting number of blocks.");
    }

    if(res && (scsicmd->scsi_Command[0] == SCSI_INQUIRY))
    {
        if((!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_INQ36)))
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_FIX_INQ36;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling Trim Inquiry.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
        }
        else if((!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_FAKE_INQUIRY)))
        {
            ncm->ncm_CDC->cdc_PatchFlags |= PFF_FAKE_INQUIRY;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Fallback: Enabling Fake Inquiry.");
            GM_UNIQUENAME(nStoreConfig)(ncm);
        }
    }

    if((ncm->ncm_DeviceType == PDT_WORM) ||
       (ncm->ncm_DeviceType == PDT_CDROM))
    {
        // cd roms are always write protected
        ncm->ncm_WriteProtect = TRUE;
        return(res);
    }
    // don't try to obtain write protection, when there's no media inserted.
    if(!ncm->ncm_UnitReady)
    {
        return(res);
    }
    if((!res) && (scsicmd->scsi_Command[0] == SCSI_MODE_SENSE_6))
    {
        if(((UBYTE *) scsicmd->scsi_Data)[2] & 0x80)
        {
            if(!ncm->ncm_WriteProtect)
            {
                if(pf & PFF_REM_SUPPORT)
                {
                    ncm->ncm_ChangeCount++;
                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                    {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Diskchange: Sense6WriteProtect On (count = %ld)",
                                       ncm->ncm_ChangeCount);
                    }
                }
                ncm->ncm_WriteProtect = TRUE;
            }
        } else {
            if(ncm->ncm_WriteProtect)
            {
                if(pf & PFF_REM_SUPPORT)
                {
                    ncm->ncm_ChangeCount++;
                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                    {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Diskchange: Sense6WriteProtect Off (count = %ld)",
                                       ncm->ncm_ChangeCount);
                    }
                }
                ncm->ncm_WriteProtect = FALSE;
            }
        }
    }
    if((!res) && (scsicmd->scsi_Command[0] == SCSI_MODE_SENSE_10))
    {
        if(((UBYTE *) scsicmd->scsi_Data)[3] & 0x80)
        {
            if(!ncm->ncm_WriteProtect)
            {
                if(pf & PFF_REM_SUPPORT)
                {
                    ncm->ncm_ChangeCount++;
                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                    {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Diskchange: Sense10WriteProtect On (count = %ld)",
                                       ncm->ncm_ChangeCount);
                    }
                }
                ncm->ncm_WriteProtect = TRUE;
            }
        } else {
            if(ncm->ncm_WriteProtect)
            {
                if(pf & PFF_REM_SUPPORT)
                {
                    ncm->ncm_ChangeCount++;
                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                    {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Diskchange: Sense10WriteProtect Off (count = %ld)",
                                       ncm->ncm_ChangeCount);
                    }
                }
                ncm->ncm_WriteProtect = FALSE;
            }
        }
    }
    return(res);
}
/* \\\ */

/* /// "nScsiDirectBulk()" */
LONG nScsiDirectBulk(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    LONG ioerr;
    struct PsdPipe *pp;
    struct UsbMSCmdBlkWrapper umscbw;
    struct UsbMSCmdStatusWrapper umscsw;
    ULONG datalen;
    LONG rioerr;
    UWORD retrycnt = 0;
    UBYTE cmdstrbuf[16*3+2];

    KPRINTF(10, ("\n"));

    GM_UNIQUENAME(nHexString)(scsicmd->scsi_Command, (ULONG) (scsicmd->scsi_CmdLength < 16 ? scsicmd->scsi_CmdLength : 16), cmdstrbuf);

    if(scsicmd->scsi_Flags & 0x80) /* Autoretry */
    {
        retrycnt = 1;
    }
    umscbw.dCBWSignature = AROS_LONG2LE(0x43425355);
    scsicmd->scsi_Status = SCSI_GOOD;
    nLockXFer(ncm);
    do
    {
        KPRINTF(10, ("retrycnt %ld\n",retrycnt));
        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        /*nBulkReset(ncm);*/

        rioerr = 0;

        /*psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                     USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
        ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);*/

        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
        {
            psdDelayMS(1);
        }

        datalen = scsicmd->scsi_Length;
        umscbw.dCBWTag = (IPTR) scsicmd + ++ncm->ncm_TagCount;
        umscbw.dCBWDataTransferLength = AROS_LONG2LE(datalen);
        umscbw.bmCBWFlags = scsicmd->scsi_Flags & SCSIF_READ ? 0x80 : 0x00;
        umscbw.bCBWLUN = ncm->ncm_UnitLUN;
        if((scsicmd->scsi_CmdLength) >= 16)
        {
            CopyMemQuick(scsicmd->scsi_Command, umscbw.CBWCB, 16);
            umscbw.bCBWCBLength = scsicmd->scsi_CmdActual = 16;
        } else {
            memset(umscbw.CBWCB, 0, 16);
            CopyMem(scsicmd->scsi_Command, umscbw.CBWCB, (ULONG) scsicmd->scsi_CmdLength);
            umscbw.bCBWCBLength = scsicmd->scsi_CmdActual = scsicmd->scsi_CmdLength;
        }
        //psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Issueing command %s Dlen=%ld", cmdstrbuf, datalen);

        KPRINTF(2, ("command block phase, tag %08lx, len %ld, flags %02lx...\n",
                umscbw.dCBWTag, scsicmd->scsi_CmdLength, scsicmd->scsi_Flags));
        KPRINTF(2, ("command: %s\n", cmdstrbuf));
        ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
        if(ioerr == UHIOERR_STALL) /* Retry on stall */
        {
            KPRINTF(2, ("stall...\n"));
            nBulkClear(ncm);
            ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
        }
        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        if(!ioerr)
        {
            if(datalen)
            {
                KPRINTF(2, ("data phase %ld bytes...\n", datalen));
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
                {
                    psdDelayMS(1);
                }
                pp = (scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInPipe : ncm->ncm_EPOutPipe;
                ioerr = psdDoPipe(pp, scsicmd->scsi_Data, datalen);
                scsicmd->scsi_Actual = psdGetPipeActual(pp);
                if(ioerr == UHIOERR_OVERFLOW)
                {
                    KPRINTF(10, ("Extra Data received, but ignored!\n"));
                    ioerr = 0;
                }
                else if(ioerr == UHIOERR_STALL) /* Accept on stall */
                {
                    KPRINTF(2, ("stall...\n"));
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                }
                else if(ioerr == UHIOERR_RUNTPACKET)
                {
                    KPRINTF(10, ("Runt packet ignored...\n"));
                    ioerr = 0;
                    /*psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ncm->ncm_EPInNum|URTF_IN);
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);*/
                }
            } else {
                ioerr = 0;
                scsicmd->scsi_Actual = 0;
            }
            if(!ioerr)
            {
                KPRINTF(2, ("command status phase...\n"));
                ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                if(ioerr == UHIOERR_STALL) /* Retry on stall */
                {
                    KPRINTF(2, ("stall...\n"));
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                    /*nBulkClear(ncm);*/
                    ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                }
                if(ioerr == UHIOERR_RUNTPACKET)
                {
                    // well, retry then
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Command status block truncated (%ld bytes), retrying...",
                                   psdGetPipeActual(ncm->ncm_EPInPipe));
                    ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                }
                if(ioerr == UHIOERR_OVERFLOW)
                {
                    KPRINTF(10, ("Extra Status received, but ignored!\n"));
                    ioerr = 0;
                }
                if(ncm->ncm_DenyRequests)
                {
                    rioerr = HFERR_Phase;
                    break;
                }
                if(!ioerr)
                {
                    KPRINTF(2, ("Status:\n"
                                "  Signature: %08lx\n"
                                "  Tag      : %08lx\n"
                                "  Residue  : %08lx\n"
                                "  Status   : %02lx\n",
                                umscsw.dCSWSignature,
                                umscsw.dCSWTag,
                                umscsw.dCSWDataResidue,
                                umscsw.bCSWStatus));
                    if(((umscsw.dCSWSignature != AROS_LONG2LE(0x53425355)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_CSS_BROKEN))) || (umscsw.dCSWTag != umscbw.dCBWTag))
                    {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Illegal command status block (Sig:%08lx, Tag=%08lx/%08lx (TX/RX), Len=%ld)",
                                       umscsw.dCSWSignature,
                                       umscbw.dCBWTag,
                                       umscsw.dCSWTag,
                                       psdGetPipeActual(ncm->ncm_EPInPipe));
                        scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                        rioerr = HFERR_Phase;
                        nBulkReset(ncm);
                        continue;
                    }
                    /* ignore this: too many firmwares report shit */
                    //scsicmd->scsi_Actual = datalen - AROS_LONG2LE(umscsw.dCSWDataResidue);
                    if((scsicmd->scsi_Actual > 7) && ((AROS_LONG2BE(*((ULONG *) scsicmd->scsi_Data))>>8) == 0x555342) && (((ULONG *) scsicmd->scsi_Data)[1] == umscbw.dCBWTag))
                    {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Your MSD has a very bad firmware! Havoc!");
                        scsicmd->scsi_Actual = 0;
                        umscsw.bCSWStatus = USMF_CSW_FAIL;
                    }
                    scsicmd->scsi_Status = umscsw.bCSWStatus;
                    if(umscsw.bCSWStatus)
                    {
                        if(umscsw.bCSWStatus == USMF_CSW_PHASEERR)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed: %ld", cmdstrbuf, umscsw.bCSWStatus);
                            nBulkReset(ncm);
                        }
                        /* Autosensing required? */
                        if(scsicmd->scsi_Flags & SCSIF_AUTOSENSE)
                        {
                            /*nBulkClear(ncm);*/

                            datalen = scsicmd->scsi_SenseLength;
                            umscbw.dCBWTag = (IPTR) scsicmd + ++ncm->ncm_TagCount;
                            umscbw.dCBWDataTransferLength = AROS_LONG2LE(datalen);
                            umscbw.bmCBWFlags = 0x80;
                            /*umscbw.bCBWLUN = ncm->ncm_UnitLun;*/
                            umscbw.bCBWCBLength = 6;
                            umscbw.CBWCB[0] = SCSI_REQUEST_SENSE;
                            umscbw.CBWCB[1] = 0x00;
                            umscbw.CBWCB[2] = 0x00;
                            umscbw.CBWCB[3] = 0x00;
                            umscbw.CBWCB[4] = datalen;
                            umscbw.CBWCB[5] = 0;
                            KPRINTF(2, ("sense command block phase...\n"));
                            ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
                            if(ioerr == UHIOERR_STALL) /* Retry on stall */
                            {
                                KPRINTF(2, ("stall...\n"));
                                nBulkClear(ncm);
                                ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
                            }
                            if(!ioerr)
                            {
                                KPRINTF(2, ("sense data phase %ld bytes...\n", datalen));
                                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
                                {
                                    psdDelayMS(1);
                                }
                                ioerr = psdDoPipe(ncm->ncm_EPInPipe, scsicmd->scsi_SenseData, datalen);
                                scsicmd->scsi_SenseActual = psdGetPipeActual(ncm->ncm_EPInPipe);
                                if(ioerr == UHIOERR_STALL) /* Accept on stall */
                                {
                                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
                                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                                }
                                if((ioerr == UHIOERR_RUNTPACKET) || (ioerr == UHIOERR_OVERFLOW))
                                {
                                    KPRINTF(10, ("Extra or less data received, but ignored!\n"));
                                    ioerr = 0;
                                }

                                if(!ioerr)
                                {
                                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
                                    {
                                        psdDelayMS(1);
                                    }
                                    KPRINTF(2, ("sense command status phase...\n"));
                                    ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                                    if(ioerr == UHIOERR_STALL) /* Retry on stall */
                                    {
                                        KPRINTF(2, ("stall...\n"));
                                        psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                                     USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
                                        ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                                        ioerr |= psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                                    }
                                    if(ioerr == UHIOERR_RUNTPACKET)
                                    {
                                        // well, retry then
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                          "Command (sense) status block truncated (%ld bytes), retrying...",
                                                          psdGetPipeActual(ncm->ncm_EPInPipe));
                                        ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                                    }

                                    if(ioerr == UHIOERR_OVERFLOW)
                                    {
                                        KPRINTF(10, ("Extra Status received, but ignored!\n"));
                                        ioerr = 0;
                                    }
                                    if(!ioerr)
                                    {
                                        KPRINTF(2, ("sense Status:\n"
                                                    "  Signature: %08lx\n"
                                                    "  Tag      : %08lx\n"
                                                    "  Residue  : %08lx\n"
                                                    "  Status   : %02lx\n",
                                                    umscsw.dCSWSignature,
                                                    umscsw.dCSWTag,
                                                    umscsw.dCSWDataResidue,
                                                    umscsw.bCSWStatus));
                                        if(((umscsw.dCSWSignature != AROS_LONG2LE(0x53425355)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_CSS_BROKEN))) || (umscsw.dCSWTag != umscbw.dCBWTag))
                                        {
                                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                          "Illegal command (sense) status block (Sig:%08lx, Tag=%08lx/%08lx (TX/RX), Len=%ld)",
                                                          umscsw.dCSWSignature,
                                                          umscbw.dCBWTag,
                                                          umscsw.dCSWTag,
                                                          psdGetPipeActual(ncm->ncm_EPInPipe));
                                            scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                                            rioerr = HFERR_Phase;
                                            nBulkReset(ncm);
                                            continue;
                                        }
                                        /* ignore this: too many firmwares report shit */
                                        //scsicmd->scsi_SenseActual = datalen - AROS_LONG2LE(umscsw.dCSWDataResidue);
                                        if((scsicmd->scsi_SenseActual > 7) && ((AROS_LONG2BE(*((ULONG *) scsicmd->scsi_SenseData))>>8) == 0x555342) && (((ULONG *) scsicmd->scsi_SenseData)[1] == umscbw.dCBWTag))
                                        {
                                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                          "Your MSD has a very bad firmware! Havoc!");
                                            scsicmd->scsi_Actual = 0;
                                            umscsw.bCSWStatus = USMF_CSW_FAIL;
                                        }

                                        if(umscsw.bCSWStatus)
                                        {
                                            /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                           "Sense failed: %ld",
                                                           umscsw.bCSWStatus);*/
                                            if(umscsw.bCSWStatus == USMF_CSW_PHASEERR)
                                            {
                                                nBulkReset(ncm);
                                            }
                                        } else {
                                            switch(scsicmd->scsi_SenseData[2] & SK_MASK)
                                            {
                                                case SK_ILLEGAL_REQUEST:
                                                case SK_NOT_READY:
                                                    retrycnt = 0;
                                                    break;
                                                case SK_DATA_PROTECT:
                                                    if(!ncm->ncm_WriteProtect)
                                                    {
                                                        ncm->ncm_WriteProtect = TRUE;
                                                        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                                        {
                                                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                                                           "WriteProtect On: Sense Data Protect");
                                                        }
                                                    }
                                                    break;

                                                case SK_UNIT_ATTENTION:
                                                    if((ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT) &&
                                                       ((scsicmd->scsi_SenseData[12] == 0x28) ||
                                                       (scsicmd->scsi_SenseData[12] == 0x3A)))
                                                    {
                                                        ncm->ncm_ChangeCount++;
                                                        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                                        {
                                                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                                                           "Diskchange: Unit Attention (count = %ld)",
                                                                           ncm->ncm_ChangeCount);
                                                        }
                                                    }
                                                    break;
                                            }
                                            KPRINTF(10, ("Sense Key: %lx/%02lx/%02lx\n",
                                                        scsicmd->scsi_SenseData[2] & SK_MASK,
                                                        scsicmd->scsi_SenseData[12],
                                                        scsicmd->scsi_SenseData[13]));
                                            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                            {
                                                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                               "Cmd %s: Sense Key %lx/%02lx/%02lx",
                                                               cmdstrbuf,
                                                               scsicmd->scsi_SenseData[2] & SK_MASK,
                                                               scsicmd->scsi_SenseData[12],
                                                               scsicmd->scsi_SenseData[13]);
                                            }
                                        }
                                    } else {
                                        KPRINTF(10, ("Sense status failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) okay, but:", cmdstrbuf);
                                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "Sense status failed: %s (%ld)",
                                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                        nBulkReset(ncm);
                                    }
                                } else {
                                    KPRINTF(10, ("Sense data failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) okay, but:", cmdstrbuf);
                                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "Sense data failed: %s (%ld)",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    nBulkReset(ncm);
                                }
                            } else {
                                KPRINTF(10, ("Sense block failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) okay, but:", cmdstrbuf);
                                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                               "Sense block failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                /*nBulkReset(ncm);*/
                            }
                        }
                        rioerr = HFERR_BadStatus;
                    }
                } else {
                    KPRINTF(10, ("Command status failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                  "Command status failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                    rioerr = HFERR_Phase;
                    nBulkReset(ncm);
                }
            } else {
                KPRINTF(10, ("Data phase failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "Data phase failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                rioerr = HFERR_Phase;
                nBulkReset(ncm);
            }
        } else {
            KPRINTF(10, ("Command block failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
            scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
            rioerr = HFERR_Phase;
            if(ioerr == UHIOERR_TIMEOUT)
            {
                break;
            }
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Command block failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            nBulkReset(ncm);
        }
        if(!rioerr)
        {
            break;
        }
        KPRINTF(1, ("Retrying...\n"));
    } while(retrycnt--);
    nUnlockXFer(ncm);
    return(rioerr);
}
/* \\\ */

/* /// "nScsiDirectCBI()" */
LONG nScsiDirectCBI(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    LONG ioerr;
    struct PsdPipe *pp;
    struct PsdPipe *backpp;
    struct UsbMSCBIStatusWrapper umscsw;
    ULONG datalen;
    LONG rioerr;
    UWORD retrycnt = 0;
    UBYTE sensedata[18];
    UBYTE *senseptr;
    UBYTE asc;
    BOOL datadone;
    BOOL statusdone;
    UBYTE cmdstrbuf[16*3+2];

    GM_UNIQUENAME(nHexString)(scsicmd->scsi_Command, (ULONG) (scsicmd->scsi_CmdLength < 16 ? scsicmd->scsi_CmdLength : 16), cmdstrbuf);

    if(scsicmd->scsi_Flags & 0x80) /* Autoretry */
    {
        retrycnt = 1;
    }
    scsicmd->scsi_Status = SCSI_GOOD;
    nLockXFer(ncm);
    do
    {
        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        rioerr = 0;
        datalen = scsicmd->scsi_Length;

        KPRINTF(2, ("command block phase, tag %08lx, len %ld, flags %02lx...\n",
                scsicmd, scsicmd->scsi_CmdLength, scsicmd->scsi_Flags));

        KPRINTF(2, ("command: %s\n", cmdstrbuf));

        //nBulkClear(ncm);
        scsicmd->scsi_CmdActual = scsicmd->scsi_CmdLength;
        /*if(datalen)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                         (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }*/
        psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                     UMSR_ADSC, 0, (ULONG) ncm->ncm_UnitIfNum);
        ioerr = psdDoPipe(ncm->ncm_EP0Pipe, scsicmd->scsi_Command, (ULONG) scsicmd->scsi_CmdLength);

        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        if(!ioerr)
        {
            datadone = statusdone = FALSE;
            if(datalen)
            {
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_CLEAR_EP)
                {
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                }

                KPRINTF(2, ("data phase %ld bytes...\n", datalen));
                pp = (scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInPipe : ncm->ncm_EPOutPipe;

                if(ncm->ncm_TPType == MS_PROTO_CBI)
                {
                    /* okay, this is a major pain in the arse.
                       we have to do this asynchroneously */
                    umscsw.bType = 0;
                    umscsw.bValue = USMF_CSW_PHASEERR;
                    psdSendPipe(ncm->ncm_EPIntPipe, &umscsw, sizeof(struct UsbMSCBIStatusWrapper));
                    psdSendPipe(pp, scsicmd->scsi_Data, datalen);
                    do
                    {
                        WaitPort(ncm->ncm_TaskMsgPort);
                        while((backpp = (struct PsdPipe *) GetMsg(ncm->ncm_TaskMsgPort)))
                        {
                            if(backpp == pp)
                            {
                                /* data transfer finished */
                                datadone = TRUE;
                            }
                            else if(backpp == ncm->ncm_EPIntPipe)
                            {
                                /* status returned */
                                statusdone = TRUE;
                            }
                        }
                    } while(!statusdone);
                    if(!datadone)
                    {
                        psdAbortPipe(pp);
                        psdWaitPipe(pp);
                        ioerr = 0;
                    } else {
                        ioerr = psdGetPipeError(pp);
                    }

                } else {
                    ioerr = psdDoPipe(pp, scsicmd->scsi_Data, datalen);
                }

                scsicmd->scsi_Actual = psdGetPipeActual(pp);
                if(ioerr == UHIOERR_OVERFLOW)
                {
                    KPRINTF(10, ("Extra Data received, but ignored!\n"));
                    ioerr = 0;
                }
                if(ioerr == UHIOERR_STALL) /* Accept on stall */
                {
                    KPRINTF(2, ("stall...\n"));
                    //nBulkClear(ncm);
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
                    psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                    ioerr = 0;
                }
            } else {
                ioerr = 0;
                scsicmd->scsi_Actual = 0;
            }
            if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
            {
                KPRINTF(2, ("command status phase...\n"));
                if(ncm->ncm_TPType == MS_PROTO_CBI)
                {
                    if(!statusdone)
                    {
                        if(ncm->ncm_CSType == MS_UFI_SUBCLASS)
                        {
                            umscsw.bType = 0x04;
                            umscsw.bValue = 0;
                        } else {
                            umscsw.bType = 0;
                            umscsw.bValue = USMF_CSW_PHASEERR;
                        }
                        ioerr = psdDoPipe(ncm->ncm_EPIntPipe, &umscsw, sizeof(struct UsbMSCBIStatusWrapper));
                    } else {
                        ioerr = psdGetPipeError(ncm->ncm_EPIntPipe);
                    }
                    umscsw.bValue &= 0x0f; /* mask out upper nibble */
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Status interrupt failed: %s (%ld)",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        nBulkReset(ncm);
                    }
                    if(ncm->ncm_CSType == MS_UFI_SUBCLASS)
                    {
                        asc = umscsw.bType;
                        if((scsicmd->scsi_Command[0] == SCSI_REQUEST_SENSE) ||
                           (scsicmd->scsi_Command[0] == SCSI_INQUIRY))
                        {
                            umscsw.bType = 0;
                            umscsw.bValue = USMF_CSW_PASS;
                        } else {
                            umscsw.bType = 0;
                            umscsw.bValue = asc ? USMF_CSW_FAIL : USMF_CSW_PASS;
                            if(umscsw.bValue)
                            {
                                rioerr = HFERR_BadStatus;
                            }
                        }
                    } else {
                        umscsw.bValue &= USMF_CSW_PERSIST; /* mask out other bits */
                    }
                } else {
                    umscsw.bType = 0;
                    umscsw.bValue = USMF_CSW_PASS;
                    ioerr = 0;
                }
                if(ncm->ncm_DenyRequests)
                {
                    rioerr = HFERR_Phase;
                    break;
                }
                if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
                {
                    scsicmd->scsi_Status = umscsw.bValue;
                    if(umscsw.bValue == USMF_CSW_PHASEERR)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "Command (%s) phase error: %ld",
                                       cmdstrbuf,
                                       umscsw.bValue);
                        nBulkReset(ncm);
                    }
                    else if(umscsw.bValue == USMF_CSW_PERSIST)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "Command (%s) persistant error: %ld",
                                       cmdstrbuf,
                                       umscsw.bValue);
                        nBulkReset(ncm);
                    }

                    /* Autosensing required? */
                    if(((umscsw.bValue && (scsicmd->scsi_Flags & SCSIF_AUTOSENSE))) ||
                       (ncm->ncm_TPType == MS_PROTO_CB) || (ncm->ncm_TPType == MS_PROTO_CBI))
                    {
                        if(scsicmd->scsi_Flags & SCSIF_AUTOSENSE)
                        {
                            datalen = scsicmd->scsi_SenseLength;
                            senseptr = scsicmd->scsi_SenseData;
                        } else {
                            datalen = 18;
                            senseptr = sensedata;
                        }
                        if(!(scsicmd->scsi_SenseActual = nCBIRequestSense(ncm, senseptr, datalen)))
                        {
                            nBulkReset(ncm);
                        }
                        if(senseptr[2] & SK_MASK)
                        {
                            rioerr = HFERR_BadStatus;
                            scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                        }
                    }
                } else {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                  "Command status failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                    rioerr = HFERR_Phase;
                    nBulkReset(ncm);
                }
            } else {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "Data phase failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                rioerr = HFERR_Phase;
                nBulkClear(ncm);
            }
        } else {
            scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
            rioerr = HFERR_Phase;
            if(ioerr == UHIOERR_TIMEOUT)
            {
                break;
            }
            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "Command block failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(scsicmd->scsi_Flags & SCSIF_AUTOSENSE)
            {
                datalen = scsicmd->scsi_SenseLength;
                senseptr = scsicmd->scsi_SenseData;
            } else {
                datalen = 18;
                senseptr = sensedata;
            }
            if(!(scsicmd->scsi_SenseActual = nCBIRequestSense(ncm, senseptr, datalen)))
            {
                nBulkReset(ncm);
                retrycnt = 0;
            } else {
                if(senseptr[2] & SK_MASK)
                {
                    rioerr = HFERR_BadStatus;
                    if((senseptr[2] & SK_MASK) == SK_NOT_READY)
                    {
                        retrycnt = 0;
                    }
                }
            }
        }
        if(!rioerr)
        {
            break;
        }
        KPRINTF(1, ("Retrying...\n"));
    } while(retrycnt--);
    nUnlockXFer(ncm);
    return(rioerr);
}
/* \\\ */

/* /// "nLockXFer()" */
void nLockXFer(struct NepClassMS *ncm)
{
    if(ncm->ncm_MaxLUN)
    {
        KPRINTF(1, ("PING(lock)\n"));
        ObtainSemaphore(&ncm->ncm_UnitLUN0->ncm_XFerLock);
        KPRINTF(1, ("PONG(lock)\n"));
    }
}
/* \\\ */

/* /// "nUnlockXFer()" */
void nUnlockXFer(struct NepClassMS *ncm)
{
    if(ncm->ncm_MaxLUN)
    {
        KPRINTF(1, ("PING(unlock)\n"));
        ReleaseSemaphore(&ncm->ncm_UnitLUN0->ncm_XFerLock);
        KPRINTF(1, ("PONG(unlock)\n"));
    }
}
/* \\\ */

/* /// "nStoreConfig()" */
BOOL GM_UNIQUENAME(nStoreConfig)(struct NepClassMS *ncm)
{
    APTR pic;
    struct NepClassMS *cncm;
    if(ncm->ncm_Interface)
    {
        pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), ncm->ncm_DevIDString, ncm->ncm_IfIDString);
        if(!pic)
        {
            psdSetUsbDevCfg(GM_UNIQUENAME(libname), ncm->ncm_DevIDString, ncm->ncm_IfIDString, NULL);
            pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), ncm->ncm_DevIDString, ncm->ncm_IfIDString);
        }
        if(pic)
        {
            psdAddCfgEntry(pic, ncm->ncm_CDC);
            cncm = ncm;
            while(((struct Node *) cncm)->ln_Succ)
            {
                if(cncm->ncm_UnitLUN0 != ncm)
                {
                    break;
                }
                psdAddCfgEntry(pic, cncm->ncm_CUC);
                cncm = (struct NepClassMS *) ((struct Node *) cncm)->ln_Succ;
            }
            return(TRUE);
        }
    } else {
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

/* ********************************************************************* */

/* /// "BSTR Macros" */
#ifdef __AROS__

#define b2cstr(bstr, cstr)\
    do\
    {\
        UWORD _i = 0;\
        UWORD _len = AROS_BSTR_strlen(bstr);\
        while(_i < _len)\
        {\
            cstr[_i] = AROS_BSTR_getchar(bstr, _i);\
            _i++;\
        }\
        cstr[_i] = '\0';\
    } while(0)

#define c2bstr(cstr, bstr)\
    do\
    {\
        UWORD _i = 0;\
        while(cstr[_i] != '\0')\
        {\
            AROS_BSTR_putchar(bstr, _i, cstr[_i]);\
            _i++;\
        }\
        AROS_BSTR_setstrlen(bstr, _i);\
    } while(0)
#else

#define b2cstr(bstr, cstr) { ULONG i; for (i = 0; i < bstr[0]; i++) cstr[i] = bstr[i + 1]; cstr[i] = 0x00; }

#define c2bstr(cstr, bstr)\
    do\
    {\
        int i = 0;\
        UBYTE c;\
        STRPTR cp = (STRPTR) (cstr);\
        STRPTR bp = (STRPTR) (bstr);\
        while((c = cp[i]))\
        {\
            bp[++i] = c;\
        }\
        bp[0] = i;\
    } while(0)

#endif
/* \\\ */

#undef  ps
#define ps nh->nh_PsdBase

/* /// "nRemovableTask()" */
AROS_UFH0(void, GM_UNIQUENAME(nRemovableTask))
{
    AROS_USERFUNC_INIT

    struct NepMSBase *nh;
    struct NepClassMS *ncm;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    struct SCSICmd scsicmd;
    UBYTE cmd6[6];
    UBYTE sensedata[18];
    struct IOStdReq *ioreq;
    BOOL dontquit = TRUE;

    if((nh = GM_UNIQUENAME(nAllocRT)()))
    {
        Forbid();
        if(nh->nh_ReadySigTask)
        {
            Signal(nh->nh_ReadySigTask, 1L<<nh->nh_ReadySignal);
        }
        Permit();
        /* Main task */
        sigmask = (1L<<nh->nh_TimerMsgPort->mp_SigBit)|
                  SIGBREAKF_CTRL_C;
        do
        {
            while((ioreq = (struct IOStdReq *) GetMsg(nh->nh_TimerMsgPort)))
            {
                dontquit = FALSE;
                KPRINTF(2, ("Timer interrupt\n"));
                ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
                while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
                {
                    if(ncm->ncm_Task && (!ncm->ncm_DenyRequests))
                    {
                        dontquit = TRUE;

                        if(ncm->ncm_Removable && ncm->ncm_Running)
                        {
                            scsicmd.scsi_Data = NULL;
                            scsicmd.scsi_Length = 0;
                            scsicmd.scsi_Command = cmd6;
                            scsicmd.scsi_CmdLength = 6;
                            scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
                            scsicmd.scsi_SenseData = sensedata;
                            scsicmd.scsi_SenseLength = 18;
                            cmd6[0] = SCSI_TEST_UNIT_READY;
                            cmd6[1] = 0;
                            cmd6[2] = 0;
                            cmd6[3] = 0;
                            cmd6[4] = 0;
                            cmd6[5] = 0;
                            if((ioerr = nScsiDirectTunnel(ncm, &scsicmd)))
                            {
                                KPRINTF(1, ("Test unit ready yielded: %ld/%ld\n", sensedata[2], sensedata[12]));
                                /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                               "SCSI_TEST_UNIT_READY failed: %ld",
                                               ioerr);*/
                                /* Check for MEDIUM NOT PRESENT */
                                if(((sensedata[2] & SK_MASK) == SK_NOT_READY) &&
                                   ((sensedata[12] == 0x3a) || (sensedata[12] == 0x04)))
                                {
                                    if(ncm->ncm_UnitReady)
                                    {
                                        ncm->ncm_UnitReady = FALSE;
                                        ncm->ncm_ChangeCount++;
                                        KPRINTF(10, ("Diskchange: Medium removed (count = %ld)!\n", ncm->ncm_ChangeCount));
                                        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                        {
                                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                                           "Diskchange: Medium removed (count = %ld)",
                                                           ncm->ncm_ChangeCount);
                                        }
                                    }
                                }
                            } else {
                                if(!ncm->ncm_UnitReady)
                                {
                                    ncm->ncm_UnitReady = TRUE;
                                    ncm->ncm_ChangeCount++;
                                    KPRINTF(10, ("Diskchange: Medium inserted (count = %ld)!\n", ncm->ncm_ChangeCount));
                                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                    {
                                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                                       "Diskchange: Medium inserted (count = %ld)",
                                                       ncm->ncm_ChangeCount);
                                    }
                                    if(ncm->ncm_CSType == MS_UFI_SUBCLASS)
                                    {
                                        nh->nh_IOReq.io_Command = CMD_START;
                                        nIOCmdTunnel(ncm, &nh->nh_IOReq);
                                    }
                                } else {
                                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT)
                                    {
                                        nGetWriteProtect(ncm);
                                    }
                                }
                            }
                        }
                        if(ncm->ncm_LastChange != ncm->ncm_ChangeCount)
                        {
                            if(ncm->ncm_UnitReady)
                            {
                                nGetWriteProtect(ncm);
                                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT)
                                {
                                    nh->nh_IOReq.io_Command = TD_GETGEOMETRY;
                                    nh->nh_IOReq.io_Data = &ncm->ncm_Geometry;
                                    nh->nh_IOReq.io_Length = sizeof(ncm->ncm_Geometry);
                                    nIOCmdTunnel(ncm, &nh->nh_IOReq);
                                }
                            }
                            ioreq = (struct IOStdReq *) ncm->ncm_DCInts.lh_Head;
                            while(((struct Node *) ioreq)->ln_Succ)
                            {
                                Cause(ioreq->io_Data);
                                ioreq = (struct IOStdReq *) ((struct Node *) ioreq)->ln_Succ;
                            }
                            if(ncm->ncm_UnitReady)
                            {
                                // obtain blocksize first
                                if(!ncm->ncm_BlockSize)
                                {
                                    nh->nh_IOReq.io_Command = TD_GETGEOMETRY;
                                    nh->nh_IOReq.io_Data = &ncm->ncm_Geometry;
                                    nh->nh_IOReq.io_Length = sizeof(ncm->ncm_Geometry);
                                    nIOCmdTunnel(ncm, &nh->nh_IOReq);
                                }
                                ncm->ncm_HasMounted = TRUE;

                                // find and mount partitions
                                if(!CheckPartitions(ncm) && ncm->ncm_CUC->cuc_AutoMountFAT)
                                {
                                    // check for FAT volume with no partition table
                                    CheckFATPartition(ncm, 0);
                                }
                                if((ncm->ncm_BlockSize == 2048) &&
                                   ((ncm->ncm_DeviceType == PDT_WORM) || (ncm->ncm_DeviceType == PDT_CDROM)))
                                {
                                    if(ncm->ncm_CUC->cuc_AutoMountCD)
                                    {
                                        CheckISO9660(ncm);
                                    }
                                }
                            }
                            ncm->ncm_LastChange = ncm->ncm_ChangeCount;
                        }
                    } else {
                        if(ncm->ncm_DenyRequests && ncm->ncm_CUC->cuc_AutoUnmount && ncm->ncm_HasMounted)
                        {
                            nUnmountPartition(ncm);
                            ncm->ncm_HasMounted = FALSE;
                        }
                    }
                    ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
                }
                nh->nh_TimerIOReq->tr_time.tv_secs = 3;
                nh->nh_TimerIOReq->tr_time.tv_micro = 0;
                SendIO((struct IORequest *) nh->nh_TimerIOReq);
            }

            if(nh->nh_RemovableTask->tc_Node.ln_Type == NT_TASK)
            {
                APTR doslib;
                if((doslib = OpenLibrary("dos.library", 39)))
                {
                    CloseLibrary(doslib);
                    // increase disk change count to force mounting
                    ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
                    while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
                    {
                        ncm->ncm_ChangeCount++;
                        ncm->ncm_ForceRTCheck = TRUE;
                        ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
                    }

                    /* restart task */
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "DOS found, stopping removable task...");
                    nh->nh_RestartIt = TRUE;
                    break;
                }
                // don't quit task, otherwise nobody will be there to restart it and retry mounting stuff
                dontquit = TRUE;
            }

            if(!dontquit)
            {
                break;
            }
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));
        ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
        while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncm->ncm_DenyRequests && ncm->ncm_CUC->cuc_AutoUnmount && ncm->ncm_HasMounted)
            {
                nUnmountPartition(ncm);
                ncm->ncm_HasMounted = FALSE;
            }
            ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        KPRINTF(20, ("Going down the river!\n"));
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Removable Task stopped.");
        GM_UNIQUENAME(nFreeRT)(nh);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocRT()" */
struct NepMSBase * GM_UNIQUENAME(nAllocRT)(void)
{
    struct Task *thistask;
    struct NepMSBase *nh;

    thistask = FindTask(NULL);
    nh = thistask->tc_UserData;
#undef ExpansionBase
#define ExpansionBase nh->nh_ExpansionBase
#undef PartitionBase
#define PartitionBase nh->nh_PartitionBase
    do
    {
        if(!(ExpansionBase = OpenLibrary("expansion.library", 37)))
        {
            Alert(AG_OpenLib | AO_ExpansionLib);
            break;
        }
        if(!(PartitionBase = OpenLibrary("partition.library", 1)))
        {
            Alert(AG_OpenLib | AO_Unknown);
            break;
        }
        if(!(ps = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib | AO_Unknown);
            break;
        }
        if(!(nh->nh_IOMsgPort = CreateMsgPort()))
        {
            break;
        }
        nh->nh_IOReq.io_Message.mn_ReplyPort = nh->nh_IOMsgPort;
        if(!(nh->nh_TimerMsgPort = CreateMsgPort()))
        {
            break;
        }
        if(!(nh->nh_TimerIOReq = (struct timerequest *) CreateIORequest(nh->nh_TimerMsgPort, sizeof(struct timerequest))))
        {
            break;
        }
        if(OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *) nh->nh_TimerIOReq, 0))
        {
            break;
        }
        /* Start removable interrupt */
        nh->nh_TimerIOReq->tr_node.io_Command = TR_ADDREQUEST;
        nh->nh_TimerIOReq->tr_time.tv_secs = 0;
        nh->nh_TimerIOReq->tr_time.tv_micro = 50;
        SendIO((struct IORequest *) nh->nh_TimerIOReq);
        nh->nh_RemovableTask = thistask;
        return(nh);
    } while(FALSE);
    if(ExpansionBase)
    {
        CloseLibrary(ExpansionBase);
        ExpansionBase = NULL;
    }
    if(PartitionBase)
    {
        CloseLibrary(PartitionBase);
        PartitionBase = NULL;
    }
    if(ps)
    {
        CloseLibrary(ps);
        ps = NULL;
    }

    if(nh->nh_TimerIOReq)
    {
        if(nh->nh_TimerIOReq->tr_node.io_Device)
        {
            CloseDevice((struct IORequest *) nh->nh_TimerIOReq);
        }
        DeleteIORequest((struct IORequest *) nh->nh_TimerIOReq);
        nh->nh_TimerIOReq = NULL;
    }
    if(nh->nh_TimerMsgPort)
    {
        DeleteMsgPort(nh->nh_TimerMsgPort);
        nh->nh_TimerMsgPort = NULL;
    }
    if(nh->nh_IOMsgPort)
    {
        DeleteMsgPort(nh->nh_IOMsgPort);
        nh->nh_IOMsgPort = NULL;
    }
    Forbid();
    nh->nh_RemovableTask = NULL;
    if(nh->nh_ReadySigTask)
    {
        Signal(nh->nh_ReadySigTask, 1L<<nh->nh_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeRT()" */
void GM_UNIQUENAME(nFreeRT)(struct NepMSBase *nh)
{

    psdFreeVec(nh->nh_OneBlock);
    nh->nh_OneBlock = NULL;
    nh->nh_OneBlockSize = 0;

    if(nh->nh_DOSBase)
    {
        CloseLibrary(nh->nh_DOSBase);
        nh->nh_DOSBase = NULL;
    }
    CloseLibrary(ExpansionBase);
    ExpansionBase = NULL;
    CloseLibrary(PartitionBase);
    PartitionBase = NULL;
    CloseLibrary(ps);
    ps = NULL;

    AbortIO((struct IORequest *) nh->nh_TimerIOReq);
    WaitIO((struct IORequest *) nh->nh_TimerIOReq);
    CloseDevice((struct IORequest *) nh->nh_TimerIOReq);
    DeleteIORequest((struct IORequest *) nh->nh_TimerIOReq);
    DeleteMsgPort(nh->nh_TimerMsgPort);
    nh->nh_TimerMsgPort = NULL;
    nh->nh_TimerIOReq = NULL;

    Forbid();
    nh->nh_RemovableTask = NULL;
    if(nh->nh_ReadySigTask)
    {
        Signal(nh->nh_ReadySigTask, 1L<<nh->nh_ReadySignal);
    }
    if(nh->nh_RestartIt)
    {
        // wake up every task to relaunch removable task
        struct NepClassMS *ncm;
        ncm = (struct NepClassMS *) nh->nh_Units.lh_Head;
        while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncm->ncm_Task)
            {
                Signal(ncm->ncm_Task, 1L<<ncm->ncm_TaskMsgPort->mp_SigBit);
            }
            ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        nh->nh_RestartIt = FALSE;
    }
}
/* \\\ */

/* /// "GM_UNIQUENAME(nOpenDOS)()" */
BOOL GM_UNIQUENAME(nOpenDOS)(struct NepMSBase *nh)
{
    if(nh->nh_DOSBase)
    {
        return(TRUE);
    }
    if(nh->nh_RemovableTask->tc_Node.ln_Type != NT_PROCESS)
    {
        return(FALSE);
    }
    if((nh->nh_DOSBase = OpenLibrary("dos.library", 39)))
    {
        KPRINTF(10, ("Opened dos.library!\n"));
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

#undef DOSBase
#define	DOSBase	nh->nh_DOSBase

/* /// "nUnmountPartition()" */
void nUnmountPartition(struct NepClassMS *ncm)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct DosList *list;
    struct DeviceNode *node;
    struct DeviceNode *oldnode = NULL;
    char partname[32];
    UBYTE *bstr;

    if(!GM_UNIQUENAME(nOpenDOS)(nh))
    {
        return;
    }
    while((node = FindMatchingDevice(ncm, NULL)))
    {
        if(oldnode == node)
        {
            break;
        }
        bstr = (UBYTE *) BADDR(node->dn_Name);
        b2cstr(bstr, partname);
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Unmounting partition %s...",
                       partname);
        DoPkt(node->dn_Task, ACTION_INHIBIT, TRUE, 0, 0, 0, 0);
        DoPkt(node->dn_Task, ACTION_DIE, 0, 0, 0, 0, 0);
        if((list = LockDosList(LDF_DEVICES | LDF_WRITE)))
        {
            list = FindDosEntry(list, partname, LDF_DEVICES);
            if(list)
            {
                RemDosEntry(list);
            }
            UnLockDosList(LDF_DEVICES | LDF_WRITE);
        }
        /*psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Unmounting %s done.",
                       partname);*/
        oldnode = node;
    }
}
/* \\\ */

/* /// "nIOCmdTunnel()" */
LONG nIOCmdTunnel(struct NepClassMS *ncm, struct IOStdReq *ioreq)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    Forbid();
    if(ncm->ncm_DenyRequests)
    {
        Permit();
        return(ioreq->io_Error = IOERR_ABORTED);
    }
    PutMsg(&ncm->ncm_Unit.unit_MsgPort, (struct Message *) ioreq);
    Permit();
    while(!GetMsg(nh->nh_IOMsgPort))
    {
        WaitPort(nh->nh_IOMsgPort);
    }
    return(ioreq->io_Error);
}
/* \\\ */

/* /// "nScsiDirectTunnel()" */
LONG nScsiDirectTunnel(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct IOStdReq *ioreq = &nh->nh_IOReq;
    ioreq->io_Command = HD_SCSICMD;
    ioreq->io_Data = scsicmd;
    ioreq->io_Length = sizeof(*scsicmd);
    return(nIOCmdTunnel(ncm, ioreq));
}
/* \\\ */

/* /// "nGetWriteProtect()" */
LONG nGetWriteProtect(struct NepClassMS *ncm)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    UBYTE cmd10[10];
    struct SCSICmd scsicmd;
    UBYTE inquirydata[256];
    UBYTE sensedata[18];
    LONG ioerr;

    if((ncm->ncm_DeviceType == PDT_WORM) ||
       (ncm->ncm_DeviceType == PDT_CDROM))
    {
        // cd roms are always write protected
        return(ncm->ncm_WriteProtect = TRUE);
    }
    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_SIMPLE_SCSI)
    {
        return(ncm->ncm_WriteProtect = FALSE);
    }
    scsicmd.scsi_Data = (UWORD *) inquirydata;
    scsicmd.scsi_Length = 8;
    scsicmd.scsi_Command = cmd10;
    scsicmd.scsi_CmdLength = 10;
    scsicmd.scsi_Flags = SCSIF_READ|SCSIF_AUTOSENSE|0x80;
    scsicmd.scsi_SenseData = sensedata;
    scsicmd.scsi_SenseLength = 18;
    cmd10[0] = SCSI_MODE_SENSE_10;
    cmd10[1] = 0x00; /* no block descriptors */
    cmd10[2] = 0x3f; /* no page, only header */
    cmd10[3] = 0;
    cmd10[4] = 0;
    cmd10[5] = 0;
    cmd10[6] = 0;
    cmd10[7] = 0;
    cmd10[8] = 8;
    cmd10[9] = 0;
    if((ioerr = nScsiDirectTunnel(ncm, &scsicmd)))
    {
        cmd10[2] = 0x00; /* try again with vendor page, only header */
        if((ioerr = nScsiDirectTunnel(ncm, &scsicmd)))
        {
            cmd10[2] = 0x3f; /* try again with 255 length */
            cmd10[8] = 0xff;
            if((ioerr = nScsiDirectTunnel(ncm, &scsicmd)))
            {
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Failed to get write protection state: %ld",
                                   ioerr);
                }
                if(!(ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK))
                {
                    ncm->ncm_CDC->cdc_PatchFlags |= PFF_SIMPLE_SCSI;
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Fallback: Enabling Simple SCSI.");
                }
                GM_UNIQUENAME(nStoreConfig)(ncm);
                return(0);
            }
        }
    }
    if(inquirydata[3] & 0x80)
    {
        if(!ncm->ncm_WriteProtect)
        {
            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT)
            {
                ncm->ncm_ChangeCount++;
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Diskchange: GetWriteProtect On (count = %ld)",
                                   ncm->ncm_ChangeCount);
                }
            }
            ncm->ncm_WriteProtect = TRUE;
        }
    } else {
        if(ncm->ncm_WriteProtect)
        {
            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT)
            {
                ncm->ncm_ChangeCount++;
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Diskchange: GetWriteProtect Off (count = %ld)",
                                   ncm->ncm_ChangeCount);
                }
            }
            ncm->ncm_WriteProtect = FALSE;
        }
    }
    return(ncm->ncm_WriteProtect);
}
/* \\\ */

/* /// "SearchHardBlock()" */
APTR SearchHardBlock(struct NepClassMS *ncm,
                     struct IOStdReq *ioreq,
                     ULONG id,
                     ULONG start)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    ULONG curBlock;

    if(!nh->nh_OneBlock || (nh->nh_OneBlockSize < ncm->ncm_BlockSize))
    {
        psdFreeVec(nh->nh_OneBlock);
        if(!(nh->nh_OneBlock = psdAllocVec(ncm->ncm_BlockSize)))
        {
            return(NULL);
        }
        nh->nh_OneBlockSize = ncm->ncm_BlockSize;
    }

    curBlock = start;
    do
    {
        ioreq->io_Command = TD_READ64;
        ioreq->io_Data = nh->nh_OneBlock;
        ioreq->io_Length = ncm->ncm_BlockSize;
        ioreq->io_Offset = curBlock<<ncm->ncm_BlockShift;
        ioreq->io_Actual = curBlock>>(32-ncm->ncm_BlockShift);

        if(!nIOCmdTunnel(ncm, ioreq))
        {
            curBlock++;
            if((*(ULONG *) nh->nh_OneBlock) == id)
            {
                return(nh->nh_OneBlock);
            }
        } else {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Error searching hardblock in block %ld.", curBlock);
            return(NULL);
        }
    } while(curBlock <= RDB_LOCATION_LIMIT);
    return(NULL);
}
/* \\\ */

/* /// "ReadRDSK()" */
BOOL ReadRDSK(struct NepClassMS *ncm,
              struct IOStdReq *ioreq,
              struct RigidDiskBlock *rdb)
{
    APTR blkaddr = SearchHardBlock(ncm, ioreq, IDNAME_RIGIDDISK, 0);
    if(blkaddr)
    {
        CopyMemQuick(blkaddr, rdb, sizeof(struct RigidDiskBlock));
        // endianess conversion
        rdb->rdb_ID = AROS_BE2LONG(rdb->rdb_ID);
        rdb->rdb_SummedLongs = AROS_BE2LONG(rdb->rdb_SummedLongs);
        rdb->rdb_ChkSum = AROS_BE2LONG(rdb->rdb_ChkSum);
        rdb->rdb_HostID = AROS_BE2LONG(rdb->rdb_HostID);
        rdb->rdb_BlockBytes = AROS_BE2LONG(rdb->rdb_BlockBytes);
        rdb->rdb_PartitionList = AROS_BE2LONG(rdb->rdb_PartitionList);
        rdb->rdb_FileSysHeaderList = AROS_BE2LONG(rdb->rdb_FileSysHeaderList);
        rdb->rdb_DriveInit = AROS_BE2LONG(rdb->rdb_DriveInit);
        rdb->rdb_Cylinders = AROS_BE2LONG(rdb->rdb_Cylinders);
        rdb->rdb_Sectors = AROS_BE2LONG(rdb->rdb_Sectors);
        rdb->rdb_Heads = AROS_BE2LONG(rdb->rdb_Heads);
        rdb->rdb_Interleave = AROS_BE2LONG(rdb->rdb_Interleave);
        rdb->rdb_Park = AROS_BE2LONG(rdb->rdb_Park);
        rdb->rdb_WritePreComp = AROS_BE2LONG(rdb->rdb_WritePreComp);
        rdb->rdb_ReducedWrite = AROS_BE2LONG(rdb->rdb_ReducedWrite);
        rdb->rdb_StepRate = AROS_BE2LONG(rdb->rdb_StepRate);
        rdb->rdb_RDBBlocksLo = AROS_BE2LONG(rdb->rdb_RDBBlocksLo);
        rdb->rdb_RDBBlocksHi = AROS_BE2LONG(rdb->rdb_RDBBlocksHi);
        rdb->rdb_LoCylinder = AROS_BE2LONG(rdb->rdb_LoCylinder);
        rdb->rdb_HiCylinder = AROS_BE2LONG(rdb->rdb_HiCylinder);
        rdb->rdb_CylBlocks = AROS_BE2LONG(rdb->rdb_CylBlocks);
        rdb->rdb_AutoParkSeconds = AROS_BE2LONG(rdb->rdb_AutoParkSeconds);
        rdb->rdb_HighRDSKBlock = AROS_BE2LONG(rdb->rdb_HighRDSKBlock);
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

/* /// "ReadPART()" */
BOOL ReadPART(struct NepClassMS *ncm,
              struct IOStdReq *ioreq,
              struct PartitionBlock *part,
              ULONG which)
{
    APTR blkaddr = SearchHardBlock(ncm, ioreq, IDNAME_PARTITION, which);
    if(blkaddr)
    {
        UWORD cnt;
        CopyMemQuick(blkaddr, part, sizeof(struct PartitionBlock));
        // endianess conversion
        part->pb_ID = AROS_BE2LONG(part->pb_ID);
        part->pb_SummedLongs = AROS_BE2LONG(part->pb_SummedLongs);
        part->pb_ChkSum = AROS_BE2LONG(part->pb_ChkSum);
        part->pb_HostID = AROS_BE2LONG(part->pb_HostID);
        part->pb_Next = AROS_BE2LONG(part->pb_Next);
        part->pb_Flags = AROS_BE2LONG(part->pb_Flags);
        part->pb_DevFlags = AROS_BE2LONG(part->pb_DevFlags);
        for(cnt = 0; cnt < 20; cnt++)
        {
            part->pb_Environment[cnt] = AROS_BE2LONG(part->pb_Environment[cnt]);
        }
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

/* /// "ReadFSHD()" */
BOOL ReadFSHD(struct NepClassMS *ncm,
              struct IOStdReq *ioreq,
              struct FileSysHeaderBlock *fshd,
              ULONG which)
{
    APTR blkaddr = SearchHardBlock(ncm, ioreq, IDNAME_FILESYSHEADER, which);
    if(blkaddr)
    {
        CopyMemQuick(blkaddr, fshd, sizeof(struct FileSysHeaderBlock));
        // endianess conversion
        fshd->fhb_ID = AROS_BE2LONG(fshd->fhb_ID);
        fshd->fhb_SummedLongs = AROS_BE2LONG(fshd->fhb_SummedLongs);
        fshd->fhb_ChkSum = AROS_BE2LONG(fshd->fhb_ChkSum);
        fshd->fhb_HostID = AROS_BE2LONG(fshd->fhb_HostID);
        fshd->fhb_Next = AROS_BE2LONG(fshd->fhb_Next);
        fshd->fhb_Flags = AROS_BE2LONG(fshd->fhb_Flags);
        fshd->fhb_DosType = AROS_BE2LONG(fshd->fhb_DosType);
        fshd->fhb_Version = AROS_BE2LONG(fshd->fhb_Version);
        fshd->fhb_PatchFlags = AROS_BE2LONG(fshd->fhb_PatchFlags);
        fshd->fhb_Type = AROS_BE2LONG(fshd->fhb_Type);
        fshd->fhb_Task = AROS_BE2LONG(fshd->fhb_Task);
        fshd->fhb_Lock = AROS_BE2LONG(fshd->fhb_Lock);
        fshd->fhb_Handler = AROS_BE2LONG(fshd->fhb_Handler);
        fshd->fhb_StackSize = AROS_BE2LONG(fshd->fhb_StackSize);
        fshd->fhb_Priority = AROS_BE2LONG(fshd->fhb_Priority);
        fshd->fhb_Startup = AROS_BE2LONG(fshd->fhb_Startup);
        fshd->fhb_SegListBlocks = AROS_BE2LONG(fshd->fhb_SegListBlocks);
        fshd->fhb_GlobalVec = AROS_BE2LONG(fshd->fhb_GlobalVec);
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

/* /// "ReadLSEG()" */
struct LoadSegBlock * ReadLSEG(struct NepClassMS *ncm,
              struct IOStdReq *ioreq,
              ULONG which)
{
    return((struct LoadSegBlock *) SearchHardBlock(ncm, ioreq, IDNAME_LOADSEG, which));
}
/* \\\ */

/* /// "FindFileSystem()" */
struct FileSysEntry * FindFileSystem(struct NepClassMS *ncm, ULONG dosType)
{
    struct FileSysResource *fsr;
    struct FileSysEntry *fse;

    KPRINTF(10, ("looking up %08lx fs\n", dosType));
    if((fsr = (struct FileSysResource *) OpenResource(FSRNAME)))
    {
        Forbid();
        fse = (struct FileSysEntry *) fsr->fsr_FileSysEntries.lh_Head;
        while(fse->fse_Node.ln_Succ)
        {
            if(fse->fse_DosType == dosType)
            {
                Permit();
                return(fse);
            }
            fse = (struct FileSysEntry *) fse->fse_Node.ln_Succ;
        }
        Permit();
    }

    return(NULL);
}
/* \\\ */

/* /// "BuildFileSystem()" */
ULONG BuildFileSystem(struct NepClassMS *ncm,
                      UBYTE *fsBuffer,
                      BOOL readAndCopy)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct RigidDisk *rdsk = &nh->nh_RDsk;
    ULONG result = 0;
    ULONG nextLSEG;
    ULONG add;
    struct LoadSegBlock *lseg;

    nextLSEG = rdsk->rdsk_FSHD.fhb_SegListBlocks;

    do
    {
        if((lseg = ReadLSEG(ncm, &nh->nh_IOReq, nextLSEG)))
        {
            add = (AROS_BE2LONG(lseg->lsb_SummedLongs) - 5) * sizeof(ULONG);
            if(readAndCopy)
            {
                CopyMem(lseg->lsb_LoadData, fsBuffer, add);
                fsBuffer += add;
            }
            result += add;
            nextLSEG = lseg->lsb_Next;
        } else {
            result = 0;
            break;
        }
    } while(nextLSEG != NIL_PTR);
    return(result);
}
/* \\\ */

/* /// "LoadFileSystem()" */
BPTR LoadFileSystem(struct NepClassMS *ncm, ULONG dosType, struct FileSysEntry *fse)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct RigidDisk *rdsk = &nh->nh_RDsk;
    ULONG nextFSHD;
    BPTR fh, seg = BNULL;
    ULONG fsLength;
    UBYTE *fsBuffer;
    UBYTE fsFile[32];
    BOOL ok;

    if(rdsk->rdsk_RDB.rdb_FileSysHeaderList != NIL_PTR)
    {
        nextFSHD = rdsk->rdsk_RDB.rdb_FileSysHeaderList;
        do
        {
            if(ReadFSHD(ncm, &nh->nh_IOReq, &rdsk->rdsk_FSHD, nextFSHD))
            {
                nextFSHD = rdsk->rdsk_FSHD.fhb_Next;
            } else {
                break;
            }
            KPRINTF(10, ("Found 0x%08lx FS in FSHD...\n", rdsk->rdsk_FSHD.fhb_ID));
        } while((rdsk->rdsk_FSHD.fhb_DosType != dosType) && (nextFSHD != NIL_PTR));

        if(rdsk->rdsk_FSHD.fhb_DosType == dosType)
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Found filesystem %s in RDB!",
                           nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName);
            KPRINTF(10, ("found matching fs in FSHD, trying to load from LSEG blocks\n"));

            CopyMem(&rdsk->rdsk_FSHD.fhb_DosType, &fse->fse_DosType, sizeof(struct FileSysEntry) - sizeof(struct Node));

            if(rdsk->rdsk_FSHD.fhb_SegListBlocks > 0)
            {
                fsLength = BuildFileSystem(ncm, NULL, FALSE);

                if(fsLength > 0)
                {
                    if((fsBuffer = psdAllocVec(fsLength)))
                    {
                        BuildFileSystem(ncm, fsBuffer, TRUE);

                        if(GM_UNIQUENAME(nOpenDOS)(nh))
                        {
                            psdSafeRawDoFmt(fsFile, 32, "T:UMSD_%08lx.fs", dosType);
                            if((fh = Open(fsFile, MODE_NEWFILE)))
                            {
                                ok = (Write(fh, fsBuffer, fsLength) == fsLength);
                                Close(fh);
                                if(ok)
                                {
                                    seg = LoadSeg(fsFile);
                                }
                            }
                            DeleteFile(fsFile);
                        } else {
                            KPRINTF(10, ("No DOS available, trying own load seg stuff\n"));
                            // FIXME this code is unavailable and doesn't make sense for AROS as it doesn't use DOS_HUNK format
                            //seg = CreateSegment(ncm, (const ULONG *) fsBuffer);
                        }
                        psdFreeVec(fsBuffer);
                    }
                }
            }
        }
    }

    if(!seg)
    {
        if(GM_UNIQUENAME(nOpenDOS)(nh))
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Loading filesystem %s from RDB failed. Trying DOS...",
                           nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName);
            KPRINTF(10, ("loading fs from LSEG blocks failed, trying fs file %s mentioned in FSHD\n", (char *) nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName));
            //seg = LoadSeg(rdsk->rdsk_FSHD.fhb_FileSysName);
            seg = LoadSeg((char *) nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName);
            if(seg)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Loaded filesystem %s via DOS!",
                               nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName);
            } else {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Loading filesystem %s via DOS failed!",
                               nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName);
            }
        } else {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Loading filesystem %s from RDB failed.",
                           nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName);
        }
    }
    if(seg)
    {
        fse->fse_SegList = seg;
    }
    return(seg);
}
/* \\\ */

/* /// "MatchPartition()" */
BOOL MatchPartition(struct NepClassMS *ncm,
                    struct DosEnvec *envec1,
                    struct FileSysStartupMsg *fssm)
{
    BOOL result = FALSE;
    UBYTE *bstr;
    UBYTE device[256];
    struct DosEnvec *envec2;

    if(fssm)
    {
        envec2 = (struct DosEnvec *) BADDR(fssm->fssm_Environ);

        if(envec2)
        {
            bstr = (UBYTE *) BADDR(fssm->fssm_Device);
            b2cstr(bstr, device);

            if(envec1)
            {
                if((envec1->de_DosType & 0xffffff00) == 0x46415400)
                {
                    result = ((ncm->ncm_UnitNo == fssm->fssm_Unit) &&
                              (strcmp(DEVNAME, device) == 0) &&
                              (envec1->de_DosType        == envec2->de_DosType));
                } else {
                    result = ((ncm->ncm_UnitNo == fssm->fssm_Unit) &&
                              (strcmp(DEVNAME, device) == 0) &&
                              (envec1->de_SizeBlock      == envec2->de_SizeBlock) &&
                              (envec1->de_Surfaces       == envec2->de_Surfaces) &&
                              (envec1->de_SectorPerBlock == envec2->de_SectorPerBlock) &&
                              (envec1->de_BlocksPerTrack == envec2->de_BlocksPerTrack) &&
                              (envec1->de_Reserved       == envec2->de_Reserved) &&
                              (envec1->de_PreAlloc       == envec2->de_PreAlloc) &&
                              (envec1->de_Interleave     == envec2->de_Interleave) &&
                              (envec1->de_LowCyl         == envec2->de_LowCyl) &&
                              (envec1->de_HighCyl        == envec2->de_HighCyl) &&
                              (envec1->de_DosType        == envec2->de_DosType));
                }
            } else {
                result = (ncm->ncm_UnitNo == fssm->fssm_Unit) &&
                          (strcmp(DEVNAME, device) == 0);
            }
        }
    }
    return(result);
}
/* \\\ */

/* /// "FindDeviceNode()" */
struct DeviceNode * FindDeviceNode(struct NepClassMS *ncm, STRPTR device)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct DosList *list;
    struct DeviceNode *node = NULL;

    if(!GM_UNIQUENAME(nOpenDOS)(nh))
    {
        return(NULL);
    }

    if((list = LockDosList(LDF_DEVICES | LDF_READ)))
    {
        node = (struct DeviceNode *) FindDosEntry(list, device, LDF_DEVICES);
        UnLockDosList(LDF_DEVICES | LDF_READ);
    }
    return(node);
}
/* \\\ */

/* /// "CheckVolumesOrAssignsMatch()" */
BOOL CheckVolumesOrAssignsMatch(struct NepClassMS *ncm, STRPTR device)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct DosList *list;
    BOOL found = FALSE;

    if(!GM_UNIQUENAME(nOpenDOS)(nh))
    {
        return(FALSE);
    }

    if((list = LockDosList(LDF_ALL | LDF_READ)))
    {
        if(FindDosEntry(list, device, LDF_ALL))
        {
            found = TRUE;
        }
        UnLockDosList(LDF_ALL | LDF_READ);
    }
    return(found);
}
/* \\\ */

/* /// "FindMatchingDevice()" */
struct DeviceNode * FindMatchingDevice(struct NepClassMS *ncm, struct DosEnvec *envec)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct DosList *list;
    struct DeviceNode *node = NULL;
    struct FileSysStartupMsg *fssm;

    if(!GM_UNIQUENAME(nOpenDOS)(nh))
    {
        return(NULL);
    }

    if((list = LockDosList(LDF_DEVICES | LDF_READ)))
    {
        while((list = NextDosEntry(list, LDF_DEVICES | LDF_READ)))
        {
            fssm = NULL;

            /*if((!(((ULONG) list->dol_misc.dol_handler.dol_Startup) >> 30)) &&
               TypeOfMem(BADDR(list->dol_misc.dol_handler.dol_Startup)))*/
            {
                fssm = BADDR(list->dol_misc.dol_handler.dol_Startup);
            }

            if(fssm > (struct FileSysStartupMsg *) 0x1000)
            {
//                if((*((UBYTE *) fssm)) == 0)
                {
                    struct DosEnvec *de = BADDR(fssm->fssm_Environ);
                    STRPTR devname = BADDR(fssm->fssm_Device);

                    if(TypeOfMem(de) && TypeOfMem(devname) && (de->de_TableSize > 0) && (de->de_TableSize < 32))
                    /*if((!((ULONG) de >> 30)) && TypeOfMem(de) &&
                       (!((ULONG) devname >> 30)) && TypeOfMem(devname) &&
                       (de->de_TableSize > 0) && (de->de_TableSize < 32))*/
                    {
                        if(MatchPartition(ncm, envec, fssm))
                        {
                            node = (struct DeviceNode *) list;
                            break;
                        }
                    }
                }
            }
        }
        UnLockDosList(LDF_DEVICES | LDF_READ);
    }
    return(node);
}
/* \\\ */

/* /// "MountPartition()" */
BOOL MountPartition(struct NepClassMS *ncm, STRPTR dosDevice)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct RigidDisk *rdsk = &nh->nh_RDsk;
    IPTR *params;
    struct DeviceNode *node;
    struct FileSysEntry *fse;
    struct FileSysEntry patch;
    BPTR segList = BNULL;
    BOOL fsFound = FALSE;
    BOOL result = FALSE;
    STRPTR devname = DEVNAME;

    if((fse = FindFileSystem(ncm, rdsk->rdsk_PART.pb_Environment[DE_DOSTYPE])))
    {
        KPRINTF(10, ("fs found in filesys resource\n"));
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Found FS in filesystem.resource!");

        CopyMem(fse, &patch, sizeof(struct FileSysEntry));
        fsFound = TRUE;
    } else {
        memset(&patch, 0x00, sizeof(struct FileSysEntry));
        patch.fse_DosType = rdsk->rdsk_PART.pb_Environment[DE_DOSTYPE];

        if((segList = LoadFileSystem(ncm, rdsk->rdsk_PART.pb_Environment[DE_DOSTYPE], &patch)))
        {
            KPRINTF(10, ("fs loaded from RDB\n"));

            patch.fse_PatchFlags = 0x0080|0x0010;
            patch.fse_SegList = segList;
            patch.fse_StackSize = 16384;
            //if(((patch.fse_DosType & 0xffffff00) == 0x46415400) || (patch.fse_DosType == 0x4d534800))
            {
                KPRINTF(10, ("setting up certain fs values for MS-DOS fs\n"));
                // Stack, SegList, Pri und GlobVec eintragen
                patch.fse_PatchFlags |= 0x0020|0x0100;
                patch.fse_Priority = 10;
                patch.fse_GlobalVec = (BPTR) 0xffffffff;
            }

            fsFound = TRUE;
        }
    }

    if(!fsFound)
    {
        STRPTR handler = (STRPTR) nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName;
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Experimental AROS patch to load %s", handler);
        patch.fse_Handler = MKBADDR(AllocVec(AROS_BSTR_MEMSIZE4LEN(strlen(handler)), MEMF_PUBLIC | MEMF_CLEAR));
        if(patch.fse_Handler)
        {
            c2bstr(handler, patch.fse_Handler);
            patch.fse_PatchFlags |= 0x0008;
            fsFound = TRUE;
        }
    }

    if(fsFound)
    {
        if((params = psdAllocVec(sizeof(struct DosEnvec) + 4 * sizeof(IPTR))))
        {
            params[0] = (IPTR) dosDevice;
            params[1] = (IPTR) DEVNAME;
            params[2] = ncm->ncm_UnitNo;
            params[3] = 0x00; // Flags for OpenDevice
            CopyMem(rdsk->rdsk_PART.pb_Environment, &params[4], sizeof(struct DosEnvec));

            if((node = MakeDosNode(params)))
            {
                BOOL installboot;
                KPRINTF(10, ("MakeDosNode() succeeded, patchflags %04lx\n", patch.fse_PatchFlags));
                node->dn_StackSize = 16384;

                /*node->dn_Priority = 5;*/
                if(patch.fse_PatchFlags & 0x0001) node->dn_Type = patch.fse_Type;
                if(patch.fse_PatchFlags & 0x0002) node->dn_Task = (struct MsgPort *) patch.fse_Task;
                if(patch.fse_PatchFlags & 0x0004) node->dn_Lock = patch.fse_Lock;
                if(patch.fse_PatchFlags & 0x0008) node->dn_Handler = patch.fse_Handler;
                if(patch.fse_PatchFlags & 0x0010) node->dn_StackSize = patch.fse_StackSize;
                if(patch.fse_PatchFlags & 0x0020) node->dn_Priority = patch.fse_Priority;
                if(patch.fse_PatchFlags & 0x0040) node->dn_Startup = patch.fse_Startup;
                if(patch.fse_PatchFlags & 0x0080) node->dn_SegList = patch.fse_SegList;
                if(patch.fse_PatchFlags & 0x0100) node->dn_GlobalVec = patch.fse_GlobalVec;

                KPRINTF(10, ("dn_Next      = %08lx\n"
                             "dn_Type      = %08lx\n"
                             "dn_Task      = %08lx\n"
                             "dn_Lock      = %08lx\n"
                             "dn_Handler   = %08lx\n"
                             "dn_StackSize = %08ld\n"
                             "dn_Priority  = %08ld\n"
                             "dn_Startup   = %08lx\n"
                             "dn_SegList   = %08lx\n"
                             "dn_GlobalVec = %08lx\n"
                             "dn_Name      = %08lx\n",
                             node->dn_Next,
                             node->dn_Type,
                             node->dn_Task,
                             node->dn_Lock,
                             node->dn_Handler,
                             node->dn_StackSize,
                             node->dn_Priority,
                             node->dn_Startup,
                             node->dn_SegList,
                             node->dn_GlobalVec,
                             node->dn_Name));

                installboot = ncm->ncm_CUC->cuc_BootRDB;
                if((nh->nh_RemovableTask->tc_Node.ln_Type == NT_PROCESS) ||
                   (!(nh->nh_RDsk.rdsk_PART.pb_Flags & PBFF_BOOTABLE)))
                {
                    installboot = FALSE;
                }
                if(installboot)
                {
                    // avoid sys partition being unmounted (actually it should better check at
                    // unmounting, but I can't think of a clever way yet to retrieve the SYS:
                    // device
                    ncm->ncm_CUC->cuc_AutoUnmount = FALSE;
                    GM_UNIQUENAME(nStoreConfig)(ncm);
                }

                if(AddBootNode(nh->nh_RDsk.rdsk_PART.pb_Environment[DE_BOOTPRI], ADNF_STARTPROC, node, NULL))
                {
                    KPRINTF(10, ("AddBootNode() succeeded\n"));
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Mounted %s unit %ld as %s:",
                                   devname, ncm->ncm_UnitNo, dosDevice);

                    result = TRUE;
                } else {
                    KPRINTF(10, ("AddBootNode() failed\n"));
                    /* There is a memory leak here! No way to deallocate the node created by
                       MakeDosNode()! */
                }
            }
            psdFreeVec(params);
        }

        if(!result)
        {
            if(GM_UNIQUENAME(nOpenDOS)(nh))
            {
                UnLoadSeg(segList);
            }
        }
    } else {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Couldn't find/load filesystem for %s unit %ld as %s:",
                       devname, ncm->ncm_UnitNo, dosDevice);
        KPRINTF(10, ("fs %08lx not found\n", rdsk->rdsk_PART.pb_Environment[DE_DOSTYPE]));
    }

    return(result);
}
/* \\\ */

/* /// "CheckPartition()" */
void CheckPartition(struct NepClassMS *ncm)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct RigidDisk *rdsk = &nh->nh_RDsk;
    struct DosEnvec *envec;
    UBYTE dosDevice[32], temp[32];
    ULONG spareNum;
    struct DeviceNode *node;
    BOOL done = FALSE, doMount = TRUE;
    STRPTR devname = DEVNAME;
    BOOL bump;
    ULONG slen;

    envec = (struct DosEnvec *) rdsk->rdsk_PART.pb_Environment;
    if((node = FindMatchingDevice(ncm, envec)))
    {
        KPRINTF(10, ("found suitable device entry, no need to mount anything new\n"));

        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Matching partition for %s unit %ld already found. No remount required.",
                       devname, ncm->ncm_UnitNo);
        doMount = FALSE;
    } else {
        spareNum = 0;

        b2cstr(rdsk->rdsk_PART.pb_DriveName, dosDevice);

        KPRINTF(10, ("trying to mount partition \"%s\"\n", dosDevice));

        /*if(envec->de_TableSize >= DE_DOSTYPE) SHOWVALUE(envec->de_DosType);*/
        do
        {
            bump = FALSE;
            if((node = FindDeviceNode(ncm, dosDevice)))
            {
                KPRINTF(10, ("%s is already mounted, comparing fssm\n", dosDevice));

                if(MatchPartition(ncm, envec, BADDR(node->dn_Startup)))
                {
                    KPRINTF(10, ("fssm match, no need to mount\n"));

                    doMount = FALSE;
                    done = TRUE;
                } else {
                    bump = TRUE;
                }
            } else {
                if(CheckVolumesOrAssignsMatch(ncm, dosDevice))
                {
                    bump = TRUE;
                } else {
                    done = TRUE;
                }
            }
            if(bump)
            {
                slen = strlen(dosDevice);
                if((slen > 0) && (dosDevice[slen-1] >= '0') && (dosDevice[slen-1] <= '9'))
                {
                    if(dosDevice[slen-1] == '9')
                    {
                        if((slen > 1) && (dosDevice[slen-2] >= '0') && (dosDevice[slen-2] <= '8'))
                        {
                            dosDevice[slen-2]++;
                            dosDevice[slen-1] = '0';
                        } else {
                            if(slen < 30)
                            {
                                dosDevice[slen-1] = '1';
                                dosDevice[slen] = '0';
                                dosDevice[slen+1] = 0;
                            } else {
                                break;
                            }
                        }
                    } else {
                        dosDevice[slen-1]++;
                    }
                } else {
                    b2cstr(rdsk->rdsk_PART.pb_DriveName, temp);
                    psdSafeRawDoFmt(dosDevice, 32, "%s.%ld", temp, spareNum);
                }
                KPRINTF(10, ("fssm don't match, trying as %s\n", dosDevice));
                spareNum++;
            }
        } while(!done && (spareNum < 16));
    }

    if(done && doMount)
    {
        KPRINTF(10, ("mounting %s\n", dosDevice));

        MountPartition(ncm, dosDevice);
    }
}
/* \\\ */

/* /// "IsFATSuperBlock()" */
BOOL IsFATSuperBlock(struct FATSuperBlock *fsb)
{
    BOOL result;
    result = (BOOL)(strncmp(fsb->fsb_Vendor, "MSDOS", 5) == 0 ||
                    strncmp(fsb->fsb_Vendor, "MSWIN", 5) == 0 ||
                    strncmp(fsb->fsb_FileSystem, "FAT12", 5) == 0 ||
                    strncmp(fsb->fsb_FileSystem, "FAT16", 5) == 0 ||
                    strncmp(fsb->fsb_FileSystem2, "FAT32", 5) == 0);

    return(result);
}
/* \\\ */

/* /// "GetFATDosType()" */
ULONG GetFATDosType(struct FATSuperBlock *fsb)
{
    ULONG result = 0x46415400;
    if(strncmp(fsb->fsb_FileSystem2, "FAT32", 5) == 0)
        result |= 2;
    else if(strncmp(fsb->fsb_FileSystem, "FAT16", 5) == 0)
        result |= 1;

    return(result);
}
/* \\\ */

/* /// "CheckFATPartition()" */
void CheckFATPartition(struct NepClassMS *ncm, ULONG startblock)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct MasterBootRecord *mbr;
    struct DosEnvec *envec;
    struct IOStdReq *stdIO = &nh->nh_IOReq;
    struct DriveGeometry *tddg = &ncm->ncm_Geometry;
    BOOL isfat = FALSE;
    BOOL isntfs = FALSE;

    mbr = (struct MasterBootRecord *) psdAllocVec(ncm->ncm_BlockSize<<1);
    if(!mbr)
    {
        return;
    }

    stdIO->io_Command = TD_READ64;
    stdIO->io_Offset = startblock<<ncm->ncm_BlockShift;
    stdIO->io_Actual = startblock>>(32-ncm->ncm_BlockShift);
    stdIO->io_Length = ncm->ncm_BlockSize;
    stdIO->io_Data = mbr;
    if(!nIOCmdTunnel(ncm, stdIO))
    {
        /* do (super)floppy check */
        if(IsFATSuperBlock((struct FATSuperBlock *) mbr))
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Media is FAT formatted!");
            isfat = TRUE;
            nh->nh_RDsk.rdsk_PART.pb_DevFlags = 0;

            if(*(ncm->ncm_CUC->cuc_FATDOSName))
            {
                c2bstr(ncm->ncm_CUC->cuc_FATDOSName, nh->nh_RDsk.rdsk_PART.pb_DriveName);
            } else {
                c2bstr("UF0", nh->nh_RDsk.rdsk_PART.pb_DriveName);
            }

            envec = (struct DosEnvec *) nh->nh_RDsk.rdsk_PART.pb_Environment;
            memset(envec, 0x00, sizeof(struct DosEnvec));
            stdIO->io_Command = TD_GETGEOMETRY;
            stdIO->io_Data = tddg;
            stdIO->io_Length = sizeof(*tddg);

            if(nIOCmdTunnel(ncm, stdIO))
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Couldn't read drive geometry, using floppy defaults");
                envec->de_SizeBlock = ncm->ncm_BlockSize>>2;
                envec->de_Surfaces = 2;
                envec->de_BlocksPerTrack = 18;
                envec->de_LowCyl = 0;
                envec->de_HighCyl = 79;
            } else {
                envec->de_SizeBlock = ncm->ncm_BlockSize>>2;
                envec->de_Surfaces = tddg->dg_Heads;
                envec->de_BlocksPerTrack = tddg->dg_TrackSectors;
                envec->de_LowCyl = 0;
                envec->de_HighCyl = tddg->dg_Cylinders-1;
            }
            envec->de_TableSize = DE_BOOTBLOCKS;
            envec->de_SectorPerBlock = 1;
            envec->de_NumBuffers = ncm->ncm_CUC->cuc_FATBuffers;
            envec->de_BufMemType = MEMF_PUBLIC;
            envec->de_MaxTransfer = (1UL<<(ncm->ncm_CDC->cdc_MaxTransfer+16))-1;
            envec->de_Mask = 0xffffffff;
            envec->de_BootPri = 0;
            envec->de_Baud = 1200;
            if(*ncm->ncm_CDC->cdc_FATControl)
            {
                UBYTE *bptr = ncm->ncm_FATControlBSTR;
                bptr = (UBYTE *) ((((IPTR) bptr) + 3) & (~3));
                c2bstr(ncm->ncm_CDC->cdc_FATControl, bptr);
                envec->de_Control = (IPTR) MKBADDR(bptr);
            } else {
                envec->de_Control = 0;
            }
            envec->de_BootBlocks = 0;
            envec->de_Interleave = 0;
            envec->de_DosType = ncm->ncm_CDC->cdc_FATDosType; //0x46415401; // FAT1
            if((ncm->ncm_CDC->cdc_FATDosType & 0xffffff00) == 0x46415400)
            {
                envec->de_DosType =
                    GetFATDosType((struct FATSuperBlock *) mbr);
            }

            // we have no FSHD and LSEG blocks
            nh->nh_RDsk.rdsk_RDB.rdb_FileSysHeaderList = NIL_PTR;
            nh->nh_RDsk.rdsk_FSHD.fhb_SegListBlocks = 0;

            KPRINTF(5, ("building FAT95 style environment\n"));

            strncpy((char *) nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName, ncm->ncm_CDC->cdc_FATFSName, 84);
            CheckPartition(ncm);
        }
        if(!(isfat || isntfs))
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "Media does not seem to be FAT nor NTFS formatted.");
        }
    } else {
        KPRINTF(10, ("failed to read MBR\n"));
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Failed to read MasterBootRecord for FAT/NTFS AutoMounting.");
        }
    }
    psdFreeVec(mbr);
}
/* \\\ */

/* /// "CheckISO9660()" */
void CheckISO9660(struct NepClassMS *ncm)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    UBYTE *blockbuf;
    struct IOStdReq *stdIO = &nh->nh_IOReq;

    blockbuf = (UBYTE *) psdAllocVec(ncm->ncm_BlockSize);
    if(!blockbuf)
    {
        return;
    }
    stdIO->io_Command = TD_READ64;
    stdIO->io_Offset = 0x8000;
    stdIO->io_Actual = 0;
    stdIO->io_Length = ncm->ncm_BlockSize;
    stdIO->io_Data = blockbuf;
    if(!nIOCmdTunnel(ncm, stdIO))
    {
        if((((ULONG *) blockbuf)[0] == AROS_LONG2BE(0x01434430)) && (((ULONG *) blockbuf)[1] == AROS_LONG2BE(0x30310100)))
        {
            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Media is ISO9660.");
            AutoMountCD(ncm);
        }
    } else {
        KPRINTF(10, ("failed to read ISO sector\n"));
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Failed to read block 16 for CDFS AutoMounting.");
        }
    }
    psdFreeVec(blockbuf);
}
/* \\\ */

/* /// "AutoMountCD()" */
void AutoMountCD(struct NepClassMS *ncm)
{
    struct NepMSBase *nh = ncm->ncm_ClsBase;
    struct DosEnvec *envec;

    nh->nh_RDsk.rdsk_PART.pb_DevFlags = 0;

    if(*(ncm->ncm_CUC->cuc_FATDOSName))
    {
        c2bstr(ncm->ncm_CUC->cuc_FATDOSName, nh->nh_RDsk.rdsk_PART.pb_DriveName);
    } else {
        c2bstr("UCD0", nh->nh_RDsk.rdsk_PART.pb_DriveName);
    }

    envec = (struct DosEnvec *) nh->nh_RDsk.rdsk_PART.pb_Environment;
    memset(envec, 0x00, sizeof(struct DosEnvec));

    envec->de_TableSize = DE_BOOTBLOCKS;
    envec->de_SizeBlock = ncm->ncm_BlockSize>>2;
    envec->de_Surfaces = 1;
    envec->de_SectorPerBlock = 1;
    envec->de_Reserved = 0xffffffff;
    envec->de_NumBuffers = ncm->ncm_CUC->cuc_FATBuffers;
    envec->de_BufMemType = MEMF_PUBLIC;
    envec->de_MaxTransfer = (1UL<<(ncm->ncm_CDC->cdc_MaxTransfer+16))-1;
    envec->de_Mask = 0xffffffff;
    envec->de_BootPri = 0;
    envec->de_Baud = 1200;
    if(*ncm->ncm_CDC->cdc_CDControl)
    {
        UBYTE *bptr = ncm->ncm_FATControlBSTR;
        bptr = (UBYTE *) ((((IPTR) bptr) + 3) & (~3));
        c2bstr(ncm->ncm_CDC->cdc_CDControl, bptr);
        envec->de_Control = (IPTR) MKBADDR(bptr);
    } else {
        envec->de_Control = 0;
    }
    envec->de_BootBlocks = 0;

    // we have no FSHD and LSEG blocks
    nh->nh_RDsk.rdsk_RDB.rdb_FileSysHeaderList = NIL_PTR;
    nh->nh_RDsk.rdsk_FSHD.fhb_SegListBlocks = 0;

    KPRINTF(5, ("building CDFS style environment\n"));

    envec->de_BlocksPerTrack = 1;
    envec->de_Interleave = 0;
    envec->de_DosType = ncm->ncm_CDC->cdc_CDDosType;
    envec->de_LowCyl = 0;
    envec->de_HighCyl = 1;

    strncpy((char *) nh->nh_RDsk.rdsk_FSHD.fhb_FileSysName, ncm->ncm_CDC->cdc_CDFSName, 84);
    CheckPartition(ncm);
}
/* \\\ */

/**************************************************************************/

static const char *MaxTransferStrings[] =
{
    " 64 KB",
    "128 KB",
    "256 KB",
    "512 KB",
    "  1 MB",
    "  2 MB",
    NULL
};

static char *MainGUIPages[] = { "Device Settings", "LUN Settings", NULL };
static char *MainGUIPagesDefault[] = { "Device Defaults", "LUN Defaults", NULL };

/* /// "nGetDosType()" */
ULONG nGetDosType(STRPTR tmpstr)
{
    ULONG dostype = 0;
    UBYTE ch;

    while((ch = *tmpstr++))
    {
        dostype <<= 4;
        if((ch >= '0') && (ch <= '9'))
        {
            dostype += ch - '0';
        }
        else if((ch >= 'a') && (ch <= 'f'))
        {
            dostype += ch - 'a' + 10;
        }
        else if((ch >= 'A') && (ch <= 'F'))
        {
            dostype += ch - 'A' + 10;
        }
    }
    return(dostype);
}
/* \\\ */

/* /// "nGUITask()" */
AROS_UFH0(void, GM_UNIQUENAME(nGUITask))
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepMSBase *nh;
    struct NepClassMS *ncm;
    struct NepClassMS *cncm;
    struct NepClassMS *curncm = NULL;
    APTR pic;
    char dostypebuf[10];
    char cddostypebuf[10];
    char ntfsdostypebuf[10];
    char bar[] = "BAR,";

    thistask = FindTask(NULL);
#undef ps
#define ps ncm->ncm_PsdBase
#undef IntuitionBase
#define IntuitionBase ncm->ncm_IntBase
#undef MUIMasterBase
#define MUIMasterBase ncm->ncm_MUIBase

    ncm = thistask->tc_UserData;
    nh = ncm->ncm_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(ncm);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(ncm);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(ncm);
        return;
    }

    ncm->ncm_LUNListDisplayHook.h_Data = NULL;
    ncm->ncm_LUNListDisplayHook.h_Entry = (APTR) GM_UNIQUENAME(LUNListDisplayHook);

    psdSafeRawDoFmt(dostypebuf, 10, "%08lx", ncm->ncm_CDC->cdc_FATDosType);
    psdSafeRawDoFmt(ntfsdostypebuf, 10, "%08lx", ncm->ncm_CDC->cdc_NTFSDosType);
    psdSafeRawDoFmt(cddostypebuf, 10, "%08lx", ncm->ncm_CDC->cdc_CDDosType);

    ncm->ncm_App = (APTR) ApplicationObject,
        MUIA_Application_Title      , (IPTR) GM_UNIQUENAME(libname),
        MUIA_Application_Version    , (IPTR) VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR) "©2002-2009 Chris Hodges",
        MUIA_Application_Author     , (IPTR) "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR) "Settings for the massstorage.class",
        MUIA_Application_Base       , (IPTR) "MASSSTORAGE",
        MUIA_Application_HelpFile   , (IPTR) "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , (IPTR) MenustripObject,
            Child, (IPTR) MenuObjectT((IPTR) "Project"),
                Child, (IPTR) (ncm->ncm_AboutMI = (APTR) MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR) "About...",
                    MUIA_Menuitem_Shortcut, (IPTR) "?",
                    End),
                End,
            Child, (IPTR) MenuObjectT((IPTR) "Settings"),
                Child, (IPTR) (ncm->ncm_UseMI = (APTR) MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR) "Save",
                    MUIA_Menuitem_Shortcut, (IPTR) "S",
                    End),
                Child, (IPTR) (ncm->ncm_SetDefaultMI = (APTR) MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR) "Save as Default",
                    MUIA_Menuitem_Shortcut, (IPTR) "D",
                    End),
                Child, (IPTR) MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR) NM_BARLABEL,
                    End,
                Child, (IPTR) (ncm->ncm_MUIPrefsMI = (APTR) MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR) "MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR) "M",
                    End),
                End,
            End,

        SubWindow, (IPTR) (ncm->ncm_MainWindow = (APTR) WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR) GM_UNIQUENAME(libname),
            MUIA_HelpNode, (IPTR) GM_UNIQUENAME(libname),

            WindowContents, (IPTR) VGroup,
                Child, (IPTR) RegisterGroup(ncm->ncm_Interface ? MainGUIPages : MainGUIPagesDefault),
                    MUIA_CycleChain, 1,
                    MUIA_Register_Frame, TRUE,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) VSpace(0),
                        Child, (IPTR) ColGroup(2),
                            Child, (IPTR) Label("NAK Timeout:"),
                            Child, (IPTR) (ncm->ncm_NakTimeoutObj = (APTR) SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 600,
                                MUIA_Numeric_Value, ncm->ncm_CDC->cdc_NakTimeout,
                                MUIA_Numeric_Format, (IPTR) "%ld00ms",
                                End),
                            Child, (IPTR) Label("Startup delay:"),
                            Child, (IPTR) (ncm->ncm_StartupDelayObj = (APTR) SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 100,
                                MUIA_Numeric_Value, ncm->ncm_CDC->cdc_StartupDelay,
                                MUIA_Numeric_Format, (IPTR) "%ld00ms",
                                End),
                            Child, (IPTR) Label("Single LUN:"),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_SingleLunObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_SINGLE_LUN,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) HSpace(0),
                                Child, (IPTR) Label("No Initial Reset:"),
                                Child, (IPTR) (ncm->ncm_InitialResetObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_RESET,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR) Label("Simple SCSI:"),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_SimpleSCSIObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_SIMPLE_SCSI,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) HSpace(0),
                                Child, (IPTR) Label("Translate CMD6->CMD10:"),
                                Child, (IPTR) (ncm->ncm_XLate610Obj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_MODE_XLATE,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR) Label("Fake Inquiry:"),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_FakeInquiryObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_FAKE_INQUIRY,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) HSpace(0),
                                Child, (IPTR) Label("Better Removable Support:"),
                                Child, (IPTR) (ncm->ncm_RemSupportObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR) Label("Trim Inquiry:"),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_FixInquiryObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_INQ36,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) HSpace(0),
                                Child, (IPTR) Label("Ignore broken CSS-ID:"),
                                Child, (IPTR) (ncm->ncm_CSSBrokenObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_CSS_BROKEN,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR) Label("Fix Capacity:"),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_FixCapacityObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_FIX_CAPACITY,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) HSpace(0),
                                Child, (IPTR) Label("Emulate on larger block sizes:"),
                                Child, (IPTR) (ncm->ncm_EmulLargeBlkObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_EMUL_LARGE_BLK,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR) Label("No Fallback:"),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_NoFallbackObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_NO_FALLBACK,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) HSpace(0),
                                Child, (IPTR) Label("Debug:"),
                                Child, (IPTR) (ncm->ncm_DebugObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                End,
                            Child, (IPTR) Label("Max Transfer:"),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_MaxTransferObj = (APTR) CycleObject,
                                    MUIA_Cycle_Entries, (IPTR) MaxTransferStrings,
                                    MUIA_Cycle_Active, ncm->ncm_CDC->cdc_MaxTransfer,
                                    End),
                                //Child, HSpace(0),
                                Child, (IPTR) (ncm->ncm_AutoDtxMaxTransObj = (APTR) TextObject, ButtonFrame,
                                    MUIA_Disabled, !ncm->ncm_Interface,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    MUIA_Text_Contents, (IPTR) "\33c Auto-detect ",
                                    End),
                                End,
                            End,
                        Child, (IPTR) VSpace(0),

                        Child, (IPTR) ColGroup(6),
                            Child, (IPTR) Label("FAT:"),
                            Child, (IPTR) PopaslObject,
                                MUIA_Popstring_String, (IPTR) (ncm->ncm_FatFSObj = (APTR) StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR) ncm->ncm_CDC->cdc_FATFSName,
                                    MUIA_String_MaxLen, 63,
                                    End),
                                MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopFile),
                                ASLFR_TitleText, (IPTR) "Select filesystem to use with FAT partitions...",
                                End,
                            Child, (IPTR) Label("DosType:"),
                            Child, (IPTR) (ncm->ncm_FatDosTypeObj = (APTR) StringObject,
                                StringFrame,
                                MUIA_HorizWeight, 50,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR) dostypebuf,
                                MUIA_String_Accept, (IPTR) "0123456789abcdefABCDEF",
                                MUIA_String_MaxLen, 9,
                                End),
                            Child, (IPTR) Label("Ctrl:"),
                            Child, (IPTR) (ncm->ncm_FatControlObj = (APTR) StringObject,
                                StringFrame,
                                MUIA_HorizWeight, 50,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR) ncm->ncm_CDC->cdc_FATControl,
                                MUIA_String_MaxLen, 63,
                                End),
                            Child, (IPTR) Label("NTFS:"),
                            Child, (IPTR) PopaslObject,
                                MUIA_Popstring_String, (IPTR) (ncm->ncm_NTFSObj = (APTR) StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR) ncm->ncm_CDC->cdc_NTFSName,
                                    MUIA_String_MaxLen, 63,
                                    End),
                                MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopFile),
                                ASLFR_TitleText, (IPTR) "Select filesystem to use with NTFS partitions...",
                                End,
                            Child, (IPTR) Label("DosType:"),
                            Child, (IPTR) (ncm->ncm_NTFSDosTypeObj = (APTR) StringObject,
                                StringFrame,
                                MUIA_HorizWeight, 50,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR) ntfsdostypebuf,
                                MUIA_String_Accept, (IPTR) "0123456789abcdefABCDEF",
                                MUIA_String_MaxLen, 9,
                                End),
                            Child, (IPTR) Label("Ctrl:"),
                            Child, (IPTR) (ncm->ncm_NTFSControlObj = (APTR) StringObject,
                                StringFrame,
                                MUIA_HorizWeight, 50,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR) ncm->ncm_CDC->cdc_NTFSControl,
                                MUIA_String_MaxLen, 63,
                                End),
                            Child, (IPTR) Label("CD/DVD:"),
                            Child, (IPTR) PopaslObject,
                                MUIA_Popstring_String, (IPTR) (ncm->ncm_CDFSObj = (APTR) StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR) ncm->ncm_CDC->cdc_CDFSName,
                                    MUIA_String_MaxLen, 63,
                                    End),
                                MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopFile),
                                ASLFR_TitleText, (IPTR) "Select filesystem to use with CD/DVD partitions...",
                                End,
                            Child, (IPTR) Label("DosType:"),
                            Child, (IPTR) (ncm->ncm_CDDosTypeObj = (APTR) StringObject,
                                StringFrame,
                                MUIA_HorizWeight, 50,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR) cddostypebuf,
                                MUIA_String_Accept, (IPTR) "0123456789abcdefABCDEF",
                                MUIA_String_MaxLen, 9,
                                End),
                            Child, (IPTR) Label("Ctrl:"),
                            Child, (IPTR) (ncm->ncm_CDControlObj = (APTR) StringObject,
                                StringFrame,
                                MUIA_HorizWeight, 50,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_Contents, (IPTR) ncm->ncm_CDC->cdc_CDControl,
                                MUIA_String_MaxLen, 63,
                                End),
                            End,
                        Child, (IPTR) VSpace(0),
                        End,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) ListviewObject,
                            MUIA_CycleChain, 1,
                            MUIA_Listview_List, (IPTR) (ncm->ncm_LunLVObj = (APTR) ListObject,
                                ReadListFrame,
                                MUIA_List_Format, (IPTR) bar,
                                MUIA_List_Title, TRUE,
                                MUIA_List_DisplayHook, (IPTR) &ncm->ncm_LUNListDisplayHook,
                                End),
                            End,
                        Child, (IPTR) (ncm->ncm_LunGroupObj = (APTR) VGroup,
                            MUIA_Disabled, TRUE,
                            Child, (IPTR) VSpace(0),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_AutoMountRDBObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, TRUE,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) Label("AutoMount RDB partitions"),
                                Child, (IPTR) HSpace(0),
                                End,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_BootRDBObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, FALSE,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) Label("Boot from RDB partitions"),
                                Child, (IPTR) HSpace(0),
                                End,
                            Child, (IPTR) VSpace(0),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_AutoMountFATObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, TRUE,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) Label("AutoMount FAT/NTFS partitions"),
                                Child, (IPTR) HSpace(0),
                                End,
                           Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_MountAllFATObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, TRUE,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) Label("Mount all FAT partitions"),
                                Child, (IPTR) HSpace(0),
                                End,
                            Child, (IPTR) VSpace(0),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_AutoMountCDObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, TRUE,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) Label("AutoMount CD/DVD"),
                                Child, (IPTR) HSpace(0),
                                End,
                            Child, (IPTR) VSpace(0),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (ncm->ncm_UnmountObj = (APTR) ImageObject, ImageButtonFrame,
                                    MUIA_Background, MUII_ButtonBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_Image_Spec, MUII_CheckMark,
                                    MUIA_Image_FreeVert, TRUE,
                                    MUIA_Selected, FALSE,
                                    MUIA_ShowSelState, FALSE,
                                    End),
                                Child, (IPTR) HGroup,
                                    Child, (IPTR) Label("Unmount partitions after removal"),
                                    Child, (IPTR) HSpace(0),
                                    End,
                                End,
                            Child, (IPTR) VSpace(0),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) Label("DOSName:"),
                                Child, (IPTR) (ncm->ncm_FatDOSNameObj = (APTR) StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Contents, (IPTR) "UMSD",
                                    MUIA_String_Reject, (IPTR) "/ :?#*",
                                    MUIA_String_MaxLen, 31,
                                    End),
                                Child, (IPTR) Label("Buffers:"),
                                Child, (IPTR) (ncm->ncm_FatBuffersObj = (APTR) StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Integer, 100,
                                    MUIA_String_Accept, (IPTR) "0123456789",
                                    End),
                                End,
                            Child, (IPTR) VSpace(0),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) Label("Default " DEVNAME " unit:"),
                                Child, (IPTR) (ncm->ncm_UnitObj = (APTR) StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_AdvanceOnCR, TRUE,
                                    MUIA_String_Integer, 0,
                                    MUIA_String_Accept, (IPTR) "0123456789",
                                    End),
                                End,
                            End),
                        End,
                    End,
                Child, (IPTR) HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR) (ncm->ncm_UseObj = (APTR) TextObject, ButtonFrame,
                        MUIA_ShowMe, (IPTR) ncm->ncm_Interface,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR) "\33c Save ",
                        End),
                    Child, (IPTR) (ncm->ncm_SetDefaultObj = (APTR) TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, ncm->ncm_Interface ? (IPTR) "\33c Save as Default " : (IPTR) "\33c Save Defaults ",
                        End),
                    Child, (IPTR) (ncm->ncm_CloseObj = (APTR) TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR) "\33c Use ",
                        End),
                    End,
                End,
            End),
        End;

    if(!ncm->ncm_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        GM_UNIQUENAME(nGUITaskCleanup)(ncm);
        return;
    }

    if(ncm->ncm_Interface)
    {
        cncm = ncm;
        while(((struct Node *) cncm)->ln_Succ)
        {
            if(cncm->ncm_UnitLUN0 != ncm)
            {
                break;
            }
            DoMethod(ncm->ncm_LunLVObj, MUIM_List_InsertSingle, cncm, MUIV_List_Insert_Bottom);
            cncm = (struct NepClassMS *) ((struct Node *) cncm)->ln_Succ;
        }
    } else {
        DoMethod(ncm->ncm_LunLVObj, MUIM_List_InsertSingle, ncm, MUIV_List_Insert_Bottom);
    }
    DoMethod(ncm->ncm_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(ncm->ncm_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(ncm->ncm_SetDefaultObj, MUIM_Notify, MUIA_Pressed, FALSE,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(ncm->ncm_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(ncm->ncm_AutoDtxMaxTransObj, MUIM_Notify, MUIA_Pressed, FALSE,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, ID_AUTODTXMAXTX);
    DoMethod(ncm->ncm_LunLVObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, ID_SELECT_LUN);

    DoMethod(ncm->ncm_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(ncm->ncm_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(ncm->ncm_SetDefaultMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncm->ncm_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(ncm->ncm_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             ncm->ncm_App, 2, MUIM_Application_OpenConfigWindow, 0);

    set(ncm->ncm_LunLVObj, MUIA_List_Active, MUIV_List_Active_Top);
    {
        IPTR  isopen = 0;
        IPTR  iconify = 0;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(ncm->ncm_App, MUIA_Application_Iconified, &iconify);
        set(ncm->ncm_MainWindow, MUIA_Window_Open, TRUE);
        get(ncm->ncm_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            GM_UNIQUENAME(nGUITaskCleanup)(ncm);
            return;
        }
        sigmask = 0;
        do
        {
            retid = DoMethod(ncm->ncm_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_DEF_CONFIG:
                case ID_STORE_CONFIG:
                case MUIV_Application_ReturnID_Quit:
                {
                    ULONG tmpflags;
                    ULONG patchflags;
                    STRPTR tmpstr;

                    get(ncm->ncm_NakTimeoutObj, MUIA_Numeric_Value, &ncm->ncm_CDC->cdc_NakTimeout);
                    get(ncm->ncm_StartupDelayObj, MUIA_Numeric_Value, &ncm->ncm_CDC->cdc_StartupDelay);
                    patchflags = ncm->ncm_CDC->cdc_PatchFlags & ~(PFF_SINGLE_LUN|PFF_FAKE_INQUIRY|PFF_SIMPLE_SCSI|PFF_NO_RESET|PFF_MODE_XLATE|PFF_DEBUG|PFF_NO_FALLBACK|PFF_REM_SUPPORT|PFF_FIX_INQ36|PFF_CSS_BROKEN|PFF_FIX_CAPACITY|PFF_EMUL_LARGE_BLK);
                    tmpflags = 0;
                    get(ncm->ncm_SingleLunObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_SINGLE_LUN;
                    tmpflags = 0;
                    get(ncm->ncm_FakeInquiryObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_FAKE_INQUIRY;
                    tmpflags = 0;
                    get(ncm->ncm_SimpleSCSIObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_SIMPLE_SCSI;
                    tmpflags = 0;
                    get(ncm->ncm_InitialResetObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_NO_RESET;
                    tmpflags = 0;
                    get(ncm->ncm_XLate610Obj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_MODE_XLATE;
                    tmpflags = 0;
                    get(ncm->ncm_RemSupportObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_REM_SUPPORT;
                    tmpflags = 0;
                    get(ncm->ncm_FixInquiryObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_FIX_INQ36;
                    tmpflags = 0;
                    get(ncm->ncm_CSSBrokenObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_CSS_BROKEN;
                    tmpflags = 0;
                    get(ncm->ncm_FixCapacityObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_FIX_CAPACITY;
                    tmpflags = 0;
                    get(ncm->ncm_EmulLargeBlkObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_EMUL_LARGE_BLK;
                    tmpflags = 0;
                    get(ncm->ncm_NoFallbackObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_NO_FALLBACK;
                    tmpflags = 0;
                    get(ncm->ncm_DebugObj, MUIA_Selected, &tmpflags);
                    if(tmpflags) patchflags |= PFF_DEBUG;
                    ncm->ncm_CDC->cdc_PatchFlags = patchflags;

                    get(ncm->ncm_MaxTransferObj, MUIA_Cycle_Active, &ncm->ncm_CDC->cdc_MaxTransfer);

                    tmpstr = "";
                    get(ncm->ncm_FatFSObj, MUIA_String_Contents, &tmpstr);
                    strncpy(ncm->ncm_CDC->cdc_FATFSName, tmpstr, 63);
                    tmpstr = "";
                    get(ncm->ncm_FatControlObj, MUIA_String_Contents, &tmpstr);
                    strncpy(ncm->ncm_CDC->cdc_FATControl, tmpstr, 63);
                    tmpstr = "";
                    get(ncm->ncm_FatDosTypeObj, MUIA_String_Contents, &tmpstr);
                    ncm->ncm_CDC->cdc_FATDosType = nGetDosType(tmpstr);

                    tmpstr = "";
                    get(ncm->ncm_NTFSObj, MUIA_String_Contents, &tmpstr);
                    strncpy(ncm->ncm_CDC->cdc_NTFSName, tmpstr, 63);
                    tmpstr = "";
                    get(ncm->ncm_NTFSControlObj, MUIA_String_Contents, &tmpstr);
                    strncpy(ncm->ncm_CDC->cdc_NTFSControl, tmpstr, 63);
                    tmpstr = "";
                    get(ncm->ncm_NTFSDosTypeObj, MUIA_String_Contents, &tmpstr);
                    ncm->ncm_CDC->cdc_NTFSDosType = nGetDosType(tmpstr);

                    tmpstr = "";
                    get(ncm->ncm_CDFSObj, MUIA_String_Contents, &tmpstr);
                    strncpy(ncm->ncm_CDC->cdc_CDFSName, tmpstr, 63);
                    tmpstr = "";
                    get(ncm->ncm_CDControlObj, MUIA_String_Contents, &tmpstr);
                    strncpy(ncm->ncm_CDC->cdc_CDControl, tmpstr, 63);
                    tmpstr = "";
                    get(ncm->ncm_CDDosTypeObj, MUIA_String_Contents, &tmpstr);
                    ncm->ncm_CDC->cdc_CDDosType = nGetDosType(tmpstr);

                    if(ncm->ncm_Interface)
                    {
                        /* copy device config to all luns */
                        cncm = (struct NepClassMS *) ((struct Node *) ncm)->ln_Succ;
                        while(((struct Node *) cncm)->ln_Succ)
                        {
                            if(cncm->ncm_UnitLUN0 != ncm)
                            {
                                break;
                            }
                            *(cncm->ncm_CDC) = *(ncm->ncm_CDC);
                            cncm = (struct NepClassMS *) ((struct Node *) cncm)->ln_Succ;
                        }
                    }
                    if(curncm)
                    {
                        get(ncm->ncm_AutoMountFATObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoMountFAT);
                        get(ncm->ncm_MountAllFATObj, MUIA_Selected, &curncm->ncm_CUC->cuc_MountAllFAT);
                        get(ncm->ncm_AutoMountCDObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoMountCD);
                        tmpstr = "";
                        get(ncm->ncm_FatDOSNameObj, MUIA_String_Contents, &tmpstr);
                        strncpy(curncm->ncm_CUC->cuc_FATDOSName, tmpstr, 31);
                        get(ncm->ncm_FatBuffersObj, MUIA_String_Integer, &curncm->ncm_CUC->cuc_FATBuffers);
                        get(ncm->ncm_AutoMountRDBObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoMountRDB);
                        get(ncm->ncm_BootRDBObj, MUIA_Selected, &curncm->ncm_CUC->cuc_BootRDB);
                        get(ncm->ncm_UnitObj, MUIA_String_Integer, &curncm->ncm_CUC->cuc_DefaultUnit);
                        get(ncm->ncm_UnmountObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoUnmount);
                    }

                    if(retid == ID_DEF_CONFIG)
                    {
                        pic = psdGetClsCfg(GM_UNIQUENAME(libname));
                        if(!pic)
                        {
                            psdSetClsCfg(GM_UNIQUENAME(libname), NULL);
                            pic = psdGetClsCfg(GM_UNIQUENAME(libname));
                        }
                        if(pic)
                        {
                            psdAddCfgEntry(pic, ncm->ncm_CDC);
                            psdAddCfgEntry(pic, ncm->ncm_CUC);
                            psdSaveCfgToDisk(NULL, FALSE);
                        }
                    }
                    if(GM_UNIQUENAME(nStoreConfig)(ncm))
                    {
                        if(retid != MUIV_Application_ReturnID_Quit)
                        {
                            psdSaveCfgToDisk(NULL, FALSE);
                        }
                        retid = MUIV_Application_ReturnID_Quit;
                    }
                    break;
                }

                case ID_SELECT_LUN:
                {
                    STRPTR tmpstr;
                    DoMethod(ncm->ncm_LunLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &cncm);
                    if(curncm != cncm)
                    {
                        if(curncm)
                        {
                            get(ncm->ncm_AutoMountFATObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoMountFAT);
                            get(ncm->ncm_MountAllFATObj, MUIA_Selected, &curncm->ncm_CUC->cuc_MountAllFAT);
                            get(ncm->ncm_AutoMountCDObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoMountCD);
                            tmpstr = "";
                            get(ncm->ncm_FatDOSNameObj, MUIA_String_Contents, &tmpstr);
                            strncpy(curncm->ncm_CUC->cuc_FATDOSName, tmpstr, 31);
                            get(ncm->ncm_FatBuffersObj, MUIA_String_Integer, &curncm->ncm_CUC->cuc_FATBuffers);
                            get(ncm->ncm_AutoMountRDBObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoMountRDB);
                            get(ncm->ncm_BootRDBObj, MUIA_Selected, &curncm->ncm_CUC->cuc_BootRDB);
                            get(ncm->ncm_UnitObj, MUIA_String_Integer, &curncm->ncm_CUC->cuc_DefaultUnit);
                            get(ncm->ncm_UnmountObj, MUIA_Selected, &curncm->ncm_CUC->cuc_AutoUnmount);
                        }
                    }
                    if((curncm = cncm))
                    {
                        set(ncm->ncm_AutoMountFATObj, MUIA_Selected, curncm->ncm_CUC->cuc_AutoMountFAT);
                        set(ncm->ncm_MountAllFATObj, MUIA_Selected, curncm->ncm_CUC->cuc_MountAllFAT);
                        set(ncm->ncm_AutoMountCDObj, MUIA_Selected, curncm->ncm_CUC->cuc_AutoMountCD);
                        set(ncm->ncm_FatDOSNameObj, MUIA_String_Contents, curncm->ncm_CUC->cuc_FATDOSName);
                        set(ncm->ncm_FatBuffersObj, MUIA_String_Integer, curncm->ncm_CUC->cuc_FATBuffers);
                        set(ncm->ncm_AutoMountRDBObj, MUIA_Selected, curncm->ncm_CUC->cuc_AutoMountRDB);
                        set(ncm->ncm_BootRDBObj, MUIA_Selected, curncm->ncm_CUC->cuc_BootRDB);
                        set(ncm->ncm_UnitObj, MUIA_String_Integer, curncm->ncm_CUC->cuc_DefaultUnit);
                        set(ncm->ncm_UnmountObj, MUIA_Selected, curncm->ncm_CUC->cuc_AutoUnmount);
                        set(ncm->ncm_LunGroupObj, MUIA_Disabled, FALSE);
                    } else {
                        set(ncm->ncm_LunGroupObj, MUIA_Disabled, TRUE);
                    }
                    break;
                }
                case ID_AUTODTXMAXTX:
                {
                    DoMethod(ncm->ncm_LunLVObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &cncm);
                    set(ncm->ncm_App, MUIA_Application_Sleep, TRUE);
                    AutoDetectMaxTransfer(cncm);
                    set(ncm->ncm_App, MUIA_Application_Sleep, FALSE);
                    set(ncm->ncm_MaxTransferObj, MUIA_Cycle_Active, ncm->ncm_CDC->cdc_MaxTransfer);
                    break;
                }

                case ID_ABOUT:
                    MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Blimey!", VERSION_STRING "\n\nCode for AutoMounting based\non work by Thore Böckelmann.", NULL);
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
        set(ncm->ncm_MainWindow, MUIA_Window_Open, FALSE);
    }
    GM_UNIQUENAME(nGUITaskCleanup)(ncm);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "AutoDetectMaxTransfer()" */
void AutoDetectMaxTransfer(struct NepClassMS *cncm)
{
    LONG res;
    UBYTE *memory;
    UBYTE *orgbuffer;
    UBYTE *cmpbuffer;
    ULONG *lbufptr;
    struct MsgPort *mp;
    struct IOStdReq *ioreq;
    struct NepClassMS *ncm = cncm->ncm_UnitLUN0;
    ULONG numblocks;
    ULONG memsize = 4<<20;
    ULONG block;
    ULONG maxtrans;
    LONG ioerr;
    BOOL bail = FALSE;
    ULONG cnt;

    res = MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Continue|Cancel",
        "Auto-detection of the maximum transfer rate\n"
        "will need some media inserted in the selected\n"
        "LUN of the drive. Moreover, the contents of the\n"
        "media may not be empty.\n"
        "The test will need about 4 MB of temporary memory!\n"
        "No data is written to the disk!", NULL);
    if(!res)
    {
        return;
    }
    if(!cncm->ncm_UnitReady)
    {
        MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Argh!", "No disk/media in drive!", NULL);
        return;
    }
    memory = (UBYTE *) AllocVec(memsize, MEMF_CLEAR);
    if(!memory)
    {
        MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Argh!", "Sorry, out of memory!", NULL);
        return;
    }
    mp = CreateMsgPort();
    if(mp)
    {
        ioreq = (struct IOStdReq *) CreateIORequest(mp, sizeof(struct IOStdReq));
        if(ioreq)
        {
            if(!OpenDevice(DEVNAME, cncm->ncm_UnitNo, (struct IORequest *) ioreq, 0))
            {
                ncm->ncm_CDC->cdc_MaxTransfer = 0;
                if(!cncm->ncm_BlockSize)
                {
                    ioreq->io_Command = TD_GETGEOMETRY;
                    ioreq->io_Data = &cncm->ncm_Geometry;
                    ioreq->io_Length = sizeof(cncm->ncm_Geometry);
                    DoIO((struct IORequest *) ioreq);
                }

                numblocks = (memsize>>1) / cncm->ncm_BlockSize;
                orgbuffer = memory;
                cmpbuffer = &memory[memsize>>1];

                do
                {
                    for(block = 0; block < numblocks; block++)
                    {
                        ioreq->io_Command = TD_READ64;
                        ioreq->io_Actual = 0;
                        ioreq->io_Offset = block*cncm->ncm_BlockSize;
                        ioreq->io_Length = cncm->ncm_BlockSize;
                        ioreq->io_Data = &orgbuffer[ioreq->io_Offset];
                        ioerr = DoIO((struct IORequest *) ioreq);
                        if(ioerr)
                        {
                            MUI_Request(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, ">:-{", "Error %ld while reading block %ld!\nAborting process...", ioerr, block);
                            bail = TRUE;
                            break;
                        }
                    }
                    if(bail) break;

                    do
                    {
                        maxtrans = (1UL<<(ncm->ncm_CDC->cdc_MaxTransfer+16));
                        // do a quick check on block contents
                        numblocks = (memsize>>1) / maxtrans;
                        if(numblocks)
                        {
                            for(block = 0; block < numblocks-1; block++)
                            {
                                if(!memcmp(&orgbuffer[block * maxtrans], &orgbuffer[(block + 1) * maxtrans], (size_t) maxtrans))
                                {
                                    res = MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Continue|Abort", "Sorry, media contents do not\nseem to be diversive enough!\nResults might be wrong!", NULL);
                                    if(!res)
                                    {
                                        bail = TRUE;
                                    }
                                    break;
                                }
                            }
                        }
                        if(bail) break;

                        // fill compare buffer with garbage
                        cnt = (memsize>>3);
                        lbufptr = (ULONG *) cmpbuffer;
                        do
                        {
                            *lbufptr = (IPTR) lbufptr;
                            lbufptr++;
                        }
                        while(--cnt);

                        // start reading the chunks
                        numblocks = (memsize>>1) / maxtrans;
                        for(block = 0; block < numblocks; block++)
                        {
                            ioreq->io_Command = TD_READ64;
                            ioreq->io_Actual = 0;
                            ioreq->io_Offset = block*maxtrans;
                            ioreq->io_Length = maxtrans;
                            ioreq->io_Data = &cmpbuffer[ioreq->io_Offset];
                            ioerr = DoIO((struct IORequest *) ioreq);
                            if(ioerr)
                            {
                                MUI_Request(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, ">:-{", "Error %ld while reading at offset %ld!\nAborting process...", ioerr, &ioreq->io_Offset);
                                bail = TRUE;
                                break;
                            }
                            if(memcmp(&orgbuffer[ioreq->io_Offset], &cmpbuffer[ioreq->io_Offset], (size_t) maxtrans))
                            {
                                MUI_Request(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Hmpf!", "Reading failed at MaxTrans = %08lx", maxtrans-1);
                                if(ncm->ncm_CDC->cdc_MaxTransfer)
                                {
                                    ncm->ncm_CDC->cdc_MaxTransfer--;
                                }
                                bail = TRUE;
                                break;
                            }
                        }
                        if(bail) break;
                        //MUI_Request(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Wow!", "Test with %ld (%ld) succeeded!", maxtrans, numblocks);
                        if(ncm->ncm_CDC->cdc_MaxTransfer < 5)
                        {
                            ncm->ncm_CDC->cdc_MaxTransfer++;
                        } else {
                            break;
                        }
                    } while(TRUE);
                    if(!bail)
                    {
                        MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Wow!", "Test succeeded, setting MaxTrans to maximum value!", NULL);
                    }
                } while(FALSE);
                CloseDevice((struct IORequest *) ioreq);
            } else {
                MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Argh!", "Couldn't open device!", NULL);
            }
            DeleteIORequest((struct IORequest *) ioreq);
        } else {
            MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Argh!", "No IOReq!", NULL);
        }
        DeleteMsgPort(mp);
    } else {
        MUI_RequestA(ncm->ncm_App, ncm->ncm_MainWindow, 0, NULL, "Argh!", "No MsgPort!", NULL);
    }
    FreeVec(memory);
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void GM_UNIQUENAME(nGUITaskCleanup)(struct NepClassMS *ncm)
{
    if(ncm->ncm_App)
    {
        MUI_DisposeObject(ncm->ncm_App);
        ncm->ncm_App = NULL;
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
    ncm->ncm_GUIBinding = NULL;
    ncm->ncm_GUITask = NULL;
    --ncm->ncm_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */

/* /// "LUNListDisplayHook()" */
AROS_UFH3(LONG, GM_UNIQUENAME(LUNListDisplayHook),
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(char **, strarr, A2),
          AROS_UFHA(struct NepClassMS *, ncm, A1))
{
    AROS_USERFUNC_INIT

    //struct NepMSBase *nh = ncm->ncm_ClsBase;
    if(ncm)
    {
        *strarr++ = ncm->ncm_LUNNumStr;
        *strarr = ncm->ncm_LUNIDStr;
    } else {
        *strarr++ = "\33l\33uLUN";
        *strarr = "\33l\33uID";
    }
    return(0);

    AROS_USERFUNC_EXIT
}
/* \\\ */

