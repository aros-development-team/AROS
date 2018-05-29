/*
 *----------------------------------------------------------------------------
 *                         hid class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "numtostr.h"
#include "hid.class.h"

#include <proto/datatypes.h>

static LONG _rand(struct NepHidBase *nh)
{
    nh->nh_Seed = nh->nh_Seed * 1103515245 + 12345;

    return nh->nh_Seed;
}

/* /// "Lib Stuff" */
const STRPTR GM_UNIQUENAME(libname) = MOD_NAME_STRING;

//#define LowLevelBase nh->nh_LowLevelBase

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh)
{
    struct NepClassHid *nch;
    struct NepHidBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define UtilityBase     nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Interfaces);
        nch = &nh->nh_DummyNCH;
        nch->nch_ClsBase = nh;
        nch->nch_Interface = NULL;

        nch->nch_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
        if(nch->nch_CDC)
        {
            ret = nh;
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
        Forbid();
        nh->nh_ReadySignal = SIGB_SINGLE;
        nh->nh_ReadySigTask = FindTask(NULL);
        if(nh->nh_DispatcherTask)
        {
            Signal(nh->nh_DispatcherTask, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nh->nh_DispatcherTask)
        {
            Wait(1L<<nh->nh_ReadySignal);
        }
        //FreeSignal(nh->nh_ReadySignal);
    }
    KPRINTF(5, ("libClose: lib_OpenCnt = %ld\n", nh->nh_Library.lib_OpenCnt));
    return(TRUE);
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_LowLevelBase)
    {
        APTR ourvec;
        Disable();
        ourvec = SetFunction(nh->nh_LowLevelBase, -5 * LIB_VECTSIZE, nh->nh_LLOldReadJoyPort);
        if(ourvec != AROS_SLIB_ENTRY(nReadJoyPort, hid, 5))
        {
            SetFunction(nh->nh_LowLevelBase, -5 * LIB_VECTSIZE, ourvec);
            Enable();
            return(FALSE); /* we couldn't remove the patch! */
        }
        ourvec = SetFunction(nh->nh_LowLevelBase, -22 * LIB_VECTSIZE, nh->nh_LLOldSetJoyPortAttrsA);
        if(ourvec != AROS_SLIB_ENTRY(nSetJoyPortAttrsA, hid, 22))
        {
            SetFunction(nh->nh_LowLevelBase, -22 * LIB_VECTSIZE, ourvec);
            Enable();
            return(FALSE); /* we couldn't remove the patch! */
        }
        Enable();
        CloseLibrary(nh->nh_LowLevelBase);
        nh->nh_LowLevelBase = NULL;
    }

    CloseLibrary((struct Library *) UtilityBase);

    FreeVec(nh->nh_DummyNCH.nch_CDC);

    KPRINTF(5, ("libExpunge: Unloading done! hid.class expunged!\n\n"));

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
struct NepClassHid * GM_UNIQUENAME(usbAttemptInterfaceBinding)(struct NepHidBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    //IPTR subclass;
    //IPTR proto;

    KPRINTF(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    //IFA_SubClass, &subclass,
                    //IFA_Protocol, &proto,
                    TAG_DONE);
        CloseLibrary(ps);

        if(ifclass == HID_CLASSCODE)
        {
            return(GM_UNIQUENAME(usbForceInterfaceBinding)(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassHid * GM_UNIQUENAME(usbForceInterfaceBinding)(struct NepHidBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassHid *nch;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;
    STRPTR ifidstr;
    STRPTR devidstr;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepHidForceInterfaceBinding(%08lx)\n", pif));

    nInstallLLPatch(nh);
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Config, &pc,
                    IFA_IDString, &ifidstr,
                    TAG_DONE);
        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    TAG_END);
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductName, &devname,
                    DA_IDString, &devidstr,
                    TAG_END);
        if((nch = psdAllocVec(sizeof(struct NepClassHid))))
        {
            nch->nch_ClsBase = nh;
            nch->nch_CDC = psdAllocVec(sizeof(struct ClsDevCfg));
            if(!nch->nch_CDC)
            {
                psdFreeVec(nch);
                CloseLibrary(ps);
                return(NULL);
            }

            nch->nch_Device = pd;
            nch->nch_Interface = pif;
            nch->nch_DevIDString = devidstr;
            nch->nch_IfIDString = ifidstr;

            GM_UNIQUENAME(nLoadBindingConfig)(nch, FALSE);

            psdSafeRawDoFmt(buf, 64, "hid.class<%08lx>", nch);
            nch->nch_ReadySignal = SIGB_SINGLE;
            nch->nch_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);

            if((tmptask = psdSpawnSubTask(buf, GM_UNIQUENAME(nHidTask), nch)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);
                if(nch->nch_Task)
                {
                    nch->nch_ReadySigTask = NULL;
                    //FreeSignal(nch->nch_ReadySignal);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "HID the road, '%s'!",
                                   devname);
                    Forbid();
                    AddTail(&nh->nh_Interfaces, &nch->nch_Node);
                    Permit();
                    CloseLibrary(ps);
                    return(nch);
                }
            }
            nch->nch_ReadySigTask = NULL;
            //FreeSignal(nch->nch_ReadySignal);
            psdFreeVec(nch->nch_CDC);
            psdFreeVec(nch);
        }
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void GM_UNIQUENAME(usbReleaseInterfaceBinding)(struct NepHidBase *nh, struct NepClassHid *nch)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    KPRINTF(1, ("nepHidReleaseInterfaceBinding(%08lx)\n", nch));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        nch->nch_ReadySignal = SIGB_SINGLE;
        nch->nch_ReadySigTask = FindTask(NULL);
        if(nch->nch_GUITask)
        {
            Signal(nch->nch_GUITask, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nch->nch_GUITask)
        {
            Wait(1L<<nch->nch_ReadySignal);
        }

        Forbid();
        if(nch->nch_HCGUITask)
        {
            Signal(nch->nch_HCGUITask, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nch->nch_HCGUITask)
        {
            Wait(1L<<nch->nch_ReadySignal);
        }

        Forbid();
        if(nch->nch_Task)
        {
            Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nch->nch_Task)
        {
            Wait(1L<<nch->nch_ReadySignal);
        }
        //FreeSignal(nch->nch_ReadySignal);
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "And don't you come back no more, '%s'!",
                       devname);
        Forbid();
        Remove(&nch->nch_Node);
        Permit();
        psdFreeVec(nch->nch_CDC);
        psdFreeVec(nch);
        CloseLibrary(ps);
    }
}
/* \\\ */

/* /// "usbGetAttrsA()" */
AROS_LH3(LONG, usbGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, hid)
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
                 *((STRPTR *) ti->ti_Data) = "Generic support for human interface devices";
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
                 *((IPTR *) ti->ti_Data) = nh->nh_DummyNCH.nch_UsingDefaultCfg;
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
                 *((IPTR *) ti->ti_Data) = ((struct NepClassHid *) usbstruct)->nch_UsingDefaultCfg;
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
         LIBBASETYPEPTR, nh, 6, hid)
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
         LIBBASETYPEPTR, nh, 7, hid)
{
    AROS_LIBFUNC_INIT

    struct NepClassHid *nch;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) GM_UNIQUENAME(usbAttemptInterfaceBinding)(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) GM_UNIQUENAME(usbForceInterfaceBinding)(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            GM_UNIQUENAME(usbReleaseInterfaceBinding)(nh, (struct NepClassHid *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(GM_UNIQUENAME(nOpenBindingCfgWindow)(nh, &nh->nh_DummyNCH));

        case UCM_OpenBindingCfgWindow:
            return(GM_UNIQUENAME(nOpenBindingCfgWindow)(nh, (struct NepClassHid *) methoddata[0]));

        case UCM_DOSAvailableEvent:
            nInstallLLPatch(nh);
            return(TRUE);

        case UCM_ConfigChangedEvent:
            GM_UNIQUENAME(nLoadClassConfig)(nh);

            Forbid();
            nch = (struct NepClassHid *) nh->nh_Interfaces.lh_Head;
            while(nch->nch_Node.ln_Succ)
            {
                GM_UNIQUENAME(nLoadBindingConfig)(nch, TRUE);
                nch = (struct NepClassHid *) nch->nch_Node.ln_Succ;
            }
            Permit();
            return(TRUE);

        case UCM_AttemptSuspendDevice:
            nch = (struct NepClassHid *) methoddata[0];
            nch->nch_Running = FALSE;
            return(TRUE);

        case UCM_AttemptResumeDevice:
            nch = (struct NepClassHid *) methoddata[0];
            nch->nch_Running = TRUE;
            Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "nInstallLLPatch()" */
void nInstallLLPatch(struct NepHidBase *nh)
{
    if(!nh->nh_LowLevelBase)
    {
        if((nh->nh_LowLevelBase = OpenLibrary("lowlevel.library", 40)))
        {
            Disable();
            nh->nh_LLOldReadJoyPort = SetFunction(nh->nh_LowLevelBase, -5 * LIB_VECTSIZE, AROS_SLIB_ENTRY(nReadJoyPort, hid, 5));
            nh->nh_LLOldSetJoyPortAttrsA = SetFunction(nh->nh_LowLevelBase, -22 * LIB_VECTSIZE, AROS_SLIB_ENTRY(nSetJoyPortAttrsA, hid, 22));
            Enable();
        }
    }
}
/* \\\ */

/* /// "nLoadClassConfig()" */
BOOL GM_UNIQUENAME(nLoadClassConfig)(struct NepHidBase *nh)
{
    struct NepClassHid *nch = &nh->nh_DummyNCH;
    struct Library *ps;
    struct KeymapCfg *kmc;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(nch->nch_GUITask)
    {
        return(FALSE);
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Create default config */
    kmc = &nch->nch_KeymapCfg;
    kmc->kmc_ChunkID = AROS_LONG2BE(MAKE_ID('K','M','A','P'));
    kmc->kmc_Length = AROS_LONG2BE(sizeof(struct KeymapCfg)-8);
    CopyMemQuick(usbkeymap, kmc->kmc_Keymap, AROS_LONG2BE(kmc->kmc_Length));

    cdc = nch->nch_CDC;
    cdc->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('H','I','D','C'));
    cdc->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    cdc->cdc_EnableKBReset = TRUE;
    cdc->cdc_EnableRH = TRUE;
    cdc->cdc_ResetDelay = 10;
    cdc->cdc_ShellStack = AROS_STACKSIZE;
    strcpy(cdc->cdc_ShellCon, "CON:///130/HID Rocket Bay/CLOSE/AUTO/WAIT");
    cdc->cdc_PollingMode = FALSE;
    cdc->cdc_LLPortMode[1] = 2;
    cdc->cdc_HIDCtrlOpen = FALSE;
    strcpy(cdc->cdc_HIDCtrlRexx, "HIDCTRL");
    strcpy(cdc->cdc_HIDCtrlTitle, "HID Control");
    cdc->cdc_LLRumblePort = 2;
    cdc->cdc_TurboMouse = 0;

    nch->nch_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = psdGetClsCfg(GM_UNIQUENAME(libname));
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(nch->nch_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nch->nch_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nch->nch_CDC->cdc_Length)));
            psdFreeVec(cdc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
        kmc = psdGetCfgChunk(pic, AROS_LONG2BE(nch->nch_KeymapCfg.kmc_ChunkID));
        if(kmc)
        {
            CopyMem(((UBYTE *) kmc) + 8, ((UBYTE *) &nch->nch_KeymapCfg) + 8, min(AROS_LONG2BE(kmc->kmc_Length), AROS_LONG2BE(nch->nch_KeymapCfg.kmc_Length)));
            psdFreeVec(kmc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nCalcConfigCRC()" */
ULONG nCalcConfigCRC(struct NepClassHid *nch, struct Library *ps, struct PsdIFFContext *pic)
{
    ULONG *buf = (ULONG *) psdWriteCfg(pic);
    ULONG crc = 0;
    if(buf)
    {
        UWORD *ptr = (UWORD *) buf;
        ULONG len = (AROS_LONG2BE(buf[1]) + 8)>>1;
        crc = len;
        if(len)
        {
            do
            {
                crc = ((crc<<1)|(crc>>31))^(*ptr++);
            } while(--len);
        }
        psdFreeVec(buf);
    }
    return(crc);
}
/* \\\ */

/* /// "nLoadBindingConfig()" */
BOOL GM_UNIQUENAME(nLoadBindingConfig)(struct NepClassHid *nch, BOOL gui)
{
    struct NepHidBase *nh = nch->nch_ClsBase;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct KeymapCfg *kmc;
    struct PsdIFFContext *pic;
    ULONG crc = 0;

    KPRINTF(10, ("Loading Binding Config...\n"));
    if(nch->nch_GUITask)
    {
        return(FALSE);
    }
    //GM_UNIQUENAME(nLoadClassConfig)(nh);
    *nch->nch_CDC = *nh->nh_DummyNCH.nch_CDC;
    nch->nch_KeymapCfg = nh->nh_DummyNCH.nch_KeymapCfg;
    nch->nch_UsingDefaultCfg = TRUE;

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), nch->nch_DevIDString, nch->nch_IfIDString);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(nch->nch_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nch->nch_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nch->nch_CDC->cdc_Length)));
            psdFreeVec(cdc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
        kmc = psdGetCfgChunk(pic, AROS_LONG2BE(nch->nch_KeymapCfg.kmc_ChunkID));
        if(kmc)
        {
            CopyMem(((UBYTE *) kmc) + 8, ((UBYTE *) &nch->nch_KeymapCfg) + 8, min(AROS_LONG2BE(kmc->kmc_Length), AROS_LONG2BE(nch->nch_KeymapCfg.kmc_Length)));
            psdFreeVec(kmc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
        crc = nCalcConfigCRC(nch, ps, pic);
    }
    if(crc != nch->nch_LastCfgCRC)
    {
        nch->nch_ReloadCfg = TRUE;
        nch->nch_LastCfgCRC = crc;
        if(nch->nch_Task)
        {
            Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
        }
    }
    if(gui && !nch->nch_HCGUITask)
    {
        psdSpawnSubTask(MOD_NAME_STRING " Control GUI", GM_UNIQUENAME(nHIDCtrlGUITask), nch);
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nOpenBindingCfgWindow()" */
LONG GM_UNIQUENAME(nOpenBindingCfgWindow)(struct NepHidBase *nh, struct NepClassHid *nch)
{
    struct Library *ps;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!nch->nch_GUITask)
    {
        if((nch->nch_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", GM_UNIQUENAME(nGUITask), nch)))
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

/* /// "Keymap Table" */
UBYTE usbkeymap[256] =
{
    0xff, 0xff, 0xff, 0xff, 0x20, 0x35, 0x33, 0x22,  /* 0x00 */
    0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28,  /* 0x08 */
    0x37, 0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14,  /* 0x10 */
    0x16, 0x34, 0x11, 0x32, 0x15, 0x31, 0x01, 0x02,  /* 0x18 */
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,  /* 0x20 */
    0x44, 0x45, 0x41, 0x42, 0x40, 0x0b, 0x0c, 0x1a,  /* 0x28 */
    0x1b, 0x0d, 0x2b, 0x29, 0x2a, 0x00, 0x38, 0x39,  /* 0x30 */
    0x3a, 0x62, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,  /* 0x38 */
    0x56, 0x57, 0x58, 0x59, 0x5f, 0x67, 0xff, 0xff,  /* 0x40 */
    0x6e, 0x47, 0x70, 0x48, 0x46, 0x71, 0x49, 0x4e,  /* 0x48 */
    0x4f, 0x4d, 0x4c, 0xff, 0x5c, 0x5d, 0x4a, 0x5e,  /* 0x50 */
    0x43, 0x1d, 0x1e, 0x1f, 0x2d, 0x2e, 0x2f, 0x3d,  /* 0x58 */
    0x3e, 0x3f, 0x0f, 0x3c, 0x30, 0x67, 0xff, 0xff,  /* 0x60 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x68 F13-F24 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0x5f, 0xff, 0xff,  /* 0x70 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x78 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x80 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x88 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x90 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x44, 0xff,  /* 0x98 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xa0 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xa8 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xb0 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xb8 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xc0 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xc8 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xd0 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xd8 */
    0x63, 0x60, 0x64, 0x66, 0x63, 0x61, 0x65, 0x67,  /* 0xe0 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xe8 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0xf0 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff   /* 0xf8 */
};

/* \\\ */

#undef  ps
#define ps nch->nch_Base

/* /// "nHidTask()" */
AROS_UFH0(void, GM_UNIQUENAME(nHidTask))
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    UBYTE *buf;
    UBYTE *bufreal;
    ULONG buflen;
    LONG ioerr;
    BOOL wacomgood;
    LONG lastioerr = 0;
    ULONG errcount = 0;

    UWORD reportid;
    UWORD count;
    struct NepHidReport *nhr;
    struct NepHidItem **nhiptr;
    struct NepHidItem *nhi;

    if((nch = GM_UNIQUENAME(nAllocHid())))
    {
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;

        pp = nch->nch_EP0Pipe;
        /* initial output states */
#if 0
        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            if(nhr->nhr_OutItemCount)
            {
                bufreal = nch->nch_EPOutBuf;
                if(nch->nch_UsesReportID)
                {
                    bufreal++;
                    psdPipeSetup(pp, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                                 UHR_GET_REPORT, (ULONG) nhr->nhr_ReportID|0x0200, nch->nch_IfNum);
                } else {
                    psdPipeSetup(pp, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                                 UHR_GET_REPORT, 0x0200, nch->nch_IfNum);
                }
                if((ioerr = psdDoPipe(pp, nch->nch_EPOutBuf, (nhr->nhr_ReportOutSize+7)>>3)))
                {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "GET_REPORT(%lx, %ld) failed: %s (%ld)!",
                                   nhr->nhr_ReportID|0x0200, (nhr->nhr_ReportOutSize+7)>>3,
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                } else {
                    count = nhr->nhr_OutItemCount;
                    nhiptr = nhr->nhr_OutItemMap;
                    if(nhiptr)
                    {
                        do
                        {
                            nhi = *nhiptr++;
                            nProcessItem(nch, nhi, bufreal);
                        } while(--count);
                    } else {
                        KPRINTF(10, ("Huh? report %ld, nhiptr == NULL!\n", nhr->nhr_ReportID));
                    }
                }
            }
            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
        }
#endif

        if(nch->nch_XtraInitItem)
        {
            struct NepHidAction *nha = (struct NepHidAction *) nch->nch_XtraInitItem->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                KPRINTF(1, ("Executing InitItem %08lx\n", nha));
                nDoAction(nch, nha, nch->nch_XtraInitItem, 0, 1, TRUE);
                nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
            }
        }
        nFlushEvents(nch);

        Forbid();
        if(!nch->nch_HCGUITask)
        {
            psdSpawnSubTask(MOD_NAME_STRING " Control GUI", GM_UNIQUENAME(nHIDCtrlGUITask), nch);
        }
        Permit();

        buf = nch->nch_EPInBuf;
        //nch->nch_CDC->cdc_PollingMode = TRUE;
        if(!nch->nch_HasInItems)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "HID Device has no input items, input polling disabled.");
        }
        nch->nch_ReloadCfg = FALSE;
        nch->nch_Running = TRUE;
        nch->nch_IOStarted = FALSE;
        do
        {
            if(nch->nch_CDC->cdc_PollingMode)
            {
                sigs = SetSignal(0, 0);
#if 0
                UWORD rptcount;
                sigs = SetSignal(0, 0);
                pp = nch->nch_EP0Pipe;
                for(rptcount = 0; rptcount <= nch->nch_MaxReportID; rptcount++)
                {
                    if(nch->nch_ReportMap[rptcount])
                    {
                        if(nch->nch_UsesReportID)
                        {
                            psdPipeSetup(pp, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                                         UHR_GET_REPORT, (ULONG) rptcount|0x0100, nch->nch_IfNum);
                        } else {
                            psdPipeSetup(pp, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                                         UHR_GET_REPORT, 0x0100, nch->nch_IfNum);
                        }
                        ioerr = psdDoPipe(pp, buf, nch->nch_MaxInSize);
                        if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
                        {
                            buflen = psdGetPipeActual(pp);
                            if(nch->nch_UsesReportID)
                            {
                                reportid = buf[0];
                                bufreal = &buf[1];
                            } else {
                                reportid = 0;
                                bufreal = buf;
                            }
                            if(reportid <= nch->nch_MaxReportID)
                            {
                                if((nhr = nch->nch_ReportMap[reportid]))
                                {
                                    KPRINTF(40, ("Report %ld\n", reportid));
                                    count = nhr->nhr_InItemCount;
                                    nhiptr = nhr->nhr_InItemMap;
                                    if(count && nhiptr)
                                    {
                                        do
                                        {
                                            if((nhi = *nhiptr++))
                                            {
                                                nProcessItem(nch, nhi, bufreal);
                                            } else {
                                                KPRINTF(10, ("Huh? Hole in Item Lookup Map!\n"));
                                            }
                                        } while(--count);
                                    } else {
                                        KPRINTF(10, ("Huh? report %ld, count %ld or nhiptr == NULL!\n", reportid, count));
                                    }
                                } else {
                                    KPRINTF(10, ("Illegal report ID %ld received!\n", reportid));
                                }
                            }
                        } else {
                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                           "GET_REPORT(%lx, %ld) failed: %s (%ld)!",
                                           rptcount, nch->nch_MaxInSize,
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                           "Reverting back to interrupt mode instead of polling!");

                            nch->nch_CDC->cdc_PollingMode = FALSE;
                            if(nch->nch_Running)
                            {
                                psdSendPipe(nch->nch_EPInPipe, buf, nch->nch_MaxInSize);
                                nch->nch_IOStarted = TRUE;
                            }
                            break;
                        }
                    }
                }
                psdDelayMS(nch->nch_EPInInterval);
#endif
            } else {
                if(nch->nch_Running && (!nch->nch_IOStarted) && nch->nch_HasInItems)
                {
                    psdSendPipe(nch->nch_EPInPipe, buf, nch->nch_MaxInSize);
                    nch->nch_IOStarted = TRUE;
                }
                sigs = Wait(sigmask);
                while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort)))
                {
                    if(pp == nch->nch_EPInPipe)
                    {
                        nch->nch_IOStarted = FALSE;
                        ioerr = psdGetPipeError(pp);
                        if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
                        {
                            buflen = psdGetPipeActual(pp);
                            wacomgood = FALSE;
                            errcount = 0;
                            if(nch->nch_WacomMode)
                            {
                                wacomgood = nParseWacom(nch, buf, buflen);
                            }
                            if(!wacomgood)
                            {
                                // if the parser failed, use standard methods
                                if(nch->nch_UsesReportID)
                                {
                                    reportid = buf[0];
                                    bufreal = &buf[1];
                                } else {
                                    reportid = 0;
                                    bufreal = buf;
                                }
                                if(reportid <= nch->nch_MaxReportID)
                                {
                                    if((nhr = nch->nch_ReportMap[reportid]))
                                    {
                                        KPRINTF(40, ("Report %ld\n", reportid));
                                        count = nhr->nhr_InItemCount;
                                        nhiptr = nhr->nhr_InItemMap;
                                        if(count && nhiptr)
                                        {
                                            do
                                            {
                                                if((nhi = *nhiptr++))
                                                {
                                                    nProcessItem(nch, nhi, bufreal);
                                                } else {
                                                    KPRINTF(10, ("Huh? Hole in Item Lookup Map!\n"));
                                                }
                                            } while(--count);
                                        } else {
                                            KPRINTF(10, ("Huh? count or nhiptr == NULL!\n"));
                                        }

                                    } else {
                                        KPRINTF(10, ("Illegal report ID %ld received!\n", reportid));
                                    }
                                }
                            }
                            //dumpmem(buf, buflen);
                        } else {
                            KPRINTF(10, ("Int Pipe failed %ld\n", ioerr));
                            if(nch->nch_Running)
                            {
                                if(lastioerr != ioerr)
                                {
                                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "Interrupt pipe failed: %s (%ld)!",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    errcount = 0;
                                } else {
                                    errcount++;
                                    if(errcount > 20)
                                    {
                                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                                       "That's it, that device pissed me off long enough!");
                                        sigs |= SIGBREAKF_CTRL_C;
                                    }
                                }
                                lastioerr = ioerr;
                                {
                                    IPTR epnum;
                                    psdGetAttrs(PGA_ENDPOINT, nch->nch_EPIn,
                                                EA_EndpointNum, &epnum,
                                                TAG_END);
                                    psdPipeSetup(nch->nch_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) epnum|URTF_IN);
                                    ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                }
                                psdDelayMS(50);
                            }
                        }
                    }
                }
            }
            nFlushEvents(nch);
            if(nch->nch_TrackEvents || nch->nch_TrackKeyEvents || (nch->nch_ReportValues && nch->nch_ItemChanged))
            {
                if(nch->nch_GUITask)
                {
                    Signal(nch->nch_GUITask, (ULONG) (1<<nch->nch_TrackingSignal));
                }
            }
            if(!nch->nch_HIntBase) /* attempt to open intuition */
            {
                nch->nch_HIntBase = OpenLibrary("intuition.library", 39);
            }

            /* need to update prefs? */
            if(nch->nch_ReloadCfg)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Reloading configuration...");
                Forbid();
                nch->nch_ReadySignal = SIGB_SINGLE;
                nch->nch_ReadySigTask = FindTask(NULL);
                if(nch->nch_GUITask)
                {
                    Signal(nch->nch_GUITask, SIGBREAKF_CTRL_C);
                }
                Permit();
                while(nch->nch_GUITask)
                {
                    Wait(1L<<nch->nch_ReadySignal);
                }
                Forbid();
                if(nch->nch_HCGUITask)
                {
                    Signal(nch->nch_HCGUITask, SIGBREAKF_CTRL_C);
                }
                Permit();
                while(nch->nch_HCGUITask)
                {
                    Wait(1L<<nch->nch_ReadySignal);
                }
                //FreeSignal(nch->nch_ReadySignal);
                Forbid();
                nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
                while(nhr->nhr_Node.ln_Succ)
                {
                    nFreeReport(nch, nhr);
                    nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
                }
                psdFreeVec(nch->nch_ReportMap);
                nch->nch_ReportMap = NULL;
                psdFreeVec(nch->nch_HidDesc);
                nch->nch_HidDesc = NULL;
                Permit();
                if(nReadReports(nch))
                {
                    nAddExtraReport(nch);
                    nDetectWacom(nch);
                } else {
                    psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Error parsing report descriptors!");
                    sigs |= SIGBREAKF_CTRL_C;
                }
                nch->nch_ReloadCfg = FALSE;
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        if(nch->nch_XtraQuitItem)
        {
            struct NepHidAction *nha = (struct NepHidAction *) nch->nch_XtraQuitItem->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                nDoAction(nch, nha, nch->nch_XtraQuitItem, 0, 1, TRUE);
                nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
            }
        }

        KPRINTF(20, ("Going down the river!\n"));
        if(nch->nch_IOStarted)
        {
            psdAbortPipe(nch->nch_EPInPipe);
            psdWaitPipe(nch->nch_EPInPipe);
        }
        GM_UNIQUENAME(nFreeHid)(nch);
    }

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocHid()" */
struct NepClassHid * GM_UNIQUENAME(nAllocHid)(void)
{
    struct Task *thistask;
    struct NepClassHid *nch;
    LONG ioerr;
    IPTR subclass;
    IPTR protocol;

    thistask = FindTask(NULL);
#undef IntuitionBase
    nch = thistask->tc_UserData;

