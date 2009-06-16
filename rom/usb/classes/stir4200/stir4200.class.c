/*
 *----------------------------------------------------------------------------
 *                         stir4200 class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "stir4200.class.h"

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
    struct NepClassSTIr4200 *ncp;
    struct NepSTIr4200Base *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct NepSTIrDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct NepSTIrDevBase), NULL)))
        {
            nh->nh_DevBase->np_ClsBase = nh;
            ncp = &nh->nh_DummyNCP;
            ncp->ncp_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
            if(ncp->ncp_CDC)
            {
                ncp->ncp_ClsBase = nh;
                ncp->ncp_Device = NULL;
                Forbid();
                AddDevice((struct Device *) nh->nh_DevBase);
                nh->nh_DevBase->np_Library.lib_OpenCnt++;
                Permit();
                ret = nh;
            }
        } else {
            KPRINTF(20, ("failed to create usbstir4200.device\n"));
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
    struct NepClassSTIr4200 *ncp;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->np_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);

        ncp = (struct NepClassSTIr4200 *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) ncp);
            FreeVec(ncp->ncp_CDC);
            FreeVec(ncp);
            ncp = (struct NepClassSTIr4200 *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->np_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! stir4200.class expunged!\n\n"));
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

/* /// "usbAttemptDeviceBinding()" */
struct NepClassSTIr4200 * usbAttemptDeviceBinding(struct NepSTIr4200Base *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    IPTR prodid;
    IPTR vendid;
    KPRINTF(1, ("nepSTIr4200AttemptDeviceBinding(%08lx)\n", pd));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_VendorID, &vendid,
                    DA_ProductID, &prodid,
                    TAG_END);
        CloseLibrary(ps);
        if((vendid == 0x066F) && (prodid == 0x4200))
        {
                return(usbForceDeviceBinding(nh, pd));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceDeviceBinding()" */
struct NepClassSTIr4200 * usbForceDeviceBinding(struct NepSTIr4200Base *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct NepClassSTIr4200 *ncp;
    struct NepClassSTIr4200 *tmpncp;
    struct ClsDevCfg *cdc;
    STRPTR devname;
    STRPTR devidstr;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    BOOL  unitfound;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepSTIr4200ForceDeviceBinding(%08lx)\n", pd));

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
        ncp = (struct NepClassSTIr4200 *) nh->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(!ncp->ncp_Task)
            {
                unitno = ncp->ncp_UnitNo;
                unitfound = TRUE;
                break;
            }
            ncp = (struct NepClassSTIr4200 *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(ncp = AllocVec(sizeof(struct NepClassSTIr4200), MEMF_PUBLIC|MEMF_CLEAR)))
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
        ncp->ncp_Device = pd;
        ncp->ncp_UnitProdID = prodid;
        ncp->ncp_UnitVendorID = vendid;
        ncp->ncp_DevIDString = devidstr;

        nLoadBindingConfig(ncp);

        /* Find next free unit number */
        if(unitno == (ULONG) -1)
        {
            unitno = ncp->ncp_CDC->cdc_DefaultUnit;
            tmpncp = (struct NepClassSTIr4200 *) nh->nh_Units.lh_Head;
            while(tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                if(tmpncp->ncp_UnitNo == unitno)
                {
                    unitno++;
                    tmpncp = (struct NepClassSTIr4200 *) nh->nh_Units.lh_Head;
                } else {
                    tmpncp = (struct NepClassSTIr4200 *) tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
                }
            }
        }
        ncp->ncp_UnitNo = unitno;
        Permit();

        psdSafeRawDoFmt(buf, 64, "stir4200.class<%08lx>", ncp);
        ncp->ncp_ReadySignal = SIGB_SINGLE;
        ncp->ncp_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nSTIr4200Task, ncp)))
        {
            psdBorrowLocksWait(tmptask, 1UL<<ncp->ncp_ReadySignal);
            if(ncp->ncp_Task)
            {
                ncp->ncp_ReadySigTask = NULL;
                //FreeSignal(ncp->ncp_ReadySignal);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Look at this infra red face '%s' at %s unit %ld!",
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
        FreeVec(ncp);*/
        Permit();
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseDeviceBinding()" */
void usbReleaseDeviceBinding(struct NepSTIr4200Base *nh, struct NepClassSTIr4200 *ncp)
{
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("nepSTIr4200ReleaseDeviceBinding(%08lx)\n", ncp));
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
                       "'%s' turned into a infrared dwarf.",
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

    KPRINTF(1, ("nepSTIr4200GetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "IrDA Bridge for STIr4200 chipsets via usbstir4200.device";
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
                 *((IPTR *) ti->ti_Data) = ((struct NepClassSTIr4200 *) usbstruct)->ncp_UsingDefaultCfg;
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

    struct NepClassSTIr4200 *ncp;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptDeviceBinding:
            return((IPTR) usbAttemptDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ForceDeviceBinding:
            return((IPTR) usbForceDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ReleaseDeviceBinding:
            usbReleaseDeviceBinding(nh, (struct NepClassSTIr4200 *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenBindingCfgWindow(nh, &nh->nh_DummyNCP));

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassSTIr4200 *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            Forbid();
            ncp = (struct NepClassSTIr4200 *) nh->nh_Units.lh_Head;
            while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
            {
                nLoadBindingConfig(ncp);
                ncp = (struct NepClassSTIr4200 *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
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
BOOL nLoadClassConfig(struct NepSTIr4200Base *nh)
{
    struct NepClassSTIr4200 *ncp = &nh->nh_DummyNCP;
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
    cdc->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('S','T','I','R'));
    cdc->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    cdc->cdc_DefaultUnit = 0;
    cdc->cdc_StackAuto = TRUE;
    cdc->cdc_TXPower = 0;
    cdc->cdc_RXSense = 1;
    ncp->ncp_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = psdGetClsCfg(libname);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, ncp->ncp_CDC->cdc_ChunkID);
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
BOOL nLoadBindingConfig(struct NepClassSTIr4200 *ncp)
{
    struct NepSTIr4200Base *nh = ncp->ncp_ClsBase;
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
        cdc = psdGetCfgChunk(pic, ncp->ncp_CDC->cdc_ChunkID);
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
LONG nOpenBindingCfgWindow(struct NepSTIr4200Base *nh, struct NepClassSTIr4200 *ncp)
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

/* /// "FCS Checksum tables" */
static UWORD FCS16Table[256] =
{
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static ULONG FCS32Table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
/* \\\ */

#if 0
/* /// "nCalcFCS16()" */
UWORD nCalcFCS16(UBYTE *buf, ULONG len)
{
    register UWORD crc = 0;
    if(len)
    {
        do
        {
            crc = ((crc>>8) & 0xff) ^ FCS16Table[(crc ^ (*buf++)) & 0xff)];
        } while(--len);
    }
    return(crc ^ 0xffff);
}
/* \\\ */

/* /// "nCalcFCS32()" */
ULONG nCalcFCS16(UBYTE *buf, ULONG len)
{
    register ULONG crc = 0;
    if(len)
    {
        do
        {
            crc = ((crc>>8) & 0xff) ^ FCS32Table[(crc ^ (*buf++)) & 0xff)];
        } while(--len);
    }
    return(crc ^ 0xffffffff);
}
/* \\\ */

#define CRC32_POLYNOMIAL 0x04c11db7

/* /// "nCalcFCS32()" */
ULONG nCalcFCS32(UBYTE *buf, ULONG len)
{
    ULONG crc = 0xffffffff;
    if(len)
    {
        do
        {
            UBYTE data = *buf++;
            UWORD cnt;
            // could be optimized by lookup table.
            cnt = 8;
            do
            {
                if(((LONG) (crc ^ (data<<31))) < 0)
                {
                    crc <<= 1;
                    crc ^= CRC32_POLYNOMIAL;
                } else {
                    crc <<= 1;
                }
                data >>= 1;
            } while(--cnt);
        } while(--len);
    }
    return(crc);
}
/* \\\ */
#endif

/* /// "nSetBaudrate()" */
BOOL nSetBaudrate(struct NepClassSTIr4200 *ncp, ULONG baudrate)
{
    ULONG mode = SMF_FFSPRST|SMF_FASTRXEN;
    ULONG clk = 0;
    BOOL res;
    if(!baudrate)
    {
        return(FALSE);
    }

    res = nSetReg(ncp, STREG_CTRL, SCF_SRESET);
    res &= nSetReg(ncp, STREG_DPLLTUNE, 0x15);
    ncp->ncp_FIRMode = FALSE;
    switch(baudrate)
    {
        case 2400:
            mode |= SMF_PDCLK8;
            clk = PDCLK_2400;
        case 9600:
            if(!clk) clk = PDCLK_9600;
        case 19200:
            if(!clk) clk = PDCLK_19200;
        case 38400:
            if(!clk) clk = PDCLK_38400;
        case 57600:
            if(!clk) clk = PDCLK_57600;
        case 115200:
            if(!clk) clk = PDCLK_115200;
            KPRINTF(1, ("Setting SIR Mode (%ld baud)!\n", baudrate));
            nSetReg(ncp, STREG_BAUDRATE, clk);
            mode |= SMF_SIR;
            break;

        case 4000000:
            KPRINTF(1, ("Setting FIR Mode!\n"));
            ncp->ncp_FIRMode = TRUE;
            res &= nSetReg(ncp, STREG_BAUDRATE, PDCLK_4000000);
            mode |= SMF_FIR|SMF_FASTRSTEN;
            break;

        default:
            KPRINTF(10, ("Unsupported baudrate %ld\n", baudrate));
            // set 9600 as default
            nSetReg(ncp, STREG_BAUDRATE, PDCLK_9600);
            mode |= SMF_SIR;
            res = FALSE;
            ncp->ncp_BaudRate = 9600;
    }
    if(res)
    {
        ncp->ncp_BaudRate = baudrate;
    }
    res &= nSetReg(ncp, STREG_MODE, mode);
    res &= nSetReg(ncp, STREG_CTRL, SCF_SDMODE|SCF_TXPWR_HIGH);
    res &= nSetReg(ncp, STREG_CTRL, SCF_TXPWR_HIGH);
    res &= nSetReg(ncp, STREG_SENSITIVITY, 1<<SSB_RXDSNS);
    return(res);
}
/* \\\ */

#define STUFFBYTEFIR(data) \
    if((data >= 0x7d) && (data <= 0x7f)) \
    { \
        data ^= 0x20; \
        *buf++ = 0x7d; \
    } \
    *buf++ = data

#define STUFFBYTEFIRFCS(data) \
    crc = ((crc>>8) & 0xff) ^ FCS32Table[(crc ^ data) & 0xff]; \
    STUFFBYTEFIR(data)

#define STUFFBYTESIR(data) \
    if((data == 0xc0) || (data == 0xc1) || (data == 0x7d)) \
    { \
        data ^= 0x20; \
        *buf++ = 0x7d; \
    } \
    *buf++ = data

#define STUFFBYTESIRFCS(data) \
    crc = ((crc>>8) & 0xff) ^ FCS16Table[(crc ^ data) & 0xff]; \
    STUFFBYTESIR(data)

/* /// "nSendFrame()" */
BOOL nSendFrame(struct NepClassSTIr4200 *ncp, struct IOIrDAReq *ioreq)
{
    UBYTE *buf = ncp->ncp_WriteBuffer;
    ULONG framelen;
    ULONG datalen = ioreq->ioir_Length;
    UBYTE *dataptr = ioreq->ioir_Data;

    if(ncp->ncp_FIRMode)
    {
        register ULONG crc = 0xffffffff;
        // maximum size of frame: 2 (header) + 2 (len) + 16 (preamble) + 2 (BOF) + 4 (AC) + 2*datalen + 8 (FCS) + 2 (EOF)
        if(datalen<<1 > DEFBUFFERSIZE - 36)
        {
            ioreq->ioir_Req.io_Error = IRIOERR_OUTOFMEMORY;
            return(FALSE);
        }
        *buf++ = 0x55; // header
        *buf++ = 0xaa;
        buf += 2; // length of frame needs to be filled out later
        // preamble
        *((ULONG *) buf) = 0x7f7f7f7f;
        ((ULONG *) buf)[1] = 0x7f7f7f7f;
        ((ULONG *) buf)[2] = 0x7f7f7f7f;
        ((ULONG *) buf)[3] = 0x7f7f7f7f;
        buf += 16;
        *buf++ = 0x7e;
        *buf++ = 0x7e;
        STUFFBYTEFIRFCS(ioreq->ioir_Address); // address byte
        STUFFBYTEFIRFCS(ioreq->ioir_Control); // control byte
        {
            register UBYTE data;
            if(datalen)
            {
                do
                {
                    data = *dataptr++;
                    STUFFBYTEFIRFCS(data);
                } while(--datalen);
            }
            crc ^= 0xffffffff;
            data = crc;
            STUFFBYTEFIR(data);
            crc >>= 8;
            data = crc;
            STUFFBYTEFIR(data);
            crc >>= 8;
            data = crc;
            STUFFBYTEFIR(data);
            crc >>= 8;
            data = crc;
            STUFFBYTEFIR(data);
        }
        *buf++ = 0x7e;
        *buf++ = 0x7e;
    } else {
        register UWORD crc = 0xffff;
        // maximum size of frame: 2 (header) + 2 (len) + NumBOFs + 4 (AC) + 2*datalen + 4 (FCS) + 1 (EOF)
        if(datalen<<1 > DEFBUFFERSIZE - 13 - ioreq->ioir_NumBOFs)
        {
            ioreq->ioir_Req.io_Error = IRIOERR_OUTOFMEMORY;
            return(FALSE);
        }
        *buf++ = 0x55; // header
        *buf++ = 0xaa;
        buf += 2; // length of frame needs to be filled out later
        // preamble
        if((framelen = ioreq->ioir_NumBOFs))
        {
            do
            {
                *buf++ = 0xc0;
            } while(--framelen);
        }
        STUFFBYTESIRFCS(ioreq->ioir_Address); // address byte
        STUFFBYTESIRFCS(ioreq->ioir_Control); // control byte
        {
            register UBYTE data;
            if(datalen)
            {
                do
                {
                    data = *dataptr++;
                    STUFFBYTESIRFCS(data);
                } while(--datalen);
            }
            crc ^= 0xffff;
            data = crc;
            STUFFBYTESIR(data);
            crc >>= 8;
            data = crc;
            STUFFBYTESIR(data);
        }
        *buf++ = 0xc1;
    }
    framelen = ((IPTR) buf) - ((IPTR) &ncp->ncp_WriteBuffer[4]);
    ncp->ncp_WriteBuffer[2] = framelen;
    ncp->ncp_WriteBuffer[3] = framelen>>8;
    KPRINTF(10, ("Sending Frame (%ld bytes)\n", framelen));
    DB(dumpmem(ncp->ncp_WriteBuffer, framelen+4));
    psdSendPipe(ncp->ncp_EPOutPipe, ncp->ncp_WriteBuffer, framelen+4);
    ncp->ncp_WritePending = ioreq;
    return(TRUE);
}
/* \\\ */

/* /// "nReceiveFrame()" */
ULONG nReceiveFrame(struct NepClassSTIr4200 *ncp, UBYTE *buf, ULONG len)
{
    struct IOIrDAReq *ioreq;
    UBYTE *dataptr;
    ULONG datalen;
    ULONG cnt;
    UBYTE data;

    ioreq = (struct IOIrDAReq *) ncp->ncp_ReadQueue.lh_Head;
    if(!ioreq->ioir_Req.io_Message.mn_Node.ln_Succ)
    {
        return(len);
    }
    //KPRINTF(10, ("Receive frame:\n"));
    //DB(dumpmem(buf, len));
    if(ncp->ncp_FIRMode)
    {
        KPRINTF(10, ("FIR Mode not yet implemented\n"));
    } else {
        register UWORD crc = 0;
        if(ncp->ncp_UnescapeFirst)
        {
            *buf ^= 0x20;
            ncp->ncp_UnescapeFirst = FALSE;
        }
        datalen = ioreq->ioir_Length + 2;

        do
        {
            len--;
            if(ncp->ncp_FrameStarted)
            {
                switch(data = *buf++)
                {
                    case 0xc0:
                        if(ioreq->ioir_Actual)
                        {
                            KPRINTF(1, ("Unexpected BOF\n"));
                            ioreq->ioir_NumBOFs = 0;
                        } else {
                            KPRINTF(1, ("More BOFs...\n"));
                            ioreq->ioir_NumBOFs++;
                        }
                        ioreq->ioir_Actual = 0;
                        crc = 0;
                        break;

                    case 0xc1:
                        KPRINTF(1, ("EOF\n"));
                        ncp->ncp_FrameStarted = FALSE;
                        // frame completed
                        if(ioreq->ioir_Actual > 3) // address+control field+FCS
                        {
                            ioreq->ioir_Actual -= 4;
                            crc = 0xff ^ FCS16Table[(0xff ^ ioreq->ioir_Address) & 0xff];
                            crc = ((crc>>8) & 0xff) ^ FCS16Table[(crc ^ ioreq->ioir_Control) & 0xff];
                            cnt = ioreq->ioir_Actual + 2;
                            dataptr = ioreq->ioir_Data;
                            do
                            {
                                crc = ((crc>>8) & 0xff) ^ FCS16Table[(crc ^ *dataptr++) & 0xff];
                            } while(--cnt);
                            KPRINTF(10, ("CRC=%04lx for %ld bytes (%ld remaining)\n", crc, ioreq->ioir_Actual, len));
                            if(crc != 0xf0b8)
                            {
                                KPRINTF(10, ("CRC Error\n"));
                                break;
                            }
                        } else {
                            KPRINTF(10, ("Frame too short\n"));
                            break;
                        }
                        // reply frame
                        Remove((struct Node *) ioreq);
                        ReplyMsg((struct Message *) ioreq);
                        // is there another request pending?
                        ioreq = (struct IOIrDAReq *) ncp->ncp_ReadQueue.lh_Head;
                        if(ioreq->ioir_Req.io_Message.mn_Node.ln_Succ)
                        {
                            // don't try to switch baudrates, if a write request is pending with different baudrate
                            if(ncp->ncp_BaudRate != ioreq->ioir_Baud)
                            {
                                KPRINTF(10, ("Baudrate change requested\n"));
                                return(0); // obviously, remaining frames (if any) are discarded
                            }
                            datalen = ioreq->ioir_Length + 2;
                        } else {
                            return(len);
                        }
                        break;

                    case 0x7d:
                        KPRINTF(1, ("Unescaping\n"));
                        if(len--)
                        {
                            data = *buf++ ^ 0x20;
                            if(!((data == 0xc0) || (data == 0xc1) || (data == 0x7d)))
                            {
                                // illegal escaped sequence
                                KPRINTF(10, ("Illegal escaped code %02lx\n", data));
                                ncp->ncp_FrameStarted = FALSE;
                                break;
                            }
                        } else {
                            KPRINTF(10, ("End of buffer, unescape first set\n"));
                            ncp->ncp_UnescapeFirst = TRUE;
                            return(0);
                        }
                        // fall through

                    default:
                        if(ioreq->ioir_Actual > 1)
                        {
                            if(ioreq->ioir_Actual < datalen)
                            {
                                ((UBYTE *) ioreq->ioir_Data)[ioreq->ioir_Actual-2] = data;
                            } else {
                                KPRINTF(10, ("Buffer overflow!\n"));
                                ncp->ncp_FrameStarted = FALSE;
                                ioreq->ioir_Req.io_Error = IRIOERR_OVERFLOW;
                                ioreq->ioir_Actual -= 2;
                                Remove((struct Node *) ioreq);
                                ReplyMsg((struct Message *) ioreq);
                                return(len);
                            }
                        } else {
                            if(ioreq->ioir_Actual)
                            {
                                KPRINTF(1, ("Control: %02lx\n", data));
                                ioreq->ioir_Control = data;
                            } else {
                                KPRINTF(1, ("Address: %02lx\n", data));
                                ioreq->ioir_Address = data;
                            }
                        }
                        ioreq->ioir_Actual++;
                        break;
                }
            } else {
                if(*buf++ == 0xc0)
                {
                    KPRINTF(1, ("BOF!\n"));
                    ncp->ncp_FrameStarted = TRUE;
                    ioreq->ioir_Actual = 0;
                    ioreq->ioir_NumBOFs = 1;
                    crc = 0;
                }
            }
        } while(len);
        KPRINTF(1, ("Last byte: %02lx\n", buf[-1]));
    }
    return(len);
}
/* \\\ */

/* /// "nSTIr4200Task()" */
AROS_UFH0(void, nSTIr4200Task)
{
    AROS_USERFUNC_INIT

    struct NepClassSTIr4200 *ncp;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    LONG ioerr;
    ULONG len;
    struct IOIrDAReq *ioreq;
    struct IOIrDAReq *ioreq2;
    ULONG mediabusycnt = 0;

    if((ncp = nAllocSTIr4200()))
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
            while((pp = (struct PsdPipe *) GetMsg(ncp->ncp_TaskMsgPort)))
            {
                KPRINTF(1, ("Pipe back %08lx\n", pp));
                if(pp == ncp->ncp_EPInPipe)
                {
                    ULONG len;
                    ioerr = psdGetPipeError(pp);
                    len = psdGetPipeActual(pp);
                    KPRINTF(1, ("Len = %ld\n", len));
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "STIr4200 receive failed: %s (%ld)",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    }
                    if(!len)
                    {
                        psdDelayMS(50);
                        mediabusycnt++;
                    } else {
                        mediabusycnt = 0;
                        DB(dumpmem(ncp->ncp_ReadBuffer, len));
                        ncp->ncp_RBufRemain = nReceiveFrame(ncp, ncp->ncp_ReadBuffer, len);
                        if(ncp->ncp_RBufRemain)
                        {
                            // data not fully read -- wait for next request to arrive
                            ncp->ncp_RBufOffset = len - ncp->ncp_RBufRemain;
                            KPRINTF(1, ("%ld Bytes at %ld remaining...\n", ncp->ncp_RBufRemain, ncp->ncp_RBufOffset));
                        }
                    }
                    ncp->ncp_ReadPending = NULL;
                }
                if(pp == ncp->ncp_EPOutPipe)
                {
                    if((ioreq = ncp->ncp_WritePending))
                    {
                        ioerr = psdGetPipeError(pp);
                        if(ioerr)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "STIr4200 send failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            ioreq->ioir_Req.io_Error = IRIOERR_HOSTERROR;
                            ioreq->ioir_Actual = 0;
                        } else {
                            ioreq->ioir_Actual = ioreq->ioir_Length;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        ncp->ncp_WritePending = NULL;
                    }
                }
            }

            while((ioreq = (struct IOIrDAReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
            {
                KPRINTF(5, ("command ioreq: 0x%08lx cmd: %lu len: %ld\n",
                        ioreq, ioreq->ioir_Req.io_Command, ioreq->ioir_Length));
                switch(ioreq->ioir_Req.io_Command)
                {
                    case CMD_READ:
                        ioreq->ioir_Actual = 0;
                        Forbid();
                        AddTail(&ncp->ncp_ReadQueue, &ioreq->ioir_Req.io_Message.mn_Node);
                        Permit();
                        break;

                    case CMD_WRITE:
                        ioreq->ioir_Actual = 0;
                        Forbid();
                        AddTail(&ncp->ncp_WriteQueue, &ioreq->ioir_Req.io_Message.mn_Node);
                        Permit();
                        break;

                    case CMD_CLEAR:
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                        /* Reset does a flush too */
                    case CMD_FLUSH:
                        ioreq2 = (struct IOIrDAReq *) ncp->ncp_ReadQueue.lh_Head;
                        while(ioreq2->ioir_Req.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->ioir_Req.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOIrDAReq *) ncp->ncp_ReadQueue.lh_Head;
                        }
                        ioreq2 = (struct IOIrDAReq *) ncp->ncp_WriteQueue.lh_Head;
                        while(ioreq2->ioir_Req.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->ioir_Req.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOIrDAReq *) ncp->ncp_WriteQueue.lh_Head;
                        }
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->ioir_Req.io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            ioreq = (struct IOIrDAReq *) ncp->ncp_ReadQueue.lh_Head;
            while((!ncp->ncp_ReadPending) && ioreq->ioir_Req.io_Message.mn_Node.ln_Succ)
            {
                // don't try to switch baudrates, if a write request is pending with different baudrate
                if(ncp->ncp_WritePending)
                {
                    if(ncp->ncp_WritePending->ioir_Baud != ioreq->ioir_Baud)
                    {
                        break;
                    }
                }
                if(ioreq->ioir_Baud != ncp->ncp_BaudRate)
                {
                    nSetBaudrate(ncp, ioreq->ioir_Baud);
                }
                if(ncp->ncp_RBufRemain)
                {
                    len = ncp->ncp_RBufRemain;
                    ncp->ncp_RBufRemain = nReceiveFrame(ncp, &ncp->ncp_ReadBuffer[ncp->ncp_RBufOffset], ncp->ncp_RBufRemain);
                    if(ncp->ncp_RBufRemain)
                    {
                        // data not fully read -- wait for next request to arrive
                        ncp->ncp_RBufOffset += len - ncp->ncp_RBufRemain;
                        break;
                    }
                }
                KPRINTF(1, ("Sending out read pipe...\n"));
                psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_ReadBuffer, DEFBUFFERSIZE);
                ncp->ncp_ReadPending = ioreq;
                break;
            }
            //if(!ncp->ncp_ReadPending)
            {
                mediabusycnt = 10; // avoid infinite waits
            }
            ioreq = (struct IOIrDAReq *) ncp->ncp_WriteQueue.lh_Head;
            while((mediabusycnt >= 10) && (!ncp->ncp_WritePending) && ioreq->ioir_Req.io_Message.mn_Node.ln_Succ)
            {
                // don't try to switch baudrates, if a read request is pending with different baudrate
                if(ncp->ncp_ReadPending)
                {
                    if(ncp->ncp_ReadPending->ioir_Baud != ioreq->ioir_Baud)
                    {
                        break;
                    }
                }
                Remove((struct Node *) ioreq);
                if(ioreq->ioir_Baud != ncp->ncp_BaudRate)
                {
                    nSetBaudrate(ncp, ioreq->ioir_Baud);
                }
                if(!nSendFrame(ncp, ioreq))
                {
                    // an error occurred!
                    ReplyMsg((struct Message *) ioreq);
                    ioreq = (struct IOIrDAReq *) ncp->ncp_WriteQueue.lh_Head;
                } else {
                    break;
                }
            }
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));
        Forbid();
        /* Now remove all requests still pending *anywhere* */
        ncp->ncp_DenyRequests = TRUE;
        /* Current transfers */
        if(ncp->ncp_ReadPending)
        {
            KPRINTF(1, ("Aborting pending read...\n"));
            psdAbortPipe(ncp->ncp_EPInPipe);
            psdWaitPipe(ncp->ncp_EPInPipe);
            ncp->ncp_ReadPending = NULL;
        }
        if((ioreq = ncp->ncp_WritePending))
        {
            KPRINTF(1, ("Aborting pending write...\n"));
            psdAbortPipe(ncp->ncp_EPOutPipe);
            psdWaitPipe(ncp->ncp_EPOutPipe);
            ncp->ncp_WritePending = NULL;
        }
        /* Read/Write queues */
        ioreq = (struct IOIrDAReq *) ncp->ncp_WriteQueue.lh_Head;
        while(ioreq->ioir_Req.io_Message.mn_Node.ln_Succ)
        {
            KPRINTF(1, ("Removing write request...\n"));
            Remove((struct Node *) ioreq);
            ioreq->ioir_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOIrDAReq *) ncp->ncp_WriteQueue.lh_Head;
        }
        ioreq = (struct IOIrDAReq *) ncp->ncp_ReadQueue.lh_Head;
        while(ioreq->ioir_Req.io_Message.mn_Node.ln_Succ)
        {
            KPRINTF(1, ("Removing read request...\n"));
            Remove((struct Node *) ioreq);
            ioreq->ioir_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
            ioreq = (struct IOIrDAReq *) ncp->ncp_ReadQueue.lh_Head;
        }
        /* Command queue */
        while((ioreq = (struct IOIrDAReq *) GetMsg(&ncp->ncp_Unit.unit_MsgPort)))
        {
            KPRINTF(1, ("Aborting pending requests...\n"));
            ioreq->ioir_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *) ioreq);
        }
        Permit();
        KPRINTF(20, ("Going down the river!\n"));
        nFreeSTIr4200(ncp);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocSTIr4200()" */
struct NepClassSTIr4200 * nAllocSTIr4200(void)
{
    struct Task *thistask;
    struct NepClassSTIr4200 *ncp;

    thistask = FindTask(NULL);
    do
    {
        ncp = thistask->tc_UserData;
        if(!(ncp->ncp_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        ncp->ncp_Interface = psdFindInterface(ncp->ncp_Device, NULL, TAG_END);
        if(!ncp->ncp_Interface)
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Interface missing!");
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
        if(!(ncp->ncp_ReadBuffer = AllocVec(DEFBUFFERSIZE, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            KPRINTF(1, ("Out of memory for read buffer\n"));
            break;
        }
        if(!(ncp->ncp_WriteBuffer = AllocVec(DEFBUFFERSIZE, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            KPRINTF(1, ("Out of memory for read buffer\n"));
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
                if((ncp->ncp_EPOutPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPOut)))
                {
                    /* Turn off short packets */
                    psdSetAttrs(PGA_PIPE, ncp->ncp_EPOutPipe,
                                PPA_NoShortPackets, TRUE,
                                PPA_NakTimeout, TRUE,
                                PPA_NakTimeoutTime, 1000,
                                TAG_END);
                    if((ncp->ncp_EPInPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPIn)))
                    {
                        /* Turn off short packets */
                        psdSetAttrs(PGA_PIPE, ncp->ncp_EPInPipe,
                                    PPA_NakTimeout, FALSE,
                                    PPA_AllowRuntPackets, TRUE,
                                    TAG_END);

                        nSetBaudrate(ncp, 9600);

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
    if(ncp->ncp_ReadBuffer)
    {
        FreeVec(ncp->ncp_ReadBuffer);
        ncp->ncp_ReadBuffer = NULL;
    }
    if(ncp->ncp_WriteBuffer)
    {
        FreeVec(ncp->ncp_WriteBuffer);
        ncp->ncp_WriteBuffer = NULL;
    }
    Forbid();
    ncp->ncp_Task = NULL;
    if(ncp->ncp_ReadySigTask)
    {
        Signal(ncp->ncp_ReadySigTask, 1L<<ncp->ncp_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeSTIr4200()" */
void nFreeSTIr4200(struct NepClassSTIr4200 *ncp)
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
    if(ncp->ncp_WriteBuffer)
    {
        FreeVec(ncp->ncp_WriteBuffer);
        ncp->ncp_WriteBuffer = NULL;
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

/* /// "nSetReg()" */
BOOL nSetReg(struct NepClassSTIr4200 *ncp, ULONG reg, ULONG value)
{
    LONG ioerr;
    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_VENDOR|URTF_DEVICE,
                 USTR_WRITE_REG, value, reg);
    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        KPRINTF(10, ("Writing reg %1lx with %02lx failed: %s (%ld)",
                       reg, value,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Writing reg %1lx with %02lx failed: %s (%ld)",
                       reg, value,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(FALSE);
    }
    return(TRUE);
}
/* \\\ */

/**************************************************************************/

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepSTIr4200Base *nh;
    struct NepClassSTIr4200 *ncp;
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
        MUIA_Application_Description, "Settings for the stir4200.class",
        MUIA_Application_Base       , "STIR4200",
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
                Child, HGroup, GroupFrameT(ncp->ncp_Device ? "Device Settings" : "Default Device Settings"),
                    Child, HSpace(0),
                    Child, ColGroup(2),
                        Child, Label((ULONG) "Autostart IrDA Stack:"),
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
                        MUIA_ShowMe, ncp->ncp_Device,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Save ",
                        End,
                    Child, ncp->ncp_SetDefaultObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, ncp->ncp_Device ? "\33c Save as Default " : "\33c Save Defaults ",
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
                    if(ncp->ncp_Device)
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
void nGUITaskCleanup(struct NepClassSTIr4200 *ncp)
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