    NewList(&nch->nch_HidReports);
    do
    {
        if(!(nch->nch_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        if(!(nch->nch_HIntBase = OpenLibrary("intuition.library", 39)))
        {
            /* we don't mind anymore */
            //Alert(AG_OpenLib|AO_Intuition);
            //break;
        }
#define IntuitionBase nch->nch_HIntBase
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                    IFA_Config, &nch->nch_Config,
                    IFA_InterfaceNum, &nch->nch_IfNum,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &protocol,
                    TAG_END);
        psdGetAttrs(PGA_CONFIG, nch->nch_Config,
                    CA_Device, &nch->nch_Device,
                    TAG_END);

        nch->nch_EPIn = psdFindEndpoint(nch->nch_Interface, NULL,
                                        EA_IsIn, TRUE,
                                        EA_TransferType, USEAF_INTERRUPT,
                                        TAG_END);
        nch->nch_EPOut = psdFindEndpoint(nch->nch_Interface, NULL,
                                         EA_IsIn, FALSE,
                                         EA_TransferType, USEAF_INTERRUPT,
                                         TAG_END);

        if((nch->nch_InpMsgPort = CreateMsgPort()))
        {
            if((nch->nch_InpIOReq = (struct IOStdReq *) CreateIORequest(nch->nch_InpMsgPort, sizeof(struct IOStdReq))))
            {
                if(!OpenDevice("input.device", 0, (struct IORequest *) nch->nch_InpIOReq, 0))
                {
                    nch->nch_InputBase = (struct Library *) nch->nch_InpIOReq->io_Device;
#define InputBase nch->nch_InputBase
                    nch->nch_OS4Hack = TRUE;
                    nch->nch_ClsBase->nh_OS4Hack = TRUE;
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Using AROS IND_ADDEVENT workaround to fix some mouse & keyboard problems.");

                    if((nch->nch_TaskMsgPort = CreateMsgPort()))
                    {
                        if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
                        {
                            psdSetAttrs(PGA_PIPE, nch->nch_EP0Pipe,
                                        PPA_NakTimeout, TRUE,
                                        PPA_NakTimeoutTime, 5000,
                                        TAG_END);
                            if(nch->nch_EPIn)
                            {
                                psdGetAttrs(PGA_ENDPOINT, nch->nch_EPIn,
                                            EA_Interval, &nch->nch_EPInInterval,
                                            TAG_END);

                                if((nch->nch_EPInPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPIn)))
                                {
                                    if(nch->nch_CDC->cdc_TurboMouse)
                                    {
                                        psdSetAttrs(PGA_PIPE, nch->nch_EPInPipe,
                                                    PPA_Interval, 1<<nch->nch_CDC->cdc_TurboMouse,
                                                    PPA_AllowRuntPackets, TRUE,
                                                    TAG_END);
                                    }
                                }
                            } else {
                                nch->nch_EPInPipe = NULL;
                            }
                            if(subclass == HID_BOOT_SUBCLASS)
                            {
                                psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                                             UHR_SET_IDLE, (ULONG) ((protocol == HID_PROTO_KEYBOARD) ? 64 : 0), nch->nch_IfNum);
                                ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                if(ioerr)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                   "SET_IDLE=0 failed: %s (%ld)!",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                }

                                psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                                             UHR_SET_PROTOCOL, HID_PROTO_REPORT, nch->nch_IfNum);
                                ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                if(ioerr)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                   "SET_PROTOCOL=REPORT failed: %s (%ld)!",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                }
                            }
#if 0
                            {
                                IPTR epnum;
                                psdGetAttrs(PGA_ENDPOINT, nch->nch_EPIn,
                                        EA_EndpointNum, &epnum,
                                        TAG_END);
                                psdPipeSetup(nch->nch_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                             USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) epnum|URTF_IN);
                                ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                if(ioerr)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                   "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                                                   epnum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                }
                            }
#endif
                            nQuirkPS3Controller(nch);

                            if(nReadReports(nch))
                            {
                                BOOL fail = FALSE;
                                nAddExtraReport(nch);
                                nDetectWacom(nch);
                                if(!nch->nch_EPInPipe)
                                {
                                    if(nch->nch_HasInItems)
                                    {
                                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "This device does not have an interrupt IN endpoint, but it needs one for input items!");
                                        fail = TRUE;
                                    } else {
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "This device does not have an interrupt IN endpoint!");
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "This means it can only be used for OUTPUT, if at all. Check your HID Control window!");
                                    }
                                }
                                if(!fail)
                                {
                                    if((nch->nch_EPInBuf = psdAllocVec(nch->nch_MaxInSize)))
                                    {
                                        if((nch->nch_EPOutBuf = psdAllocVec(nch->nch_MaxReportSize)))
                                        {
                                            nch->nch_Task = thistask;
                                            return(nch);
                                        }
                                        psdFreeVec(nch->nch_EPInBuf);
                                    }
                                }
                            } else {
                                psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Error parsing report descriptors!");
                            }
                            psdFreePipe(nch->nch_EPInPipe);
                            psdFreePipe(nch->nch_EP0Pipe);
                        }
                        DeleteMsgPort(nch->nch_TaskMsgPort);
                    }
                    CloseDevice((struct IORequest *) nch->nch_InpIOReq);
                }
                DeleteIORequest((struct IORequest *) nch->nch_InpIOReq);
            }
            DeleteMsgPort(nch->nch_InpMsgPort);
        }
    } while(FALSE);
    CloseLibrary(nch->nch_HIntBase);
    CloseLibrary(nch->nch_Base);
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeHid()" */
void GM_UNIQUENAME(nFreeHid)(struct NepClassHid *nch)
{
    struct NepHidReport *nhr;
    nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
    while(nhr->nhr_Node.ln_Succ)
    {
        nFreeReport(nch, nhr);
        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
    }
    psdFreeVec(nch->nch_ReportMap);
    psdFreeVec(nch->nch_HidDesc);
    psdFreeVec(nch->nch_EPInBuf);
    psdFreeVec(nch->nch_EPOutBuf);
    psdFreePipe(nch->nch_EPInPipe);
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    CloseDevice((struct IORequest *) nch->nch_InpIOReq);
    DeleteIORequest((struct IORequest *) nch->nch_InpIOReq);
    DeleteMsgPort(nch->nch_InpMsgPort);
    CloseLibrary(nch->nch_HIntBase);
    CloseLibrary(nch->nch_Base);
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
}
/* \\\ */

/* /// "nFreeItem()" */
void nFreeItem(struct NepClassHid *nch, struct NepHidItem *nhi)
{
    struct NepHidAction *nha;
    struct List *lst;
    ULONG count;

    Remove(&nhi->nhi_Node);
    psdFreeVec(nhi->nhi_UsageMap);
    /* Oh, this was an array */
    if((lst = nhi->nhi_ActionMap))
    {
        /* We need to free all lists available */
        count = nhi->nhi_MapSize;
        do
        {
           /* And each list can have several entries */
           nha = (struct NepHidAction *) lst->lh_Head;
           while(nha->nha_Node.ln_Succ)
           {
               Remove(&nha->nha_Node);
               psdFreeVec(nha);
               nha = (struct NepHidAction *) lst->lh_Head;
           }
           lst++;
        } while(--count);
        psdFreeVec(nhi->nhi_ActionMap);
    }
    /* Free action list */
    nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        Remove(&nha->nha_Node);
        psdFreeVec(nha);
        nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
    }
    psdFreeVec(nhi->nhi_Buffer);
    psdFreeVec(nhi);
}
/* \\\ */

/* /// "nCleanCollection()" */
void nCleanCollection(struct NepClassHid *nch, struct NepHidCollection *nhc)
{
    struct NepHidItem *nhi;
    struct NepHidAction *nha;
    struct List *lst;
    ULONG count;

    nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
    while(nhi->nhi_Node.ln_Succ)
    {
        /* Oh, this was an array */
        if((lst = nhi->nhi_ActionMap))
        {
            /* We need to free all lists available */
            count = nhi->nhi_MapSize;
            do
            {
               /* And each list can have several entries */
               nha = (struct NepHidAction *) lst->lh_Head;
               while(nha->nha_Node.ln_Succ)
               {
                   Remove(&nha->nha_Node);
                   psdFreeVec(nha);
                   nha = (struct NepHidAction *) lst->lh_Head;
               }
               lst++;
            } while(--count);
        }
        /* Free action list */
        nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
        while(nha->nha_Node.ln_Succ)
        {
            Remove(&nha->nha_Node);
            psdFreeVec(nha);
            nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
        }
        nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "nFreeReport()" */
void nFreeReport(struct NepClassHid *nch, struct NepHidReport *nhr)
{
    struct NepHidCollection *nhc;
    struct NepHidItem *nhi;

    Remove(&nhr->nhr_Node);
    nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
    while(nhc->nhc_Node.ln_Succ)
    {
        Remove(&nhc->nhc_Node);
        nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
        while(nhi->nhi_Node.ln_Succ)
        {
            nFreeItem(nch, nhi);
            nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
        }
        psdFreeVec(nhc->nhc_Name);
        psdFreeVec(nhc);
        nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
    }
    psdFreeVec(nhr->nhr_InItemMap);
    psdFreeVec(nhr->nhr_ReportBuf);
    psdFreeVec(nhr);
}
/* \\\ */

/* /// "nAddItemAction()" */
void nAddItemAction(struct NepClassHid *nch, struct PsdIFFContext *rppic, struct NepHidItem *nhi, UWORD id)
{
    struct NepHidCollection *nhc = nhi->nhi_Collection;
    NewList(&nhi->nhi_ActionList);
    SET_WTYPE(&nhi->nhi_ActionList, id + 0xf001);
    AddTail(&nhc->nhc_Items, &nhi->nhi_Node);
    if(rppic)
    {
        if(!nLoadItem(nch, rppic, &nhi->nhi_ActionList, 0xf001))
        {
            nDetectDefaultAction(nch, nhi, &nhi->nhi_ActionList, nhc, nhi->nhi_Usage);
        }
    } else {
        nDetectDefaultAction(nch, nhi, &nhi->nhi_ActionList, nhc, nhi->nhi_Usage);
    }
}
/* \\\ */

/* /// "nAddExtraReport()" */
BOOL nAddExtraReport(struct NepClassHid *nch)
{
    struct NepHidCollection *nhc;
    struct NepHidReport *nhr;
    struct NepHidItem *nhi;
    struct PsdIFFContext *pic;
    struct PsdIFFContext *rppic = NULL;
    UWORD item;
    ULONG pos;

    /* find rumble motors for rumble pack */
    if((item = nFindItemUsage(nch, 0x080045, REPORT_MAIN_OUTPUT)))
    {
        nch->nch_RumbleMotors[0] = nFindItemID(nch, item, REPORT_MAIN_OUTPUT, &pos);
    }
    if((item = nFindItemUsage(nch, 0x080046, REPORT_MAIN_OUTPUT)))
    {
        nch->nch_RumbleMotors[1] = nFindItemID(nch, item, REPORT_MAIN_OUTPUT, &pos);
    }

    pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), nch->nch_DevIDString, nch->nch_IfIDString);
    if(pic)
    {
        rppic = psdFindCfgForm(pic, MAKE_ID('X','R','P','T'));
    }
    if((nhr = psdAllocVec(sizeof(struct NepHidReport))))
    {
        NewList(&nhr->nhr_Collections);
        AddTail(&nch->nch_HidReports, &nhr->nhr_Node);
        nhr->nhr_ReportID = 0xffff;
        nhr->nhr_ItemIDBase = 0xf001;
        if((nhc = psdAllocVec(sizeof(struct NepHidCollection))))
        {
            NewList(&nhc->nhc_Items);
            AddTail(&nhr->nhr_Collections, &nhc->nhc_Node);
            nhc->nhc_Report = nhr;
            nhc->nhc_Name = psdCopyStr("[Extra]");
            if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 1;
                nhi->nhi_Usage = 0x1fff0;
                nAddItemAction(nch, rppic, nhi, 0xff0);
                nch->nch_XtraInitItem = nhi;
            }
            if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 1;
                nhi->nhi_Usage = 0x1fff1;
                nAddItemAction(nch, rppic, nhi, 0xff1);
                nch->nch_XtraQuitItem = nhi;
            }
        }
    }

    return(FALSE);
}
/* \\\ */

/* /// "Wacom Tables" */

static struct WacomCaps WacomCapsTable[] =
{
    { 0x0000, WACOM_PENPARTNER,   5040,  3780,  8, "PenPartner" },
    { 0x0003, WACOM_PLX,         20480, 15360,  9, "Cintiq Partner" },
    { 0x0010, WACOM_GRAPHIRE,    10206,  7422,  9, "Graphire" },
    { 0x0011, WACOM_GRAPHIRE,    10206,  7422,  9, "Graphire 2 4x5" },
    { 0x0012, WACOM_GRAPHIRE,    13918, 10206,  9, "Graphire 2 5x7" },
    { 0x0013, WACOM_GRAPHIRE,    10208,  7424,  9, "Graphire 3" },
    { 0x0014, WACOM_GRAPHIRE,    16704, 12064,  9, "Graphire 3 6x8" },
    { 0x0015, WACOM_GRAPHIRE4,   10208,  7424,  9, "Graphire 4 4x5" },
    { 0x0016, WACOM_GRAPHIRE4,   16704, 12064,  9, "Graphire 4 6x8" },
    { 0x0020, WACOM_INTUOS,      12700, 10600, 10, "Intuos 4x5" },
    { 0x0021, WACOM_INTUOS,      20320, 16240, 10, "Intuos 6x8" },
    { 0x0022, WACOM_INTUOS,      30480, 24060, 10, "Intuos 9x12" },
    { 0x0023, WACOM_INTUOS,      30480, 31680, 10, "Intuos 12x12" },
    { 0x0024, WACOM_INTUOS,      45720, 31680, 10, "Intuos 12x18" },
    { 0x0030, WACOM_PL,           5408,  4056,  8, "PL-400" },
    { 0x0031, WACOM_PL,           6144,  4608,  8, "PL-500" },
    { 0x0032, WACOM_PL,           6126,  4604,  8, "PL-600" },
    { 0x0033, WACOM_PL,           6260,  5016,  8, "PL-600 SX" },
    { 0x0034, WACOM_PL2,          6144,  4608,  9, "PL-550" },
    { 0x0035, WACOM_PL2,          7220,  5780,  9, "PL-800" },
    { 0x0037, WACOM_PL2,          6758,  5406,  9, "PL-700" },
    { 0x0038, WACOM_PL2,          6282,  4762,  9, "PL-510" },
    { 0x0039, WACOM_PL2,         34080, 27660,  9, "PL-710" },
    { 0x003f, WACOM_CINTIQ,      87200, 65600, 10, "Cintiq 21 UX" },
    { 0x0041, WACOM_INTUOS,      12700, 10600, 10, "Intuos 2 4x5" },
    { 0x0042, WACOM_INTUOS,      20320, 16240, 10, "Intuos 2 6x8" },
    { 0x0043, WACOM_INTUOS,      30480, 24060, 10, "Intuos 2 9x12" },
    { 0x0044, WACOM_INTUOS,      30480, 31680, 10, "Intuos 2 12x12" },
    { 0x0045, WACOM_INTUOS,      45720, 31680, 10, "Intuos 2 12x18" },
    { 0x0047, WACOM_INTUOS,      20320, 16240, 10, "Intuos 2 6x8" },
    { 0x0060, WACOM_GRAPHIRE,     5104,  3712,  9, "Volito" },
    { 0x0061, WACOM_GRAPHIRE,     3250,  2320,  8, "PenStation 2" },
    { 0x0062, WACOM_GRAPHIRE,     5104,  3712,  9, "Volito 2 4x5" },
    { 0x0063, WACOM_GRAPHIRE,     3248,  2320,  9, "Volito 2 2x3" },
    { 0x0064, WACOM_GRAPHIRE,     3250,  2320,  8, "PenPartner 2" },
    { 0x00b0, WACOM_INTUOS3,     25400, 20320, 10, "Intuos 3 4x5" },
    { 0x00b1, WACOM_INTUOS3,     40640, 30480, 10, "Intuos 3 6x8" },
    { 0x00b2, WACOM_INTUOS3,     60960, 45720, 10, "Intuos 3 9x12" },
    { 0x00b5, WACOM_INTUOS3,     54204, 31750, 10, "Intuos 3 6x11" },
    { 0x00c0, WACOM_PL2,          6858,  5506,  9, "DTF-720" },
    { 0xffff, 0,                     0,     0,  0, NULL }
};
/* \\\ */

/* /// "nParseWacom()" */
BOOL nParseWacom(struct NepClassHid *nch, UBYTE *buf, ULONG len)
{
    struct WacomReport wr = { ~0, ~0, 0xffff, 0, 0, 0x80, 0x80, 0 };
    struct NepHidItem *nhi;
    BOOL take = FALSE;
    UWORD tmpval;
    KPRINTF(1, ("Wacom parsing %ld (%ld)...\n", nch->nch_WacomMode, len));
    switch(nch->nch_WacomMode)
    {
        case WACOM_PENPARTNER:
            //      7     6     5     4     3     2     1     0
            //     80    40    20    10    08    04    02    01
            // 1:  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
            // 2:  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
            // 3:  yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
            // 4:  yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
            // 5:        rmb  outrng
            // 6: [signed pressure]
            if((buf[0] != 2) || (len < 7))
            {
                break;
            }
            wr.wr_PosX = buf[1] | (buf[2]<<8);
            wr.wr_PosY = buf[3] | (buf[4]<<8);
            wr.wr_Pressure = (((BYTE *) buf)[6]) + 127;
            wr.wr_InProximity = buf[5] & 0x20 ? 0x00 : 0xff;
            if(wr.wr_InProximity && (wr.wr_Pressure > 50))
            {
                wr.wr_Buttons = 0x01;
            }
            if(buf[5] & 0x40)
            {
                wr.wr_Buttons |= 0x02;
            }
            take = TRUE;
            break;

        case WACOM_GRAPHIRE:
        case WACOM_GRAPHIRE4:
            //      7     6     5     4     3     2     1     0
            //     80    40    20    10    08    04    02    01
            // 1:  vld   tooltype   prox         mmb   rmb   lmb
            // 2:  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
            // 3:  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
            // 4:  yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
            // 5:  yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
            // 6: [wheel bits                                  ]  G3 MOUSE
            // 6: [distance                                    ]  G4 MOUSE
            // 7: [distance                                    ]  G3 MOUSE
            // 7:                              wh_sgn wheel_bits  G4 MOUSE
            // 6: [ pressure bits 0-7                          ]
            // 7:                                          pres8
            if((buf[0] != 2) || (len < 8))
            {
                break;
            }
            if(buf[1] & 0x10)
            {
                wr.wr_InProximity = 0xff;
                if(buf[1] & 0x40) // is a mouse
                {
                    if(buf[1] & 0x20) // mouse without a scrollwheel
                    {
                        // only two buttons, no wheel
                        wr.wr_Buttons = buf[1] & 0x03;
                    } else {
                        // three buttons and wheel
                        wr.wr_Buttons = buf[1] & 0x07;
                        if(nch->nch_WacomMode == WACOM_GRAPHIRE4)
                        {
                            wr.wr_Wheel = buf[7] & 0x03;
                            if(buf[7] & 0x04)
                            {
                                wr.wr_Wheel = -wr.wr_Wheel;
                            }
                        } else {
                            wr.wr_Wheel = ((BYTE *) buf)[6];
                            KPRINTF(1, ("Wheel %ld\n", wr.wr_Wheel));
                        }
                    }
                } else {
                    wr.wr_Buttons = buf[1] & 0x07;
                    wr.wr_Pressure = buf[6] | ((buf[7] & 0x01)<<8);
                }
                wr.wr_PosX = buf[2] | (buf[3]<<8);
                wr.wr_PosY = buf[4] | (buf[5]<<8);
            } else {
                wr.wr_InProximity = 0;
            }
            take = TRUE;
            break;

        case WACOM_INTUOS:
        case WACOM_INTUOS3:
        case WACOM_CINTIQ:
            //      7     6     5     4     3     2     1     0
            //     80    40    20    10    08    04    02    01
            // 1:   1           1     0     0    mmb  rmb   tool = pen packet
            // 1:   1           1     1     0     0         tool = airbrush
            // 1:   1           1     0     1                    = mouse
            // 1:   1           1     1     0     0     0        = lens?
            // 1:   1     1     0     0     0     0              = tool in
            // 1:   1     0     0     0     0     0     0        = tool out
            // 2:  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
            // 3:  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
            // 4:  yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
            // 5:  yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
            // 6:              [ pressure/wheel bits 8-6      ]
            // 7:  [press0-1]  [ tiltx 6-1]
            // 8: tiltx0 [ tilty 6-0                          ]
            // 9:                                      x16  y16
            if(buf[0] == 12)
            {
                wr.wr_PadButtons = (buf[5] & 0x0f)|(buf[6]<<4);
                take = TRUE;
                break;
            }
            if((buf[0] != 2) && (buf[0] != 5) && (buf[0] != 6))
            {
                break;
            }
            // needs some improvement on the various tools
            if(buf[1] & 0x40)
            {
                wr.wr_InProximity = 0xff;
                if(nch->nch_WacomMode == WACOM_INTUOS)
                {
                    wr.wr_PosX = (buf[2]<<8) | buf[3];
                    wr.wr_PosY = (buf[4]<<8) | buf[5];
                    if(buf[1] & 0x10)
                    {
                        wr.wr_Buttons = (buf[8] & 0x07) | ((buf[8] & 0x30)>>1);
                    }
                } else {
                    wr.wr_PosX = (buf[2]<<9) | (buf[3] << 1) | ((buf[9] & 0x02)>>1);
                    wr.wr_PosY = (buf[4]<<9) | (buf[5] << 1) | (buf[9] & 0x01);
                }
                if((buf[1] & 0xa0) == 0xa0)
                {
                    wr.wr_TiltX = ((buf[7] & 0x3f) << 1) | (buf[8] >> 7);
                    wr.wr_TiltY = buf[8] & 0x7f;
                }
                tmpval = (buf[6] << 2) | ((buf[7] & 0xc0)>>6);
                if((buf[1] & 0xb8) == 0xa0)
                {
                    wr.wr_Pressure = tmpval;
                    wr.wr_Buttons = (buf[1] & 0x06) | ((wr.wr_Pressure > 16) ? 1 : 0);
                }
                if((buf[1] & 0xbc) == 0xb4)
                {
                    wr.wr_Wheel = tmpval;
                }
            } else {
                wr.wr_InProximity = 0;
            }
            take = TRUE;
            break;

        case WACOM_PL:
        case WACOM_PL2:
            if((buf[0] != 2) || (len < 8))
            {
                break;
            }
            wr.wr_InProximity = buf[1] & 0x40 ? 0xff : 0;
            if(wr.wr_InProximity)
            {
                wr.wr_Pressure = (BYTE) ((buf[7]<<1) | ((buf[4] & 0x04)>>2));
                if(nch->nch_WacomMode == WACOM_PL2)
                {
                    wr.wr_Pressure <<= 1;
                    wr.wr_Pressure += ((buf[4] >> 6) & 1) + 256;
                } else {
                    wr.wr_Pressure += 128;
                }
                wr.wr_PosX = ((buf[1] & 0x03)<<14) | ((buf[2] & 0x7f)<<7) | (buf[3] & 0x7f);
                wr.wr_PosY = ((buf[4] & 0x03)<<14) | ((buf[5] & 0x7f)<<7) | (buf[6] & 0x7f);
                wr.wr_Buttons = (buf[4] & 0x38)>>3;
            }
            take = TRUE;
            break;

        case WACOM_PLX:
            if((buf[0] != 2) || (len < 8))
            {
                break;
            }
            wr.wr_InProximity = 0xff;
            wr.wr_PosX = buf[2] | (buf[3]<<8);
            wr.wr_PosY = buf[4] | (buf[5]<<8);
            wr.wr_Pressure = buf[6] | (buf[7]<<8);
            if(buf[1] & 0x02)
            {
                wr.wr_Buttons |= 0x01;
            }
            if(buf[1] & 0x10)
            {
                wr.wr_Buttons |= 0x02;
            }
            take = TRUE;
            break;
    }
    if(take)
    {
        // fix endianess
        wr.wr_PosX = AROS_LONG2LE(wr.wr_PosX);
        wr.wr_PosY = AROS_LONG2LE(wr.wr_PosY);
        wr.wr_Pressure = AROS_WORD2LE(wr.wr_Pressure);

        nhi = (struct NepHidItem *) nch->nch_WacomColl->nhc_Items.lh_Head;
        while(nhi->nhi_Node.ln_Succ)
        {
            nProcessItem(nch, nhi, (UBYTE *) &wr);
            nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
        }
    }
    return(take);
}
/* \\\ */

/* /// "nDetectWacom()" */
BOOL nDetectWacom(struct NepClassHid *nch)
{
    struct NepHidCollection *nhc;
    struct NepHidReport *nhr;
    struct NepHidItem *nhi;
    struct PsdIFFContext *pic;
    struct PsdIFFContext *rppic = NULL;
    struct WacomCaps *wc;
    IPTR vendid;
    IPTR prodid;
    ULONG caps = 0;
    UWORD butno;
    UWORD numbuts = 3;

    psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                DA_VendorID, &vendid,
                DA_ProductID, &prodid,
                TAG_END);
    if(vendid != 0x056a)
    {
        return(FALSE);
    }

    for(wc = WacomCapsTable; wc->wc_ProdID != prodid; wc++)
    {
        if(wc->wc_ProdID == 0xffff)
        {
            return FALSE;
        }
    }
    switch(wc->wc_Type)
    {
        case WACOM_PENPARTNER:
        case WACOM_PLX:
            numbuts = 2;
            break;

        case WACOM_INTUOS:
        case WACOM_INTUOS3:
        case WACOM_CINTIQ:
            caps |= WACOM_HASTILT;
        case WACOM_GRAPHIRE4:
            if(wc->wc_Type != WACOM_INTUOS)
            {
                caps |= WACOM_HASBUTTONS;
            }
        case WACOM_GRAPHIRE:
            caps |= WACOM_HASWHEEL;
            break;
    }

    psdAddErrorMsg(RETURN_OK, GM_UNIQUENAME(libname), "Adding Wacom %s support.", wc->wc_Name);

    pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), nch->nch_DevIDString, nch->nch_IfIDString);
    if(pic)
    {
        rppic = psdFindCfgForm(pic, MAKE_ID('W','C','O','M'));
    }
    if((nhr = psdAllocVec(sizeof(struct NepHidReport))))
    {
        NewList(&nhr->nhr_Collections);
        AddTail(&nch->nch_HidReports, &nhr->nhr_Node);
        nhr->nhr_ReportID = 0xfffe;
        nhr->nhr_ItemIDBase = 0xf001;
        if((nhc = psdAllocVec(sizeof(struct NepHidCollection))))
        {
            NewList(&nhc->nhc_Items);
            AddTail(&nhr->nhr_Collections, &nhc->nhc_Node);
            nhc->nhc_Report = nhr;
            nhc->nhc_Usage = 0x0d0001;
            nhc->nhc_Name = psdCopyStr("[Wacom Support]");
            nch->nch_WacomColl = nhc;
            nch->nch_WacomMode = wc->wc_Type;
            // buttons
            for(butno = 0; butno < numbuts; butno++)
            {
                if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
                {
                    nhi->nhi_Collection = nhc;
                    nhi->nhi_Type = REPORT_MAIN_INPUT;
                    nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                    nhi->nhi_Offset = (offsetof(struct WacomReport, wr_Buttons)<<3) + butno;
                    nhi->nhi_Size = 1;

                    nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                    nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 1;
                    nhi->nhi_Usage = 0x090001+butno;
                    nAddItemAction(nch, rppic, nhi, 0xfd0+butno);
                }
            }
            // absolute X
            if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                nhi->nhi_Offset = offsetof(struct WacomReport, wr_PosX)<<3;
                nhi->nhi_Size = sizeof(((struct WacomReport *) NULL)->wr_PosX)<<3;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = wc->wc_ResX-1;
                nhi->nhi_Usage = 0x010030;
                nAddItemAction(nch, rppic, nhi, 0xfe1);
            }
            // absolute Y
            if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                nhi->nhi_Offset = offsetof(struct WacomReport, wr_PosY)<<3;
                nhi->nhi_Size = sizeof(((struct WacomReport *) NULL)->wr_PosY)<<3;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = wc->wc_ResY-1;
                nhi->nhi_Usage = 0x010031;
                nAddItemAction(nch, rppic, nhi, 0xfe2);
            }
            // wheel
            if((caps & WACOM_HASWHEEL) && ((nhi = psdAllocVec(sizeof(struct NepHidItem)))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                nhi->nhi_Offset = offsetof(struct WacomReport, wr_Wheel)<<3;
                nhi->nhi_Size = sizeof(((struct WacomReport *) NULL)->wr_Wheel)<<3;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = -127;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 127;
                nhi->nhi_IsSigned = TRUE;
                nhi->nhi_Usage = 0x010038;
                nAddItemAction(nch, rppic, nhi, 0xfe0);
            }
            // pressure
            if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                nhi->nhi_Offset = offsetof(struct WacomReport, wr_Pressure)<<3;
                nhi->nhi_Size = sizeof(((struct WacomReport *) NULL)->wr_Pressure)<<3;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = (1UL<<wc->wc_PressBits)-1;
                nhi->nhi_Usage = 0x0d0030;
                nAddItemAction(nch, rppic, nhi, 0xfe4);
            }
            // in proximity
            if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                nhi->nhi_Offset = offsetof(struct WacomReport, wr_InProximity)<<3;
                nhi->nhi_Size = 1;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 1;
                nhi->nhi_Usage = 0x0d0032;
                nAddItemAction(nch, rppic, nhi, 0xfe5);
            }
            // x tilt
            if((caps & WACOM_HASTILT) && ((nhi = psdAllocVec(sizeof(struct NepHidItem)))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                nhi->nhi_Offset = offsetof(struct WacomReport, wr_TiltX)<<3;
                nhi->nhi_Size = sizeof(((struct WacomReport *) NULL)->wr_TiltX)<<3;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 127;
                nhi->nhi_Usage = 0x0d003d;
                nAddItemAction(nch, rppic, nhi, 0xfe6);
            }
            // y tilt
            if((caps & WACOM_HASTILT) && ((nhi = psdAllocVec(sizeof(struct NepHidItem)))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                nhi->nhi_Offset = offsetof(struct WacomReport, wr_TiltY)<<3;
                nhi->nhi_Size = sizeof(((struct WacomReport *) NULL)->wr_TiltY)<<3;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 127;
                nhi->nhi_Usage = 0x0d003e;
                nAddItemAction(nch, rppic, nhi, 0xfe7);
            }
            // pad buttons
            if(caps & WACOM_HASBUTTONS)
            {
                for(butno = 0; butno < 8; butno++)
                {
                    if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
                    {
                        nhi->nhi_Collection = nhc;
                        nhi->nhi_Type = REPORT_MAIN_INPUT;
                        nhi->nhi_Flags = RPF_MAIN_VARIABLE;
                        nhi->nhi_Offset = (offsetof(struct WacomReport, wr_PadButtons)<<3) + butno;
                        nhi->nhi_Size = 1;

                        nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                        nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 1;
                        nhi->nhi_Usage = 0x090009+butno;
                        nAddItemAction(nch, rppic, nhi, 0xfd8+butno);
                    }
                }
            }
        }
    }

    return(FALSE);
}
/* \\\ */

/* /// "nQuirkPS3Controller()" */
void nQuirkPS3Controller(struct NepClassHid *nch)
{
    LONG ioerr;
    UBYTE buf[18];

    IPTR vendid;
    IPTR prodid;

    psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                DA_VendorID, &vendid,
                DA_ProductID, &prodid,
                TAG_END);

    if(vendid == 0x054c && prodid == 0x0268) {
        psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE, UHR_GET_REPORT, 0x03f2, nch->nch_IfNum);
        ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, sizeof(buf));
        if(ioerr) {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Unable to add PS3 controller support!");
        }else{
            psdAddErrorMsg(RETURN_OK, GM_UNIQUENAME(libname), "Adding PS3 controller support.");
        }
    }
}
/* \\\ */

/* /// "nReadReports()" */
BOOL nReadReports(struct NepClassHid *nch)
{
    struct NepHidReport *nhr;
    struct NepHidCollection *nhc;
    struct NepHidItem *nhi;
    struct PsdDescriptor *pdd;

    LONG ioerr;
    UBYTE buf[256];
    ULONG num;
    UBYTE *bptr;
    BOOL res = FALSE;
    BOOL thisres;

    UWORD idnum;
    ULONG count;
    struct List *alistptr;
    ULONG actlen;

    pdd = psdFindDescriptor(nch->nch_Device, NULL,
                            DDA_DescriptorType, UDT_HID,
                            DDA_Interface, nch->nch_Interface,
                            TAG_END);
    if(pdd)
    {
        UBYTE *descdata;
        psdGetAttrs(PGA_DESCRIPTOR, pdd,
                    DDA_DescriptorData, &descdata,
                    TAG_END);

        actlen = descdata[0];
        memcpy(buf, descdata, (size_t) actlen);
        ioerr = 0;
        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Using HID Descriptor from initial configuration run.");
    } else {
        psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_STANDARD|URTF_INTERFACE,
                     USR_GET_DESCRIPTOR, UDT_HID<<8, nch->nch_IfNum);
        ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 256);
        if(ioerr == UHIOERR_RUNTPACKET)
        {
            ioerr = 0;
        }
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "GET_HID_DESCRIPTOR(%ld) failed: %s (%ld)!",
                           256, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            psdDelayMS(100);
            ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 256);
            if(ioerr == UHIOERR_RUNTPACKET)
            {
                ioerr = 0;
            }
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "GET_HID_DESCRIPTOR(%ld) (2nd) failed: %s (%ld)!",
                               256, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_STANDARD|URTF_INTERFACE,
                             USR_GET_DESCRIPTOR, UDT_HID<<8, nch->nch_IfNum);
                ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 1);
                if(ioerr)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "GET_HID_DESCRIPTOR(%ld) (3rd) failed: %s (%ld)!",
                                   1, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
            } else {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Burstroem retry successful ;)");
            }
        }
        actlen = psdGetPipeActual(nch->nch_EP0Pipe);
    }
    if((!ioerr) || (ioerr == UHIOERR_OVERFLOW))
    {
        if(buf[0] < 9)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "bLength field (%ld) probably wrong, fixing to %ld!",
                           buf[0], actlen);
            buf[0] = actlen;
        }
        if(actlen < 6)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Read less than 6 bytes (%ld read) for HID Descriptor!",
                           actlen);
        }
        if((nch->nch_HidDesc = psdAllocVec((ULONG) buf[0])))
        {
            /*ioerr = psdDoPipe(nch->nch_EP0Pipe, nch->nch_HidDesc, (ULONG) buf[0]);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "GET_HID_DESCRIPTOR(%ld) failed: %s (%ld)!",
                               buf[0], psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                psdDelayMS(100);
                ioerr = psdDoPipe(nch->nch_EP0Pipe, nch->nch_HidDesc, (ULONG) buf[0]);
                if(!ioerr)
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Burstroem retry successful ;)");
                }
            }
            if(!ioerr)
            {*/
                CopyMem(buf, nch->nch_HidDesc, (ULONG) buf[0]);
                //dumpmem(nch->nch_HidDesc, (ULONG) buf);
                if(!nch->nch_HidDesc->bNumDescriptors)
                {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "Device reports that it doesn't have any descriptors!");
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "Debug info: %ld, %02lx, %02lx, %ld, %ld, %02lx, %ld, %ld!",
                                   nch->nch_HidDesc->bLength,
                                   nch->nch_HidDesc->bDescriptorType,
                                   nch->nch_HidDesc->bcdHID,
                                   nch->nch_HidDesc->bCountryCode,
                                   nch->nch_HidDesc->bNumDescriptors,
                                   nch->nch_HidDesc->bDescType,
                                   nch->nch_HidDesc->wDescLength,
                                   nch->nch_HidDesc->wPad0);
                }
                if(6 + nch->nch_HidDesc->bNumDescriptors*3 > actlen)
                {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "Too few bytes read for HID %ld descriptor (%ld of %ld)!",
                                   nch->nch_HidDesc->bNumDescriptors,
                                   actlen,
                                   6 + nch->nch_HidDesc->bNumDescriptors*3);
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "Debug info: %ld, %02lx, %02lx, %ld, %ld, %02lx, %ld, %ld!",
                                   nch->nch_HidDesc->bLength,
                                   nch->nch_HidDesc->bDescriptorType,
                                   nch->nch_HidDesc->bcdHID,
                                   nch->nch_HidDesc->bCountryCode,
                                   nch->nch_HidDesc->bNumDescriptors,
                                   nch->nch_HidDesc->bDescType,
                                   nch->nch_HidDesc->wDescLength,
                                   nch->nch_HidDesc->wPad0);
                }

                KPRINTF(1, ("%ld Report descriptors\n", nch->nch_HidDesc->bNumDescriptors));
                bptr = &nch->nch_HidDesc->bDescType;
                for(num = 0; num < nch->nch_HidDesc->bNumDescriptors; num++)
                {
                    if(*bptr == UDT_REPORT)
                    {
                        if((nhr = psdAllocVec(sizeof(struct NepHidReport))))
                        {
                            NewList(&nhr->nhr_Collections);
                            AddTail(&nch->nch_HidReports, &nhr->nhr_Node);
                            thisres = FALSE;
                            nhr->nhr_ReportLength = bptr[1] + (bptr[2]<<8);
                            if(!nhr->nhr_ReportLength)
                            {
                                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                               "Report Descriptor #%ld is said to have 0 bytes!",
                                               num);
                            } else {
                                if((nhr->nhr_ReportBuf = psdAllocVec(nhr->nhr_ReportLength)))
                                {
                                    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_STANDARD|URTF_INTERFACE,
                                                 USR_GET_DESCRIPTOR, (UDT_REPORT<<8)|num, nch->nch_IfNum);
                                    ioerr = psdDoPipe(nch->nch_EP0Pipe, nhr->nhr_ReportBuf, nhr->nhr_ReportLength);
                                    if(!ioerr)
                                    {
                                        //dumpmem(nhr->nhr_ReportBuf, nhr->nhr_ReportLength);
                                        thisres = nParseReport(nch, nhr);
                                        //nDebugReport(nch, nhr);
                                        res |= thisres;
                                    } else {
                                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "GET_REPORT_DESCRIPTOR(%ld) failed: %s (%ld)!",
                                                       nhr->nhr_ReportLength, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    }
                                }
                            }
                            if(!thisres)
                            {
                                nFreeReport(nch, nhr);
                            }
                        }
                    }
                    bptr += 3;
                }
            /*} else {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "GET_HID_DESCRIPTOR(%ld) failed: %s (%ld)!",
                               buf[0], psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }*/
        } else {
            KPRINTF(1, ("No Hid Descriptor memory!\n"));
        }
    } else {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "GET_HID_DESCRIPTOR(%ld) failed: %s (%ld)!",
                       256, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    if(res)
    {
        ULONG repsize;
        /* Get maximal package size */
        nch->nch_MaxReportID = 0;
        nch->nch_MaxInSize = 0;
        nch->nch_MaxOutSize = 0;
        nch->nch_MaxFeatSize = 0;
        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            KPRINTF(5, ("Report %ld: InBits=%ld, OutBits=%ld, FeatBits=%ld\n",
                    nhr->nhr_ReportID, nhr->nhr_ReportInSize,
                    nhr->nhr_ReportOutSize, nhr->nhr_ReportFeatSize));
            if(nhr->nhr_ReportID)
            {
                nch->nch_UsesReportID = TRUE;
                if(nch->nch_MaxReportID < nhr->nhr_ReportID)
                {
                    nch->nch_MaxReportID = nhr->nhr_ReportID;
                }
            }
            repsize = (nhr->nhr_ReportInSize+7)>>3;
            if(nch->nch_MaxInSize < repsize)
            {
                nch->nch_MaxInSize = repsize;
            }
            repsize = (nhr->nhr_ReportOutSize+7)>>3;
            if(nch->nch_MaxOutSize < repsize)
            {
                nch->nch_MaxOutSize = repsize;
            }
            repsize = (nhr->nhr_ReportFeatSize+7)>>3;
            if(nch->nch_MaxFeatSize < repsize)
            {
                nch->nch_MaxFeatSize = repsize;
            }
            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
        }
        if(nch->nch_UsesReportID)
        {
            nch->nch_MaxInSize++;
            nch->nch_MaxOutSize++;
            nch->nch_MaxFeatSize++;
        }

        nch->nch_MaxReportSize = nch->nch_MaxInSize;
        if(nch->nch_MaxOutSize > nch->nch_MaxReportSize)
        {
            nch->nch_MaxReportSize = nch->nch_MaxOutSize;
        }
        if(nch->nch_MaxFeatSize > nch->nch_MaxReportSize)
        {
            nch->nch_MaxReportSize = nch->nch_MaxFeatSize;
        }

        KPRINTF(5, ("MaxReportID=%ld, MaxReportSize=%ld\n", nch->nch_MaxReportID, nch->nch_MaxReportSize));

        if((nch->nch_ReportMap = psdAllocVec((ULONG) sizeof(struct NepHidReport *) * (nch->nch_MaxReportID+1))))
        {
            /* Fill in Report Map */
            nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
            while(nhr->nhr_Node.ln_Succ)
            {
                KPRINTF(5, ("Report Map slot %ld/%ld filled with %08lx\n", nhr->nhr_ReportID, nch->nch_MaxReportID, nhr));
                nch->nch_ReportMap[nhr->nhr_ReportID] = nhr;
                nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
            }
        } else {
            KPRINTF(10, ("Out of memory for Report Map!\n"));
            res = FALSE;
        }
    }

    if(res)
    {
        struct NepHidItem **nhiptr;
        struct NepHidItem **nhiinptr;
        struct NepHidItem **nhioutptr;
        struct NepHidItem **nhifeatptr;

        /* Count items */
        idnum = 1;
        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            KPRINTF(1, ("Report %ld:\n", nhr->nhr_ReportID));
            nhr->nhr_ItemIDBase = idnum;
            nhr->nhr_InItemCount = nhr->nhr_OutItemCount = nhr->nhr_FeatItemCount = 0;
            nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
            while(nhc->nhc_Node.ln_Succ)
            {
                KPRINTF(1, ("  Collection %s:\n", nhc->nhc_Name));
                nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
                while(nhi->nhi_Node.ln_Succ)
                {
                    switch(nhi->nhi_Type)
                    {
                        case REPORT_MAIN_INPUT:
                            KPRINTF(1, ("  Input Item %lx:\n", nhi->nhi_Usage));
                            nhr->nhr_InItemCount++;
                            break;

                        case REPORT_MAIN_OUTPUT:
                            KPRINTF(1, ("  Output Item %lx:\n", nhi->nhi_Usage));
                            nhr->nhr_OutItemCount++;
                            break;

                        case REPORT_MAIN_FEATURE:
                            KPRINTF(1, ("  Feature Item %lx:\n", nhi->nhi_Usage));
                            nhr->nhr_FeatItemCount++;
                            break;
                    }

                    // generate id numbers
                    SET_WTYPE(&nhi->nhi_ActionList, idnum++);
                    if(!(nhi->nhi_Flags & RPF_MAIN_VARIABLE))
                    {
                        alistptr = nhi->nhi_ActionMap;
                        count = nhi->nhi_MapSize;
                        do
                        {
                            SET_WTYPE(alistptr, idnum++);
                            alistptr++;
                        } while(--count);
                    }

                    nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                }
                nhc = (struct NepHidCollection *) nhc->nhc_Node.ln_Succ;
            }
            /*psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "R=%02lx: %ld input items, %ld output items, %ld feature items, %ld items total!",
                           nhr->nhr_ReportID, nhr->nhr_InItemCount, nhr->nhr_OutItemCount, nhr->nhr_FeatItemCount,
                           idnum-1);*/
            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
        }

        /* Create quick item lookup table */
        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            KPRINTF(1, ("Report %ld, allocating %ld/%ld/%ld items\n",
                    nhr->nhr_ReportID,
                    nhr->nhr_InItemCount, nhr->nhr_OutItemCount, nhr->nhr_FeatItemCount));
            if((nhiptr = psdAllocVec(sizeof(struct NepHidItem *) * (nhr->nhr_InItemCount +
                                     nhr->nhr_OutItemCount + nhr->nhr_FeatItemCount))))
            {
                nhiinptr = nhr->nhr_InItemMap = nhiptr;
                nhioutptr = nhr->nhr_OutItemMap = &nhiptr[nhr->nhr_InItemCount];
                nhifeatptr = nhr->nhr_FeatItemMap = &nhioutptr[nhr->nhr_OutItemCount];
                nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
                while(nhc->nhc_Node.ln_Succ)
                {
                    nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;

                    while(nhi->nhi_Node.ln_Succ)
                    {
                        /* Add quick lookup entry */
                        switch(nhi->nhi_Type)
                        {
                            case REPORT_MAIN_INPUT:
                                *nhiinptr++ = nhi;
                                nch->nch_HasInItems = TRUE;
                                break;

                            case REPORT_MAIN_OUTPUT:
                                *nhioutptr++ = nhi;
                                break;

                            case REPORT_MAIN_FEATURE:
                                *nhifeatptr++ = nhi;
                                break;
                        }
                        nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
                    }
                    nhc = (struct NepHidCollection *) nhc->nhc_Node.ln_Succ;
                }
            } else {
                res = FALSE;
            }
            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
        }

        nLoadActionConfig(nch);
    }
    return(res);
}
/* \\\ */

/* /// "nLoadActionConfig()" */
void nLoadActionConfig(struct NepClassHid *nch)
{
    struct NepHidReport *nhr;
    struct NepHidCollection *nhc;
    struct NepHidItem *nhi;

    ULONG count;
    struct List *alistptr;
    ULONG *usageptr;
    struct PsdIFFContext *pic;
    struct PsdIFFContext *rppic;

    pic = psdGetUsbDevCfg(GM_UNIQUENAME(libname), nch->nch_DevIDString, nch->nch_IfIDString);
    if(pic)
    {
        nch->nch_LastCfgCRC = nCalcConfigCRC(nch, nch->nch_Base, pic);
    } else {
        nch->nch_LastCfgCRC = 0;
    }

    /* Create default action mapping */
    nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
    while(nhr->nhr_Node.ln_Succ)
    {
        KPRINTF(1, ("Report %ld:\n", nhr->nhr_ReportID));
        if(pic)
        {
            rppic = psdFindCfgForm(pic, MAKE_ID('R','P','T','0')+nhr->nhr_ReportID);
        } else {
            rppic = NULL;
        }
        nhc = (struct NepHidCollection *) nhr->nhr_Collections.lh_Head;
        while(nhc->nhc_Node.ln_Succ)
        {
            KPRINTF(1, ("  Collection %s:\n", nhc->nhc_Name));
            nhi = (struct NepHidItem *) nhc->nhc_Items.lh_Head;
            while(nhi->nhi_Node.ln_Succ)
            {
                usageptr = &nhi->nhi_Usage;
                alistptr = &nhi->nhi_ActionList;
                if(rppic)
                {
                    if(!(nLoadItem(nch, rppic, alistptr, nhr->nhr_ItemIDBase)))
                    {
                        nDetectDefaultAction(nch, nhi, alistptr, nhc, *usageptr);
                    }
                } else {
                    nDetectDefaultAction(nch, nhi, alistptr, nhc, *usageptr);
                }

                if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                {
                    KPRINTF(1, ("    Variable %lx: %s\n", nhi->nhi_Usage, nNumToStr(nch, NTS_USAGEID, nhi->nhi_Usage, "unknown")));
                } else {
                    usageptr = nhi->nhi_UsageMap;
                    alistptr = nhi->nhi_ActionMap;
                    count = nhi->nhi_MapSize;
                    KPRINTF(1, ("    Array %lx: %s\n", *usageptr, nNumToStr(nch, NTS_USAGEID, *usageptr, "unknown")));
                    do
                    {
                        if(rppic)
                        {
                            if(!(nLoadItem(nch, rppic, alistptr, nhr->nhr_ItemIDBase)))
                            {
                                nDetectDefaultAction(nch, nhi, alistptr, nhc, *usageptr);
                            }
                        } else {
                            nDetectDefaultAction(nch, nhi, alistptr, nhc, *usageptr);
                        }
                        alistptr++;
                        usageptr++;
                    } while(--count);
                }
                nhi = (struct NepHidItem *) nhi->nhi_Node.ln_Succ;
            }
            nhc = (struct NepHidCollection *) nhc->nhc_Node.ln_Succ;
        }
        nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "nDebugMem()" */
void nDebugMem(struct NepClassHid *nch, UBYTE *rptr, ULONG rptlen)
{
    char  fmtstr[108];
    STRPTR fmtptr;
    UWORD cnt;
    UWORD pos = 0;

    while(rptlen)
    {
        fmtptr = fmtstr;
        cnt = 16;
        *fmtptr++ = '%';
        *fmtptr++ = '0';
        *fmtptr++ = '4';
        *fmtptr++ = 'l';
        *fmtptr++ = 'x';
        *fmtptr++ = ':';
        *fmtptr++ = ' ';
        do
        {
            *fmtptr++ = '%';
            *fmtptr++ = '0';
            *fmtptr++ = '2';
            *fmtptr++ = 'l';
            *fmtptr++ = 'x';
            if(--cnt)
            {
                *fmtptr++ = ' ';
            }
            --rptlen;
        } while(cnt && rptlen);
        *fmtptr = 0;
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), fmtstr, pos,
                       rptr[0], rptr[1], rptr[2], rptr[3], rptr[4], rptr[5], rptr[6], rptr[7],
                       rptr[8], rptr[9], rptr[10], rptr[11], rptr[12], rptr[13], rptr[14], rptr[15]);
        rptr += 16;
        pos += 16;
    }
}
/* \\\ */

/* /// "nDebugReport()" */
void nDebugReport(struct NepClassHid *nch, struct NepHidReport *nhr)
{
    UBYTE *rptr;
    ULONG rptlen;
    if(!nhr)
    {
        psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Can't debug HID Report: nhr=NULL!\n");
        return;
    }
    rptr = nhr->nhr_ReportBuf;
    rptlen = nhr->nhr_ReportLength;
    if(!rptr)
    {
        psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Can't debug HID Report: ReportBuf=NULL!\n");
        return;
    }

    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                   "HID Report %02lx Debug (%ld bytes):", nhr->nhr_ReportID, rptlen);
    nDebugMem(nch, rptr, rptlen);
}
/* \\\ */

/* /// "nParseReport()" */
BOOL nParseReport(struct NepClassHid *nch, struct NepHidReport *nhr)
{
    UBYTE *rptr = nhr->nhr_ReportBuf;
    UBYTE *rptrend = &nhr->nhr_ReportBuf[nhr->nhr_ReportLength];
    struct NepHidCollection *nhc = NULL;
    struct Node *node;
    UWORD itag;
    UWORD isize;
    UWORD itype;
    ULONG len = 0;
    LONG data = 0;
    ULONG udata = 0;
    ULONG usageid, designid, stringid;
    ULONG usagemin, usagemax;
    ULONG designmin, designmax;
    ULONG stringmin, stringmax;
    ULONG reportid = 0;
    ULONG bitpos = 0;
    LONG count;
    UWORD delim = 0;
    ULONG reportpos;

    NewList(&nch->nch_HidStack);
    NewList(&nch->nch_HidUsages);
    NewList(&nch->nch_HidDesigns);
    NewList(&nch->nch_HidStrings);

    nch->nch_HidGlobal.nhg_UsagePage =
      nch->nch_HidGlobal.nhg_LogicalMin =
      nch->nch_HidGlobal.nhg_LogicalMax =
      nch->nch_HidGlobal.nhg_PhysicalMin =
      nch->nch_HidGlobal.nhg_PhysicalMax =
      nch->nch_HidGlobal.nhg_UnitExp =
      nch->nch_HidGlobal.nhg_Unit = HID_PARAM_UNDEF;

    nch->nch_HidGlobal.nhg_ReportID =
      nch->nch_HidGlobal.nhg_ReportSize =
      nch->nch_HidGlobal.nhg_ReportCount = 0;

    usagemin = usagemax =
      designmin = designmax =
      stringmin = stringmax = HID_PARAM_UNDEF;

    KPRINTF(1, ("Starting parsing...\n"));
    /* Parse the items */
    while(rptr && (rptr < rptrend))
    {
        reportpos = ((IPTR) rptr) - ((IPTR) nhr->nhr_ReportBuf);
        itag = *rptr & REPORT_ITAG_MASK;
        isize = *rptr & REPORT_ISIZE_MASK;
        itype = *rptr & REPORT_ITYPE_MASK;
        if(*rptr++ == REPORT_LONGITEM) /* Is this a long item? */
        {
            len = *rptr++;
            itag = *rptr++;
            rptr += len;
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "HID Error at %04lx: Long items (%lx, len=%ld) are not specified in the spec!", reportpos, itag, len);
        } else {
            switch(isize)
            {
                case REPORT_ISIZE_0:
                    len = 0;
                    udata = data = 0;
                    break;

                case REPORT_ISIZE_1:
                    len = 1;
                    data = *((BYTE *) rptr);
                    udata = *rptr++;
                    break;

                case REPORT_ISIZE_2:
                    len = 2;
                    udata = data = *rptr++;
                    data |= (*((BYTE *) rptr))<<8;
                    udata |= (*rptr++)<<8;
                    break;

                case REPORT_ISIZE_4:
                    len = 4;
                    data = *rptr++;
                    data |= (*rptr++)<<8;
                    data |= (*rptr++)<<16;
                    data |= (*rptr++)<<24;
                    udata = data;
                    break;
            }
        }
        switch(itype)
        {
            case REPORT_ITYPE_MAIN:
                //KPRINTF(1, ("Main "));
                if(reportid != nch->nch_HidGlobal.nhg_ReportID)
                {
                    struct NepHidReport *oldnhr = nhr;
                    if(reportid)
                    {
                        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
                        while(nhr->nhr_Node.ln_Succ)
                        {
                            if(nhr->nhr_ReportID == nch->nch_HidGlobal.nhg_ReportID)
                            {
                                break;
                            }
                            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
                        }
                        if(!nhr->nhr_Node.ln_Succ)
                        {
                            if((nhr = psdAllocVec(sizeof(struct NepHidReport))))
                            {
                                KPRINTF(1, ("Generating new report ID %ld\n", nch->nch_HidGlobal.nhg_ReportID));
                                NewList(&nhr->nhr_Collections);
                                nhr->nhr_ReportID = nch->nch_HidGlobal.nhg_ReportID;
                                AddTail(&nch->nch_HidReports, &nhr->nhr_Node);
                            } else {
                                KPRINTF(1, ("Out of memory creating new report.\n"));
                                rptr = NULL;
                                break;
                            }
                        } else {
                            KPRINTF(1, ("Switched back to report ID %ld\n", nch->nch_HidGlobal.nhg_ReportID));
                        }
                        if(nhc)
                        {
                            KPRINTF(1, ("Collection open, checking need for cloning.\n"));
                            if(nhc->nhc_Items.lh_Head->ln_Succ)
                            {
                                struct NepHidCollection *tmpnhc;
                                struct NepHidCollection *newnhc;
                                struct NepHidCollection *childnhc = NULL;

                                KPRINTF(1, ("Not empty, assigning collection to current report and cloning!\n"));
                                tmpnhc = nhc;
                                nhc = NULL;
                                do
                                {
                                    KPRINTF(1, ("Cloning %08lx (%s)\n", tmpnhc, tmpnhc->nhc_Name));
                                    if((newnhc = psdAllocVec(sizeof(struct NepHidCollection))))
                                    {
                                        if(!nhc)
                                        {
                                            nhc = newnhc;
                                        }
                                        NewList(&newnhc->nhc_Items);
                                        if(childnhc)
                                        {
                                            childnhc->nhc_Parent = newnhc;
                                        }
                                        newnhc->nhc_Usage = tmpnhc->nhc_Usage;
                                        newnhc->nhc_Name = psdCopyStr(tmpnhc->nhc_Name);
                                    } else {
                                        KPRINTF(10, ("Out of memory cloning nhc!\n"));
                                        break;
                                    }
                                    AddTail(&oldnhr->nhr_Collections, &tmpnhc->nhc_Node);
                                    tmpnhc->nhc_Report = oldnhr;
                                    childnhc = newnhc;
                                } while((tmpnhc = nhc->nhc_Parent));
                            }
                        }
                    } else {
                        KPRINTF(1, ("Setting first report ID %ld\n", nch->nch_HidGlobal.nhg_ReportID));
                        nhr->nhr_ReportID = nch->nch_HidGlobal.nhg_ReportID;
                    }
                    reportid = nch->nch_HidGlobal.nhg_ReportID;
                }
                switch(itag)
                {
                    case REPORT_MAIN_INPUT:
                        bitpos = nhr->nhr_ReportInSize;
                        break;
                    case REPORT_MAIN_OUTPUT:
                        bitpos = nhr->nhr_ReportOutSize;
                        break;
                    case REPORT_MAIN_FEATURE:
                        bitpos = nhr->nhr_ReportFeatSize;
                        break;
                }
                switch(itag)
                {
                    case REPORT_MAIN_INPUT:
                    case REPORT_MAIN_OUTPUT:
                    case REPORT_MAIN_FEATURE:
                        {
                            struct NepHidItem *nhi;
                            struct NepHidUsage *nhu;
                            BOOL fakelogmin = FALSE;
                            BOOL fakelogmax = FALSE;
                            if(!delim)
                            {
                                KPRINTF(1, ("%s (%lx)\n", (itag == REPORT_MAIN_INPUT) ? "Input" : ((itag == REPORT_MAIN_OUTPUT) ? "Output" : "Feature"), udata));
                                if(!nhc)
                                {
                                    KPRINTF(10, ("No collection open!\n"));
                                    psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                                   "HID Error at %04lx: No collection open!", reportpos);
                                    nDebugReport(nch, nhr);
                                    rptr = NULL;
                                    break;
                                }
                                if(nch->nch_HidGlobal.nhg_LogicalMin == HID_PARAM_UNDEF)
                                {
                                    nch->nch_HidGlobal.nhg_LogicalMin = 0;
                                    fakelogmin = TRUE;
                                    if(nch->nch_HidGlobal.nhg_LogicalMax == HID_PARAM_UNDEF)
                                    {
                                        fakelogmax = TRUE;
                                        if(nch->nch_HidGlobal.nhg_ReportSize < 32)
                                        {
                                            nch->nch_HidGlobal.nhg_LogicalMax = (1UL<<nch->nch_HidGlobal.nhg_ReportSize)-1;
                                        } else {
                                            nch->nch_HidGlobal.nhg_LogicalMin = 0x80000000;
                                            nch->nch_HidGlobal.nhg_LogicalMax = 0x7fffffff;
                                        }
                                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "HID Error at %04lx: LogicalMin and Max for item missing, assuming %ld-%ld (%ld)!",
                                                       reportpos,
                                                       nch->nch_HidGlobal.nhg_LogicalMin,
                                                       nch->nch_HidGlobal.nhg_LogicalMax);
                                    } else {
                                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "HID Error at %04lx: LogicalMin for item missing, assuming 0!", reportpos);
                                    }
                                }
                                else if(nch->nch_HidGlobal.nhg_LogicalMax == HID_PARAM_UNDEF)
                                {
                                    fakelogmax = TRUE;
                                    nch->nch_HidGlobal.nhg_LogicalMax = (1UL<<nch->nch_HidGlobal.nhg_ReportSize)-1;
                                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "HID Error at %04lx: LogicalMax for item missing, assuming %ld!", reportpos, nch->nch_HidGlobal.nhg_LogicalMax);

                                }
                                if((nch->nch_HidGlobal.nhg_UsagePage == HID_PARAM_UNDEF) ||
                                   //(nch->nch_HidGlobal.nhg_LogicalMin == HID_PARAM_UNDEF) ||
                                   //(nch->nch_HidGlobal.nhg_LogicalMax == HID_PARAM_UNDEF) ||
                                   (!nch->nch_HidGlobal.nhg_ReportSize)// ||
                                   //(!nch->nch_HidGlobal.nhg_ReportCount) // ReportCount == 0 seems to be a valid input (sigh), just avoid error message
                                   )
                                {
                                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "HID Error at %04lx: Mandatory item missing!", reportpos);
                                    nDebugReport(nch, nhr);
                                    KPRINTF(10, ("Mandatory item missing!\n"));
                                    rptr = NULL;
                                } else {
                                    ULONG *usageptr;
                                    struct List *alistptr;
                                    usageid = designid = stringid = HID_PARAM_UNDEF;
                                    if(udata & RPF_MAIN_VARIABLE)
                                    {
                                        for(count = 0; count < nch->nch_HidGlobal.nhg_ReportCount; count++)
                                        {
                                            nhu = (struct NepHidUsage *) nch->nch_HidDesigns.lh_Head;
                                            if(nhu->nhu_Node.ln_Succ)
                                            {
                                                designid = nhu->nhu_Usage++;
                                                if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                {
                                                    Remove(&nhu->nhu_Node);
                                                    psdFreeVec(nhu);
                                                }
                                            }
                                            nhu = (struct NepHidUsage *) nch->nch_HidStrings.lh_Head;
                                            if(nhu->nhu_Node.ln_Succ)
                                            {
                                                stringid = nhu->nhu_Usage++;
                                                if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                {
                                                    Remove(&nhu->nhu_Node);
                                                    psdFreeVec(nhu);
                                                }
                                            }
                                            nhu = (struct NepHidUsage *) nch->nch_HidUsages.lh_Head;
                                            if(nhu->nhu_Node.ln_Succ)
                                            {
                                                usageid = nhu->nhu_Usage++;
                                                if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                {
                                                    KPRINTF(1, ("Removing usage %08lx\n", nhu));
                                                    Remove(&nhu->nhu_Node);
                                                    psdFreeVec(nhu);
                                                }
                                            }
                                            if(usageid != HID_PARAM_UNDEF)
                                            {
                                                if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
                                                {
                                                    nhi->nhi_Collection = nhc;
                                                    nhi->nhi_Type = itag;
                                                    nhi->nhi_Flags = udata;
                                                    nhi->nhi_Offset = bitpos;
                                                    nhi->nhi_Size = nch->nch_HidGlobal.nhg_ReportSize;

                                                    nhi->nhi_LogicalMin = nch->nch_HidGlobal.nhg_LogicalMin;
                                                    nhi->nhi_LogicalMax = nch->nch_HidGlobal.nhg_LogicalMax;
                                                    nhi->nhi_IsSigned = (nhi->nhi_LogicalMin < 0) || (nhi->nhi_LogicalMax < 0);
                                                    nhi->nhi_PhysicalMin = nch->nch_HidGlobal.nhg_PhysicalMin;
                                                    nhi->nhi_PhysicalMax = nch->nch_HidGlobal.nhg_PhysicalMax;
                                                    nhi->nhi_UnitExp = nch->nch_HidGlobal.nhg_UnitExp;
                                                    nhi->nhi_Unit = nch->nch_HidGlobal.nhg_Unit;
                                                    nhi->nhi_Usage = usageid;
                                                    nhi->nhi_DesignIndex = designid;
                                                    nhi->nhi_StringIndex = stringid;
                                                    NewList(&nhi->nhi_ActionList);
                                                    AddTail(&nhc->nhc_Items, &nhi->nhi_Node);
                                                    KPRINTF(1, ("New Variable Item: %ld.%ld (%ld-%ld) %lx\n",
                                                                nhi->nhi_Offset>>3, nhi->nhi_Offset & 7,
                                                                nhi->nhi_LogicalMin, nhi->nhi_LogicalMax,
                                                                nhi->nhi_Usage));
                                                }
                                            }
                                            bitpos += nch->nch_HidGlobal.nhg_ReportSize;
                                        }
                                    } else {
                                        nhu = (struct NepHidUsage *) nch->nch_HidUsages.lh_Head;
                                        if(nhu->nhu_Node.ln_Succ)
                                        {
                                            if((nhi = psdAllocVec(sizeof(struct NepHidItem))))
                                            {
                                                nhi->nhi_Collection = nhc;
                                                nhi->nhi_Type = itag;
                                                nhi->nhi_Flags = udata;
                                                nhi->nhi_Offset = bitpos;
                                                nhi->nhi_Size = nch->nch_HidGlobal.nhg_ReportSize;
                                                nhi->nhi_Count = nch->nch_HidGlobal.nhg_ReportCount;

                                                nhi->nhi_LogicalMin = nch->nch_HidGlobal.nhg_LogicalMin;
                                                nhi->nhi_LogicalMax = nch->nch_HidGlobal.nhg_LogicalMax;

                                                nhi->nhi_MapSize = (nhi->nhi_LogicalMax - nhi->nhi_LogicalMin)+1;
                                                nhi->nhi_UsageMap = psdAllocVec(sizeof(ULONG) * nhi->nhi_MapSize);
                                                nhi->nhi_ActionMap = psdAllocVec(sizeof(struct List) * nhi->nhi_MapSize);
                                                nhi->nhi_Buffer = psdAllocVec(2 * sizeof(LONG) * nhi->nhi_Count);
                                                nhi->nhi_OldBuffer = &nhi->nhi_Buffer[nhi->nhi_Count];
                                                nhi->nhi_PhysicalMin = nch->nch_HidGlobal.nhg_PhysicalMin;
                                                nhi->nhi_PhysicalMax = nch->nch_HidGlobal.nhg_PhysicalMax;
                                                nhi->nhi_UnitExp = nch->nch_HidGlobal.nhg_UnitExp;
                                                nhi->nhi_Unit = nch->nch_HidGlobal.nhg_Unit;

                                                usageptr = nhi->nhi_UsageMap;
                                                alistptr = nhi->nhi_ActionMap;
                                                if(!(usageptr && alistptr && nhi->nhi_Buffer))
                                                {
                                                    psdFreeVec(usageptr);
                                                    psdFreeVec(alistptr);
                                                    psdFreeVec(nhi->nhi_Buffer);
                                                    psdFreeVec(nhi);
                                                    KPRINTF(10, ("Out of memory\n"));
                                                    rptr = NULL;
                                                    break;
                                                }

                                                nhi->nhi_Usage = nhi->nhi_DesignIndex = nhi->nhi_StringIndex = HID_PARAM_UNDEF;
                                                NewList(&nhi->nhi_ActionList);

                                                KPRINTF(1, ("New Array(%ld*%ld) Item: %ld.%ld (%ld-%ld)\n",
                                                            nhi->nhi_Count, nhi->nhi_MapSize,
                                                            nhi->nhi_Offset>>3, nhi->nhi_Offset & 7,
                                                            nhi->nhi_LogicalMin, nhi->nhi_LogicalMax));

                                                nhi->nhi_SameUsages = TRUE;
                                                for (count = nhi->nhi_MapSize; count; count--)
                                                {
                                                    nhu = (struct NepHidUsage *) nch->nch_HidDesigns.lh_Head;
                                                    if(nhu->nhu_Node.ln_Succ)
                                                    {
                                                        designid = nhu->nhu_Usage++;
                                                        if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                        {
                                                            Remove(&nhu->nhu_Node);
                                                            psdFreeVec(nhu);
                                                        }
                                                    }
                                                    nhu = (struct NepHidUsage *) nch->nch_HidStrings.lh_Head;
                                                    if(nhu->nhu_Node.ln_Succ)
                                                    {
                                                        stringid = nhu->nhu_Usage++;
                                                        if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                        {
                                                            Remove(&nhu->nhu_Node);
                                                            psdFreeVec(nhu);
                                                        }
                                                    }
                                                    nhu = (struct NepHidUsage *) nch->nch_HidUsages.lh_Head;
                                                    if(nhu->nhu_Node.ln_Succ)
                                                    {
                                                        usageid = nhu->nhu_Usage++;
                                                        if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                        {
                                                            KPRINTF(1, ("Removing usage %08lx\n", nhu));
                                                            Remove(&nhu->nhu_Node);
                                                            psdFreeVec(nhu);
                                                        }
                                                    }

                                                    // check if usages are all the same
                                                    if(count < nhi->nhi_MapSize)
                                                    {
                                                        if(usageid != usageptr[-1])
                                                        {
                                                            nhi->nhi_SameUsages = FALSE;
                                                        }
                                                    }

                                                    *usageptr++ = usageid;
                                                    NewList(alistptr);
                                                    alistptr++;
                                                }

                                                if(nhi->nhi_SameUsages)
                                                {
                                                    // if it's all the same, we can assign a usage id to this array
                                                    nhi->nhi_Usage = nhi->nhi_UsageMap[0];
                                                }
                                                if(nhi->nhi_MapSize)
                                                {
                                                    AddTail(&nhc->nhc_Items, &nhi->nhi_Node);
                                                }
                                            }
                                        } /* FIXME: Clean string/delimiter stack if no usage? */
                                        bitpos += nch->nch_HidGlobal.nhg_ReportSize * nch->nch_HidGlobal.nhg_ReportCount;
                                    }
                                }
                                if(fakelogmin)
                                {
                                    nch->nch_HidGlobal.nhg_LogicalMin = HID_PARAM_UNDEF;
                                }
                                if(fakelogmax)
                                {
                                    nch->nch_HidGlobal.nhg_LogicalMax = HID_PARAM_UNDEF;
                                }
							}
							break;
						}

					case REPORT_MAIN_COLLECT:
						{
							struct NepHidUsage *nhu;
							struct NepHidCollection *oldnhc = nhc;
							KPRINTF(1, ("Collection(%lx)\n", udata));
							if((nhc = psdAllocVec(sizeof(struct NepHidCollection))))
							{
								NewList(&nhc->nhc_Items);
								nhc->nhc_Parent = oldnhc;
								nhu = (struct NepHidUsage *) nch->nch_HidUsages.lh_Head;
								if(nhu->nhu_Node.ln_Succ)
								{
									nhc->nhc_Usage = nhu->nhu_Usage;
									nhc->nhc_Name = nGetUsageName(nch, nhu->nhu_Usage);
								} else {
									if(nhc->nhc_Parent)
									{
										nhc->nhc_Usage = nhc->nhc_Parent->nhc_Usage;
										nhc->nhc_Name = psdCopyStr(nhc->nhc_Parent->nhc_Name);
									} else {
										nhc->nhc_Name = psdCopyStr("Argl!");
									}
								}
							}
							node = nch->nch_HidUsages.lh_Head;
							while(node->ln_Succ)
							{
								KPRINTF(1, ("Removing usage %08lx\n", node));
								Remove(node);
								psdFreeVec(node);
								node = nch->nch_HidUsages.lh_Head;
							}
						break;
						}

                    case REPORT_MAIN_ENDCOLL:
                        KPRINTF(1, ("EndCollection(%lx)\n", udata));
                        if(nhc)
                        {
                            /* Get rid of empty collection */
                            //if(!nhc->nhc_Items.lh_Head->ln_Succ)
                            if(0) /* don't do this, we might need the parent collection! */
                            {
                                struct NepHidCollection *oldnhc = nhc;
                                KPRINTF(1, ("Deleting empty collection %lx (%s)\n", nhc, nhc->nhc_Name));
                                nhc = nhc->nhc_Parent;
                                psdFreeVec(oldnhc->nhc_Name);
                                psdFreeVec(oldnhc);
                            } else {
                                AddTail(&nhr->nhr_Collections, &nhc->nhc_Node);
                                nhc->nhc_Report = nhr;
                                nhc = nhc->nhc_Parent;
                            }
                        }
                        break;

                    default:
                        KPRINTF(1, ("Reserved_%02lx(%lx)\n", itag, data));
                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID Error at %04lx: Reserved_MAIN_%02lx(%lx)", reportpos, itag, data);
                        nDebugReport(nch, nhr);
                        rptr = NULL;
                        break;
                }
                /* Remove remaining usages */
                node = nch->nch_HidUsages.lh_Head;
                while(node->ln_Succ)
                {
                    KPRINTF(1, ("MainEnd: Removing usage %08lx\n", node));
                    Remove(node);
                    psdFreeVec(node);
                    node = nch->nch_HidUsages.lh_Head;
                }
                /* Remove remaining designators */
                node = nch->nch_HidDesigns.lh_Head;
                while(node->ln_Succ)
                {
                    KPRINTF(1, ("MainEnd: Removing designator %08lx\n", node));
                    Remove(node);
                    psdFreeVec(node);
                    node = nch->nch_HidDesigns.lh_Head;
                }
                /* Remove remaining strings */
                node = nch->nch_HidStrings.lh_Head;
                while(node->ln_Succ)
                {
                    KPRINTF(1, ("MainEnd: Removing strings %08lx\n", node));
                    Remove(node);
                    psdFreeVec(node);
                    node = nch->nch_HidStrings.lh_Head;
                }

                switch(itag)
                {
                    case REPORT_MAIN_INPUT:
                        nhr->nhr_ReportInSize = bitpos;
                        break;
                    case REPORT_MAIN_OUTPUT:
                        nhr->nhr_ReportOutSize = bitpos;
                        break;
                    case REPORT_MAIN_FEATURE:
                        nhr->nhr_ReportFeatSize = bitpos;
                        break;
                }

                break;

            case REPORT_ITYPE_GLOBAL:
                //KPRINTF(1, ("Global "));
                switch(itag)
                {
                    case REPORT_GLOB_USAGE:
                        KPRINTF(1, ("Usage Page(%ld) %s\n", udata, nNumToStr(nch, NTS_USAGEPAGE, udata, "unknown")));
                        nch->nch_HidGlobal.nhg_UsagePage = udata;
                        break;

                    case REPORT_GLOB_LOGMIN:
                        KPRINTF(1, ("LogMin(%ld)\n", data));
                        nch->nch_HidGlobal.nhg_LogicalMin = data;
                        break;

                    case REPORT_GLOB_LOGMAX:
                        /* Some devices (like usb-kbd in QEMU) have wrong descriptors
                         * Try to detect and correct this here.
                         * This only works if LogMin is defined before LogMax but that's likely common
                         * FIXME: Is there a better way to handle this? */
                        nch->nch_HidGlobal.nhg_LogicalMax = (data < nch->nch_HidGlobal.nhg_LogicalMin ? udata : data);
                        KPRINTF(1, ("LogMax(%ld)\n", nch->nch_HidGlobal.nhg_LogicalMax));
                        break;

                    case REPORT_GLOB_PHYMIN:
                        KPRINTF(1, ("PhyMin(%ld)\n", data));
                        nch->nch_HidGlobal.nhg_PhysicalMin = data;
                        break;

                    case REPORT_GLOB_PHYMAX:
                        KPRINTF(1, ("PhyMax(%ld)\n", data));
                        nch->nch_HidGlobal.nhg_PhysicalMax = data;
                        break;

                    case REPORT_GLOB_UNITEXP:
                        KPRINTF(1, ("UnitExp(%ld)\n", data));
                        nch->nch_HidGlobal.nhg_UnitExp = data;
                        break;

                    case REPORT_GLOB_UNIT:
                        KPRINTF(1, ("Unit(%ld)\n", data));
                        nch->nch_HidGlobal.nhg_Unit = data;
                        break;

                    case REPORT_GLOB_RPSIZE:
                        KPRINTF(1, ("ReportSize(%ld)\n", udata));
                        nch->nch_HidGlobal.nhg_ReportSize = udata;
                        break;

                    case REPORT_GLOB_RPID:
                        KPRINTF(1, ("ReportID(%ld)\n", udata));
                        nch->nch_HidGlobal.nhg_ReportID = udata;
                        break;

                    case REPORT_GLOB_RPCOUNT:
                        KPRINTF(1, ("ReportCount(%ld)\n", udata));
                        nch->nch_HidGlobal.nhg_ReportCount = udata;
                        break;

                    case REPORT_GLOB_PUSH:
                    {
                        struct NepHidGlobal *nhg = psdAllocVec(sizeof(struct NepHidGlobal));
                        KPRINTF(1, ("Push(%ld)\n", data));
                        if(nhg)
                        {
                            *nhg = nch->nch_HidGlobal;
                            AddTail(&nch->nch_HidStack, &nhg->nhg_Node);
                        } else {
                            KPRINTF(10, ("Out of HID stack memory!\n"));
                            rptr = NULL;
                        }
                        break;
                    }

                    case REPORT_GLOB_POP:
                    {
                        struct NepHidGlobal *nhg = (struct NepHidGlobal *) nch->nch_HidStack.lh_Tail;
                        KPRINTF(1, ("Pop(%ld)\n", data));
                        if(nhg->nhg_Node.ln_Pred)
                        {
                            nch->nch_HidGlobal = *nhg;
                            Remove(&nhg->nhg_Node);
                            psdFreeVec(nhg);
                        } else {
                            KPRINTF(10, ("Pop from HID stack without push!\n"));
                            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                           "HID Error at %04lx: Pop from HID stack without push!", reportpos);
                            nDebugReport(nch, nhr);
                            rptr = NULL;
                        }
                        break;
                    }

                    default:
                        KPRINTF(1, ("Reserved_%02lx(%lx)\n", itag, data));
                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID Error at %04lx: Reserved_GLOBAL_%02lx(%lx)", reportpos, itag, data);
                        nDebugReport(nch, nhr);
                        rptr = NULL;
                        break;
                }
                break;

            case REPORT_ITYPE_LOCAL:
                //KPRINTF(1, ("Local "));
                switch(itag)
                {
                    case REPORT_LOCL_USAGE:
                        if(len <= 2)
                        {
                            udata |= nch->nch_HidGlobal.nhg_UsagePage<<16;
                        }
                        KPRINTF(1, ("Usage (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        if(!nAddUsage(nch, &nch->nch_HidUsages, udata, udata))
                        {
                            rptr = NULL;
                        }
                        break;

                    case REPORT_LOCL_USEMIN:
                        if(len <= 2)
                        {
                            udata |= nch->nch_HidGlobal.nhg_UsagePage<<16;
                        }
                        KPRINTF(1, ("UsageMin (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        usagemin = udata;
                        if(usagemax != HID_PARAM_UNDEF)
                        {
                            if(!nAddUsage(nch, &nch->nch_HidUsages, usagemin, usagemax))
                            {
                                rptr = NULL;
                            }
                            usagemin = usagemax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_USEMAX:
                        if(len <= 2)
                        {
                            udata |= nch->nch_HidGlobal.nhg_UsagePage<<16;
                        }
                        KPRINTF(1, ("UsageMax (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        usagemax = udata;
                        if(usagemin != HID_PARAM_UNDEF)
                        {
                            if(!nAddUsage(nch, &nch->nch_HidUsages, usagemin, usagemax))
                            {
                                rptr = NULL;
                            }
                            usagemin = usagemax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_DESIDX:
                        KPRINTF(1, ("Designator index (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        if(!nAddUsage(nch, &nch->nch_HidDesigns, udata, udata))
                        {
                            rptr = NULL;
                        }
                        break;

                    case REPORT_LOCL_DESMIN:
                        KPRINTF(1, ("DesignMin (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        designmin = udata;
                        if(designmax != HID_PARAM_UNDEF)
                        {
                            if(!nAddUsage(nch, &nch->nch_HidDesigns, designmin, designmax))
                            {
                                rptr = NULL;
                            }
                            designmin = designmax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_DESMAX:
                        KPRINTF(1, ("DesignMax (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        designmin = udata;
                        if(designmax != HID_PARAM_UNDEF)
                        {
                            if(!nAddUsage(nch, &nch->nch_HidDesigns, designmin, designmax))
                            {
                                rptr = NULL;
                            }
                            designmin = designmax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_STRIDX:
                        KPRINTF(1, ("String index (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        if(!nAddUsage(nch, &nch->nch_HidStrings, udata, udata))
                        {
                            rptr = NULL;
                        }
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID String Index %ld", udata);
                        break;

                    case REPORT_LOCL_STRMIN:
                        KPRINTF(1, ("StringMin (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        stringmin = udata;
                        if(stringmax != HID_PARAM_UNDEF)
                        {
                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                           "HID String Index Range %ld - %ld", stringmin, stringmax);
                            if(!nAddUsage(nch, &nch->nch_HidStrings, stringmin, stringmax))
                            {
                                rptr = NULL;
                            }
                            stringmin = stringmax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_STRMAX:
                        KPRINTF(1, ("StringMax (%lx) %s\n", udata, nNumToStr(nch, NTS_USAGEID, udata, "unknown")));
                        stringmax = udata;
                        if(stringmin != HID_PARAM_UNDEF)
                        {
                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                           "HID String Index Range %ld - %ld", stringmin, stringmax);
                            if(!nAddUsage(nch, &nch->nch_HidStrings, stringmin, stringmax))
                            {
                                rptr = NULL;
                            }
                            stringmin = stringmax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_DELIM:
                        KPRINTF(1, ("Delimiter (%ld)\n", udata));
                        if(udata)
                        {
                            delim++;
                        } else {
                            delim--;
                        }
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "HID Report Delimiters not really supported!");
                        break;

                    default:
                        KPRINTF(1, ("Reserved_%02lx(%lx)\n", itag, udata));
                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID Error at %04lx: Reserved_LOCAL_%02lx(%lx)", reportpos, itag, udata);
                        nDebugReport(nch, nhr);
                        rptr = NULL;
                        break;
                }
                break;

            default:
                KPRINTF(1, ("Reserved!\n"));
                psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                               "HID Error at %04lx: Reserved_%02lx", reportpos, itag);
                nDebugReport(nch, nhr);
                rptr = NULL;
                break;
        }
    }
    KPRINTF(1, ("Parsing done, status %lx.\n", rptr));

    /* Remove remaining stack frame */
    node = nch->nch_HidStack.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing stack frame %08lx\n", node));
        Remove(node);
        psdFreeVec(node);
        node = nch->nch_HidStack.lh_Head;
    }
    /* Remove remaining usages */
    node = nch->nch_HidUsages.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing usage %08lx\n", node));
        Remove(node);
        psdFreeVec(node);
        node = nch->nch_HidUsages.lh_Head;
    }
    /* Remove remaining designators */
    node = nch->nch_HidDesigns.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing designator %08lx\n", node));
        Remove(node);
        psdFreeVec(node);
        node = nch->nch_HidDesigns.lh_Head;
    }
    /* Remove remaining strings */
    node = nch->nch_HidStrings.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing strings %08lx\n", node));
        Remove(node);
        psdFreeVec(node);
        node = nch->nch_HidStrings.lh_Head;
    }
    //nDebugReport(nch, nhr);
    return((BOOL) (rptr ? TRUE : FALSE));
}
/* \\\ */

/* /// "nFindCollID()" */
BOOL nFindCollID(struct NepClassHid *nch, struct NepHidCollection *nhc, ULONG collidmin, ULONG collidmax)
{
    do
    {
        if((nhc->nhc_Usage >= collidmin) && (nhc->nhc_Usage <= collidmax))
        {
            return(TRUE);
        }
    } while((nhc = nhc->nhc_Parent));
    return(FALSE);
}
/* \\\ */

/* /// "nFindItemID()" */
struct NepHidItem * nFindItemID(struct NepClassHid *nch, UWORD id, UWORD itype, ULONG *pos)
{
    struct NepHidReport *nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
    struct NepHidItem *nhi;
    struct NepHidItem **nhiptr;
    struct List *alistptr;
    ULONG count;
    ULONG icount;
    BOOL featmode;

    if(itype == REPORT_MAIN_OUTPUT)
    {
        featmode = FALSE;
    }
    else if(itype == REPORT_MAIN_FEATURE)
    {
        featmode = TRUE;
    } else {
        return(NULL);
    }
    while(nhr->nhr_Node.ln_Succ)
    {
        nhiptr = NULL;
        if(featmode)
        {
            if((icount = nhr->nhr_FeatItemCount))
            {
                nhiptr = nhr->nhr_FeatItemMap;
            }
        } else {
            if((icount = nhr->nhr_OutItemCount))
            {
                nhiptr = nhr->nhr_OutItemMap;
            }
        }
        if(nhiptr)
        {
            do
            {
                nhi = *nhiptr++;
                if(GET_WTYPE(&nhi->nhi_ActionList) == id)
                {
                    *pos = 0xffffffff;
                    return(nhi);
                }
                if(!(nhi->nhi_Flags & RPF_MAIN_VARIABLE))
                {
                    alistptr = nhi->nhi_ActionMap;
                    count = nhi->nhi_MapSize;
                    do
                    {
                        if(GET_WTYPE(alistptr) == id)
                        {
                            *pos = nhi->nhi_MapSize - count;
                            return(nhi);
                        }
                        alistptr++;
                    } while(--count);
                }
            }
            while(--icount);
        }
        nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
    }
    return(NULL);
}
/* \\\ */

/* /// "nFindItemUsage()" */
UWORD nFindItemUsage(struct NepClassHid *nch, ULONG usage, UWORD itype)
{
    struct NepHidReport *nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
    struct NepHidItem *nhi;
    struct NepHidItem **nhiptr;
    ULONG *usageptr;
    struct List *alistptr;
    ULONG count;
    ULONG icount;
    BOOL featmode;

    if(itype == REPORT_MAIN_OUTPUT)
    {
        featmode = FALSE;
    }
    else if(itype == REPORT_MAIN_FEATURE)
    {
        featmode = TRUE;
    } else {
        return(0);
    }

    while(nhr->nhr_Node.ln_Succ)
    {
        nhiptr = NULL;
        if(featmode)
        {
            if((icount = nhr->nhr_FeatItemCount))
            {
                nhiptr = nhr->nhr_FeatItemMap;
            }
        } else {
            if((icount = nhr->nhr_OutItemCount))
            {
                nhiptr = nhr->nhr_OutItemMap;
            }
        }
        if(nhiptr)
        {
            do
            {
                nhi = *nhiptr++;
                if(nhi->nhi_Usage == usage)
                {
                    return(GET_WTYPE(&nhi->nhi_ActionList));
                }
                if(!(nhi->nhi_Flags & RPF_MAIN_VARIABLE))
                {
                    usageptr = nhi->nhi_UsageMap;
                    alistptr = nhi->nhi_ActionMap;
                    count = nhi->nhi_MapSize;
                    do
                    {
                        if(*usageptr == usage)
                        {
                            return GET_WTYPE(alistptr);
                        }
                        usageptr++;
                        alistptr++;
                    } while(--count);
                }
            }
            while(--icount);
        }
        nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
    }
    return(0);
}
/* \\\ */

/* /// "UsbToPs2 Mapping" */
const struct UsbToPs2Map usbtops2map[] =
{
    { 0x010081, 0x5e }, // System Power Down
    { 0x010082, 0x5f }, // System Sleep
    { 0x010083, 0x63 }, // System Wake Up

    { 0x070046, 0x4c }, // Keyboard Print Screen
    { 0x070047, 0x64 }, // Keyboard Scroll Lock
    { 0x070048, 0x45 }, // Keyboard Pause
    { 0x070049, 0x0a }, // Keyboard Insert

    { 0x070066, 0x5e }, // Keyboard Power

    { 0x07007A, 0x08 }, // Keyboard Undo
    { 0x07007B, 0x17 }, // Keyboard Cut
    { 0x07007C, 0x18 }, // Keyboard Copy
    { 0x07007D, 0x0a }, // Keyboard Paste
    { 0x07007F, 0x20 }, // Keyboard Mute
    { 0x070080, 0x30 }, // Keyboard Volume Up
    { 0x070081, 0x2e }, // Keyboard Volume Down

    { 0x0C0030, 0x5e }, // Power
    { 0x0C0032, 0x5f }, // Sleep
    { 0x0C0033, 0x5f }, // Sleep After
    { 0x0C0034, 0x5f }, // Sleep Mode

    { 0x0C0094, 0x40 }, // Quit
    { 0x0C0095, 0x3b }, // Help
    { 0x0C00B0, 0x22 }, // Play
    { 0x0C00B1, 0x22 }, // Pause
    { 0x0C00B5, 0x19 }, // Scan Next Track
    { 0x0C00B6, 0x10 }, // Scan Previous Track
    { 0x0C00B7, 0x24 }, // Stop
    { 0x0C00CC, 0x24 }, // Stop/Eject
    { 0x0C00CD, 0x22 }, // Play/Pause
    { 0x0C00CE, 0x22 }, // Play/Skip

    { 0x0C00E2, 0x20 }, // Mute
    { 0x0C00E9, 0x30 }, // Volume Increment
    { 0x0C00EA, 0x2e }, // Volume Decrement

    { 0x0C0183, 0x6d }, // AL Consumer Control Configuration
    { 0x0C0184, 0x13 }, // AL Word Processor
    { 0x0C0185, 0x13 }, // AL Text Editor
    { 0x0C0186, 0x14 }, // AL Spreadsheet
    { 0x0C018A, 0x6c }, // AL Email Reader
    { 0x0C018F, 0x3d }, // AL Task/Project Manager
    { 0x0C0192, 0x21 }, // AL Calculator
    { 0x0C0194, 0x3c }, // AL Local Machine Browser
    { 0x0C0196, 0x3c }, // AL Internet Browser
    { 0x0C019B, 0x16 }, // AL Logon
    { 0x0C019C, 0x16 }, // AL Logoff
    { 0x0C019D, 0x16 }, // AL Logon/Logoff
    { 0x0C01A1, 0x3d }, // AL Process/Task Manager
    { 0x0C01A3, 0x1e }, // AL Next Task/Application
    { 0x0C01A4, 0x09 }, // AL Previous Task/Application
    { 0x0C01A6, 0x3b }, // AL Integrated Help Center
    { 0x0C01A7, 0x6b }, // AL Documents
    { 0x0C01A8, 0x23 }, // AL Thesaurus
    { 0x0C01A9, 0x23 }, // AL Dictionary
    { 0x0C01AB, 0x23 }, // AL Spell Check
    { 0x0C01AC, 0x23 }, // AL Grammar Check
    { 0x0C01B4, 0x05 }, // AL File Browser
    { 0x0C01BD, 0x3b }, // AL OEM Features/ Tips/Tutorial Browser
    { 0x0C01BE, 0x3b }, // AL OEM Help

    { 0x0C0201, 0x3e }, // AC New
    { 0x0C0202, 0x3f }, // AC Open
    { 0x0C0203, 0x40 }, // AC Close
    { 0x0C0204, 0x40 }, // AC Exit
    { 0x0C0207, 0x57 }, // AC Save
    { 0x0C0208, 0x58 }, // AC Print
    { 0x0C021A, 0x08 }, // AC Undo
    { 0x0C021B, 0x18 }, // AC Copy
    { 0x0C021C, 0x17 }, // AC Cut
    { 0x0C021D, 0x0a }, // AC Paste
    { 0x0C021F, 0x65 }, // AC Find
    { 0x0C0220, 0x65 }, // AC Find and Replace
    { 0x0C0221, 0x65 }, // AC Search
    { 0x0C0223, 0x32 }, // AC Home
    { 0x0C0224, 0x6a }, // AC Back
    { 0x0C0225, 0x69 }, // AC Forward
    { 0x0C0226, 0x68 }, // AC Stop
    { 0x0C0227, 0x67 }, // AC Refresh
    { 0x0C0228, 0x6a }, // AC Previous Link
    { 0x0C0229, 0x69 }, // AC Next Link
    { 0x0C022A, 0x66 }, // AC Bookmarks
    { 0x0C022B, 0x66 }, // AC History
    { 0x0C022C, 0x66 }, // AC Subscriptions
    { 0x0C0279, 0x07 }, // AC Redo/Repeat
    { 0x0C0288, 0x43 }, // AC Send To
    { 0x0C0289, 0x41 }, // AC Reply
    { 0x0C028A, 0x41 }, // AC Reply All
    { 0x0C028B, 0x42 }, // AC Forward Msg
    { 0x0C028C, 0x43 }, // AC Send

    { 0x000000, 0x00 }
};
/* \\\ */

/* /// "nDetectDefaultAction()" */
BOOL nDetectDefaultAction(struct NepClassHid *nch,  struct NepHidItem *nhi, struct List *lst, struct NepHidCollection *nhc, ULONG uid)
{
    BOOL res = FALSE;
    UWORD usageid = uid;
    struct NepHidAction *nha;
    struct WacomCaps *wc;
    const struct UsbToPs2Map *utp = usbtops2map;

    switch(uid>>16)
    {
        case 0x01: /* generic desktop page */
            if(usageid == 0xfff0) /* extra init */
            {
                /* AipTek USB Tablets Support */
                IPTR vendid = 0;
                IPTR prodid = 0;
                ULONG command;
                ULONG data;

                psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                            DA_VendorID, &vendid,
                            DA_ProductID, &prodid,
                            TAG_END);
                if((vendid == 0x08ca) &&
                   ((prodid == 0x0001) || (prodid == 0x0010) ||
                    ((prodid >= 0x0020) && (prodid <= 0x0024))) &&
                   (!nhi->nhi_ActionList.lh_Head->ln_Succ))
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Generating special AipTek USB Tablet init sequence.");
                    command = nFindItemUsage(nch, 0x0D003F, REPORT_MAIN_FEATURE);
                    data = nFindItemUsage(nch, 0x0D0040, REPORT_MAIN_FEATURE);
                    if(command && data)
                    {
                        /* set resolution to 500DPI */
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x18;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x04;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }

                        /* set absolute tablet mode */
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x10;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x01;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }

                        /* enable macro keys */
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x11;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x02;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }

                        /* enable auto gain */
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x12;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0xff;
                        }
                        /*if((nha = nAllocAction(nch, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }*/
                    } else {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "AipTek: No command (%ld) or data (%ld) feature!", command, data);
                    }
                }
                if(vendid == 0x056a)
                {
                    for(wc = WacomCapsTable; wc->wc_ProdID != prodid; wc++)
                    {
                        if(wc->wc_ProdID == 0xffff)
                        {
                            wc = NULL;
                            break;
                        }
                    }
                } else {
                    wc = NULL;
                }
                if(wc)
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Generating special Wacom USB Tablet init sequence.");
                    command = nFindItemUsage(nch, 0xff000001, REPORT_MAIN_FEATURE);
                    if(command)
                    {
                        /* activate returning of absolute tablet data */
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x02;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x02;
                        }
                    }
                }

                if((vendid == 0x046d) &&
                   (!nhi->nhi_ActionList.lh_Head->ln_Succ))
                {
                    command = nFindItemUsage(nch, 0xff000001, REPORT_MAIN_OUTPUT);
                    if(command)
                    {
                        psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Adding special HID2HCI Logitech Bluetooth init sequence, but deactivated!");
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "This init sequence would disable HID usage and enable pure Bluetooth use!");
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "If you want to use this dongle for Bluetooth, switch the NOP-Startup-Actions to type Output!");
                        // send three commands to output report 0x10
                        if((nha = nAllocAction(nch, lst, HUA_NOP|HUA_ANY)))
                        {
                            nha->nha_OutItem = command;
                            strcpy(nha->nha_OutArray, "0xff, 0x80, 0x80, 0x01, 0x00, 0x00");
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_NOP|HUA_ANY)))
                        {
                            nha->nha_OutItem = command;
                            strcpy(nha->nha_OutArray, "0xff, 0x80, 0x00, 0x00, 0x30, 0x00");
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_NOP|HUA_ANY)))
                        {
                            nha->nha_OutItem = command;
                            strcpy(nha->nha_OutArray, "0xff, 0x81, 0x80, 0x00, 0x00, 0x00");
                        }
                    }
                }
            }

            if(nFindCollID(nch, nhc, 0x010002, 0x010002) || /* Mice */
               nFindCollID(nch, nhc, 0x0d0001, 0x0d000d)) /* Tablets */
            {
                switch(usageid)
                {
                    case 0x30: /* Mouse X */
                        if((nha = nAllocAction(nch, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
                        {
                            if(nhi->nhi_Flags & RPF_MAIN_RELATIVE)
                            {
                                nha->nha_MouseAxis = HUAT_DELTAX;
                            } else {
                                nha->nha_MouseAxis = HUAT_ABSX;
                                nha->nha_ClipEnable = TRUE;
                                nha->nha_ClipMin = 5;
                                nha->nha_ClipMax = 95;
                                nha->nha_ClipStretch = TRUE;
                            }
                        }
                        break;

                    case 0x31: /* Mouse Y */
                        if((nha = nAllocAction(nch, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
                        {
                            if(nhi->nhi_Flags & RPF_MAIN_RELATIVE)
                            {
                                nha->nha_MouseAxis = HUAT_DELTAY;
                            } else {
                                nha->nha_MouseAxis = HUAT_ABSY;
                                nha->nha_ClipEnable = TRUE;
                                nha->nha_ClipMin = 5;
                                nha->nha_ClipMax = 95;
                                nha->nha_ClipStretch = TRUE;
                            }
                        }
                        break;

                    case 0x38: /* Wheel */
                        if((nha = nAllocAction(nch, lst, HUA_WHEEL|HUA_ALWAYS)))
                        {
                            nha->nha_WheelMode = HUAT_DELTAY;
                            nha->nha_WheelDist = 1;
                        }
                        break;
                }
            }

            if(nFindCollID(nch, nhc, 0x010004, 0x010005)) /* Joystick, Game Pad */
            {
                switch(usageid)
                {
                    case 0x39: /* Hatswitch */
                        if((nha = nAllocAction(nch, lst, HUA_DIGJOY|HUA_NAN)))
                        {
                            nha->nha_JoypadOp = HUAT_CLEAR;
                            nha->nha_JoypadFeat = HUAT_HATSWITCH;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_DIGJOY|HUA_ALWAYS)))
                        {
                            nha->nha_JoypadOp = HUAT_ASSIGN;
                            nha->nha_JoypadFeat = HUAT_HATSWITCH;
                            nha->nha_JoypadPort = 1;
                        }
                        break;

                    case 0x30: /* X axis */
                    case 0x35: /* Z rotation */
                    case 0x31: /* Y axis */
                    case 0x32: /* Z axis */
                    {
                        ULONG minpos = ((nhi->nhi_LogicalMax - nhi->nhi_LogicalMin + 1)>>2) + nhi->nhi_LogicalMin;
                        ULONG maxpos = (((nhi->nhi_LogicalMax - nhi->nhi_LogicalMin + 1)*3)>>2) + nhi->nhi_LogicalMin;
                        UBYTE negaxis;
                        UBYTE posaxis;
                        UBYTE alogaxis;
                        if((usageid == 0x30) || (usageid == 0x35))
                        {
                            negaxis = HUAT_LEFT;
                            posaxis = HUAT_RIGHT;
                            alogaxis = HUAT_ABSX;
                        } else {
                            negaxis = HUAT_UP;
                            posaxis = HUAT_DOWN;
                            alogaxis = HUAT_ABSY;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_GT;
                            nha->nha_CCConst2 = minpos;
                            nha->nha_JoypadOp = HUAT_CLEAR;
                            nha->nha_JoypadFeat = negaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_LE;
                            nha->nha_CCConst2 = minpos;
                            nha->nha_JoypadOp = HUAT_SET;
                            nha->nha_JoypadFeat = negaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_LT;
                            nha->nha_CCConst2 = maxpos;
                            nha->nha_JoypadOp = HUAT_CLEAR;
                            nha->nha_JoypadFeat = posaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_GE;
                            nha->nha_CCConst2 = maxpos;
                            nha->nha_JoypadOp = HUAT_SET;
                            nha->nha_JoypadFeat = posaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_ANALOGJOY|HUA_ANY)))
                        {
                            nha->nha_APadFeat = alogaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        break;
                    }
                }
            }

            if(nFindCollID(nch, nhc, 0x010008, 0x010008) && (nhi->nhi_Flags & RPF_MAIN_RELATIVE)) /* Multi-axis controller */
            {
                switch(usageid)
                {
                    case 0x30: /* Mouse X */
                        if((nha = nAllocAction(nch, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
                        {
                            nha->nha_MouseAxis = HUAT_DELTAX;
                            nha->nha_ClipEnable = TRUE;
                            nha->nha_ClipMin = 10;
                            nha->nha_ClipMax = 45;
                            nha->nha_ClipStretch = TRUE;
                            nha->nha_ScaleEnable = TRUE;
                            nha->nha_ScaleMin = -32;
                            nha->nha_ScaleMax = 0;
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCVar1 = HUAT_OITEMVALUE;
                            nha->nha_CCCond = HUAT_LT;
                            nha->nha_CCConst2 = 0;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
                        {
                            nha->nha_MouseAxis = HUAT_DELTAX;
                            nha->nha_ClipEnable = TRUE;
                            nha->nha_ClipMin = 55;
                            nha->nha_ClipMax = 90;
                            nha->nha_ClipStretch = TRUE;
                            nha->nha_ScaleEnable = TRUE;
                            nha->nha_ScaleMin = 0;
                            nha->nha_ScaleMax = 32;
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCVar1 = HUAT_OITEMVALUE;
                            nha->nha_CCCond = HUAT_GT;
                            nha->nha_CCConst2 = 0;
                        }
                        break;

                    case 0x31: /* Mouse Y */
                        if((nha = nAllocAction(nch, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
                        {
                            nha->nha_MouseAxis = HUAT_DELTAY;
                            nha->nha_ClipEnable = TRUE;
                            nha->nha_ClipMin = 10;
                            nha->nha_ClipMax = 45;
                            nha->nha_ClipStretch = TRUE;
                            nha->nha_ScaleEnable = TRUE;
                            nha->nha_ScaleMin = -32;
                            nha->nha_ScaleMax = 0;
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCVar1 = HUAT_OITEMVALUE;
                            nha->nha_CCCond = HUAT_LT;
                            nha->nha_CCConst2 = 0;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
                        {
                            nha->nha_MouseAxis = HUAT_DELTAY;
                            nha->nha_ClipEnable = TRUE;
                            nha->nha_ClipMin = 55;
                            nha->nha_ClipMax = 90;
                            nha->nha_ClipStretch = TRUE;
                            nha->nha_ScaleEnable = TRUE;
                            nha->nha_ScaleMin = 0;
                            nha->nha_ScaleMax = 32;
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCVar1 = HUAT_OITEMVALUE;
                            nha->nha_CCCond = HUAT_GT;
                            nha->nha_CCConst2 = 0;
                        }
                        break;

                    case 0x32: /* Mouse Z */
                    {
                        LONG minpos = (((nhi->nhi_LogicalMax - nhi->nhi_LogicalMin + 1)*3)>>3) + nhi->nhi_LogicalMin;
                        LONG maxpos = (((nhi->nhi_LogicalMax - nhi->nhi_LogicalMin + 1)*5)>>3) + nhi->nhi_LogicalMin;
                        UWORD led = nFindItemUsage(nch, 0x08004B, REPORT_MAIN_OUTPUT); // blue LED
                        if(maxpos > 0)
                        {
                            if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                            {
                                nha->nha_ButtonMode = HUAT_SET;
                                nha->nha_ButtonNo = 1;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_GT;
                                nha->nha_CCConst2 = maxpos;
                            }
                            if(led && ((nha = nAllocAction(nch, lst, HUA_OUTPUT|HUA_ANY))))
                            {
                                nha->nha_OutOp = HUAT_SET;
                                nha->nha_OutItem = led;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_GT;
                                nha->nha_CCConst2 = maxpos;
                            }
                            if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                            {
                                nha->nha_ButtonMode = HUAT_CLEAR;
                                nha->nha_ButtonNo = 1;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_LT;
                                nha->nha_CCConst2 = maxpos;
                            }
                            if(led && ((nha = nAllocAction(nch, lst, HUA_OUTPUT|HUA_ANY))))
                            {
                                nha->nha_OutOp = HUAT_CLEAR;
                                nha->nha_OutItem = led;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_LT;
                                nha->nha_CCConst2 = maxpos;
                            }
                        }
                        if(minpos < 0)
                        {
                            if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                            {
                                nha->nha_ButtonMode = HUAT_SET;
                                nha->nha_ButtonNo = 2;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_LT;
                                nha->nha_CCConst2 = minpos;
                            }
                            if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                            {
                                nha->nha_ButtonMode = HUAT_CLEAR;
                                nha->nha_ButtonNo = 2;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_GT;
                                nha->nha_CCConst2 = minpos;
                            }
                        }
                        break;
                    }

                    case 0x35: /* Z rotation */
                    {
                        LONG minpos = (((nhi->nhi_LogicalMax - nhi->nhi_LogicalMin + 1)*2)>>3) + nhi->nhi_LogicalMin;
                        LONG maxpos = (((nhi->nhi_LogicalMax - nhi->nhi_LogicalMin + 1)*6)>>3) + nhi->nhi_LogicalMin;
                        if(maxpos > 0)
                        {
                            if((nha = nAllocAction(nch, lst, HUA_WHEEL|HUA_ANY)))
                            {
                                nha->nha_WheelMode = HUAT_DOWN;
                                nha->nha_WheelDist = 1;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_GT;
                                nha->nha_CCConst2 = maxpos;
                            }
                        }
                        if(minpos < 0)
                        {
                            if((nha = nAllocAction(nch, lst, HUA_WHEEL|HUA_ANY)))
                            {
                                nha->nha_WheelMode = HUAT_UP;
                                nha->nha_WheelDist = 1;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_LT;
                                nha->nha_CCConst2 = minpos;
                            }
                        }
                        break;
                    }
                }
            }
            break;

        case 0x09: /* Button page */
            if(nFindCollID(nch, nhc, 0x010002, 0x010002) || /* Mice */
               nFindCollID(nch, nhc, 0x0d0001, 0x0d000d) || /* Tablets */
               nFindCollID(nch, nhc, 0x010008, 0x010008)) /* Multi-Axis controller */
            {
                if(usageid && (usageid < 6))
                {
                    if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                    {
                        nha->nha_ButtonMode = HUAT_ASSIGN;
                        nha->nha_ButtonNo = usageid;
                    }
                }
            }
            if(nFindCollID(nch, nhc, 0x010004, 0x010005)) /* Joystick, Game Pad */
            {
                if(usageid && (usageid < 8))
                {
                    if((nha = nAllocAction(nch, lst, HUA_DIGJOY|HUA_ANY)))
                    {
                        nha->nha_JoypadOp = HUAT_ASSIGN;
                        nha->nha_JoypadFeat = HUAT_RED + usageid - 1;
                        nha->nha_JoypadPort = 1;
                    }
                }
            }
            break;

        case 0x07: /* keyboard page */
            switch(usageid)
            {
                case 0x39: /* Caps lock */
                {
                    UWORD led;
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_DOWNEVENT)))
                    {
                        nha->nha_QualMode = HUAT_TOGGLE;
                        nha->nha_Qualifier = IEQUALIFIERB_CAPSLOCK;
                    }
                    if((led = nFindItemUsage(nch, 0x080002, REPORT_MAIN_OUTPUT)))
                    {
                        if((nha = nAllocAction(nch, lst, HUA_OUTPUT|HUA_DOWNEVENT)))
                        {
                            nha->nha_OutOp = HUAT_SET;
                            nha->nha_OutItem = led;
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCVar1 = HUAT_QUALIFIERS;
                            nha->nha_CCCond = HUAT_BWAND;
                            nha->nha_CCVar2 = HUAT_CONST;
                            nha->nha_CCConst2 = IEQUALIFIER_CAPSLOCK;
                        }
                        if((nha = nAllocAction(nch, lst, HUA_OUTPUT|HUA_DOWNEVENT)))
                        {
                            nha->nha_OutOp = HUAT_CLEAR;
                            nha->nha_OutItem = led;
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCVar1 = HUAT_QUALIFIERS;
                            nha->nha_CCCond = HUAT_BWNAND;
                            nha->nha_CCVar2 = HUAT_CONST;
                            nha->nha_CCConst2 = IEQUALIFIER_CAPSLOCK;
                        }
                    }
                    break;
                }

                case 0xe0: /* Left control */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_CONTROL;
                    }
                    break;

                case 0xe1: /* Left shift */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_LSHIFT;
                    }
                    break;

                case 0xe2: /* Left alt */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_LALT;
                    }
                    break;

                case 0xe3: /* Left GUI */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_LCOMMAND;
                    }
                    break;

                case 0xe4: /* Right control */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_CONTROL;
                    }
                    break;

                case 0xe5: /* Right shift */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_RSHIFT;
                    }
                    break;

                case 0xe6: /* Right alt */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_RALT;
                    }
                    break;

                case 0x45: /* F12 */
                case 0x65: /* Application */
                case 0xe7: /* Right GUI */
                    if((nha = nAllocAction(nch, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_RCOMMAND;
                    }
                    break;
            }
            nha = nAllocAction(nch, lst, HUA_KEYMAP|HUA_ANY);
            break;

        case 0x0d: /* digitizer/tablet page */
            switch(usageid)
            {
                case 0x30: /* Tip pressure */
                case 0x31: /* Barrel pressure */
                    if((nha = nAllocAction(nch, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_PRESSURE;
                    }
                    break;

                case 0x32: /* In Range */
                case 0x37: /* Data Valid */
                    if((nha = nAllocAction(nch, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_PROX;
                    }
                    break;

                case 0x3D: /* X Tilt */
                    if((nha = nAllocAction(nch, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_XROT;
                    }
                    break;

                case 0x3E: /* Y Tilt */
                    if((nha = nAllocAction(nch, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_YROT;
                    }
                    break;

                case 0x42: /* Tip switch */
                    if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                    {
                        nha->nha_ButtonMode = HUAT_ASSIGN;
                        nha->nha_ButtonNo = 1;
                    }
                    break;

                case 0x44: /* Barrel switch */
                    if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                    {
                        nha->nha_ButtonMode = HUAT_ASSIGN;
                        nha->nha_ButtonNo = 2;
                    }
                    break;

                case 0x46: /* Tablet pick */
                    if((nha = nAllocAction(nch, lst, HUA_BUTTONS|HUA_ANY)))
                    {
                        nha->nha_ButtonMode = HUAT_ASSIGN;
                        nha->nha_ButtonNo = 3;
                    }
                    break;
            }
            break;
    }
    while(utp->utp_UsageID && (utp->utp_UsageID <= uid))
    {
        if(utp->utp_UsageID == uid)
        {
            if((nha = nAllocAction(nch, lst, HUA_EXTRAWKEY|HUA_DOWNEVENT)))
            {
                nha->nha_RawKey = utp->utp_ExtCode;
            }
            if((nha = nAllocAction(nch, lst, HUA_EXTRAWKEY|HUA_UPEVENT)))
            {
                nha->nha_RawKey = utp->utp_ExtCode|IECODE_UP_PREFIX;
            }
        }
        utp++;
    }
    return(res);
}
/* \\\ */

/* /// "nCheckForDefaultAction()" */
BOOL nCheckForDefaultAction(struct NepClassHid *nch,  struct NepHidItem *nhi, struct List *lst, struct NepHidCollection *nhc, ULONG uid)
{
    BOOL res = TRUE;
    struct NepHidAction *nha;
    struct NepHidAction *nhadef;
    struct List tmplist;

    NewList(&tmplist);
    nDetectDefaultAction(nch, nhi, &tmplist, nhc, nhi->nhi_Usage);
    nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
    nhadef = (struct NepHidAction *) tmplist.lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        if(nhadef->nha_Node.ln_Succ)
        {
            nhadef->nha_IsDefault = nha->nha_IsDefault;
            if(memcmp(&nha->nha_Type, &nhadef->nha_Type, sizeof(struct NepHidAction)-offsetof(struct NepHidAction, nha_Type)))
            {
                nha->nha_IsDefault = FALSE;
                res = FALSE;
            } else {
                nha->nha_IsDefault = TRUE;
            }
            nhadef = (struct NepHidAction *) nhadef->nha_Node.ln_Succ;
        } else {
            res = FALSE;
            nha->nha_IsDefault = FALSE;
        }
        nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
    }
    nhadef = (struct NepHidAction *) tmplist.lh_Head;
    while(nhadef->nha_Node.ln_Succ)
    {
        Remove(&nhadef->nha_Node);
        psdFreeVec(nhadef);
        nhadef = (struct NepHidAction *) tmplist.lh_Head;
    }
    return(res);
}
/* \\\ */

/* /// "nAllocAction()" */
struct NepHidAction * nAllocAction(struct NepClassHid *nch, struct List *lst, UWORD utype)
{
    struct NepHidAction *nha = psdAllocVec(sizeof(struct NepHidAction));
    if(nha)
    {
        KPRINTF(1, ("New action %lx\n", nha));
        nha->nha_Type = utype;
        nha->nha_ButtonNo = 1;
        nha->nha_SoundVolume = 64;
        nha->nha_ClipMax = 100;
        nha->nha_CCVar1 = HUAT_EITEMVALUE;
        nha->nha_CCVar2 = HUAT_CONST;
        AddTail(lst, &nha->nha_Node);
        return(nha);
    }
    return(NULL);
}
/* \\\ */

/* /// "nAddUsage()" */
BOOL nAddUsage(struct NepClassHid *nch, struct List *list, ULONG umin, ULONG umax)
{
    struct NepHidUsage *nhu;
    if(umin > umax)
    {
        KPRINTF(10, ("UsageMin %lx > UsageMax %lx\n", umin, umax));
        return(FALSE);
    }
    if((nhu = psdAllocVec(sizeof(struct NepHidUsage))))
    {
        AddTail(list, &nhu->nhu_Node);
        nhu->nhu_Usage = umin;
        nhu->nhu_UsageMax = umax;
        return(TRUE);
    } else {
        KPRINTF(10, ("Out of memory during usage allocation.\n"));
    }
    return(FALSE);
}
/* \\\ */

/* /// "nGetUsageName()" */
STRPTR nGetUsageName(struct NepClassHid *nch, ULONG uid)
{
    STRPTR uname;

    uname = nNumToStr(nch, NTS_USAGEID, uid, NULL);
    if(uname)
    {
        return(psdCopyStr(uname));
    }
    uname = nNumToStr(nch, NTS_USAGEPAGE, uid>>16, "unknown");
    return(psdCopyStrFmt("%s (0x%lx)", uname, uid));
}
/* \\\ */

/* /// "nGenerateOutReport()" */
void nGenerateOutReport(struct NepClassHid *nch, struct NepHidReport *nhr, UBYTE *buf)
{
    struct NepHidItem **nhiptr = nhr->nhr_OutItemMap;
    ULONG cnt;

    for(cnt = 0; cnt < nhr->nhr_OutItemCount; cnt++)
    {
        nEncodeItemBuffer(nch, *nhiptr++, buf);
    }
}
/* \\\ */

/* /// "nGenerateFeatReport()" */
void nGenerateFeatReport(struct NepClassHid *nch, struct NepHidReport *nhr, UBYTE *buf)
{
    struct NepHidItem **nhiptr = nhr->nhr_FeatItemMap;
    ULONG cnt;

    for(cnt = 0; cnt < nhr->nhr_FeatItemCount; cnt++)
    {
        nEncodeItemBuffer(nch, *nhiptr++, buf);
    }
}
/* \\\ */

/* /// "nEncodeItemBuffer()" */
void nEncodeItemBuffer(struct NepClassHid *nch, struct NepHidItem *nhi, UBYTE *buf)
{
    LONG value;
    LONG *valptr;
    ULONG ioffset;
    ULONG isize;
    UWORD count;
    UWORD acount;

    ioffset = nhi->nhi_Offset;
    isize = nhi->nhi_Size;
    if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
    {
        //KPRINTF(1, ("Processing var %08lx (%ld@%ld)...\n", nhi, isize, ioffset));
        value = nhi->nhi_OldValue;
        if(value < nhi->nhi_LogicalMin)
        {
            value = nhi->nhi_LogicalMin;
        }
        else if(value > nhi->nhi_LogicalMax)
        {
            value = nhi->nhi_LogicalMax;
        }
        if(((ioffset & 7) == 0) && (isize == 8))
        {
            buf[ioffset>>3] = value;
        }
        else if(isize == 1)
        {
            buf[ioffset>>3] &= ~(1UL<<(ioffset & 7));
            if(value)
            {
                buf[ioffset>>3] |= (1UL<<(ioffset & 7));
            }
        } else {
            /* I know this routine is dead slow, but hopefully, this is only a very uncommon case */
            count = isize;
            ioffset += count;
            do
            {
                ioffset--;
                buf[ioffset>>3] &= ~(1UL<<(ioffset & 7));
                if(value & 1)
                {
                    buf[ioffset>>3] |= (1UL<<(ioffset & 7));
                }
                value >>= 1;
            } while(--count);
        }
    } else {
        acount = nhi->nhi_Count;
        KPRINTF(1, ("Processing arr %08lx (%ld@%ld) * %ld...\n", nhi, isize, ioffset, acount));
        /* first generate the values */
        valptr = nhi->nhi_Buffer;
        if(((ioffset & 7) == 0) && (isize == 8))
        {
            count = ioffset>>3;
            do
            {
                buf[count++] = *valptr++;
            } while(--acount);
        }
        else if(isize == 1)
        {
            do
            {
                buf[ioffset>>3] &= ~(1UL<<(ioffset & 7));
                if(*valptr++)
                {
                    buf[ioffset>>3] |= (1UL<<(ioffset & 7));
                }
                ioffset++;
            } while(--acount);
        } else {
            /* I know this routine is dead slow, but hopefully, this is only a very uncommon case */
            do
            {
                count = isize;
                ioffset += count;
                value = *valptr++;
                do
                {
                    ioffset--;
                    buf[ioffset>>3] &= ~(1UL<<(ioffset & 7));
                    if(value & 1)
                    {
                        buf[ioffset>>3] |= (1UL<<(ioffset & 7));
                    }
                    value >>= 1;
                } while(--count);
                ioffset += count;
            } while(--acount);
        }
    }
}
/* \\\ */

/* /// "nParseArrayString()" */
void nParseArrayString(struct NepClassHid *nch, struct NepHidItem *nhi, STRPTR str)
{
    UWORD acount;
    LONG *valptr = nhi->nhi_Buffer;
    UWORD mode = 0;
    BOOL issigned = FALSE;

    if(!*str)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Empty array value string not allowed.");
        return;
    }
    acount = nhi->nhi_Count;
    if(!acount)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Target array has no space!");
    }

    // mode == 0 -> start
    // mode == 1 -> decimal value
    // mode == 2 -> hex value
    // mode == 3 -> ascii character
    // mode == 4 -> string
    // mode == 5 -> waiting for termination

    while(*str && acount)
    {
        UBYTE ch = *str++;
        switch(mode)
        {
            case 0: // normal
                if((ch == '0') && (*str == 'x'))
                {
                    mode = 2;
                    *valptr = 0;
                    str++;
                    break;
                }
                if((ch >= '0') && (ch <= '9'))
                {
                    mode = 1;
                    issigned = FALSE;
                    *valptr = ch - '0';
                    break;
                }
                else if(ch == '-')
                {
                    mode = 1;
                    issigned = TRUE;
                    *valptr = 0;
                }
                else if(ch == '\'')
                {
                    mode = 3;
                    break;
                }
                else if(ch == '"')
                {
                    mode = 4;
                    break;
                }
                else if(ch == ' ')
                {
                    break;
                }
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S0!");
                return;

            case 1: // decimal value
                if((ch >= '0') && (ch <= '9'))
                {
                    *valptr *= 10;
                    if(issigned)
                    {
                        *valptr -= ch - '0';
                    } else {
                        *valptr += ch - '0';
                    }
                    break;
                }
                else if(ch == ' ')
                {
                    mode = 5;
                    break;
                }
                else if(ch == ',')
                {
                    mode = 0;
                    valptr++;
                    acount--;
                    break;
                }
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S1!");
                return;

            case 2: // hex value
                if((ch >= '0') && (ch <= '9'))
                {
                    *valptr <<= 4;
                    *valptr += ch - '0';
                    break;
                }
                else if((ch >= 'a') && (ch <= 'f'))
                {
                    *valptr <<= 4;
                    *valptr += ch - 'a' + 10;
                    break;
                }
                else if((ch >= 'A') && (ch <= 'F'))
                {
                    *valptr <<= 4;
                    *valptr += ch - 'A' + 10;
                    break;
                }
                else if(ch == ',')
                {
                    mode = 0;
                    valptr++;
                    acount--;
                    break;
                }
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S2!");
                return;

            case 3: // ascii character
                if(*str == '\'')
                {
                    *valptr = ch;
                    str++;
                    mode = 5;
                    break;
                }
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S3!");
                return;

            case 4: // string
                if(ch == '"')
                {
                    mode = 5;
                    break;
                }
                *valptr++ = ch;
                acount--;
                break;

            case 5: // waiting for termination
                if(ch == ' ')
                {
                    break;
                }
                if(ch == ',')
                {
                    valptr++;
                    acount--;
                    mode = 0;
                    break;
                }
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S5!");
                return;
        }
    }
    if(!(*str))
    {
        if(mode == 3)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S3!");
            return;
        }
        if(mode == 4)
        {
            if(*str == '"')
            {
                return;
            }
        }
    }
    if(acount == 0)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Excessive elements ignored!");
    }
}
/* \\\ */

/* /// "nProcessItem()" */
BOOL nProcessItem(struct NepClassHid *nch, struct NepHidItem *nhi, UBYTE *buf)
{
    BOOL res = FALSE;
    LONG value;
    LONG *valptr;
    ULONG ioffset = nhi->nhi_Offset;
    ULONG isize = nhi->nhi_Size;
    UBYTE *bobuf = &buf[ioffset>>3];
    UWORD count;
    UWORD acount;
    ULONG usage;
    BOOL found;
    BOOL bytealigned = ((ioffset & 7) == 0);
    struct NepHidAction *nha;

    if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
    {
        //KPRINTF(1, ("Processing var %08lx (%ld@%ld)...\n", nhi, isize, ioffset));
        if(bytealigned && (isize == 8))
        {
            if(nhi->nhi_IsSigned)
            {
                value = *((BYTE *) bobuf);
            } else {
                value = *bobuf;
            }
        }
        else if(isize == 1)
        {
            value = (buf[ioffset>>3]>>(ioffset & 7)) & 1;
        }
        else if(bytealigned && (isize == 16))
        {
            if(nhi->nhi_IsSigned)
            {
                value = (*bobuf)|((((BYTE *) bobuf)[1])<<8);
            } else {
                value = (*bobuf)|(bobuf[1]<<8);
            }
        }
        else if(bytealigned && (isize == 32))
        {
            value = bobuf[0]|(bobuf[1]<<8)|(bobuf[2]<<16)|(bobuf[3]<<24);
        } else {
            /* I know this routine is dead slow, but hopefully, this is only a very uncommon case */
            count = isize;
            value = 0;
            ioffset += count;
            if(nhi->nhi_IsSigned)
            {
                count--;
                ioffset--;
                if((buf[ioffset>>3]>>(ioffset & 7)) & 1)
                {
                    value = -1;
                }
            }
            do
            {
                ioffset--;
                value <<= 1;
                value |= (buf[ioffset>>3]>>(ioffset & 7)) & 1;
            } while(--count);
        }
        if((value >= nhi->nhi_LogicalMin) && (value <= nhi->nhi_LogicalMax))
        {
            ULONG currmicros = 0;
            ULONG currsecs = 0;
            if(nhi->nhi_OldValue != value)
            {
                if((nhi->nhi_Usage > 0x070000) && (nhi->nhi_Usage < 0x0700E8))
                {
                    nch->nch_LastUSBKey = nhi->nhi_Usage;
                }
                nch->nch_LastItem = nhi;
                nch->nch_LastItemAList = &nhi->nhi_ActionList;
            }

            if(nch->nch_HIntBase)
            {
                CurrentTime(&currsecs, &currmicros);
                if(!DoubleClick(nhi->nhi_LastSeconds, nhi->nhi_LastMicros, currsecs, currmicros))
                {
                    nhi->nhi_ClickCount = 0;
                }
                if(nhi->nhi_OldValue < value)
                {
                    if(nhi->nhi_LastSeconds || nhi->nhi_LastMicros)
                    {
                        if(DoubleClick(nhi->nhi_LastSeconds, nhi->nhi_LastMicros, currsecs, currmicros))
                        {
                            nhi->nhi_ClickCount++;
                        } else {
                            nhi->nhi_ClickCount = 1;
                        }
                    } else {
                        nhi->nhi_ClickCount = 1;
                    }
                    nhi->nhi_HoldSeconds = nhi->nhi_LastSeconds = currsecs;
                    nhi->nhi_HoldMicros = nhi->nhi_LastMicros = currmicros;
                }
            } else {
                /* no intuition library */
                nhi->nhi_ClickCount = 0;
                if(nhi->nhi_OldValue < value)
                {
                    nhi->nhi_ClickCount = 1;
                }
            }
            nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                if((nhi->nhi_OldValue != value) || ((nha->nha_Type & HUA_TRIGMASK) == HUA_ALWAYS))
                {
                    nDoAction(nch, nha, nhi, nhi->nhi_Usage, value, (nhi->nhi_OldValue < value) || ((nha->nha_Type & HUA_TRIGMASK) == HUA_ALWAYS));
                }
                nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
            }
            if(nhi->nhi_OldValue > value)
            {
                nhi->nhi_HoldSeconds = 0;
                nhi->nhi_HoldMicros = 0;
            }
            if((nhi->nhi_OldValue != value) && nch->nch_ReportValues &&
                nch->nch_GUICurrentItem && (nch->nch_GUICurrentItem->nhgi_Item == nhi))
            {
                nch->nch_ItemChanged = TRUE;
            }
            nhi->nhi_OldValue = value;
            res = TRUE;
        } else {
            nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                if((nha->nha_Type & HUA_TRIGMASK) == HUA_NAN)
                {
                    nDoAction(nch, nha, nhi, nhi->nhi_Usage, nhi->nhi_OldValue, 0);
                }
                nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
            }
        }
    } else {
        nhi->nhi_HoldSeconds = nhi->nhi_LastSeconds = 0;
        nhi->nhi_HoldMicros = nhi->nhi_LastMicros = 0;
        acount = nhi->nhi_Count;
        KPRINTF(1, ("Processing arr %08lx (%ld@%ld) * %ld...\n", nhi, isize, ioffset, acount));
        /* first generate the values */
        valptr = nhi->nhi_Buffer;
        if(bytealigned && (isize == 8))
        {
            count = ioffset>>3;
            if(nhi->nhi_IsSigned)
            {
                do
                {
                    *valptr++ = ((BYTE *) buf)[count++];
                } while(--acount);
            } else {
                do
                {
                    *valptr++ = buf[count++];
                } while(--acount);
            }
        }
        else if(isize == 1)
        {
            do
            {
                *valptr++ = (buf[ioffset>>3]>>(ioffset & 7)) & 1;
                ioffset++;
            } while(--acount);
        } else {
            /* I know this routine is dead slow, but hopefully, this is only a very uncommon case */
            do
            {
                count = isize;
                value = 0;
                ioffset += count;
                if(nhi->nhi_IsSigned)
                {
                    ioffset--;
                    count--;
                    if((buf[ioffset>>3]>>(ioffset & 7)) & 1)
                    {
                        value = -1;
                    }
                }
                do
                {
                    ioffset--;
                    value <<= 1;
                    value |= (buf[ioffset>>3]>>(ioffset & 7)) & 1;
                } while(--count);
                ioffset += isize;
                *valptr++ = value;
            } while(--acount);
        }

        /* Look for up events first */
        acount = 0;
        do
        {
            value = nhi->nhi_OldBuffer[acount];
            if((value >= nhi->nhi_LogicalMin) && (value <= nhi->nhi_LogicalMax))
            {
                valptr = nhi->nhi_Buffer;
                count = nhi->nhi_Count;
                found = FALSE;
                do
                {
                    if(*valptr++ == value)
                    {
                        found = TRUE;
                        break;
                    }
                } while(--count);
                /* Not found in the new array, so generate up event */
                if(!found)
                {
                    KPRINTF(1, ("Up event %ld\n", acount));
                    usage = nhi->nhi_UsageMap[value - nhi->nhi_LogicalMin];
                    if(value || nhi->nhi_LogicalMin)
                    {
                        nch->nch_LastItem = nhi;
                        nch->nch_LastItemAList = &nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin];
                    }
                    nha = (struct NepHidAction *) nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin].lh_Head;
                    if(!nha->nha_Node.ln_Succ) /* Use default, no override defined */
                    {
                        nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
                    }
                    nhi->nhi_OldValue = nhi->nhi_LogicalMin - 1;
                    while(nha->nha_Node.ln_Succ)
                    {
                        nDoAction(nch, nha, nhi, usage, value, FALSE);
                        nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
                    }
                    res = TRUE;
                }
            }
        } while(++acount < nhi->nhi_Count);

        /* Look for down events */
        acount = 0;
        do
        {
            value = nhi->nhi_Buffer[acount];
            //KPRINTF(1, ("[%ld] = %ld\n", acount, value));
            if((value >= nhi->nhi_LogicalMin) && (value <= nhi->nhi_LogicalMax))
            {
                valptr = nhi->nhi_OldBuffer;
                count = nhi->nhi_Count;
                found = FALSE;
                do
                {
                    if(*valptr++ == value)
                    {
                        found = TRUE;
                        break;
                    }
                } while(--count);
                /* Not found in the old array, so generate down event */
                if(!found)
                {
                    KPRINTF(1, ("Down event %ld\n", acount));
                    usage = nhi->nhi_UsageMap[value - nhi->nhi_LogicalMin];
                    if(value || nhi->nhi_LogicalMin)
                    {
                        if((usage > 0x070000) && (usage < 0x0700E8))
                        {
                            nch->nch_LastUSBKey = usage;
                        }
                        nch->nch_LastItem = nhi;
                        nch->nch_LastItemAList = &nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin];
                    }

                    nha = (struct NepHidAction *) nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin].lh_Head;
                    if(!nha->nha_Node.ln_Succ) /* Use default, no override defined */
                    {
                        nha = (struct NepHidAction *) nhi->nhi_ActionList.lh_Head;
                    }
                    nhi->nhi_OldValue = nhi->nhi_LogicalMin - 1;
                    while(nha->nha_Node.ln_Succ)
                    {
                        nDoAction(nch, nha, nhi, usage, value, TRUE);
                        nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
                    }
                    res = TRUE;
                }
            }
        } while(++acount < nhi->nhi_Count);
        /* Copy new array */
        CopyMemQuick(nhi->nhi_Buffer, nhi->nhi_OldBuffer, sizeof(LONG) * nhi->nhi_Count);
    }
    return(res);
}
/* \\\ */

static ULONG LLHatswitchEncoding[8] = { JPF_JOY_UP, JPF_JOY_UP|JPF_JOY_RIGHT, JPF_JOY_RIGHT, JPF_JOY_RIGHT|JPF_JOY_DOWN,
                                        JPF_JOY_DOWN, JPF_JOY_DOWN|JPF_JOY_LEFT, JPF_JOY_LEFT, JPF_JOY_LEFT|JPF_JOY_UP };

/* /// "nDoAction()" */
BOOL nDoAction(struct NepClassHid *nch, struct NepHidAction *nha, struct NepHidItem *nhi, ULONG uid, LONG value, BOOL downevent)
{
    UWORD atype = nha->nha_Type & HUA_ATYPEMASK;
    BOOL res = FALSE;
    ULONG qualmask;
    LONG clipmin, clipmax;
    LONG origvalue = value;

    if(nch->nch_DisableActions)
    {
        return(FALSE);
    }

    // bail out early as nothing below will affect this
    if(!(((nha->nha_Type & HUA_DOWNEVENT) && downevent) ||
       ((nha->nha_Type & HUA_UPEVENT) && !downevent) ||
       ((nha->nha_Type & HUA_TRIGMASK) == HUA_ALWAYS) ||
       ((nha->nha_Type & HUA_TRIGMASK) == HUA_NAN)))
    {
        return(FALSE);
    }

    if(nha->nha_AbsToRel)
    {
        value = value - nhi->nhi_OldValue;
        nhi->nhi_RealMin = nhi->nhi_LogicalMin - nhi->nhi_LogicalMax;
        nhi->nhi_RealMax = nhi->nhi_LogicalMax - nhi->nhi_LogicalMin;
    } else {
        nhi->nhi_RealMin = nhi->nhi_LogicalMin;
        nhi->nhi_RealMax = nhi->nhi_LogicalMax;
    }

    if(nha->nha_ClipEnable)
    {
        if(nha->nha_ClipMin == nha->nha_ClipMax)
        {
            /* shrink to a point */
            value = ((nha->nha_ClipMin * (nhi->nhi_RealMax - nhi->nhi_RealMin)) / 100) + nhi->nhi_RealMin;
        } else {
            if(nha->nha_ClipMin < nha->nha_ClipMax)
            {
                /* normal clipping */
                clipmin = ((nha->nha_ClipMin * (nhi->nhi_RealMax - nhi->nhi_RealMin)) / 100) + nhi->nhi_RealMin;
                clipmax = ((nha->nha_ClipMax * (nhi->nhi_RealMax - nhi->nhi_RealMin)) / 100) + nhi->nhi_RealMin;
            } else {
                /* inverse clipping */
                value = nhi->nhi_RealMax - value;
                clipmin = ((nha->nha_ClipMax * (nhi->nhi_RealMax - nhi->nhi_RealMin)) / 100) + nhi->nhi_RealMin;
                clipmax = ((nha->nha_ClipMin * (nhi->nhi_RealMax - nhi->nhi_RealMin)) / 100) + nhi->nhi_RealMin;
            }
            if(value < clipmin)
            {
                value = clipmin;
            } else {
                if(value > clipmax)
                {
                    value = clipmax;
                }
            }
            if(nha->nha_ClipStretch && (clipmax - clipmin))
            {
                value = (((value - clipmin) * (nhi->nhi_RealMax - nhi->nhi_RealMin)) /
                         (clipmax - clipmin)) + nhi->nhi_RealMin;
            }
        }
    }

    if(nha->nha_ScaleEnable && (nha->nha_ScaleMax != nha->nha_ScaleMin))
    {
        value = (((value - nhi->nhi_RealMin) * (nha->nha_ScaleMax - nha->nha_ScaleMin)) /
                  (nhi->nhi_RealMax - nhi->nhi_RealMin)) + nha->nha_ScaleMin;
        nhi->nhi_RealMin = nha->nha_ScaleMin;
        nhi->nhi_RealMax = nha->nha_ScaleMax;
    }

    if(nha->nha_CCEnable)
    {
        LONG var1 = 0;
        LONG var2 = 0;
        BOOL cond = FALSE;

        switch(nha->nha_CCVar1)
        {
            case HUAT_EITEMVALUE:
                var1 = value;
                break;

            case HUAT_OITEMVALUE:
                var1 = origvalue;
                break;

            case HUAT_CONST:
                var1 = nha->nha_CCConst1;
                break;

            case HUAT_CLICKCOUNT:
                var1 = nhi->nhi_ClickCount;
                break;

            case HUAT_CLICKTIME:
            {
                ULONG currmicros;
                ULONG currsecs;

                if(nhi->nhi_HoldSeconds || nhi->nhi_HoldMicros)
                {
                    CurrentTime(&currsecs, &currmicros);
                    var1 = (currsecs - nhi->nhi_HoldSeconds) * 1000;
                    if(nhi->nhi_HoldMicros <= currmicros)
                    {
                        var1 += (currmicros - nhi->nhi_HoldMicros) / 1000;
                    } else {
                        var1 -= (nhi->nhi_HoldMicros - currmicros) / 1000;
                    }
                } else {
                    var1 = 0;
                }
                break;
            }

            case HUAT_QUALIFIERS:
                var1 = nch->nch_KeyQualifiers;
                break;

            case HUAT_ALLQUAL:
                var1 = nch->nch_KeyQualifiers|PeekQualifier();
                break;

            case HUAT_RANDOMBIT:
                var1 = _rand(nch->nch_ClsBase) & 1;
                break;

            case HUAT_RANDOMVAL:
                var1 = _rand(nch->nch_ClsBase);
                break;

            case HUAT_TIMER:
            {
                ULONG currmicros;
                ULONG currsecs;

                CurrentTime(&currsecs, &currmicros);
                var1 = (currsecs * 1000) + (currmicros / 1000);
                break;
            }

            case HUAT_LOCALVAR1:
            case HUAT_LOCALVAR2:
            case HUAT_LOCALVAR3:
            case HUAT_LOCALVAR4:
            case HUAT_LOCALVAR5:
            case HUAT_LOCALVAR6:
            case HUAT_LOCALVAR7:
            case HUAT_LOCALVAR8:
                var1 = nch->nch_LocalVars[nha->nha_CCVar1 - HUAT_LOCALVAR1];
                break;

            case HUAT_GLOBVARA:
            case HUAT_GLOBVARB:
            case HUAT_GLOBVARC:
            case HUAT_GLOBVARD:
            case HUAT_GLOBVARE:
            case HUAT_GLOBVARF:
            case HUAT_GLOBVARG:
            case HUAT_GLOBVARH:
                var1 = nch->nch_ClsBase->nh_GlobalVars[nha->nha_CCVar1 - HUAT_GLOBVARA];
                break;
        }
        switch(nha->nha_CCVar2)
        {
            case HUAT_EITEMVALUE:
                var2 = value;
                break;

            case HUAT_OITEMVALUE:
                var2 = origvalue;
                break;

            case HUAT_CONST:
                var2 = nha->nha_CCConst2;
                break;

            case HUAT_CLICKCOUNT:
                var2 = nhi->nhi_ClickCount;
                break;

            case HUAT_CLICKTIME:
            {
                ULONG currmicros;
                ULONG currsecs;

                if(nhi->nhi_HoldSeconds || nhi->nhi_HoldMicros)
                {
                    CurrentTime(&currsecs, &currmicros);
                    var2 = (currsecs - nhi->nhi_HoldSeconds) * 1000;
                    if(nhi->nhi_HoldMicros <= currmicros)
                    {
                        var2 += (currmicros - nhi->nhi_HoldMicros) / 1000;
                    } else {
                        var2 -= (nhi->nhi_HoldMicros - currmicros) / 1000;
                    }
                } else {
                    var2 = 0;
                }
                break;
            }

            case HUAT_QUALIFIERS:
                var2 = nch->nch_KeyQualifiers;
                break;

            case HUAT_ALLQUAL:
                var2 = nch->nch_KeyQualifiers|PeekQualifier();
                break;

            case HUAT_RANDOMBIT:
                var2 = _rand(nch->nch_ClsBase) & 1;
                break;

            case HUAT_RANDOMVAL:
                var2 = _rand(nch->nch_ClsBase);
                break;

            case HUAT_TIMER:
            {
                ULONG currmicros;
                ULONG currsecs;

                CurrentTime(&currsecs, &currmicros);
                var2 = (currsecs * 1000) + (currmicros / 1000);
                break;
            }

            case HUAT_LOCALVAR1:
            case HUAT_LOCALVAR2:
            case HUAT_LOCALVAR3:
            case HUAT_LOCALVAR4:
            case HUAT_LOCALVAR5:
            case HUAT_LOCALVAR6:
            case HUAT_LOCALVAR7:
            case HUAT_LOCALVAR8:
                var2 = nch->nch_LocalVars[nha->nha_CCVar2 - HUAT_LOCALVAR1];
                break;

            case HUAT_GLOBVARA:
            case HUAT_GLOBVARB:
            case HUAT_GLOBVARC:
            case HUAT_GLOBVARD:
            case HUAT_GLOBVARE:
            case HUAT_GLOBVARF:
            case HUAT_GLOBVARG:
            case HUAT_GLOBVARH:
                var2 = nch->nch_ClsBase->nh_GlobalVars[nha->nha_CCVar2 - HUAT_GLOBVARA];
                break;
        }
        switch(nha->nha_CCCond)
        {
            case HUAT_EQ:
                cond = (var1 == var2);
                break;

            case HUAT_NE:
                cond = (var1 != var2);
                break;

            case HUAT_LT:
                cond = (var1 < var2);
                break;

            case HUAT_LE:
                cond = (var1 <= var2);
                break;

            case HUAT_GT:
                cond = (var1 > var2);
                break;

            case HUAT_GE:
                cond = (var1 >= var2);
                break;

            case HUAT_AND:
                cond = (var1 && var2);
                break;

            case HUAT_NAND:
                cond = !(var1 && var2);
                break;

            case HUAT_OR:
                cond = (var1 || var2);
                break;

            case HUAT_XOR:
                cond = (var1 && (!var2)) || ((!var1) && var2);
                break;

            case HUAT_ANDNOT:
                cond = (var1 && (!var2));
                break;

            case HUAT_BWAND:
                cond = (var1 & var2) ? TRUE : FALSE;
                break;

            case HUAT_BWNAND:
                cond = (var1 & var2) ? FALSE : TRUE;
                break;

            case HUAT_BWOR:
                cond = (var1 | var2) ? TRUE : FALSE;
                break;

            case HUAT_BWXOR:
                cond = (var1 ^ var2) ? TRUE : FALSE;
                break;

            case HUAT_BWANDNOT:
                cond = (var1 & (~var2)) ? TRUE : FALSE;
                break;

        }
        if(!cond)
        {
            return(FALSE);
        }
    }

    if(nha->nha_ValEnable)
    {
        switch(nha->nha_ValVar)
        {
            case HUAT_EITEMVALUE:
                //value = value;
                break;

            case HUAT_OITEMVALUE:
                value = origvalue;
                break;

            case HUAT_CONST:
                value = nha->nha_ValConst;
                break;

            case HUAT_CLICKCOUNT:
                value = nhi->nhi_ClickCount;
                break;

            case HUAT_CLICKTIME:
            {
                ULONG currmicros;
                ULONG currsecs;

                if(nhi->nhi_HoldSeconds || nhi->nhi_HoldMicros)
                {
                    CurrentTime(&currsecs, &currmicros);
                    value = (currsecs - nhi->nhi_HoldSeconds) * 1000;
                    if(nhi->nhi_HoldMicros <= currmicros)
                    {
                        value += (currmicros - nhi->nhi_HoldMicros) / 1000;
                    } else {
                        value -= (nhi->nhi_HoldMicros - currmicros) / 1000;
                    }
                } else {
                    value = 0;
                }
                break;
            }

            case HUAT_QUALIFIERS:
                value = nch->nch_KeyQualifiers;
                break;

            case HUAT_ALLQUAL:
                value = nch->nch_KeyQualifiers|PeekQualifier();
                break;

            case HUAT_RANDOMBIT:
                value = _rand(nch->nch_ClsBase) & 1;
                break;

            case HUAT_RANDOMVAL:
                value = _rand(nch->nch_ClsBase);
                break;

            case HUAT_TIMER:
            {
                ULONG currmicros;
                ULONG currsecs;

                CurrentTime(&currsecs, &currmicros);
                value = (currsecs * 1000) + (currmicros / 1000);
                break;
            }

            case HUAT_LOCALVAR1:
            case HUAT_LOCALVAR2:
            case HUAT_LOCALVAR3:
            case HUAT_LOCALVAR4:
            case HUAT_LOCALVAR5:
            case HUAT_LOCALVAR6:
            case HUAT_LOCALVAR7:
            case HUAT_LOCALVAR8:
                value = nch->nch_LocalVars[nha->nha_ValVar - HUAT_LOCALVAR1];
                break;

            case HUAT_GLOBVARA:
            case HUAT_GLOBVARB:
            case HUAT_GLOBVARC:
            case HUAT_GLOBVARD:
            case HUAT_GLOBVARE:
            case HUAT_GLOBVARF:
            case HUAT_GLOBVARG:
            case HUAT_GLOBVARH:
                value = nch->nch_ClsBase->nh_GlobalVars[nha->nha_ValVar - HUAT_GLOBVARA];
                break;
        }
    }
    //KPRINTF(1, ("Call for action(%04lx) %lx = %ld %s\n", atype, uid, value, downevent ? "down" : "up"));
    switch(atype)
    {
        case HUA_SHELL:
        case HUA_KEYSTRING:
        case HUA_VANILLA:
        case HUA_SOUND:
        {
            struct ActionMsg *am;
            if(!nch->nch_ClsBase->nh_DTaskMsgPort)
            {
                nInstallLastActionHero(nch);
            }
            if(nch->nch_ClsBase->nh_DTaskMsgPort)
            {
                if((am = AllocVec(sizeof(struct ActionMsg), MEMF_PUBLIC)))
                {
                    am->am_Msg.mn_Length = sizeof(struct ActionMsg);
                    am->am_Msg.mn_ReplyPort = NULL;
                    am->am_NCH = nch;
                    am->am_Action = nha;
                    PutMsg(nch->nch_ClsBase->nh_DTaskMsgPort, &am->am_Msg);
                }
            }
            break;
        }

        case HUA_RAWKEY:
            nSendRawKey(nch, nha->nha_RawKey);
            break;

        case HUA_KEYMAP:
        {
            UWORD iecode;
            if((uid > 0x70000) && (uid < 0x700e8))
            {
                iecode = nch->nch_KeymapCfg.kmc_Keymap[uid & 0xff];
                KPRINTF(1,("Key %ld %s\n", iecode, downevent ? "DOWN" : "UP"));
                nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
                nch->nch_FakeEvent.ie_SubClass = 0;
                nSendRawKey(nch, downevent ? iecode : iecode|IECODE_UP_PREFIX);
            }
            break;
        }

        case HUA_MOUSEPOS:
            switch(nha->nha_MouseAxis)
            {
                case HUAT_DELTAX:
                    nch->nch_NewMouseRel = TRUE;
                    nch->nch_MouseDeltaX += value;
                    break;

                case HUAT_DELTAY:
                    nch->nch_NewMouseRel = TRUE;
                    nch->nch_MouseDeltaY += value;
                    break;

                case HUAT_ABSX:
                    if(nch->nch_MouseRangeY)
                    {
                        nch->nch_NewMouseAbs = TRUE;
                    }
                    nch->nch_MouseAbsX = value - nhi->nhi_RealMin;
                    nch->nch_MouseRangeX = (nhi->nhi_RealMax - nhi->nhi_RealMin) + 1;
                    break;

                case HUAT_ABSY:
                    if(nch->nch_MouseRangeX)
                    {
                        nch->nch_NewMouseAbs = TRUE;
                    }
                    nch->nch_MouseAbsY = value - nhi->nhi_RealMin;
                    nch->nch_MouseRangeY = (nhi->nhi_RealMax - nhi->nhi_RealMin) + 1;
                    break;
            }
            break;

        case HUA_QUALIFIER:
            qualmask = 1<<nha->nha_Qualifier;
            switch(nha->nha_QualMode)
            {
                case HUAT_SET:
                    nch->nch_KeyQualifiers |= qualmask;
                    break;

                case HUAT_CLEAR:
                    nch->nch_KeyQualifiers &= ~qualmask;
                    break;

                case HUAT_TOGGLE:
                    nch->nch_KeyQualifiers ^= qualmask;
                    break;

                case HUAT_ASSIGN:
                    nch->nch_KeyQualifiers &= ~qualmask;
                    if(downevent)
                    {
                        nch->nch_KeyQualifiers |= qualmask;
                    }
                    break;
            }
            break;

        case HUA_BUTTONS:
        {
            UWORD iecode = IECODE_NOBUTTON;
            ULONG iequal = 0;
            BOOL newmouse = FALSE;

            switch(nha->nha_ButtonNo)
            {
                case 1:
                    iequal = IEQUALIFIER_LEFTBUTTON;
                    iecode = IECODE_LBUTTON;
                    break;

                case 2:
                    iequal = IEQUALIFIER_RBUTTON;
                    iecode = IECODE_RBUTTON;
                    break;

                case 3:
                    iequal = IEQUALIFIER_MIDBUTTON;
                    iecode = IECODE_MBUTTON;
                    break;

                case 4:
                    iecode = RAWKEY_NM_BUTTON_FOURTH;
                    newmouse = TRUE;
                    break;
            }

            switch(nha->nha_ButtonMode)
            {
                case HUAT_SET:
                    if((!newmouse) && (nch->nch_KeyQualifiers & iequal))
                    {
                        iequal = 0;
                        iecode = IECODE_NOBUTTON;
                    } else {
                        nch->nch_KeyQualifiers |= iequal;
                        nch->nch_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                    }
                    break;

                case HUAT_CLEAR:
                    if(newmouse || (nch->nch_KeyQualifiers & iequal))
                    {
                        nch->nch_KeyQualifiers &= ~iequal;
                        nch->nch_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
                        iequal = 0;
                        iecode |= IECODE_UP_PREFIX;
                    } else {
                        iequal = 0;
                        iecode = IECODE_NOBUTTON;
                    }
                    break;

                case HUAT_TOGGLE:
                    if(newmouse)
                    {
                        if(nch->nch_MouseButtons & (1<<(nha->nha_ButtonNo-1)))
                        {
                            nch->nch_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
                            iecode |= IECODE_UP_PREFIX;
                        } else {
                            nch->nch_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                        }
                    } else {
                        if(nch->nch_KeyQualifiers & iequal)
                        {
                            nch->nch_KeyQualifiers &= ~iequal;
                            nch->nch_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
                            iequal = 0;
                            iecode |= IECODE_UP_PREFIX;
                        } else {
                            nch->nch_KeyQualifiers |= iequal;
                            nch->nch_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                        }
                    }
                    break;

                case HUAT_ASSIGN:
                    if(value)
                    {
                        if((!newmouse) && (nch->nch_KeyQualifiers & iequal))
                        {
                            iequal = 0;
                            iecode = IECODE_NOBUTTON;
                        } else {
                            nch->nch_KeyQualifiers |= iequal;
                            nch->nch_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                        }
                    } else {
                        if(newmouse || (nch->nch_KeyQualifiers & iequal))
                        {
                            nch->nch_KeyQualifiers &= ~iequal;
                            nch->nch_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
                            iequal = 0;
                            iecode |= IECODE_UP_PREFIX;
                        } else {
                            iequal = 0;
                            iecode = IECODE_NOBUTTON;
                        }
                    }
                    break;

                default:
                    iecode = IECODE_NOBUTTON;
                    break;
            }
            if(iecode != IECODE_NOBUTTON)
            {
                if(newmouse)
                {
                    nSendRawKey(nch, iecode);

                    nch->nch_FakeEvent.ie_Class = IECLASS_NEWMOUSE;
                    nch->nch_FakeEvent.ie_SubClass = 0;
                    nch->nch_FakeEvent.ie_Code = iecode;
                    nch->nch_FakeEvent.ie_NextEvent = NULL;
                    nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers;
                    nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                    nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                    nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                    DoIO((struct IORequest *) nch->nch_InpIOReq);
                } else {
                    nch->nch_FakeEvent.ie_X = nch->nch_MouseDeltaX;
                    nch->nch_FakeEvent.ie_Y = nch->nch_MouseDeltaY;
                    nch->nch_FakeEvent.ie_Class = IECLASS_RAWMOUSE;
                    nch->nch_FakeEvent.ie_SubClass = 0;
                    nch->nch_FakeEvent.ie_Code = iecode;
                    nch->nch_FakeEvent.ie_NextEvent = NULL;
                    nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers|IEQUALIFIER_RELATIVEMOUSE;
                    nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                    nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                    nch->nch_InpIOReq->io_Command = nch->nch_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
                    DoIO((struct IORequest *) nch->nch_InpIOReq);
                    nch->nch_NewMouseRel = FALSE;
                    nch->nch_MouseDeltaX = 0;
                    nch->nch_MouseDeltaY = 0;
                }
            }
            break;
        }

        case HUA_TABLET:
        {
            KPRINTF(1, ("Tablet!\n"));
            switch(nha->nha_TabletAxis)
            {
                case HUAT_ABSZ:
                    nch->nch_MouseAbsZ = value - nhi->nhi_RealMin;
                    nch->nch_MouseRangeZ = (nhi->nhi_RealMax - nhi->nhi_RealMin) + 1;
                    break;

                case HUAT_PRESSURE:
                    if(nhi->nhi_RealMax)
                    {
                        nch->nch_TabPressure = ((0x1fffffff / nhi->nhi_RealMax)*value)<<2;
                        nch->nch_VldPressure = TRUE;
                    }
                    break;

                case HUAT_XROT:
                    if(nhi->nhi_RealMax)
                    {
                        nch->nch_TabRotX = ((0x3fffffff / nhi->nhi_RealMax)*value)<<2;
                        nch->nch_VldRotX = TRUE;
                    }
                    break;

                case HUAT_YROT:
                    if(nhi->nhi_RealMax)
                    {
                        nch->nch_TabRotY = ((0x3fffffff / nhi->nhi_RealMax)*value)<<2;
                        nch->nch_VldRotY = TRUE;
                    }
                    break;

                case HUAT_ZROT:
                    if(nhi->nhi_RealMax)
                    {
                        nch->nch_TabRotZ = ((0x3fffffff / nhi->nhi_RealMax)*value)<<2;
                        nch->nch_VldRotZ = TRUE;
                    }
                    break;

                case HUAT_PROX:
                    nch->nch_TabProx = value;
                    nch->nch_VldProx = TRUE;
                    break;
            }
            if(nch->nch_MouseRangeX && nch->nch_MouseRangeY)
            {
                nch->nch_NewMouseAbs = TRUE;
            }
            break;
        }

        case HUA_WHEEL:
        {
            UWORD wheeldist = 0;
            UWORD wheeliecode = IECODE_NOBUTTON;
            switch(nha->nha_WheelMode)
            {
                case HUAT_DELTAX:
                    if(value > 0)
                    {
                        wheeliecode = RAWKEY_NM_WHEEL_LEFT;
                        wheeldist = value;
                    }
                    else if(value < 0)
                    {
                        wheeliecode = RAWKEY_NM_WHEEL_RIGHT;
                        wheeldist = -value;
                    }
                    break;

                case HUAT_DELTAY:
                    if(value > 0)
                    {
                        wheeliecode = RAWKEY_NM_WHEEL_UP;
                        wheeldist = value;
                    }
                    else if(value < 0)
                    {
                        wheeliecode = RAWKEY_NM_WHEEL_DOWN;
                        wheeldist = -value;
                    }
                    break;

                case HUAT_LEFT:
                    wheeliecode = RAWKEY_NM_WHEEL_LEFT;
                    wheeldist = nha->nha_WheelDist;
                    break;

                case HUAT_RIGHT:
                    wheeliecode = RAWKEY_NM_WHEEL_RIGHT;
                    wheeldist = nha->nha_WheelDist;
                    break;

                case HUAT_UP:
                    wheeliecode = RAWKEY_NM_WHEEL_UP;
                    wheeldist = nha->nha_WheelDist;
                    break;

                case HUAT_DOWN:
                    wheeliecode = RAWKEY_NM_WHEEL_DOWN;
                    wheeldist = nha->nha_WheelDist;
                    break;
            }
            if(wheeliecode != IECODE_NOBUTTON)
            {
                if(downevent)
                {
                    while(wheeldist--)
                    {
                        KPRINTF(1, ("Doing wheel %ld\n", wheeliecode));
                        nSendRawKey(nch, wheeliecode);
#if 0
                        nch->nch_FakeEvent.ie_Class = IECLASS_NEWMOUSE;
                        nch->nch_FakeEvent.ie_SubClass = 0;
                        nch->nch_FakeEvent.ie_Code = wheeliecode;
                        nch->nch_FakeEvent.ie_NextEvent = NULL;
                        nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers;
                        nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                        nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                        nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                        DoIO((struct IORequest *) nch->nch_InpIOReq);
#endif
                    }

                    nSendRawKey(nch, wheeliecode|IECODE_UP_PREFIX);
#if 0
                    nch->nch_FakeEvent.ie_Class = IECLASS_NEWMOUSE;
                    nch->nch_FakeEvent.ie_SubClass = 0;
                    nch->nch_FakeEvent.ie_Code = wheeliecode|IECODE_UP_PREFIX;
                    nch->nch_FakeEvent.ie_NextEvent = NULL;
                    nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers;
                    nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                    nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                    nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                    DoIO((struct IORequest *) nch->nch_InpIOReq);
#endif
                }
            }
            break;
        }

        case HUA_DIGJOY:
        {
            ULONG mask = 0;
            /* ULONG oldval; */
            ULONG *stateptr = &nch->nch_LLPortState[nha->nha_JoypadPort];

            switch(nha->nha_JoypadFeat)
            {
                case HUAT_LEFT:
                    mask = JPF_JOY_LEFT;
                    break;

                case HUAT_RIGHT:
                    mask = JPF_JOY_RIGHT;
                    break;

                case HUAT_UP:
                    mask = JPF_JOY_UP;
                    break;

                case HUAT_DOWN:
                    mask = JPF_JOY_DOWN;
                    break;

                case HUAT_RED:
                    mask = JPF_BUTTON_RED;
                    break;

                case HUAT_BLUE:
                    mask = JPF_BUTTON_BLUE;
                    break;

                case HUAT_GREEN:
                    mask = JPF_BUTTON_GREEN;
                    break;

                case HUAT_YELLOW:
                    mask = JPF_BUTTON_YELLOW;
                    break;

                case HUAT_FORWARD:
                    mask = JPF_BUTTON_FORWARD;
                    break;

                case HUAT_REVERSE:
                    mask = JPF_BUTTON_REVERSE;
                    break;

                case HUAT_PLAY:
                    mask = JPF_BUTTON_PLAY;
                    break;

                case HUAT_HATSWITCH:
                    mask = JP_DIRECTION_MASK;
                    stateptr = &nch->nch_LLHatswitch[nha->nha_JoypadPort];
                    break;

            }
            /* oldval = *stateptr; */
            switch(nha->nha_JoypadOp)
            {
                case HUAT_SET:
                    *stateptr |= mask;
                    break;

                case HUAT_CLEAR:
                    *stateptr &= ~mask;
                    break;

                case HUAT_TOGGLE:
                    *stateptr ^= mask;
                    break;

                case HUAT_ASSIGN:
                    if(nha->nha_JoypadFeat == HUAT_HATSWITCH)
                    {
                        value -= nhi->nhi_RealMin;
                        if((value >= 0) && (value < 8))
                        {
                            mask = LLHatswitchEncoding[value];
                        } else {
                            mask = 0;
                        }
                        /*oldval = *stateptr;
                        oldval &= ~JP_DIRECTION_MASK;
                        oldval |= mask;
                        *stateptr = oldval;*/
                        *stateptr = mask;
                    } else {
                        if(value)
                        {
                            *stateptr |= mask;
                        } else {
                            *stateptr &= ~mask;
                        }
                    }
                    break;
            }
            break;
        }

        case HUA_ANALOGJOY:
        {
            ULONG *stateptr = &nch->nch_LLAnalogue[nha->nha_JoypadPort];
            if((nhi->nhi_RealMin != 0) || (nhi->nhi_RealMax != 255))
            {
                // scale
                value -= nhi->nhi_RealMin;
                value <<= 8;
                value /= (nhi->nhi_RealMax - nhi->nhi_RealMin);
            }
            switch(nha->nha_APadFeat)
            {
                case HUAT_ABSX:
                    *stateptr = (*stateptr & ~JP_XAXIS_MASK) | (value & JP_XAXIS_MASK) | JP_TYPE_ANALOGUE;
                    break;

                case HUAT_ABSY:
                    *stateptr = (*stateptr & ~JP_YAXIS_MASK) | ((value<<8) & JP_YAXIS_MASK) | JP_TYPE_ANALOGUE;
                    break;
            }
            break;
        }

        case HUA_OUTPUT:
        {
            ULONG pos;
            struct NepHidItem *nhi;

            nhi = nFindItemID(nch, nha->nha_OutItem, REPORT_MAIN_OUTPUT, &pos);
            if(nhi)
            {
                if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                {
                    switch(nha->nha_OutOp)
                    {
                        case HUAT_SET:
                            nhi->nhi_OldValue = TRUE;
                            break;

                        case HUAT_CLEAR:
                            nhi->nhi_OldValue = FALSE;
                            break;

                        case HUAT_TOGGLE:
                            nhi->nhi_OldValue = nhi->nhi_OldValue ? FALSE : TRUE;
                            break;

                        case HUAT_ASSIGN:
                            nhi->nhi_OldValue = value;
                            break;
                    }
                } else {
                    nParseArrayString(nch, nhi, nha->nha_OutArray);
                }
                nhi->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                nch->nch_OutFeatTouched = TRUE;
            }
            break;
        }

        case HUA_FEATURE:
        {
            ULONG pos;
            struct NepHidItem *nhi;

            nhi = nFindItemID(nch, nha->nha_FeatItem, REPORT_MAIN_FEATURE, &pos);
            if(nhi)
            {
                if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                {
                    switch(nha->nha_FeatOp)
                    {
                        case HUAT_SET:
                            nhi->nhi_OldValue = TRUE;
                            break;

                        case HUAT_CLEAR:
                            nhi->nhi_OldValue = FALSE;
                            break;

                        case HUAT_TOGGLE:
                            nhi->nhi_OldValue = nhi->nhi_OldValue ? FALSE : TRUE;
                            break;

                        case HUAT_ASSIGN:
                            nhi->nhi_OldValue = value;
                            break;
                    }
                } else {
                    nParseArrayString(nch, nhi, nha->nha_OutArray);
                }
                nhi->nhi_Collection->nhc_Report->nhr_FeatTouched = TRUE;
                nch->nch_OutFeatTouched = TRUE;
            }
            break;
        }

        case HUA_MISC:
            nch->nch_FakeEvent.ie_Class = IECLASS_NULL;
            nch->nch_FakeEvent.ie_SubClass = 0;
            nch->nch_FakeEvent.ie_Code = IECODE_NOBUTTON;
            nch->nch_FakeEvent.ie_NextEvent = NULL;
            nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers;
            nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
            nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
            nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
            switch(nha->nha_MiscMode)
            {
                case HUAT_DISPLAYBEEP:
                case HUAT_WIN2FRONT:
                case HUAT_WIN2BACK:
                case HUAT_ZIPWINDOW:
                case HUAT_SCREENCYCLE:
                case HUAT_WB2FRONT:
                case HUAT_ACTWINDOW:
                {
                    struct ActionMsg *am;
                    if(!nch->nch_ClsBase->nh_DTaskMsgPort)
                    {
                        nInstallLastActionHero(nch);
                    }
                    if(nch->nch_ClsBase->nh_DTaskMsgPort)
                    {
                        if((am = AllocVec(sizeof(struct ActionMsg), MEMF_PUBLIC)))
                        {
                            am->am_Msg.mn_Length = sizeof(struct ActionMsg);
                            am->am_Msg.mn_ReplyPort = NULL;
                            am->am_NCH = nch;
                            am->am_Action = nha;
                            PutMsg(nch->nch_ClsBase->nh_DTaskMsgPort, &am->am_Msg);
                        }
                    }
                    break;
                }

                case HUAT_CLOSEWINDOW:
                    nch->nch_FakeEvent.ie_Class = IECLASS_CLOSEWINDOW;
                    DoIO((struct IORequest *) nch->nch_InpIOReq);
                    break;

                case HUAT_REBOOT:
                    nch->nch_CDC->cdc_EnableKBReset = TRUE;
                    nch->nch_KeyQualifiers |= IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND;
                    nCheckReset(nch);
                    break;

                case HUAT_FLUSHEVENTS:
                    nFlushEvents(nch);
                    break;

                default:
                    break;
            }
            break;

        case HUA_VARIABLES:
        {
            LONG *tarvarptr = NULL;

            switch(nha->nha_TarVar)
            {
                case HUAT_LOCALVAR1:
                case HUAT_LOCALVAR2:
                case HUAT_LOCALVAR3:
                case HUAT_LOCALVAR4:
                case HUAT_LOCALVAR5:
                case HUAT_LOCALVAR6:
                case HUAT_LOCALVAR7:
                case HUAT_LOCALVAR8:
                    tarvarptr = &nch->nch_LocalVars[nha->nha_TarVar - HUAT_LOCALVAR1];
                    break;

                case HUAT_GLOBVARA:
                case HUAT_GLOBVARB:
                case HUAT_GLOBVARC:
                case HUAT_GLOBVARD:
                case HUAT_GLOBVARE:
                case HUAT_GLOBVARF:
                case HUAT_GLOBVARG:
                case HUAT_GLOBVARH:
                    tarvarptr = &nch->nch_ClsBase->nh_GlobalVars[nha->nha_TarVar - HUAT_GLOBVARA];
                    break;
            }
            if(!tarvarptr)
            {
                return(FALSE);
            }
            switch(nha->nha_TarVarOp)
            {
                case HUAT_ASSIGN:
                    *tarvarptr = value;
                    break;

                case HUAT_ADD:
                    *tarvarptr += value;
                    break;

                case HUAT_SUB:
                    *tarvarptr -= value;
                    break;

                case HUAT_MULTIPLY:
                    *tarvarptr *= value;
                    break;

                case HUAT_DIVIDE:
                    if(value)
                    {
                        *tarvarptr /= value;
                    }
                    break;

                case HUAT_MODULO:
                    if(value)
                    {
                        *tarvarptr = value;
                    }
                    break;

                case HUAT_AND:
                    *tarvarptr = *tarvarptr && value;
                    break;

                case HUAT_NAND:
                    *tarvarptr = !(*tarvarptr && value);
                    break;

                case HUAT_ANDNOT:
                    *tarvarptr = *tarvarptr && (!value);
                    break;

                case HUAT_OR:
                    *tarvarptr = *tarvarptr || value;
                    break;

                case HUAT_XOR:
                    *tarvarptr = (*tarvarptr ^ value) ? TRUE : FALSE;
                    break;

                case HUAT_BWAND:
                    *tarvarptr &= value;
                    break;

                case HUAT_BWNAND:
                    *tarvarptr = ~(*tarvarptr & value);
                    break;

                case HUAT_BWANDNOT:
                    *tarvarptr &= ~value;
                    break;

                case HUAT_BWOR:
                    *tarvarptr |= value;
                    break;

                case HUAT_BWXOR:
                    *tarvarptr ^= value;
                    break;

                case HUAT_ASL:
                    *tarvarptr <<= value;
                    break;

                case HUAT_ASR:
                    *tarvarptr >>= value;
                    break;
            }
            return(TRUE);
        }

        case HUA_EXTRAWKEY:
#if 0 // FIXME looks like AROS does not support this (yet?)
            nch->nch_FakeEvent.ie_Class = IECLASS_EXTRAWKEY;
            nch->nch_FakeEvent.ie_SubClass = 0;
            nch->nch_FakeEvent.ie_Code = nha->nha_RawKey;
            // using ie.dead.ie_prev1DownCode instead of ie_extkey.ie_extkey1
            nch->nch_FakeEvent.ie_position.ie_dead.ie_prev1DownCode = 0xe0;
            nch->nch_FakeEvent.ie_position.ie_dead.ie_prev1DownQual = nha->nha_RawKey;
            nch->nch_FakeEvent.ie_NextEvent = NULL;
            nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers;
            nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
            nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
            nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
            DoIO((struct IORequest *) nch->nch_InpIOReq);
#endif
            break;

    }
    return(res);
}
/* \\\ */

/* /// "nFlushEvents()" */
void nFlushEvents(struct NepClassHid *nch)
{
    struct PsdPipe *pp;
    ULONG buflen;
    LONG ioerr;

    struct NepHidReport *nhr;

    /* output/feature pipes */
    if(nch->nch_OutFeatTouched)
    {
        nch->nch_OutFeatTouched = FALSE;
        pp = nch->nch_EP0Pipe;
        nhr = (struct NepHidReport *) nch->nch_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            if(nhr->nhr_OutTouched)
            {
                KPRINTF(1, ("Out Report %ld touched...\n", nhr->nhr_ReportID));
                buflen = (nhr->nhr_ReportOutSize+7)>>3;
                if(nch->nch_UsesReportID)
                {
                    *nch->nch_EPOutBuf = nhr->nhr_ReportID;
                    nGenerateOutReport(nch, nhr, nch->nch_EPOutBuf+1);
                    buflen++;
                    psdPipeSetup(pp, URTF_OUT|URTF_CLASS|URTF_INTERFACE,
                                 UHR_SET_REPORT, (ULONG) nhr->nhr_ReportID|0x0200, nch->nch_IfNum);
                } else {
                    nGenerateOutReport(nch, nhr, nch->nch_EPOutBuf);
                    psdPipeSetup(pp, URTF_OUT|URTF_CLASS|URTF_INTERFACE,
                                 UHR_SET_REPORT, 0x0200, nch->nch_IfNum);
                }
                KPRINTF(1, ("Len: %ld [%02lx %02lx]", buflen, nch->nch_EPOutBuf[0], nch->nch_EPOutBuf[1]));
                /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                              "HID OUT Report %02lx Debug (%ld bytes):", nhr->nhr_ReportID, buflen);
                nDebugMem(nch, nch->nch_EPOutBuf, buflen);*/

                if((ioerr = psdDoPipe(pp, nch->nch_EPOutBuf, buflen)))
                {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "SET_REPORT(%lx, %ld) failed: %s (%ld)!",
                                   nhr->nhr_ReportID|0x0200, buflen,
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
                nhr->nhr_OutTouched = FALSE;
            }
            if(nhr->nhr_FeatTouched)
            {
                KPRINTF(1, ("Feat Report %ld touched...\n", nhr->nhr_ReportID));
                buflen = (nhr->nhr_ReportFeatSize+7)>>3;
                if(nch->nch_UsesReportID)
                {
                    *nch->nch_EPOutBuf = nhr->nhr_ReportID;
                    nGenerateFeatReport(nch, nhr, nch->nch_EPOutBuf+1);
                    buflen++;
                    psdPipeSetup(pp, URTF_OUT|URTF_CLASS|URTF_INTERFACE,
                                 UHR_SET_REPORT, (ULONG) nhr->nhr_ReportID|0x0300, nch->nch_IfNum);
                } else {
                    nGenerateFeatReport(nch, nhr, nch->nch_EPOutBuf);
                    psdPipeSetup(pp, URTF_OUT|URTF_CLASS|URTF_INTERFACE,
                                 UHR_SET_REPORT, 0x0300, nch->nch_IfNum);
                }
                KPRINTF(1, ("Len: %ld [%02lx %02lx]", buflen, nch->nch_EPOutBuf[0], nch->nch_EPOutBuf[1]));
                if((ioerr = psdDoPipe(pp, nch->nch_EPOutBuf, buflen)))
                {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "SET_REPORT(%lx, %ld) failed: %s (%ld)!",
                                   nhr->nhr_ReportID|0x0300, buflen,
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }/* else {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "SET_REPORT(%lx, %ld)=%02lx%02lx okay!",
                                   nhr->nhr_ReportID|0x0300, buflen,
                                   nch->nch_EPOutBuf[1], nch->nch_EPOutBuf[2]);
                }*/
                nhr->nhr_FeatTouched = FALSE;
            }
            nhr = (struct NepHidReport *) nhr->nhr_Node.ln_Succ;
        }
    }
    if(nch->nch_NewMouseRel && (nch->nch_MouseDeltaX || nch->nch_MouseDeltaY))
    {
        KPRINTF(1, ("Mouse rel movement...\n"));
        nch->nch_FakeEvent.ie_X = nch->nch_MouseDeltaX;
        nch->nch_FakeEvent.ie_Y = nch->nch_MouseDeltaY;
        nch->nch_FakeEvent.ie_Class = IECLASS_RAWMOUSE;
        nch->nch_FakeEvent.ie_SubClass = 0;
        nch->nch_FakeEvent.ie_Code = IECODE_NOBUTTON;
        nch->nch_FakeEvent.ie_NextEvent = NULL;
        nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers|IEQUALIFIER_RELATIVEMOUSE;
        nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
        nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
        nch->nch_InpIOReq->io_Command = nch->nch_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
        DoIO((struct IORequest *) nch->nch_InpIOReq);
        nch->nch_NewMouseRel = FALSE;
        nch->nch_MouseDeltaX = 0;
        nch->nch_MouseDeltaY = 0;
    }
    if(nch->nch_NewMouseAbs)
    {
        ULONG *tagptr = nch->nch_TabTags;
        KPRINTF(1, ("Mouse abs movement...\n"));
        nch->nch_FakeEvent.ie_EventAddress = &nch->nch_TabletEvent;
        nch->nch_TabletEvent.ient_RangeX = nch->nch_MouseRangeX;
        nch->nch_TabletEvent.ient_RangeY = nch->nch_MouseRangeY;
        nch->nch_TabletEvent.ient_TabletX = nch->nch_MouseAbsX;
        nch->nch_TabletEvent.ient_TabletY = nch->nch_MouseAbsY;
        nch->nch_TabletEvent.ient_TagList = (struct TagItem *) tagptr;
        if(nch->nch_MouseRangeZ)
        {
            KPRINTF(1, ("AbsZ %ld\n", nch->nch_MouseAbsZ));
            *tagptr++ = TABLETA_TabletZ;
            *tagptr++ = nch->nch_MouseAbsZ;
            *tagptr++ = TABLETA_RangeZ;
            *tagptr++ = nch->nch_MouseRangeZ;
        }
        if(nch->nch_VldRotX)
        {
            KPRINTF(1, ("AngleX %ld\n", nch->nch_TabRotX));
            *tagptr++ = TABLETA_AngleX;
            *tagptr++ = nch->nch_TabRotX;
        }
        if(nch->nch_VldRotY)
        {
            KPRINTF(1, ("AngleY %ld\n", nch->nch_TabRotY));
            *tagptr++ = TABLETA_AngleY;
            *tagptr++ = nch->nch_TabRotY;
        }
        if(nch->nch_VldRotZ)
        {
            KPRINTF(1, ("AngleZ %ld\n", nch->nch_TabRotZ));
            *tagptr++ = TABLETA_AngleZ;
            *tagptr++ = nch->nch_TabRotZ;
        }
        if(nch->nch_VldPressure)
        {
            KPRINTF(1, ("Pressure %ld\n", nch->nch_TabPressure));
            *tagptr++ = TABLETA_Pressure;
            *tagptr++ = nch->nch_TabPressure;
        }
        *tagptr++ = TABLETA_ButtonBits;
        *tagptr++ = nch->nch_MouseButtons;
        if(nch->nch_VldProx)
        {
            KPRINTF(1, ("InProx %ld\n", nch->nch_TabProx));
            *tagptr++ = TABLETA_InProximity;
            *tagptr++ = nch->nch_TabProx;
        }
        *tagptr = TAG_END;
        nch->nch_FakeEvent.ie_Class = IECLASS_NEWPOINTERPOS;
        nch->nch_FakeEvent.ie_SubClass = IESUBCLASS_NEWTABLET;
        nch->nch_FakeEvent.ie_Code = IECODE_NOBUTTON;
        nch->nch_FakeEvent.ie_NextEvent = NULL;
        nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers;
        nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
        nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
        nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
        DoIO((struct IORequest *) nch->nch_InpIOReq);
        nch->nch_NewMouseAbs = FALSE;
    }
    nCheckReset(nch);
}
/* \\\ */

/* /// "nSendRawKey()" */
void nSendRawKey(struct NepClassHid *nch, UWORD key)
{
    nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
    nch->nch_FakeEvent.ie_SubClass = 0;
    nch->nch_FakeEvent.ie_Code = key;
    nch->nch_FakeEvent.ie_NextEvent = NULL;
    nch->nch_FakeEvent.ie_Qualifier = nch->nch_KeyQualifiers;
    nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
    nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
    nch->nch_InpIOReq->io_Command = nch->nch_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
    DoIO((struct IORequest *) nch->nch_InpIOReq);
}
/* \\\ */

/* /// "nCheckReset()" */
void nCheckReset(struct NepClassHid *nch)
{
    if(nch->nch_CDC->cdc_EnableKBReset &&
       (nch->nch_KeyQualifiers & (IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)) ==
                                 (IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND))
    {
        struct MsgPort *mp;
        struct IOStdReq *ioreq;
        struct Interrupt tempint;
        struct Node *node;
        struct List *listhead = NULL;
        BOOL haskbh = FALSE;
        KPRINTF(20, ("Reboot!\n"));

        if(nch->nch_CDC->cdc_EnableRH)
        {
            if((mp = CreateMsgPort()))
            {
                if((ioreq = (struct IOStdReq *) CreateIORequest(mp, sizeof(struct IOStdReq))))
                {
                    if(!OpenDevice("keyboard.device", 0, (struct IORequest *) ioreq, 0))
                    {
                        /* Find list header of reset handlers */
                        tempint.is_Node.ln_Pred = NULL;
                        tempint.is_Node.ln_Succ = NULL;
                        tempint.is_Node.ln_Type = NT_INTERRUPT;
                        tempint.is_Node.ln_Pri  = 32;
                        tempint.is_Node.ln_Name = "";
                        tempint.is_Code = NULL;
                        ioreq->io_Command = KBD_ADDRESETHANDLER;
                        ioreq->io_Data = &tempint;
                        Forbid();
                        DoIO((struct IORequest *) ioreq);
                        if((node = tempint.is_Node.ln_Pred))
                        {
                            while(node->ln_Pred)
                            {
                                node = node->ln_Pred;
                            }
                            listhead = (struct List *) node;
                        }
                        ioreq->io_Command = KBD_REMRESETHANDLER;
                        DoIO((struct IORequest *) ioreq);
                        Permit();
                        if(listhead)
                        {
                            node = listhead->lh_Head;
                            while(node->ln_Succ)
                            {
                                KPRINTF(20, ("Kicking %s\n", node->ln_Name));
                                Cause((struct Interrupt *) node);
                                haskbh = TRUE;
                                node = node->ln_Succ;
                            }
                            KPRINTF(20, ("Done... awaiting doom\n"));
                            if(haskbh)
                            {
                                psdDelayMS(nch->nch_CDC->cdc_ResetDelay*1000);
                            }
                        } else {
                            KPRINTF(20, ("Reset handler list not found!\n"));
                        }
                        CloseDevice((struct IORequest *) ioreq);
                    }
                    DeleteIORequest((struct IORequest *) ioreq);
                }
                DeleteMsgPort(mp);
            }
        }
        ColdReboot();
    }
    else if(nch->nch_CDC->cdc_EnableKBReset &&
        (nch->nch_KeyQualifiers & IEQUALIFIER_CONTROL) &&
        (nch->nch_KeyQualifiers & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)) &&
        nch->nch_FakeEvent.ie_Code == RAWKEY_DELETE)
    {
        KPRINTF(20, ("Reboot!\n"));
        ShutdownA(SD_ACTION_COLDREBOOT);
    }
}
/* \\\ */

/* /// "nLoadItem()" */
BOOL nLoadItem(struct NepClassHid *nch, struct PsdIFFContext *rppic, struct List *lst, UWORD idbase)
{
    struct PsdIFFContext *itpic;
    struct PsdIFFContext *actpic;
    struct NepHidAction *nha;
    ULONG newform[3];
    ULONG buf[2];
    struct NepHidActionChunk *nhac;
    STRPTR tmpstr;

    psdSafeRawDoFmt((STRPTR) buf, 8, "I%03lx", GET_WTYPE(lst) - idbase + 1);
    newform[0] = AROS_LONG2BE(ID_FORM);
    newform[1] = AROS_LONG2BE(4);
    newform[2] = *buf;
    itpic = psdFindCfgForm(rppic, AROS_LONG2BE(newform[2]));
    if(!itpic)
    {
        return(FALSE);
    }
    actpic = psdFindCfgForm(itpic, MAKE_ID('A','C','T','N'));
    while(actpic)
    {
        if((nhac = psdGetCfgChunk(actpic, MAKE_ID('A','C','D','F'))))
        {
            if((nha = nAllocAction(nch, lst, 0)))
            {
                CopyMem(&nhac->nhac_Type, &nha->nha_Type, min(AROS_LONG2BE(nhac->nhac_Length), sizeof(struct NepHidActionChunk) - 8));
                if((tmpstr = psdGetStringChunk(actpic, MAKE_ID('S','N','D','F'))))
                {
                    strncpy(nha->nha_SoundFile, tmpstr, 255);
                    psdFreeVec(tmpstr);
                }
                if((tmpstr = psdGetStringChunk(actpic, MAKE_ID('V','A','N','S'))))
                {
                    strncpy(nha->nha_VanillaString, tmpstr, 79);
                    psdFreeVec(tmpstr);
                }
                if((tmpstr = psdGetStringChunk(actpic, MAKE_ID('K','E','Y','S'))))
                {
                    strncpy(nha->nha_KeyString, tmpstr, 79);
                    psdFreeVec(tmpstr);
                }
                if((tmpstr = psdGetStringChunk(actpic, MAKE_ID('E','X','E','S'))))
                {
                    strncpy(nha->nha_ExeString, tmpstr, 79);
                    psdFreeVec(tmpstr);
                }
                if((tmpstr = psdGetStringChunk(actpic, MAKE_ID('O','A','R','R'))))
                {
                    strncpy(nha->nha_OutArray, tmpstr, 255);
                    psdFreeVec(tmpstr);
                }
            }
            psdFreeVec(nhac);
        }
        actpic = psdNextCfgForm(actpic);
    }
    return(TRUE);
}
/* \\\ */

/* /// "nSaveItem()" */
struct PsdIFFContext * nSaveItem(struct NepClassHid *nch, struct PsdIFFContext *rppic, struct List *lst, UWORD idbase)
{
    struct PsdIFFContext *itpic;
    struct PsdIFFContext *actpic;
    struct NepHidAction *nha;
    ULONG newform[3];
    ULONG buf[2];
    struct NepHidActionChunk nhac;
    BOOL isdefault = TRUE;

    if(!lst->lh_Head->ln_Succ)
    {
        return(NULL);
    }
    nha = (struct NepHidAction *) lst->lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        if(!nha->nha_IsDefault)
        {
            isdefault = FALSE;
        }
        nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
    }
    if(isdefault)
    {
        return(NULL);
    }
    psdSafeRawDoFmt((STRPTR) buf, 8, "I%03lx", GET_WTYPE(lst) - idbase + 1);
    newform[0] = AROS_LONG2BE(ID_FORM);
    newform[1] = AROS_LONG2BE(4);
    newform[2] = *buf;
    itpic = psdFindCfgForm(rppic, AROS_LONG2BE(newform[2]));
    if(!itpic)
    {
        itpic = psdAddCfgEntry(rppic, newform);
        if(!itpic)
        {
            return(NULL);
        }
    }
    nhac.nhac_ID = AROS_LONG2BE(MAKE_ID('A','C','D','F'));
    nhac.nhac_Length = AROS_LONG2BE(sizeof(struct NepHidActionChunk)-8);
    nha = (struct NepHidAction *) lst->lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        if(!nha->nha_IsDefault)
        {
            isdefault = FALSE;
        }
        newform[2] = AROS_LONG2BE(MAKE_ID('A','C','T','N'));
        actpic = psdAddCfgEntry(itpic, newform);
        if(!actpic)
        {
            break;
        }
        CopyMem(&nha->nha_Type, &nhac.nhac_Type, sizeof(struct NepHidActionChunk)-8);
        psdAddCfgEntry(actpic, &nhac);
        if(*nha->nha_SoundFile)
        {
            psdAddStringChunk(actpic, MAKE_ID('S','N','D','F'), nha->nha_SoundFile);
        }
        if(*nha->nha_VanillaString)
        {
            psdAddStringChunk(actpic, MAKE_ID('V','A','N','S'), nha->nha_VanillaString);
        }
        if(*nha->nha_KeyString)
        {
            psdAddStringChunk(actpic, MAKE_ID('K','E','Y','S'), nha->nha_KeyString);
        }
        if(*nha->nha_ExeString)
        {
            psdAddStringChunk(actpic, MAKE_ID('E','X','E','S'), nha->nha_ExeString);
        }
        if(*nha->nha_OutArray)
        {
            psdAddStringChunk(actpic, MAKE_ID('O','A','R','R'), nha->nha_OutArray);
        }
        nha = (struct NepHidAction *) nha->nha_Node.ln_Succ;
    }
    return(itpic);
}
/* \\\ */

/* /// "nReadJoyPort()" */
AROS_LH1(ULONG, nReadJoyPort,
         AROS_LHA(ULONG, port, D0),
         struct Library *, LowLevelBase, 5, hid)
{
    AROS_LIBFUNC_INIT

    struct NepHidBase *nh;
    struct NepClassHid *nch;
    ULONG result = JP_TYPE_NOTAVAIL;
    ULONG mode;

    if(!(nh = (struct NepHidBase *) FindName(&SysBase->LibList, GM_UNIQUENAME(libname))))
    {
        return(result);
    }

    result = AROS_CALL1(ULONG, nh->nh_LLOldReadJoyPort,
                       AROS_LCA(ULONG, port, D0),
                       struct Library *, nh->nh_LowLevelBase);

    KPRINTF(40, ("Before %08lx\n", result));
    if(port < 4)
    {
        Forbid();
        nch = (struct NepClassHid *) nh->nh_Interfaces.lh_Head;
        while(nch->nch_Node.ln_Succ)
        {
            mode = nch->nch_CDC->cdc_LLPortMode[port];
            if(nh->nh_LLAnalogueOverride[port] && nch->nch_LLAnalogue[port])
            {
                mode = 4;
            }

            switch(mode)
            {
//                case 0: /* don't touch */
//                    break;

                case 1: /* overwrite with USB */
                    result = nch->nch_LLPortState[port] | nch->nch_LLHatswitch[port] | JP_TYPE_GAMECTLR;
                    break;

                case 2: /* merge with USB */
                    if(((result & JP_TYPE_MASK) == JP_TYPE_GAMECTLR) ||
                       ((result & JP_TYPE_MASK) == JP_TYPE_JOYSTK) ||
                       ((result & JP_TYPE_MASK) == JP_TYPE_UNKNOWN) ||
                       (!result))
                    {
                        result = (result & (JP_BUTTON_MASK|JP_DIRECTION_MASK)) | nch->nch_LLPortState[port] | nch->nch_LLHatswitch[port] | JP_TYPE_GAMECTLR;
                    }
                    break;

                case 3: /* disable */
                    result = JP_TYPE_NOTAVAIL;
                    break;

                case 4: /* overwrite with analogue USB */
                    if(nch->nch_LLAnalogue[port])
                    {
                        result = (nch->nch_LLPortState[port] & ~JP_DIRECTION_MASK) | (nch->nch_LLAnalogue[port] & JP_XYAXIS_MASK) | nch->nch_LLHatswitch[port] | JP_TYPE_ANALOGUE;
                    }
                    break;
            }
            nch = (struct NepClassHid *) nch->nch_Node.ln_Succ;
        }
        Permit();
    }
    if((port >= JP_ANALOGUE_PORT_MAGIC) && (port < (JP_ANALOGUE_PORT_MAGIC|4)))
    {
        port &= 3;
        Forbid();
        nch = (struct NepClassHid *) nh->nh_Interfaces.lh_Head;
        while(nch->nch_Node.ln_Succ)
        {
            if(nch->nch_LLAnalogue[port])
            {
                result = (nch->nch_LLPortState[port] & ~JP_DIRECTION_MASK) | (nch->nch_LLAnalogue[port] & JP_XYAXIS_MASK) | nch->nch_LLHatswitch[port] | JP_TYPE_ANALOGUE;
            }
            nch = (struct NepClassHid *) nch->nch_Node.ln_Succ;
        }
        Permit();
    }
    KPRINTF(40, ("And after %08lx\n", result));

    return(result);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "nSetJoyPortAttrsA()" */
AROS_LH2(ULONG, nSetJoyPortAttrsA,
         AROS_LHA(ULONG, port, D0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct Library *, LowLevelBase, 22, hid)
{
    AROS_LIBFUNC_INIT

    struct NepHidBase *nh;
    struct TagItem *typetag;
    struct TagItem *motortags[2];
    struct TagItem *motorofftag;
    ULONG result = FALSE;

    if(!(nh = (struct NepHidBase *) FindName(&SysBase->LibList, GM_UNIQUENAME(libname))))
    {
        return(result);
    }

    result = AROS_CALL2(ULONG, nh->nh_LLOldSetJoyPortAttrsA,
                       AROS_LCA(ULONG, port, D0),
                       AROS_LCA(struct TagItem *, tags, A1),
                       struct Library *, nh->nh_LowLevelBase);

    KPRINTF(40, ("Before %08lx\n", result));
    if(port < 4)
    {
        typetag = FindTagItem(SJA_Type, tags);
        if(typetag)
        {
            if(typetag->ti_Data == SJA_TYPE_ANALOGUE)
            {
                nh->nh_LLAnalogueOverride[port] = TRUE;
            } else {
                nh->nh_LLAnalogueOverride[port] = FALSE;
            }
            result = TRUE;
        }
        typetag = FindTagItem(SJA_Reinitialize, tags);
        if(typetag)
        {
            if(typetag->ti_Data)
            {
                nh->nh_LLAnalogueOverride[port] = FALSE;
            }
        }
        motortags[0] = FindTagItem(SJA_RumbleSetSlowMotor, tags);
        motortags[1] = FindTagItem(SJA_RumbleSetFastMotor, tags);
        motorofftag = FindTagItem(SJA_RumbleOff, tags);
        if(motortags[0] || motortags[1] || motorofftag)
        {
            struct NepHidItem *nhi;
            struct NepClassHid *nch;
            struct TagItem nulltag = { 0, 0 };
            ULONG cnt;
            BOOL sigit = FALSE;
            if(motorofftag)
            {
                if(motorofftag->ti_Data)
                {
                    motortags[0] = &nulltag;
                    motortags[1] = &nulltag;
                }
            }
            Forbid();
            nch = (struct NepClassHid *) nh->nh_Interfaces.lh_Head;
            while(nch->nch_Node.ln_Succ)
            {
                if(nch->nch_CDC->cdc_LLRumblePort == port+1)
                {
                    for(cnt = 0; cnt < 2; cnt++)
                    {
                        if(motortags[cnt] && ((nhi = nch->nch_RumbleMotors[cnt])))
                        {
                            if(nhi->nhi_OldValue != motortags[cnt]->ti_Data)
                            {
                                nhi->nhi_OldValue = motortags[cnt]->ti_Data;
                                nhi->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                                nch->nch_OutFeatTouched = TRUE;
                                sigit = TRUE;
                            }
                        }
                    }
                    if(sigit)
                    {
                        Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
                    }
                }

                nch = (struct NepClassHid *) nch->nch_Node.ln_Succ;
            }
            Permit();
            result = TRUE;
        }
    }
    KPRINTF(40, ("And after %08lx\n", result));

    return(result);

    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "nInstallLastActionHero()" */
void nInstallLastActionHero(struct NepClassHid *nch)
{
    struct NepHidBase *nh = nch->nch_ClsBase;

    if(!nh->nh_DispatcherTask)
    {
        nh->nh_ReadySignal = SIGB_SINGLE;
        nh->nh_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if(psdSpawnSubTask("Last Action Hero", GM_UNIQUENAME(nDispatcherTask), nh))
        {
            Wait(1L<<nh->nh_ReadySignal);
            if(!nh->nh_DispatcherTask)
            {
                psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                               "It's your fault! I'm sure! Couldn't create subtask for launching!");
            } else {
                psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Last Action Hero successfully launched!");
            }
        } else {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "It's your fault! I'm sure! Couldn't create subtask for launching!");
        }
        nh->nh_ReadySigTask = NULL;
        //FreeSignal(nh->nh_ReadySignal);
    }
}
/* \\\ */

/************************************************************************/

#undef ps

/* /// "nDispatcherTask()" */
AROS_UFH0(void, GM_UNIQUENAME(nDispatcherTask))
{
    AROS_USERFUNC_INIT

    struct NepHidBase *nh;
    struct Task *thistask;
    struct ActionMsg *am;
    struct NepHidSound *nhs;

    thistask = FindTask(NULL);

#undef CxBase
#undef IntuitionBase
#undef DOSBase
#undef DataTypesBase
    nh = thistask->tc_UserData;
    NewList(&nh->nh_Sounds);
    if((nh->nh_InpMsgPort = CreateMsgPort()))
    {
        if((nh->nh_InpIOReq = (struct IOStdReq *) CreateIORequest(nh->nh_InpMsgPort, sizeof(struct IOStdReq))))
        {
            if(!OpenDevice("input.device", 0, (struct IORequest *) nh->nh_InpIOReq, 0))
            {
                if((nh->nh_DOSBase = OpenLibrary("dos.library", 39)))
                {
                    if((nh->nh_IntBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 39)))
                    {
#define IntuitionBase nh->nh_IntBase
                        if((nh->nh_DTBase = OpenLibrary("datatypes.library", 39)))
                        {
                            if((nh->nh_CxBase = OpenLibrary("commodities.library", 39)))
                            {
                                if((nh->nh_LayersBase = OpenLibrary("layers.library", 39)))
                                {
#define CxBase nh->nh_CxBase
#define DOSBase nh->nh_DOSBase
#define DataTypesBase nh->nh_DTBase
#define LayersBase nh->nh_LayersBase
                                    if((nh->nh_DTaskMsgPort = CreateMsgPort()))
                                    {
                                        nh->nh_DispatcherTask = thistask;
                                        Forbid();
                                        if(nh->nh_ReadySigTask)
                                        {
                                            Signal(nh->nh_ReadySigTask, 1L<<nh->nh_ReadySignal);
                                        }
                                        Permit();

                                        nLastActionHero(nh);

                                        Forbid();
                                        while((am = (struct ActionMsg *) GetMsg(nh->nh_DTaskMsgPort)))
                                        {
                                            FreeVec(am);
                                        }
                                        nhs = (struct NepHidSound *) nh->nh_Sounds.lh_Head;
                                        while(nhs->nhs_Node.ln_Succ)
                                        {
                                            nFreeSound(nh, nhs);
                                            nhs = (struct NepHidSound *) nh->nh_Sounds.lh_Head;
                                        }
                                        DeleteMsgPort(nh->nh_DTaskMsgPort);
                                        nh->nh_DTaskMsgPort = NULL;
                                        Permit();
                                    }
                                    CloseLibrary(nh->nh_LayersBase);
                                    nh->nh_LayersBase = NULL;
                                }
                                CloseLibrary(nh->nh_CxBase);
                                nh->nh_CxBase = NULL;
                            }
                            CloseLibrary(nh->nh_DTBase);
                            nh->nh_DTBase = NULL;
                        }
                        CloseLibrary((struct Library *) nh->nh_IntBase);
                        nh->nh_IntBase = NULL;
                    }
                    CloseLibrary((struct Library *) nh->nh_DOSBase);
                    nh->nh_DOSBase = NULL;
                }
                CloseDevice((struct IORequest *) nh->nh_InpIOReq);
            }
            DeleteIORequest((struct IORequest *) nh->nh_InpIOReq);
            nh->nh_InpIOReq = NULL;
        }
        DeleteMsgPort(nh->nh_InpMsgPort);
        nh->nh_InpMsgPort = NULL;
    }
    Forbid();
    if(nh->nh_ReadySigTask)
    {
        Signal(nh->nh_ReadySigTask, 1L<<nh->nh_ReadySignal);
    }
    nh->nh_DispatcherTask = NULL;

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nLastActionHero()" */
void nLastActionHero(struct NepHidBase *nh)
{
    ULONG sigmask;
    ULONG sigs;
    struct ActionMsg *am;
    struct NepHidAction *nha;
    struct NepClassHid *nch;
    struct Window *actwindow;
    struct Screen *actscreen;
    char buf[128];
    ULONG intlock;
    sigmask = (1L<<nh->nh_DTaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
    do
    {
        while((am = (struct ActionMsg *) GetMsg(nh->nh_DTaskMsgPort)))
        {
            nch = am->am_NCH;
            nha = am->am_Action;
            switch(nha->nha_Type & HUA_ATYPEMASK)
            {
                case HUA_VANILLA:
                    buf[0] = '<';
                    strcpy(&buf[1], nha->nha_VanillaString);
                    strcat(buf, ">");
                    if(!nSendKeyString(nh, buf))
                    {
                        nEasyRequest(nh, "'%s' is no good. Really.", "Argl!", nha->nha_VanillaString);
                    }
                    break;

                case HUA_KEYSTRING:
                    if(!nSendKeyString(nh, nha->nha_KeyString))
                    {
                        nEasyRequest(nh, "Don't you ever say\n'%s'\nto me again!", "Terribly sorry!", nha->nha_KeyString);
                    }
                    break;

                case HUA_SOUND:
                    nPlaySound(nh, nha);
                    break;

                case HUA_SHELL:
                {
                    BPTR fhandle;
                    LONG ioerr;
                    if((fhandle = Open(nch->nch_CDC->cdc_ShellCon, MODE_READWRITE)))
                    {
                        ioerr = SystemTags(nha->nha_ExeString,
                                           SYS_Input, fhandle,
                                           SYS_Output, NULL,
                                           SYS_Asynch, nha->nha_ShellAsync,
                                           NP_StackSize, nch->nch_CDC->cdc_ShellStack,
                                           TAG_END);
                        if(ioerr)
                        {
                            nEasyRequest(nh, "Offender survived execution of\n'%s'\nwith error code %ld.", "Get me a shotgun then!", nha->nha_ExeString, ioerr);
                        }
                        if((!nha->nha_ShellAsync) || ioerr)
                        {
                            Close(fhandle);
                        }
                    } else {
                        nEasyRequest(nh, "Do you really think\n'%s'\nis a nice and witty\nconsole window? Well, it's not!", "I'll use that string to hang myself!", nch->nch_CDC->cdc_ShellCon);
                    }
                    break;
                }

                case HUA_MISC:
                    switch(nha->nha_MiscMode)
                    {
                        case HUAT_ACTWINDOW:
                        {
                            struct Layer *toplayer;
                            intlock = LockIBase(0);
                            actscreen = nh->nh_IntBase->ActiveScreen;
                            actwindow = nh->nh_IntBase->ActiveWindow;
                            UnlockIBase(intlock);
                            if(actscreen)
                            {
                                LockLayerInfo(&actscreen->LayerInfo);
                                toplayer =  WhichLayer(&actscreen->LayerInfo, (LONG) actscreen->MouseX, (LONG) actscreen->MouseY);
                                UnlockLayerInfo(&actscreen->LayerInfo);

                                if(toplayer)
                                {
                                    if(toplayer->Window && (actwindow != toplayer->Window))
                                    {
                                        ActivateWindow(toplayer->Window);
                                    }
                                }
                            }
                            break;
                        }

                        case HUAT_DISPLAYBEEP:
                            DisplayBeep(NULL);
                            break;

                        case HUAT_WB2FRONT:
                            WBenchToFront();
                            break;

                        case HUAT_SCREENCYCLE:
                            intlock = LockIBase(0);
                            actscreen = nh->nh_IntBase->FirstScreen;
                            UnlockIBase(intlock);
                            if(actscreen)
                            {
                                ScreenToBack(actscreen);
                            }
                            break;

                        case HUAT_WIN2FRONT:
                            intlock = LockIBase(0);
                            actwindow = nh->nh_IntBase->ActiveWindow;
                            UnlockIBase(intlock);
                            if(actwindow)
                            {
                                WindowToFront(actwindow);
                            }
                            break;

                        case HUAT_WIN2BACK:
                            intlock = LockIBase(0);
                            actwindow = nh->nh_IntBase->ActiveWindow;
                            UnlockIBase(intlock);
                            if(actwindow)
                            {
                                WindowToBack(actwindow);
                            }
                            break;

                        case HUAT_ZIPWINDOW:
                            intlock = LockIBase(0);
                            actwindow = nh->nh_IntBase->ActiveWindow;
                            UnlockIBase(intlock);
                            if(actwindow)
                            {
                                ZipWindow(actwindow);
                            }
                            break;

                    }
                    break;

            }
            FreeVec(am);
        }
        sigs = Wait(sigmask);
    } while(!(sigs & SIGBREAKF_CTRL_C));
}
/* \\\ */

/* /// "nLoadSound()" */
struct NepHidSound * nLoadSound(struct NepHidBase *nh, STRPTR name)
{
    struct NepHidSound *nhs;
    if((nhs = AllocVec(sizeof(struct NepHidSound), MEMF_PUBLIC|MEMF_CLEAR)))
    {
        if((nhs->nhs_Node.ln_Name = AllocVec((ULONG) strlen(name)+1, MEMF_PUBLIC)))
        {
            strcpy(nhs->nhs_Node.ln_Name, name);
            AddTail(&nh->nh_Sounds, &nhs->nhs_Node);
            nhs->nhs_DTHandle = NewDTObject(name,
                                            DTA_SourceType, DTST_FILE,
                                            DTA_GroupID, GID_SOUND,
                                            SDTA_Cycles, 1L,
                                            TAG_END);
            if(!nhs->nhs_DTHandle)
            {
                nEasyRequest(nh, "Does '%s' make a sound\nin the woods, when it was chopped\nbut nobody was looking at it?", "No! Couldn't load it!", name);
            }
            return(nhs);
        }
        FreeVec(nhs);
    }
    return(NULL);
}
/* \\\ */

/* /// "nPlaySound()" */
BOOL nPlaySound(struct NepHidBase *nh, struct NepHidAction *nha)
{
    struct NepHidSound *nhs;
    struct dtTrigger playmsg;
    nhs = (struct NepHidSound *) FindName(&nh->nh_Sounds, nha->nha_SoundFile);
    if(!nhs)
    {
        nhs = nLoadSound(nh, nha->nha_SoundFile);
    }
    if(!nhs)
    {
        return(FALSE);
    }
    if(!nhs->nhs_DTHandle)
    {
        return(FALSE);
    }

    SetAttrs(nhs->nhs_DTHandle,
             SDTA_Volume, nha->nha_SoundVolume,
             TAG_END);
    playmsg.MethodID     = DTM_TRIGGER;
    playmsg.dtt_GInfo    = NULL;
    playmsg.dtt_Function = STM_PLAY;
    playmsg.dtt_Data     = NULL;
    DoMethodA(nhs->nhs_DTHandle, (Msg) &playmsg);
    return(TRUE);
}
/* \\\ */

/* /// "nFreeSound()" */
void nFreeSound(struct NepHidBase *nh, struct NepHidSound *nhs)
{
    Remove(&nhs->nhs_Node);
    if(nhs->nhs_DTHandle)
    {
        DisposeDTObject(nhs->nhs_DTHandle);
    }
    FreeVec(nhs->nhs_Node.ln_Name);
    FreeVec(nhs);
}
/* \\\ */

/* /// "nInvertString()" */
struct InputEvent *nInvertString(struct NepHidBase *nh, STRPTR str, struct KeyMap *km)
{
    struct InputEvent *prior = NULL;
    struct InputEvent *ne;
    struct InputXpression tmpIX;

    BOOL freeevents = FALSE;
    char cc;
    char *oldsptr;

    if(!str) return(NULL);
    if(!(*str)) return(NULL);
    do
    {
        if((ne = AllocMem(sizeof(struct InputEvent), MEMF_CLEAR)))
        {
            ne->ie_NextEvent = prior;
            prior = ne;
            cc = *str++;
            switch(cc)
            {
                case '<': /* do angles */
                    oldsptr = str;
                    freeevents = TRUE;
                    while((cc = *str))
                    {
                        if(cc == '>')
                        {
                            *str = '\0';
                            if(!ParseIX(oldsptr, &tmpIX))
                            {
                                freeevents = FALSE;
                                prior->ie_Class = tmpIX.ix_Class;
                                prior->ie_Code = tmpIX.ix_Code;
                                prior->ie_Qualifier = tmpIX.ix_Qualifier;
                            }
                            *str++ = cc;
                            break;
                        }
                        str++;
                    }
                    break;

                case '\\': /* do escape sequences */
                    cc = *str++;
                    switch(cc)
                    {
                        case '"':
                        case '\'':
                        case '<':
                        case '\\':
                            break;
                        case 'n':
                        case 'r':
                            cc = '\r';
                            break;
                        case 't':
                            cc = '\t';
                            break;
                        case '0':
                            cc = '\0';
                            break;
                        default:
                            freeevents = TRUE;
                            break;
                    }
                    if(freeevents) break;
                    /* note that this break is missing on purpose */
                default: /* normal key translation */
                    InvertKeyMap((ULONG) cc, prior, km);
                    break;
            }
        } else {
            freeevents = TRUE;
        }
    } while(*str && (!freeevents));
    if(freeevents)
    {
        nFreeIEvents(nh, prior);
        return(NULL);
    }
    return(prior);
}
/* \\\ */

/* /// "nFreeIEvents()" */
void nFreeIEvents(struct NepHidBase *nh, struct InputEvent *event)
{
    struct InputEvent *next;
    while(event)
    {
        next = event->ie_NextEvent;
        FreeMem(event, sizeof(struct InputEvent));
        event = next;
    }
}
/* \\\ */

/* /// "nSendKeyString()" */
BOOL nSendKeyString(struct NepHidBase *nh, STRPTR str)
{
    struct InputEvent *events = nInvertString(nh, str, NULL);
    struct InputEvent *eptr = events;
    struct InputEvent lastevent;
    int count = 0;
    int lstpos;
    if(!events)
    {
        return(FALSE);
    }
    while(eptr)
    {
        count++;
        eptr = eptr->ie_NextEvent;
    }
    while(count)
    {
        eptr = events;
        for(lstpos = --count; lstpos; lstpos--)
            eptr = eptr->ie_NextEvent;

        nh->nh_FakeEvent = *eptr;
        nh->nh_FakeEvent.ie_NextEvent = NULL;
        nh->nh_InpIOReq->io_Data = &nh->nh_FakeEvent;
        nh->nh_InpIOReq->io_Length = sizeof(struct InputEvent);
        nh->nh_InpIOReq->io_Command = nh->nh_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
        DoIO((struct IORequest *) nh->nh_InpIOReq);

        nh->nh_FakeEvent.ie_Code |= IECODE_UP_PREFIX;
        nh->nh_InpIOReq->io_Data = &nh->nh_FakeEvent;
        nh->nh_InpIOReq->io_Length = sizeof(struct InputEvent);
        nh->nh_InpIOReq->io_Command = nh->nh_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
        DoIO((struct IORequest *) nh->nh_InpIOReq);
    }
    lastevent.ie_Class = IECLASS_RAWKEY;
    lastevent.ie_SubClass = 0;
    lastevent.ie_Code = 0xFF;
    lastevent.ie_NextEvent = NULL;
    lastevent.ie_Qualifier = 0;
    nh->nh_InpIOReq->io_Data = &lastevent;
    nh->nh_InpIOReq->io_Length = sizeof(struct InputEvent);
    nh->nh_InpIOReq->io_Command = nh->nh_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
    DoIO((struct IORequest *) nh->nh_InpIOReq);
    nFreeIEvents(nh, events);
    return(TRUE);
}
/* \\\ */

/* /// "nEasyRequest()" */
LONG nEasyRequest(struct NepHidBase *nh, STRPTR body, STRPTR gadgets, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(gadgets);
    ret = nEasyRequestA(nh, body, gadgets, AROS_SLOWSTACKFORMAT_ARG(gadgets));
    AROS_SLOWSTACKFORMAT_POST(gadgets);

    return ret;
}
/* \\\ */

/* /// "nEasyRequestA()" */
LONG nEasyRequestA(struct NepHidBase *nh, STRPTR body, STRPTR gadgets, RAWARG params)
{
    struct EasyStruct es;
    es.es_StructSize = sizeof(struct EasyStruct);
    es.es_Flags = 0;
    es.es_Title = GM_UNIQUENAME(libname);
    es.es_TextFormat = body;
    es.es_GadgetFormat = gadgets;
    return(EasyRequestArgs(NULL, &es, NULL, params));
}
/* \\\ */

