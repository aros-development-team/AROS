/*
 *----------------------------------------------------------------------------
 *                    bthid class for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * Binds to the HID services of registered Bluetooth devices - HID over GATT
 * (UUID 0x1812) on LE links and classic HIDP (UUID 0x1124) over L2CAP - and
 * turns their reports into input.device events, joystick port state, shell
 * commands and so on. The report parser, the per-item action engine and the
 * configuration GUI are those of Poseidon's hid.class (Chris Hodges); only
 * the transport (bluetooth.library channels instead of USB pipes) and the
 * binding/config plumbing differ, so a Bluetooth keyboard or mouse can be
 * configured exactly like a USB one.
 */

#include "debug.h"

#include "numtostr.h"
#include "bthid.h"

#include <proto/datatypes.h>

static LONG _rand(struct BTHidBase *nh)
{
    nh->nh_Seed = nh->nh_Seed * 1103515245 + 12345;

    return nh->nh_Seed;
}

/* /// "Lib Stuff" */
const STRPTR GM_UNIQUENAME(libname) = MOD_NAME_STRING;

/* The bindings list lives in the library base; it has been found zeroed
   at the first class method call on riscv64 - report and repair rather
   than crash the event handler (which then deadlocks BTStackLoader). */
static void bCheckBase(LIBBASETYPEPTR nh, CONST_STRPTR where)
{
    struct Library *BluetoothBase;
    if(nh->nh_Interfaces.lh_Head && nh->nh_Interfaces.lh_TailPred &&
       (nh->nh_Interfaces.lh_Tail == NULL))
    {
        return;
    }
    if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "%s: class base %08lx (%ld bytes) bindings list damaged (%08lx/%08lx/%08lx) - reinitialised.",
                       where, (IPTR) nh, (ULONG) sizeof(struct BTHidBase),
                       (IPTR) nh->nh_Interfaces.lh_Head, (IPTR) nh->nh_Interfaces.lh_Tail,
                       (IPTR) nh->nh_Interfaces.lh_TailPred);
        CloseLibrary(BluetoothBase);
    }
    NewList(&nh->nh_Interfaces);
}

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh)
{
    struct BTHidBinding *nhb;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define UtilityBase     nh->nh_UtilityBase

    if(!UtilityBase)
    {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        return(FALSE);
    }
    NewList(&nh->nh_Interfaces);
    /* the class defaults live in a binding without a service */
    nhb = &nh->nh_DefaultBinding;
    nhb->nhb_ClsBase = nh;
    nhb->nhb_Service = NULL;
    nhb->nhb_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
    if(!nhb->nhb_CDC)
    {
        CloseLibrary(UtilityBase);
        return(FALSE);
    }
    KPRINTF(10, ("libInit: Ok\n"));
    return(TRUE);
}

static int GM_UNIQUENAME(libOpen)(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    bCheckBase(nh, "libOpen");
    GM_UNIQUENAME(bLoadClassConfig)(nh);
    return(TRUE);
}

static int GM_UNIQUENAME(libClose)(LIBBASETYPEPTR nh)
{
    if(nh->nh_Library.lib_OpenCnt == 0)
    {
        /* last user gone: take the shell/sound dispatcher down with us */
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
    }
    KPRINTF(5, ("libClose: lib_OpenCnt = %ld\n", nh->nh_Library.lib_OpenCnt));
    return(TRUE);
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_Interfaces.lh_Head->ln_Succ)
    {
        return(FALSE);
    }
    if(nh->nh_LowLevelBase)
    {
        APTR ourvec;
        Disable();
        ourvec = SetFunction(nh->nh_LowLevelBase, -5 * LIB_VECTSIZE, nh->nh_LLOldReadJoyPort);
        if(ourvec != AROS_SLIB_ENTRY(bReadJoyPort, bthid, 5))
        {
            SetFunction(nh->nh_LowLevelBase, -5 * LIB_VECTSIZE, ourvec);
            Enable();
            return(FALSE); /* we couldn't remove the patch! */
        }
        ourvec = SetFunction(nh->nh_LowLevelBase, -22 * LIB_VECTSIZE, nh->nh_LLOldSetJoyPortAttrsA);
        if(ourvec != AROS_SLIB_ENTRY(bSetJoyPortAttrsA, bthid, 22))
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

    FreeVec(nh->nh_DefaultBinding.nhb_CDC);

    KPRINTF(5, ("libExpunge: Unloading done! bthid.class expunged!\n\n"));

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


/* /// "bAttemptServiceBinding()" */
struct BTHidBinding * GM_UNIQUENAME(bAttemptServiceBinding)(struct BTHidBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    IPTR uuid16 = 0;
    IPTR proto = 0;
    BOOL ishid = FALSE;

    KPRINTF(1, ("bAttemptServiceBinding(%08lx)\n", bsv));
    if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        btGetAttrs(BGA_SERVICE, bsv,
                   BSVA_UUID16, &uuid16,
                   BSVA_Protocol, &proto,
                   TAG_END);
        CloseLibrary(BluetoothBase);
        /* HID over GATT (HOGP) on LE, HID profile (HIDP) over L2CAP on BR/EDR */
        ishid = ((uuid16 == 0x1812) && (proto == BSVP_ATT)) ||
                ((uuid16 == 0x1124) && (proto == BSVP_L2CAP));
    }
    return(ishid ? GM_UNIQUENAME(bForceServiceBinding)(nh, bsv) : NULL);
}
/* \\\ */

/* /// "bForceServiceBinding()" */
struct BTHidBinding * GM_UNIQUENAME(bForceServiceBinding)(struct BTHidBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    struct BTHidBinding *nhb;
    struct BTHidBinding *ex;
    struct BtDevice *bd = NULL;
    IPTR proto = 0;
    STRPTR devname = NULL;
    STRPTR devidstr = NULL;
    STRPTR svcidstr = NULL;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("bForceServiceBinding(%08lx)\n", bsv));

    bInstallLLPatch(nh);
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(NULL);
    }
    btGetAttrs(BGA_SERVICE, bsv,
               BSVA_Protocol, &proto,
               BSVA_Device, &bd,
               BSVA_IDString, &svcidstr,
               TAG_END);
    btGetAttrs(BGA_DEVICE, bd,
               BDA_Name, &devname,
               BDA_IDString, &devidstr,
               TAG_END);

    /* One HID binding per physical device. A dual-mode device offers an HID
       service on BOTH bearers (HIDP and HOGP) but it is one keyboard/mouse -
       binding both would double every keystroke, so refuse a second. */
    Forbid();
    for(ex = (struct BTHidBinding *) nh->nh_Interfaces.lh_Head;
        ex->nhb_Node.ln_Succ; ex = (struct BTHidBinding *) ex->nhb_Node.ln_Succ)
    {
        if(ex->nhb_Device == bd)
        {
            Permit();
            CloseLibrary(BluetoothBase);
            return(NULL);
        }
    }
    Permit();

    if((nhb = btAllocVec(sizeof(struct BTHidBinding))))
    {
        nhb->nhb_ClsBase = nh;
        nhb->nhb_CDC = btAllocVec(sizeof(struct ClsDevCfg));
        if(!nhb->nhb_CDC)
        {
            btFreeVec(nhb);
            CloseLibrary(BluetoothBase);
            return(NULL);
        }

        nhb->nhb_Device = bd;
        nhb->nhb_Service = bsv;
        nhb->nhb_Classic = (proto == BSVP_L2CAP) ? TRUE : FALSE;
        nhb->nhb_DevIDString = devidstr;
        nhb->nhb_SvcIDString = svcidstr;

        GM_UNIQUENAME(bLoadBindingConfig)(nhb, FALSE);

        btSafeRawDoFmt(buf, 64, "bthid.class<%08lx>", (IPTR) nhb);
        nhb->nhb_ReadySignal = SIGB_SINGLE;
        nhb->nhb_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);

        if((tmptask = btSpawnSubTask(buf, (APTR) GM_UNIQUENAME(bHidTask), nhb)))
        {
            btBorrowLocksWait(tmptask, 1UL<<nhb->nhb_ReadySignal);
            if(nhb->nhb_Task)
            {
                nhb->nhb_ReadySigTask = NULL;
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "HID input from '%s' connected to input.device.",
                               devname ? devname : (STRPTR) "device");
                Forbid();
                AddTail(&nh->nh_Interfaces, &nhb->nhb_Node);
                Permit();
                CloseLibrary(BluetoothBase);
                return(nhb);
            }
        }
        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Binding to '%s' failed: %s.",
                       devname ? devname : (STRPTR) "device",
                       tmptask ? (STRPTR) "task died during init"
                               : (STRPTR) "btSpawnSubTask failed");
        nhb->nhb_ReadySigTask = NULL;
        btFreeVec(nhb->nhb_CDC);
        btFreeVec(nhb);
    }
    CloseLibrary(BluetoothBase);
    return(NULL);
}
/* \\\ */

/* /// "bReleaseServiceBinding()" */
void GM_UNIQUENAME(bReleaseServiceBinding)(struct BTHidBase *nh, struct BTHidBinding *nhb)
{
    struct Library *BluetoothBase;
    STRPTR devname = NULL;

    KPRINTF(1, ("bReleaseServiceBinding(%08lx)\n", nhb));
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return;
    }
    Forbid();
    nhb->nhb_ReadySignal = SIGB_SINGLE;
    nhb->nhb_ReadySigTask = FindTask(NULL);
    if(nhb->nhb_GUITask)
    {
        Signal(nhb->nhb_GUITask, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(nhb->nhb_GUITask)
    {
        Wait(1L<<nhb->nhb_ReadySignal);
    }

    Forbid();
    if(nhb->nhb_HCGUITask)
    {
        Signal(nhb->nhb_HCGUITask, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(nhb->nhb_HCGUITask)
    {
        Wait(1L<<nhb->nhb_ReadySignal);
    }

    Forbid();
    if(nhb->nhb_Task)
    {
        Signal(nhb->nhb_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(nhb->nhb_Task)
    {
        Wait(1L<<nhb->nhb_ReadySignal);
    }
    btGetAttrs(BGA_DEVICE, nhb->nhb_Device, BDA_Name, &devname, TAG_END);
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "HID input from '%s' released.",
                   devname ? devname : (STRPTR) "device");
    Forbid();
    Remove(&nhb->nhb_Node);
    Permit();
    btFreeVec(nhb->nhb_CDC);
    btFreeVec(nhb);
    CloseLibrary(BluetoothBase);
}
/* \\\ */


/* /// "btcGetAttrsA()" */
AROS_LH3(LONG, btcGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, bthid)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    KPRINTF(1, ("btcGetAttrsA(%ld, %08lx, %08lx)\n", type, btstruct, tags));
    switch(type)
    {
        case BCGA_CLASS:
             if((ti = FindTagItem(BCCA_Priority, tags)))
             {
                 *((SIPTR *) ti->ti_Data) = 0;
                 count++;
             }
             if((ti = FindTagItem(BCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "HID keyboards, mice, joypads and other human interface devices";
                 count++;
             }
             if((ti = FindTagItem(BCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(BCCA_HasBindingCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(BCCA_AfterDOSRestart, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(BCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nh->nh_DefaultBinding.nhb_UsingDefaultCfg;
                 count++;
             }
             break;

         case BCGA_BINDING:
         {
             struct BTHidBinding *nhb = (struct BTHidBinding *) btstruct;
             if((ti = FindTagItem(BCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nhb->nhb_UsingDefaultCfg;
                 count++;
             }
             if((ti = FindTagItem(BCBA_Device, tags)))
             {
                 *((struct BtDevice **) ti->ti_Data) = nhb->nhb_Device;
                 count++;
             }
             if((ti = FindTagItem(BCBA_Service, tags)))
             {
                 *((struct BtService **) ti->ti_Data) = nhb->nhb_Service;
                 count++;
             }
             if((ti = FindTagItem(BCBA_Task, tags)))
             {
                 *((struct Task **) ti->ti_Data) = nhb->nhb_Task;
                 count++;
             }
             break;
         }
    }
    return(count);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcSetAttrsA()" */
AROS_LH3(LONG, btcSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, bthid)
{
    AROS_LIBFUNC_INIT
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcDoMethodA()" */
AROS_LH2(SIPTR, btcDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, nh, 7, bthid)
{
    AROS_LIBFUNC_INIT

    struct BTHidBinding *nhb;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    bCheckBase(nh, "btcDoMethodA");
    switch(methodid)
    {
        case BCM_AttemptServiceBinding:
            return((SIPTR) GM_UNIQUENAME(bAttemptServiceBinding)(nh, (struct BtService *) methoddata[0]));

        case BCM_ForceServiceBinding:
            return((SIPTR) GM_UNIQUENAME(bForceServiceBinding)(nh, (struct BtService *) methoddata[0]));

        case BCM_ReleaseServiceBinding:
            GM_UNIQUENAME(bReleaseServiceBinding)(nh, (struct BTHidBinding *) methoddata[0]);
            return(TRUE);

        case BCM_AttemptDeviceBinding:
        case BCM_ForceDeviceBinding:
            return(0); /* only service bindings */

        case BCM_OpenCfgWindow:
            return(GM_UNIQUENAME(bOpenBindingCfgWindow)(nh, &nh->nh_DefaultBinding));

        case BCM_OpenBindingCfgWindow:
            return(GM_UNIQUENAME(bOpenBindingCfgWindow)(nh, (struct BTHidBinding *) methoddata[0]));

        case BCM_DOSAvailableEvent:
            bInstallLLPatch(nh);
            return(TRUE);

        case BCM_ConfigChangedEvent:
            GM_UNIQUENAME(bLoadClassConfig)(nh);

            Forbid();
            nhb = (struct BTHidBinding *) nh->nh_Interfaces.lh_Head;
            while(nhb->nhb_Node.ln_Succ)
            {
                GM_UNIQUENAME(bLoadBindingConfig)(nhb, TRUE);
                nhb = (struct BTHidBinding *) nhb->nhb_Node.ln_Succ;
            }
            Permit();
            return(TRUE);

        case BCM_DeviceDisconnected:
            /* the binding stays; its read channels fail and are re-issued when
               the device reconnects (BCHA_AutoConnect) */
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bInstallLLPatch()" */
void bInstallLLPatch(struct BTHidBase *nh)
{
    if(!nh->nh_LowLevelBase)
    {
        if((nh->nh_LowLevelBase = OpenLibrary("lowlevel.library", 40)))
        {
            Disable();
            nh->nh_LLOldReadJoyPort = SetFunction(nh->nh_LowLevelBase, -5 * LIB_VECTSIZE, AROS_SLIB_ENTRY(bReadJoyPort, bthid, 5));
            nh->nh_LLOldSetJoyPortAttrsA = SetFunction(nh->nh_LowLevelBase, -22 * LIB_VECTSIZE, AROS_SLIB_ENTRY(bSetJoyPortAttrsA, bthid, 22));
            Enable();
        }
    }
}
/* \\\ */

/* /// "bLoadClassConfig()" */
BOOL GM_UNIQUENAME(bLoadClassConfig)(struct BTHidBase *nh)
{
    struct BTHidBinding *nhb = &nh->nh_DefaultBinding;
    struct Library *BluetoothBase;
    struct KeymapCfg *kmc;
    struct ClsDevCfg *cdc;
    struct BtIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(nhb->nhb_GUITask)
    {
        return(FALSE);
    }
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(FALSE);
    }

    Forbid();
    /* Create default config */
    kmc = &nhb->nhb_KeymapCfg;
    kmc->kmc_ChunkID = AROS_LONG2BE(MAKE_ID('K','M','A','P'));
    kmc->kmc_Length = AROS_LONG2BE(sizeof(struct KeymapCfg)-8);
    CopyMemQuick(usbkeymap, kmc->kmc_Keymap, AROS_LONG2BE(kmc->kmc_Length));

    cdc = nhb->nhb_CDC;
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

    nhb->nhb_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = btGetClsCfg(GM_UNIQUENAME(libname));
    if(pic)
    {
        cdc = btGetCfgChunk(pic, AROS_LONG2BE(nhb->nhb_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nhb->nhb_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nhb->nhb_CDC->cdc_Length)));
            btFreeVec(cdc);
            nhb->nhb_UsingDefaultCfg = FALSE;
        }
        kmc = btGetCfgChunk(pic, AROS_LONG2BE(nhb->nhb_KeymapCfg.kmc_ChunkID));
        if(kmc)
        {
            CopyMem(((UBYTE *) kmc) + 8, ((UBYTE *) &nhb->nhb_KeymapCfg) + 8, min(AROS_LONG2BE(kmc->kmc_Length), AROS_LONG2BE(nhb->nhb_KeymapCfg.kmc_Length)));
            btFreeVec(kmc);
            nhb->nhb_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(BluetoothBase);
    return(FALSE);
}
/* \\\ */

/* /// "bCalcConfigCRC()" */
ULONG bCalcConfigCRC(struct BTHidBinding *nhb, struct Library *BluetoothBase, struct BtIFFContext *pic)
{
    ULONG *buf = (ULONG *) btWriteCfg(pic);
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
        btFreeVec(buf);
    }
    return(crc);
}
/* \\\ */

/* /// "bLoadBindingConfig()" */
BOOL GM_UNIQUENAME(bLoadBindingConfig)(struct BTHidBinding *nhb, BOOL gui)
{
    struct BTHidBase *nh = nhb->nhb_ClsBase;
    struct Library *BluetoothBase;
    struct ClsDevCfg *cdc;
    struct KeymapCfg *kmc;
    struct BtIFFContext *pic;
    ULONG crc = 0;

    KPRINTF(10, ("Loading Binding Config...\n"));
    if(nhb->nhb_GUITask)
    {
        return(FALSE);
    }
    //GM_UNIQUENAME(bLoadClassConfig)(nh);
    *nhb->nhb_CDC = *nh->nh_DefaultBinding.nhb_CDC;
    nhb->nhb_KeymapCfg = nh->nh_DefaultBinding.nhb_KeymapCfg;
    nhb->nhb_UsingDefaultCfg = TRUE;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = btGetDevCfg(GM_UNIQUENAME(libname), nhb->nhb_DevIDString, nhb->nhb_SvcIDString);
    if(pic)
    {
        cdc = btGetCfgChunk(pic, AROS_LONG2BE(nhb->nhb_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nhb->nhb_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nhb->nhb_CDC->cdc_Length)));
            btFreeVec(cdc);
            nhb->nhb_UsingDefaultCfg = FALSE;
        }
        kmc = btGetCfgChunk(pic, AROS_LONG2BE(nhb->nhb_KeymapCfg.kmc_ChunkID));
        if(kmc)
        {
            CopyMem(((UBYTE *) kmc) + 8, ((UBYTE *) &nhb->nhb_KeymapCfg) + 8, min(AROS_LONG2BE(kmc->kmc_Length), AROS_LONG2BE(nhb->nhb_KeymapCfg.kmc_Length)));
            btFreeVec(kmc);
            nhb->nhb_UsingDefaultCfg = FALSE;
        }
        crc = bCalcConfigCRC(nhb, BluetoothBase, pic);
    }
    if(crc != nhb->nhb_LastCfgCRC)
    {
        nhb->nhb_ReloadCfg = TRUE;
        nhb->nhb_LastCfgCRC = crc;
        if(nhb->nhb_Task)
        {
            Signal(nhb->nhb_Task, (1L<<nhb->nhb_TaskMsgPort->mp_SigBit));
        }
    }
    if(gui && !nhb->nhb_HCGUITask)
    {
        btSpawnSubTask(MOD_NAME_STRING " Control GUI", GM_UNIQUENAME(bHIDCtrlGUITask), nhb);
    }
    Permit();
    CloseLibrary(BluetoothBase);
    return(FALSE);
}
/* \\\ */

/* /// "bOpenBindingCfgWindow()" */
LONG GM_UNIQUENAME(bOpenBindingCfgWindow)(struct BTHidBase *nh, struct BTHidBinding *nhb)
{
    struct Library *BluetoothBase;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(FALSE);
    }
    Forbid();
    if(!nhb->nhb_GUITask)
    {
        if((nhb->nhb_GUITask = btSpawnSubTask(MOD_NAME_STRING " GUI", GM_UNIQUENAME(bGUITask), nhb)))
        {
            Permit();
            CloseLibrary(BluetoothBase);
            return(TRUE);
        }
    }
    Permit();
    CloseLibrary(BluetoothBase);
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

#undef  BluetoothBase
#define BluetoothBase nhb->nhb_Base

/* /// "bSendReport()" */
/* Delivers an output (type 2) or feature (type 3) report to the device.
   HOGP: written to the Report characteristic with that Report Reference
   (no report id byte in the value). HIDP: a DATA packet on the interrupt
   channel for output reports, SET_REPORT on the control channel for
   features (the report id byte in front when the device uses ids). */
static void bSendReport(struct BTHidBinding *nhb, UWORD reportid, UBYTE type, UBYTE *data, ULONG len)
{
    APTR ch;
    LONG ioerr = 0;

    if(nhb->nhb_Classic)
    {
        struct BtEndpoint *bep;
        UBYTE pkt[BTHID_MAXREPORT + 2];
        ULONG plen = 0;
        pkt[plen++] = (type == 2) ? 0xa2 : 0x53;     /* DATA|Output / SET_REPORT|Feature */
        if(nhb->nhb_UsesReportID)
        {
            pkt[plen++] = reportid;
        }
        if(len > sizeof(pkt) - plen)
        {
            len = sizeof(pkt) - plen;
        }
        CopyMem(data, &pkt[plen], len);
        plen += len;
        bep = btFindEndpoint(nhb->nhb_Service, NULL, BEA_PSM, (type == 2) ? 0x0013 : 0x0011, TAG_END);
        if(bep && (ch = btAllocChannel(nhb->nhb_Device, nhb->nhb_TaskMsgPort, bep)))
        {
            btSetAttrs(BGA_CHANNEL, ch, BCHA_AutoConnect, TRUE, TAG_END);
            btChannelSetup(ch, BTPR_WRITE, 0, 0);
            ioerr = btDoChannel(ch, pkt, plen);
            btFreeChannel(ch);
        } else {
            ioerr = BTIOERR_NOTSUPPORTED;
        }
    } else {
        struct BtEndpoint *bep = NULL;
        IPTR handle = 0;
        UWORD n;
        for(n = 0; n < nhb->nhb_NumRepEP; n++)
        {
            if((nhb->nhb_RepEPType[n] == type) && (!nhb->nhb_UsesReportID || (nhb->nhb_RepEPID[n] == reportid)))
            {
                bep = nhb->nhb_RepEP[n];
                break;
            }
        }
        if(bep && (ch = btAllocChannel(nhb->nhb_Device, nhb->nhb_TaskMsgPort, NULL)))
        {
            btGetAttrs(BGA_ENDPOINT, bep, BEA_Handle, &handle, TAG_END);
            btSetAttrs(BGA_CHANNEL, ch, BCHA_AutoConnect, TRUE, TAG_END);
            btChannelSetup(ch, (type == 2) ? BTPR_GATTWRITENORSP : BTPR_GATTWRITE, handle, 0);
            ioerr = btDoChannel(ch, data, len);
            btFreeChannel(ch);
        } else {
            ioerr = BTIOERR_NOTSUPPORTED;
        }
    }
    if(ioerr)
    {
        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Sending %s report %ld (%ld bytes) failed: %s (%ld).",
                       (type == 2) ? "output" : "feature", (ULONG) reportid, len,
                       btNumToStr(BNTS_IOERR, ioerr, "unknown"), ioerr);
    }
}
/* \\\ */

/* /// "bHandleReport()" */
/* An input report arrived on read channel n: run every input item of that
   report through the action engine, then re-issue the read. */
static void bHandleReport(struct BTHidBinding *nhb, UWORD n)
{
    APTR ch = nhb->nhb_ReadCh[n];
    LONG ioerr = btGetChannelError(ch);

    if(!ioerr)
    {
        ULONG actual = btGetChannelActual(ch);
        UBYTE *buf = nhb->nhb_ReadBuf[n];
        UBYTE *bufreal;
        UWORD reportid;
        struct BtHidReport *nhr;

        if(nhb->nhb_Classic && actual)
        {
            /* HIDP header byte (0xa1 = DATA, Input report) */
            buf++;
            actual--;
        }
        if(actual)
        {
            if(nhb->nhb_Classic)
            {
                if(nhb->nhb_UsesReportID)
                {
                    reportid = buf[0];
                    bufreal = &buf[1];
                } else {
                    reportid = 0;
                    bufreal = buf;
                }
            } else {
                /* HOGP notifications carry no report id byte: the id is the
                   characteristic's Report Reference */
                reportid = nhb->nhb_UsesReportID ? nhb->nhb_ReadID[n] : 0;
                bufreal = buf;
            }
            if((reportid <= nhb->nhb_MaxReportID) && (nhr = nhb->nhb_ReportMap[reportid]))
            {
                UWORD count = nhr->nhr_InItemCount;
                struct BtHidItem **nhiptr = nhr->nhr_InItemMap;
                struct BtHidItem *nhi;
                KPRINTF(40, ("Report %ld\n", reportid));
                if(count && nhiptr)
                {
                    do
                    {
                        if((nhi = *nhiptr++))
                        {
                            bProcessItem(nhb, nhi, bufreal);
                        }
                    } while(--count);
                }
            } else {
                KPRINTF(10, ("Illegal report ID %ld received!\n", reportid));
            }
        }
        nhb->nhb_LastErr = 0;
    }
    else if((ioerr == IOERR_ABORTED) || (ioerr == BTIOERR_NOTCONNECTED))
    {
        /* link down: the next read waits for the device to come back
           (BCHA_AutoConnect), do not spin meanwhile */
        btDelayMS(1000);
    } else {
        if(ioerr != nhb->nhb_LastErr)
        {
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "Reading input reports failed: %s (%ld).",
                           btNumToStr(BNTS_IOERR, ioerr, "unknown"), ioerr);
            nhb->nhb_LastErr = ioerr;
        }
        btDelayMS(250);
    }
    if(!(SetSignal(0, 0) & SIGBREAKF_CTRL_C))
    {
        btSendChannel(ch, nhb->nhb_ReadBuf[n], nhb->nhb_ReadBufSize);
    }
}
/* \\\ */

/* /// "bHidTask()" */
AROS_UFH0(void, GM_UNIQUENAME(bHidTask))
{
    AROS_USERFUNC_INIT

    struct BTHidBinding *nhb;
    ULONG sigmask;
    ULONG sigs;
    UWORD n;
    struct BtHidReport *nhr;

    if((nhb = GM_UNIQUENAME(bAllocHid())))
    {
        Forbid();
        if(nhb->nhb_ReadySigTask)
        {
            Signal(nhb->nhb_ReadySigTask, 1L<<nhb->nhb_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nhb->nhb_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;

        if(nhb->nhb_XtraInitItem)
        {
            struct BtHidAction *nha = (struct BtHidAction *) nhb->nhb_XtraInitItem->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                KPRINTF(1, ("Executing InitItem %08lx\n", nha));
                bDoAction(nhb, nha, nhb->nhb_XtraInitItem, 0, 1, TRUE);
                nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
            }
        }
        bFlushEvents(nhb);

        Forbid();
        if(!nhb->nhb_HCGUITask)
        {
            btSpawnSubTask(MOD_NAME_STRING " Control GUI", (APTR) GM_UNIQUENAME(bHIDCtrlGUITask), nhb);
        }
        Permit();

        if(!nhb->nhb_HasInItems)
        {
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "HID device has no input items, input disabled.");
        }
        nhb->nhb_ReloadCfg = FALSE;
        nhb->nhb_Running = TRUE;
        /* the first read on every input report channel */
        for(n = 0; n < nhb->nhb_NumCh; n++)
        {
            btSendChannel(nhb->nhb_ReadCh[n], nhb->nhb_ReadBuf[n], nhb->nhb_ReadBufSize);
        }
        do
        {
            APTR ch;
            sigs = Wait(sigmask);
            while((ch = (APTR) GetMsg(nhb->nhb_TaskMsgPort)))
            {
                for(n = 0; n < nhb->nhb_NumCh; n++)
                {
                    if(ch == nhb->nhb_ReadCh[n])
                    {
                        bHandleReport(nhb, n);
                        break;
                    }
                }
            }
            bFlushEvents(nhb);
            if(nhb->nhb_TrackEvents || nhb->nhb_TrackKeyEvents || (nhb->nhb_ReportValues && nhb->nhb_ItemChanged))
            {
                if(nhb->nhb_GUITask)
                {
                    Signal(nhb->nhb_GUITask, (ULONG) (1<<nhb->nhb_TrackingSignal));
                }
            }
            if(!nhb->nhb_HIntBase) /* attempt to open intuition */
            {
                nhb->nhb_HIntBase = OpenLibrary("intuition.library", 39);
            }

            /* need to update prefs? */
            if(nhb->nhb_ReloadCfg)
            {
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Reloading configuration...");
                Forbid();
                nhb->nhb_ReadySignal = SIGB_SINGLE;
                nhb->nhb_ReadySigTask = FindTask(NULL);
                if(nhb->nhb_GUITask)
                {
                    Signal(nhb->nhb_GUITask, SIGBREAKF_CTRL_C);
                }
                Permit();
                while(nhb->nhb_GUITask)
                {
                    Wait(1L<<nhb->nhb_ReadySignal);
                }
                Forbid();
                if(nhb->nhb_HCGUITask)
                {
                    Signal(nhb->nhb_HCGUITask, SIGBREAKF_CTRL_C);
                }
                Permit();
                while(nhb->nhb_HCGUITask)
                {
                    Wait(1L<<nhb->nhb_ReadySignal);
                }
                nhb->nhb_ReadySigTask = NULL;
                Forbid();
                nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
                while(nhr->nhr_Node.ln_Succ)
                {
                    bFreeReport(nhb, nhr);
                    nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
                }
                btFreeVec(nhb->nhb_ReportMap);
                nhb->nhb_ReportMap = NULL;
                Permit();
                if(bReadReports(nhb))
                {
                    bAddExtraReport(nhb);
                } else {
                    btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Error parsing the HID report map!");
                    sigs |= SIGBREAKF_CTRL_C;
                }
                nhb->nhb_ReloadCfg = FALSE;
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        if(nhb->nhb_XtraQuitItem)
        {
            struct BtHidAction *nha = (struct BtHidAction *) nhb->nhb_XtraQuitItem->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                bDoAction(nhb, nha, nhb->nhb_XtraQuitItem, 0, 1, TRUE);
                nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
            }
            bFlushEvents(nhb);
        }

        KPRINTF(20, ("Going down the river!\n"));
        GM_UNIQUENAME(bFreeHid)(nhb);
    }

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bOpenReportChannels()" */
/* Allocates one read channel per input report: the HIDP interrupt channel
   (PSM 0x13) on BR/EDR, one per notifying Report characteristic (0x2a4d) on
   LE. The Report Reference descriptor (report id + type) is readable only on
   an encrypted link, so the stack's enumeration may not have it - read it
   here for every Report characteristic, output/feature ones included (they
   are what bSendReport() writes to). */
static BOOL bOpenReportChannels(struct BTHidBinding *nhb)
{
    struct BtEndpoint *bep = NULL;
    UWORD n;

    nhb->nhb_ReadBufSize = nhb->nhb_MaxInSize + 2;
    if(nhb->nhb_ReadBufSize < 64)
    {
        nhb->nhb_ReadBufSize = 64;
    }
    if(nhb->nhb_ReadBufSize > BTHID_MAXREPORT)
    {
        nhb->nhb_ReadBufSize = BTHID_MAXREPORT;
    }
    nhb->nhb_NumCh = 0;
    nhb->nhb_NumRepEP = 0;

    if(nhb->nhb_Classic)
    {
        bep = btFindEndpoint(nhb->nhb_Service, NULL, BEA_PSM, 0x0013, TAG_END);
        if(bep && (nhb->nhb_ReadCh[0] = btAllocChannel(nhb->nhb_Device, nhb->nhb_TaskMsgPort, bep)))
        {
            btSetAttrs(BGA_CHANNEL, nhb->nhb_ReadCh[0], BCHA_AutoConnect, TRUE, TAG_END);
            btChannelSetup(nhb->nhb_ReadCh[0], BTPR_READ, 0, 0);
            nhb->nhb_ReadID[0] = 0;
            nhb->nhb_NumCh = 1;
        }
    } else {
        while((bep = btFindEndpoint(nhb->nhb_Service, bep, BEA_UUID16, 0x2a4d, TAG_END)))
        {
            IPTR rtype = 0, rid = 0, ref = 0, canread = 0;
            btGetAttrs(BGA_ENDPOINT, bep, BEA_ReportType, &rtype, BEA_ReportID, &rid,
                       BEA_ReportRefHandle, &ref, BEA_CanRead, &canread, TAG_END);
            if(!rtype && ref)
            {
                APTR rch;
                if((rch = btAllocChannel(nhb->nhb_Device, nhb->nhb_TaskMsgPort, NULL)))
                {
                    UBYTE rr[4];
                    btSetAttrs(BGA_CHANNEL, rch, BCHA_AutoConnect, TRUE, TAG_END);
                    btChannelSetup(rch, BTPR_GATTREAD, ref, 0);
                    if(!btDoChannel(rch, rr, sizeof(rr)) && (btGetChannelActual(rch) >= 2))
                    {
                        rid = rr[0];
                        rtype = rr[1];
                    }
                    btFreeChannel(rch);
                }
            }
            if(nhb->nhb_NumRepEP < BTHID_MAXREPORTEPS)
            {
                nhb->nhb_RepEP[nhb->nhb_NumRepEP] = bep;
                nhb->nhb_RepEPID[nhb->nhb_NumRepEP] = (UBYTE) rid;
                nhb->nhb_RepEPType[nhb->nhb_NumRepEP] = (UBYTE) rtype;
                nhb->nhb_NumRepEP++;
            }
            if(!canread || (rtype && (rtype != 1)) || (nhb->nhb_NumCh >= BTHID_MAXCHANNELS))
            {
                continue;              /* output / feature report, or full */
            }
            if((nhb->nhb_ReadCh[nhb->nhb_NumCh] = btAllocChannel(nhb->nhb_Device, nhb->nhb_TaskMsgPort, bep)))
            {
                nhb->nhb_ReadID[nhb->nhb_NumCh] = (UBYTE) rid;
                btSetAttrs(BGA_CHANNEL, nhb->nhb_ReadCh[nhb->nhb_NumCh], BCHA_AutoConnect, TRUE, TAG_END);
                btChannelSetup(nhb->nhb_ReadCh[nhb->nhb_NumCh], BTPR_READ, 0, 0);
                nhb->nhb_NumCh++;
            }
        }
    }
    for(n = 0; n < nhb->nhb_NumCh; n++)
    {
        if(!(nhb->nhb_ReadBuf[n] = btAllocVec(nhb->nhb_ReadBufSize)))
        {
            return(FALSE);
        }
    }
    if(!nhb->nhb_NumCh && nhb->nhb_HasInItems)
    {
        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "No input report endpoint found.");
        return(FALSE);
    }
    return(TRUE);
}
/* \\\ */

/* /// "bCloseReportChannels()" */
static void bCloseReportChannels(struct BTHidBinding *nhb)
{
    UWORD n;
    for(n = 0; n < nhb->nhb_NumCh; n++)
    {
        if(nhb->nhb_ReadCh[n])
        {
            btAbortChannel(nhb->nhb_ReadCh[n]);
            btWaitChannel(nhb->nhb_ReadCh[n]);
            btFreeChannel(nhb->nhb_ReadCh[n]);
            nhb->nhb_ReadCh[n] = NULL;
        }
        btFreeVec(nhb->nhb_ReadBuf[n]);
        nhb->nhb_ReadBuf[n] = NULL;
    }
    nhb->nhb_NumCh = 0;
}
/* \\\ */

/* /// "bAllocHid()" */
struct BTHidBinding * GM_UNIQUENAME(bAllocHid)(void)
{
    struct Task *thistask;
    struct BTHidBinding *nhb;

    thistask = FindTask(NULL);
#undef IntuitionBase
    nhb = thistask->tc_UserData;

    NewList(&nhb->nhb_HidReports);
    do
    {
        if(!(nhb->nhb_Base = OpenLibrary("bluetooth.library", 1)))
        {
            Alert(AG_OpenLib);
            break;
        }
        nhb->nhb_HIntBase = OpenLibrary("intuition.library", 39); /* optional at this point */
#define IntuitionBase nhb->nhb_HIntBase

        if((nhb->nhb_InpMsgPort = CreateMsgPort()))
        {
            if((nhb->nhb_InpIOReq = (struct IOStdReq *) CreateIORequest(nhb->nhb_InpMsgPort, sizeof(struct IOStdReq))))
            {
                if(!OpenDevice("input.device", 0, (struct IORequest *) nhb->nhb_InpIOReq, 0))
                {
                    nhb->nhb_InputBase = (struct Library *) nhb->nhb_InpIOReq->io_Device;
#define InputBase nhb->nhb_InputBase
                    nhb->nhb_OS4Hack = TRUE;
                    nhb->nhb_ClsBase->nh_OS4Hack = TRUE;

                    if((nhb->nhb_TaskMsgPort = CreateMsgPort()))
                    {
                        if(bReadReports(nhb))
                        {
                            bAddExtraReport(nhb);
                            if(bOpenReportChannels(nhb))
                            {
                                if((nhb->nhb_EPOutBuf = btAllocVec(nhb->nhb_MaxReportSize + 2)))
                                {
                                    nhb->nhb_Task = thistask;
                                    return(nhb);
                                }
                            }
                            bCloseReportChannels(nhb);
                        } else {
                            btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Error parsing the HID report map!");
                        }
                        {
                            struct BtHidReport *nhr;
                            nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
                            while(nhr->nhr_Node.ln_Succ)
                            {
                                bFreeReport(nhb, nhr);
                                nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
                            }
                            btFreeVec(nhb->nhb_ReportMap);
                            nhb->nhb_ReportMap = NULL;
                        }
                        DeleteMsgPort(nhb->nhb_TaskMsgPort);
                        nhb->nhb_TaskMsgPort = NULL;
                    }
                    else
                    {
                        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "failed to create message port");
                    }
                    CloseDevice((struct IORequest *) nhb->nhb_InpIOReq);
                }
                else
                {
                    btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "failed to open input.device");
                }
                DeleteIORequest((struct IORequest *) nhb->nhb_InpIOReq);
            }
            else
            {
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "failed to allocate IO request");
            }
            DeleteMsgPort(nhb->nhb_InpMsgPort);
        }
    } while(FALSE);
    CloseLibrary(nhb->nhb_HIntBase);
    nhb->nhb_HIntBase = NULL;
    CloseLibrary(nhb->nhb_Base);
    nhb->nhb_Base = NULL;
    Forbid();
    nhb->nhb_Task = NULL;
    if(nhb->nhb_ReadySigTask)
    {
        Signal(nhb->nhb_ReadySigTask, 1L<<nhb->nhb_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "bFreeHid()" */
void GM_UNIQUENAME(bFreeHid)(struct BTHidBinding *nhb)
{
    struct BtHidReport *nhr;

    bCloseReportChannels(nhb);
    nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
    while(nhr->nhr_Node.ln_Succ)
    {
        bFreeReport(nhb, nhr);
        nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
    }
    btFreeVec(nhb->nhb_ReportMap);
    nhb->nhb_ReportMap = NULL;
    btFreeVec(nhb->nhb_EPOutBuf);
    nhb->nhb_EPOutBuf = NULL;
    DeleteMsgPort(nhb->nhb_TaskMsgPort);
    nhb->nhb_TaskMsgPort = NULL;
    CloseDevice((struct IORequest *) nhb->nhb_InpIOReq);
    DeleteIORequest((struct IORequest *) nhb->nhb_InpIOReq);
    DeleteMsgPort(nhb->nhb_InpMsgPort);
    CloseLibrary(nhb->nhb_HIntBase);
    nhb->nhb_HIntBase = NULL;
    CloseLibrary(nhb->nhb_Base);
    nhb->nhb_Base = NULL;
    Forbid();
    nhb->nhb_Task = NULL;
    if(nhb->nhb_ReadySigTask)
    {
        Signal(nhb->nhb_ReadySigTask, 1L<<nhb->nhb_ReadySignal);
    }
}
/* \\\ */

/* /// "bFreeItem()" */
void bFreeItem(struct BTHidBinding *nhb, struct BtHidItem *nhi)
{
    struct BtHidAction *nha;
    struct List *lst;
    ULONG count;

    Remove(&nhi->nhi_Node);
    btFreeVec(nhi->nhi_UsageMap);
    /* Oh, this was an array */
    if((lst = nhi->nhi_ActionMap))
    {
        /* We need to free all lists available */
        count = nhi->nhi_MapSize;
        do
        {
           /* And each list can have several entries */
           nha = (struct BtHidAction *) lst->lh_Head;
           while(nha->nha_Node.ln_Succ)
           {
               Remove(&nha->nha_Node);
               btFreeVec(nha);
               nha = (struct BtHidAction *) lst->lh_Head;
           }
           lst++;
        } while(--count);
        btFreeVec(nhi->nhi_ActionMap);
    }
    /* Free action list */
    nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        Remove(&nha->nha_Node);
        btFreeVec(nha);
        nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
    }
    btFreeVec(nhi->nhi_Buffer);
    btFreeVec(nhi);
}
/* \\\ */

/* /// "bCleanCollection()" */
void bCleanCollection(struct BTHidBinding *nhb, struct BtHidCollection *nhc)
{
    struct BtHidItem *nhi;
    struct BtHidAction *nha;
    struct List *lst;
    ULONG count;

    nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
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
               nha = (struct BtHidAction *) lst->lh_Head;
               while(nha->nha_Node.ln_Succ)
               {
                   Remove(&nha->nha_Node);
                   btFreeVec(nha);
                   nha = (struct BtHidAction *) lst->lh_Head;
               }
               lst++;
            } while(--count);
        }
        /* Free action list */
        nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
        while(nha->nha_Node.ln_Succ)
        {
            Remove(&nha->nha_Node);
            btFreeVec(nha);
            nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
        }
        nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "bFreeReport()" */
void bFreeReport(struct BTHidBinding *nhb, struct BtHidReport *nhr)
{
    struct BtHidCollection *nhc;
    struct BtHidItem *nhi;

    Remove(&nhr->nhr_Node);
    nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
    while(nhc->nhc_Node.ln_Succ)
    {
        Remove(&nhc->nhc_Node);
        nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
        while(nhi->nhi_Node.ln_Succ)
        {
            bFreeItem(nhb, nhi);
            nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
        }
        btFreeVec(nhc->nhc_Name);
        btFreeVec(nhc);
        nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
    }
    btFreeVec(nhr->nhr_InItemMap);
    btFreeVec(nhr->nhr_ReportBuf);
    btFreeVec(nhr);
}
/* \\\ */

/* /// "bAddItemAction()" */
void bAddItemAction(struct BTHidBinding *nhb, struct BtIFFContext *rppic, struct BtHidItem *nhi, UWORD id)
{
    struct BtHidCollection *nhc = nhi->nhi_Collection;
    NewList(&nhi->nhi_ActionList);
    SET_WTYPE(&nhi->nhi_ActionList, id + 0xf001);
    AddTail(&nhc->nhc_Items, &nhi->nhi_Node);
    if(rppic)
    {
        if(!bLoadItem(nhb, rppic, &nhi->nhi_ActionList, 0xf001))
        {
            bDetectDefaultAction(nhb, nhi, &nhi->nhi_ActionList, nhc, nhi->nhi_Usage);
        }
    } else {
        bDetectDefaultAction(nhb, nhi, &nhi->nhi_ActionList, nhc, nhi->nhi_Usage);
    }
}
/* \\\ */

/* /// "bAddExtraReport()" */
BOOL bAddExtraReport(struct BTHidBinding *nhb)
{
    struct BtHidCollection *nhc;
    struct BtHidReport *nhr;
    struct BtHidItem *nhi;
    struct BtIFFContext *pic;
    struct BtIFFContext *rppic = NULL;
    UWORD item;
    ULONG pos;

    /* find rumble motors for rumble pack */
    if((item = bFindItemUsage(nhb, 0x080045, REPORT_MAIN_OUTPUT)))
    {
        nhb->nhb_RumbleMotors[0] = bFindItemID(nhb, item, REPORT_MAIN_OUTPUT, &pos);
    }
    if((item = bFindItemUsage(nhb, 0x080046, REPORT_MAIN_OUTPUT)))
    {
        nhb->nhb_RumbleMotors[1] = bFindItemID(nhb, item, REPORT_MAIN_OUTPUT, &pos);
    }

    pic = btGetDevCfg(GM_UNIQUENAME(libname), nhb->nhb_DevIDString, nhb->nhb_SvcIDString);
    if(pic)
    {
        rppic = btFindCfgForm(pic, MAKE_ID('X','R','P','T'));
    }
    if((nhr = btAllocVec(sizeof(struct BtHidReport))))
    {
        NewList(&nhr->nhr_Collections);
        AddTail(&nhb->nhb_HidReports, &nhr->nhr_Node);
        nhr->nhr_ReportID = 0xffff;
        nhr->nhr_ItemIDBase = 0xf001;
        if((nhc = btAllocVec(sizeof(struct BtHidCollection))))
        {
            NewList(&nhc->nhc_Items);
            AddTail(&nhr->nhr_Collections, &nhc->nhc_Node);
            nhc->nhc_Report = nhr;
            nhc->nhc_Name = btCopyStr("[Extra]");
            if((nhi = btAllocVec(sizeof(struct BtHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 1;
                nhi->nhi_Usage = 0x1fff0;
                bAddItemAction(nhb, rppic, nhi, 0xff0);
                nhb->nhb_XtraInitItem = nhi;
            }
            if((nhi = btAllocVec(sizeof(struct BtHidItem))))
            {
                nhi->nhi_Collection = nhc;
                nhi->nhi_Type = REPORT_MAIN_INPUT;
                nhi->nhi_Flags = RPF_MAIN_VARIABLE;

                nhi->nhi_PhysicalMin = nhi->nhi_LogicalMin = 0;
                nhi->nhi_PhysicalMax = nhi->nhi_LogicalMax = 1;
                nhi->nhi_Usage = 0x1fff1;
                bAddItemAction(nhb, rppic, nhi, 0xff1);
                nhb->nhb_XtraQuitItem = nhi;
            }
        }
    }

    return(FALSE);
}
/* \\\ */





/* /// "bReadReports()" */
/* Fetches the HID report map of the service and builds the report/item
   tree from it. Classic HIDP: the descriptor came with the SDP record
   (HIDDescriptorList). HOGP: read the Report Map characteristic (0x2a4b) -
   readable on an encrypted link only, i.e. after pairing. */
BOOL bReadReports(struct BTHidBinding *nhb)
{
    struct BtHidReport *nhr;
    struct BtHidCollection *nhc;
    struct BtHidItem *nhi;
    UBYTE mapbuf[BTHID_MAXREPORTMAP];
    const UBYTE *map = mapbuf;
    ULONG maplen = 0;
    LONG ioerr = -1;
    BOOL res = FALSE;
    UWORD idnum;
    ULONG count;
    struct List *alistptr;
    STRPTR devname = NULL;

    btGetAttrs(BGA_DEVICE, nhb->nhb_Device, BDA_Name, &devname, TAG_END);
    if(!devname)
    {
        devname = "device";
    }
    if(nhb->nhb_Classic)
    {
        IPTR desc = 0, desclen = 0;
        btGetAttrs(BGA_SERVICE, nhb->nhb_Service,
                   BSVA_HIDDescriptor, &desc,
                   BSVA_HIDDescriptorLen, &desclen,
                   TAG_END);
        if(desc && desclen)
        {
            map = (const UBYTE *) desc;
            maplen = (ULONG) desclen;
            ioerr = 0;
        } else {
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "The HID SDP record of '%s' carries no report descriptor.", devname);
        }
    } else {
        struct BtEndpoint *bep = btFindEndpoint(nhb->nhb_Service, NULL, BEA_UUID16, 0x2a4b, TAG_END);
        APTR ch;
        IPTR handle = 0;
        if(!bep)
        {
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "The HID service of '%s' has no Report Map characteristic (0x2a4b) - characteristics not enumerated?",
                           devname);
        }
        else if((ch = btAllocChannel(nhb->nhb_Device, nhb->nhb_TaskMsgPort, NULL)))
        {
            btGetAttrs(BGA_ENDPOINT, bep, BEA_Handle, &handle, TAG_END);
            btSetAttrs(BGA_CHANNEL, ch, BCHA_AutoConnect, TRUE, TAG_END);
            btChannelSetup(ch, BTPR_GATTREAD, handle, 0);
            ioerr = btDoChannel(ch, mapbuf, sizeof(mapbuf));
            maplen = btGetChannelActual(ch);
            btFreeChannel(ch);
            if(ioerr)
            {
                /* ATT 0x05 insufficient authentication, 0x08 insufficient
                   authorization, 0x0f insufficient encryption: HID report maps
                   are only readable on an encrypted link, i.e. pair first. */
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "Cannot read the HID report map of '%s' (error %ld, ATT 0x%02lx)%s.",
                               devname, (LONG) ioerr, (ULONG) maplen,
                               ((maplen == 0x05) || (maplen == 0x08) || (maplen == 0x0f)) ? (STRPTR) " - the link is not encrypted, pair the device first" : (STRPTR) "");
            }
        }
    }
    if(!ioerr && maplen)
    {
        if((nhr = btAllocVec(sizeof(struct BtHidReport))))
        {
            NewList(&nhr->nhr_Collections);
            AddTail(&nhb->nhb_HidReports, &nhr->nhr_Node);
            nhr->nhr_ReportLength = maplen;
            if((nhr->nhr_ReportBuf = btAllocVec(maplen)))
            {
                CopyMem((APTR) map, nhr->nhr_ReportBuf, maplen);
                res = bParseReport(nhb, nhr);
                if(!res)
                {
                    ULONG o;
                    btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "The HID report map of '%s' (%ld bytes) could not be parsed.", devname, maplen);
                    for(o = 0; o < maplen; o += 24)
                    {
                        char hex[3 * 24 + 1];
                        ULONG n, p = 0;
                        for(n = 0; (n < 24) && (o + n < maplen); n++)
                        {
                            p += snprintf(hex + p, sizeof(hex) - p, "%02lx ", (ULONG) map[o + n]);
                        }
                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "map[%03ld]: %s", o, hex);
                    }
                }
            }
            if(!res)
            {
                bFreeReport(nhb, nhr);
            }
        }
    }
    if(res)
    {
        ULONG repsize;
        /* Get maximal package size */
        nhb->nhb_MaxReportID = 0;
        nhb->nhb_MaxInSize = 0;
        nhb->nhb_MaxOutSize = 0;
        nhb->nhb_MaxFeatSize = 0;
        nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            KPRINTF(5, ("Report %ld: InBits=%ld, OutBits=%ld, FeatBits=%ld\n",
                    nhr->nhr_ReportID, nhr->nhr_ReportInSize,
                    nhr->nhr_ReportOutSize, nhr->nhr_ReportFeatSize));
            if(nhr->nhr_ReportID)
            {
                nhb->nhb_UsesReportID = TRUE;
                if(nhb->nhb_MaxReportID < nhr->nhr_ReportID)
                {
                    nhb->nhb_MaxReportID = nhr->nhr_ReportID;
                }
            }
            repsize = (nhr->nhr_ReportInSize+7)>>3;
            if(nhb->nhb_MaxInSize < repsize)
            {
                nhb->nhb_MaxInSize = repsize;
            }
            repsize = (nhr->nhr_ReportOutSize+7)>>3;
            if(nhb->nhb_MaxOutSize < repsize)
            {
                nhb->nhb_MaxOutSize = repsize;
            }
            repsize = (nhr->nhr_ReportFeatSize+7)>>3;
            if(nhb->nhb_MaxFeatSize < repsize)
            {
                nhb->nhb_MaxFeatSize = repsize;
            }
            nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
        }
        if(nhb->nhb_UsesReportID)
        {
            nhb->nhb_MaxInSize++;
            nhb->nhb_MaxOutSize++;
            nhb->nhb_MaxFeatSize++;
        }

        nhb->nhb_MaxReportSize = nhb->nhb_MaxInSize;
        if(nhb->nhb_MaxOutSize > nhb->nhb_MaxReportSize)
        {
            nhb->nhb_MaxReportSize = nhb->nhb_MaxOutSize;
        }
        if(nhb->nhb_MaxFeatSize > nhb->nhb_MaxReportSize)
        {
            nhb->nhb_MaxReportSize = nhb->nhb_MaxFeatSize;
        }

        KPRINTF(5, ("MaxReportID=%ld, MaxReportSize=%ld\n", nhb->nhb_MaxReportID, nhb->nhb_MaxReportSize));

        if((nhb->nhb_ReportMap = btAllocVec((ULONG) sizeof(struct BtHidReport *) * (nhb->nhb_MaxReportID+1))))
        {
            /* Fill in Report Map */
            nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
            while(nhr->nhr_Node.ln_Succ)
            {
                KPRINTF(5, ("Report Map slot %ld/%ld filled with %08lx\n", nhr->nhr_ReportID, nhb->nhb_MaxReportID, nhr));
                nhb->nhb_ReportMap[nhr->nhr_ReportID] = nhr;
                nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
            }
        } else {
            KPRINTF(10, ("Out of memory for Report Map!\n"));
            res = FALSE;
        }
    }

    if(res)
    {
        struct BtHidItem **nhiptr;
        struct BtHidItem **nhiinptr;
        struct BtHidItem **nhioutptr;
        struct BtHidItem **nhifeatptr;

        /* Count items */
        idnum = 1;
        nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            KPRINTF(1, ("Report %ld:\n", nhr->nhr_ReportID));
            nhr->nhr_ItemIDBase = idnum;
            nhr->nhr_InItemCount = nhr->nhr_OutItemCount = nhr->nhr_FeatItemCount = 0;
            nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
            while(nhc->nhc_Node.ln_Succ)
            {
                KPRINTF(1, ("  Collection %s:\n", nhc->nhc_Name));
                nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
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

                    nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                }
                nhc = (struct BtHidCollection *) nhc->nhc_Node.ln_Succ;
            }
            /*btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "R=%02lx: %ld input items, %ld output items, %ld feature items, %ld items total!",
                           nhr->nhr_ReportID, nhr->nhr_InItemCount, nhr->nhr_OutItemCount, nhr->nhr_FeatItemCount,
                           idnum-1);*/
            nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
        }

        /* Create quick item lookup table */
        nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            KPRINTF(1, ("Report %ld, allocating %ld/%ld/%ld items\n",
                    nhr->nhr_ReportID,
                    nhr->nhr_InItemCount, nhr->nhr_OutItemCount, nhr->nhr_FeatItemCount));
            if((nhiptr = btAllocVec(sizeof(struct BtHidItem *) * (nhr->nhr_InItemCount +
                                     nhr->nhr_OutItemCount + nhr->nhr_FeatItemCount))))
            {
                nhiinptr = nhr->nhr_InItemMap = nhiptr;
                nhioutptr = nhr->nhr_OutItemMap = &nhiptr[nhr->nhr_InItemCount];
                nhifeatptr = nhr->nhr_FeatItemMap = &nhioutptr[nhr->nhr_OutItemCount];
                nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
                while(nhc->nhc_Node.ln_Succ)
                {
                    nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;

                    while(nhi->nhi_Node.ln_Succ)
                    {
                        /* Add quick lookup entry */
                        switch(nhi->nhi_Type)
                        {
                            case REPORT_MAIN_INPUT:
                                *nhiinptr++ = nhi;
                                nhb->nhb_HasInItems = TRUE;
                                break;

                            case REPORT_MAIN_OUTPUT:
                                *nhioutptr++ = nhi;
                                break;

                            case REPORT_MAIN_FEATURE:
                                *nhifeatptr++ = nhi;
                                break;
                        }
                        nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
                    }
                    nhc = (struct BtHidCollection *) nhc->nhc_Node.ln_Succ;
                }
            } else {
                res = FALSE;
            }
            nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
        }

        bLoadActionConfig(nhb);
    }
    return(res);
}
/* \\\ */

/* /// "bLoadActionConfig()" */
void bLoadActionConfig(struct BTHidBinding *nhb)
{
    struct BtHidReport *nhr;
    struct BtHidCollection *nhc;
    struct BtHidItem *nhi;

    ULONG count;
    struct List *alistptr;
    ULONG *usageptr;
    struct BtIFFContext *pic;
    struct BtIFFContext *rppic;

    pic = btGetDevCfg(GM_UNIQUENAME(libname), nhb->nhb_DevIDString, nhb->nhb_SvcIDString);
    if(pic)
    {
        nhb->nhb_LastCfgCRC = bCalcConfigCRC(nhb, nhb->nhb_Base, pic);
    } else {
        nhb->nhb_LastCfgCRC = 0;
    }

    /* Create default action mapping */
    nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
    while(nhr->nhr_Node.ln_Succ)
    {
        KPRINTF(1, ("Report %ld:\n", nhr->nhr_ReportID));
        if(pic)
        {
            rppic = btFindCfgForm(pic, MAKE_ID('R','P','T','0')+nhr->nhr_ReportID);
        } else {
            rppic = NULL;
        }
        nhc = (struct BtHidCollection *) nhr->nhr_Collections.lh_Head;
        while(nhc->nhc_Node.ln_Succ)
        {
            KPRINTF(1, ("  Collection %s:\n", nhc->nhc_Name));
            nhi = (struct BtHidItem *) nhc->nhc_Items.lh_Head;
            while(nhi->nhi_Node.ln_Succ)
            {
                usageptr = &nhi->nhi_Usage;
                alistptr = &nhi->nhi_ActionList;
                if(rppic)
                {
                    if(!(bLoadItem(nhb, rppic, alistptr, nhr->nhr_ItemIDBase)))
                    {
                        bDetectDefaultAction(nhb, nhi, alistptr, nhc, *usageptr);
                    }
                } else {
                    bDetectDefaultAction(nhb, nhi, alistptr, nhc, *usageptr);
                }

                if(nhi->nhi_Flags & RPF_MAIN_VARIABLE)
                {
                    KPRINTF(1, ("    Variable %lx: %s\n", nhi->nhi_Usage, bNumToStr(nhb, NTS_USAGEID, nhi->nhi_Usage, "unknown")));
                } else {
                    usageptr = nhi->nhi_UsageMap;
                    alistptr = nhi->nhi_ActionMap;
                    count = nhi->nhi_MapSize;
                    KPRINTF(1, ("    Array %lx: %s\n", *usageptr, bNumToStr(nhb, NTS_USAGEID, *usageptr, "unknown")));
                    do
                    {
                        if(rppic)
                        {
                            if(!(bLoadItem(nhb, rppic, alistptr, nhr->nhr_ItemIDBase)))
                            {
                                bDetectDefaultAction(nhb, nhi, alistptr, nhc, *usageptr);
                            }
                        } else {
                            bDetectDefaultAction(nhb, nhi, alistptr, nhc, *usageptr);
                        }
                        alistptr++;
                        usageptr++;
                    } while(--count);
                }
                nhi = (struct BtHidItem *) nhi->nhi_Node.ln_Succ;
            }
            nhc = (struct BtHidCollection *) nhc->nhc_Node.ln_Succ;
        }
        nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "bDebugMem()" */
void bDebugMem(struct BTHidBinding *nhb, UBYTE *rptr, ULONG rptlen)
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
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), fmtstr, pos,
                       rptr[0], rptr[1], rptr[2], rptr[3], rptr[4], rptr[5], rptr[6], rptr[7],
                       rptr[8], rptr[9], rptr[10], rptr[11], rptr[12], rptr[13], rptr[14], rptr[15]);
        rptr += 16;
        pos += 16;
    }
}
/* \\\ */

/* /// "bDebugReport()" */
void bDebugReport(struct BTHidBinding *nhb, struct BtHidReport *nhr)
{
    UBYTE *rptr;
    ULONG rptlen;
    if(!nhr)
    {
        btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Can't debug HID Report: nhr=NULL!\n");
        return;
    }
    rptr = nhr->nhr_ReportBuf;
    rptlen = nhr->nhr_ReportLength;
    if(!rptr)
    {
        btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname), "Can't debug HID Report: ReportBuf=NULL!\n");
        return;
    }

    btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                   "HID Report %02lx Debug (%ld bytes):", nhr->nhr_ReportID, rptlen);
    bDebugMem(nhb, rptr, rptlen);
}
/* \\\ */

/* /// "bParseReport()" */
BOOL bParseReport(struct BTHidBinding *nhb, struct BtHidReport *nhr)
{
    UBYTE *rptr = nhr->nhr_ReportBuf;
    UBYTE *rptrend = &nhr->nhr_ReportBuf[nhr->nhr_ReportLength];
    struct BtHidCollection *nhc = NULL;
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

    NewList(&nhb->nhb_HidStack);
    NewList(&nhb->nhb_HidUsages);
    NewList(&nhb->nhb_HidDesigns);
    NewList(&nhb->nhb_HidStrings);

    nhb->nhb_HidGlobal.nhg_LogicalMin =
      nhb->nhb_HidGlobal.nhg_LogicalMax =
      nhb->nhb_HidGlobal.nhg_PhysicalMin =
      nhb->nhb_HidGlobal.nhg_PhysicalMax =
      nhb->nhb_HidGlobal.nhg_UnitExp = (LONG)HID_PARAM_UNDEF;

    nhb->nhb_HidGlobal.nhg_UsagePage =
      nhb->nhb_HidGlobal.nhg_Unit = HID_PARAM_UNDEF;

    nhb->nhb_HidGlobal.nhg_ReportID =
      nhb->nhb_HidGlobal.nhg_ReportSize =
      nhb->nhb_HidGlobal.nhg_ReportCount = 0;

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
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
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
                if(reportid != nhb->nhb_HidGlobal.nhg_ReportID)
                {
                    struct BtHidReport *oldnhr = nhr;
                    if(reportid)
                    {
                        nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
                        while(nhr->nhr_Node.ln_Succ)
                        {
                            if(nhr->nhr_ReportID == nhb->nhb_HidGlobal.nhg_ReportID)
                            {
                                break;
                            }
                            nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
                        }
                        if(!nhr->nhr_Node.ln_Succ)
                        {
                            if((nhr = btAllocVec(sizeof(struct BtHidReport))))
                            {
                                KPRINTF(1, ("Generating new report ID %ld\n", nhb->nhb_HidGlobal.nhg_ReportID));
                                NewList(&nhr->nhr_Collections);
                                nhr->nhr_ReportID = nhb->nhb_HidGlobal.nhg_ReportID;
                                AddTail(&nhb->nhb_HidReports, &nhr->nhr_Node);
                            } else {
                                KPRINTF(1, ("Out of memory creating new report.\n"));
                                rptr = NULL;
                                break;
                            }
                        } else {
                            KPRINTF(1, ("Switched back to report ID %ld\n", nhb->nhb_HidGlobal.nhg_ReportID));
                        }
                        if(nhc)
                        {
                            KPRINTF(1, ("Collection open, checking need for cloning.\n"));
                            if(nhc->nhc_Items.lh_Head->ln_Succ)
                            {
                                struct BtHidCollection *tmpnhc;
                                struct BtHidCollection *newnhc;
                                struct BtHidCollection *childnhc = NULL;

                                KPRINTF(1, ("Not empty, assigning collection to current report and cloning!\n"));
                                tmpnhc = nhc;
                                nhc = NULL;
                                do
                                {
                                    KPRINTF(1, ("Cloning %08lx (%s)\n", tmpnhc, tmpnhc->nhc_Name));
                                    if((newnhc = btAllocVec(sizeof(struct BtHidCollection))))
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
                                        newnhc->nhc_Name = btCopyStr(tmpnhc->nhc_Name);
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
                        KPRINTF(1, ("Setting first report ID %ld\n", nhb->nhb_HidGlobal.nhg_ReportID));
                        nhr->nhr_ReportID = nhb->nhb_HidGlobal.nhg_ReportID;
                    }
                    reportid = nhb->nhb_HidGlobal.nhg_ReportID;
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
                            struct BtHidItem *nhi;
                            struct BtHidUsage *nhu;
                            BOOL fakelogmin = FALSE;
                            BOOL fakelogmax = FALSE;
                            if(!delim)
                            {
                                KPRINTF(1, ("%s (%lx)\n", (itag == REPORT_MAIN_INPUT) ? "Input" : ((itag == REPORT_MAIN_OUTPUT) ? "Output" : "Feature"), udata));
                                if(!nhc)
                                {
                                    KPRINTF(10, ("No collection open!\n"));
                                    btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                                   "HID Error at %04lx: No collection open!", reportpos);
                                    bDebugReport(nhb, nhr);
                                    rptr = NULL;
                                    break;
                                }
                                if(nhb->nhb_HidGlobal.nhg_LogicalMin == HID_PARAM_UNDEF)
                                {
                                    nhb->nhb_HidGlobal.nhg_LogicalMin = 0;
                                    fakelogmin = TRUE;
                                    if(nhb->nhb_HidGlobal.nhg_LogicalMax == HID_PARAM_UNDEF)
                                    {
                                        fakelogmax = TRUE;
                                        if(nhb->nhb_HidGlobal.nhg_ReportSize < 32)
                                        {
                                            nhb->nhb_HidGlobal.nhg_LogicalMax = (1UL<<nhb->nhb_HidGlobal.nhg_ReportSize)-1;
                                        } else {
                                            nhb->nhb_HidGlobal.nhg_LogicalMin = 0x80000000;
                                            nhb->nhb_HidGlobal.nhg_LogicalMax = 0x7fffffff;
                                        }
                                        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "HID Error at %04lx: LogicalMin and Max for item missing, assuming %ld-%ld (%ld)!",
                                                       reportpos,
                                                       nhb->nhb_HidGlobal.nhg_LogicalMin,
                                                       nhb->nhb_HidGlobal.nhg_LogicalMax);
                                    } else {
                                        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "HID Error at %04lx: LogicalMin for item missing, assuming 0!", reportpos);
                                    }
                                }
                                else if(nhb->nhb_HidGlobal.nhg_LogicalMax == HID_PARAM_UNDEF)
                                {
                                    fakelogmax = TRUE;
                                    nhb->nhb_HidGlobal.nhg_LogicalMax = (1UL<<nhb->nhb_HidGlobal.nhg_ReportSize)-1;
                                    btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "HID Error at %04lx: LogicalMax for item missing, assuming %ld!", reportpos, nhb->nhb_HidGlobal.nhg_LogicalMax);

                                }
                                if((nhb->nhb_HidGlobal.nhg_UsagePage == HID_PARAM_UNDEF) ||
                                   //(nhb->nhb_HidGlobal.nhg_LogicalMin == HID_PARAM_UNDEF) ||
                                   //(nhb->nhb_HidGlobal.nhg_LogicalMax == HID_PARAM_UNDEF) ||
                                   (!nhb->nhb_HidGlobal.nhg_ReportSize)// ||
                                   //(!nhb->nhb_HidGlobal.nhg_ReportCount) // ReportCount == 0 seems to be a valid input (sigh), just avoid error message
                                   )
                                {
                                    btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "HID Error at %04lx: Mandatory item missing!", reportpos);
                                    bDebugReport(nhb, nhr);
                                    KPRINTF(10, ("Mandatory item missing!\n"));
                                    rptr = NULL;
                                } else {
                                    ULONG *usageptr;
                                    struct List *alistptr;
                                    usageid = designid = stringid = HID_PARAM_UNDEF;
                                    if(udata & RPF_MAIN_VARIABLE)
                                    {
                                        for(count = 0; count < nhb->nhb_HidGlobal.nhg_ReportCount; count++)
                                        {
                                            nhu = (struct BtHidUsage *) nhb->nhb_HidDesigns.lh_Head;
                                            if(nhu->nhu_Node.ln_Succ)
                                            {
                                                designid = nhu->nhu_Usage++;
                                                if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                {
                                                    Remove(&nhu->nhu_Node);
                                                    btFreeVec(nhu);
                                                }
                                            }
                                            nhu = (struct BtHidUsage *) nhb->nhb_HidStrings.lh_Head;
                                            if(nhu->nhu_Node.ln_Succ)
                                            {
                                                stringid = nhu->nhu_Usage++;
                                                if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                {
                                                    Remove(&nhu->nhu_Node);
                                                    btFreeVec(nhu);
                                                }
                                            }
                                            nhu = (struct BtHidUsage *) nhb->nhb_HidUsages.lh_Head;
                                            if(nhu->nhu_Node.ln_Succ)
                                            {
                                                usageid = nhu->nhu_Usage++;
                                                if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                {
                                                    KPRINTF(1, ("Removing usage %08lx\n", nhu));
                                                    Remove(&nhu->nhu_Node);
                                                    btFreeVec(nhu);
                                                }
                                            }
                                            if(usageid != HID_PARAM_UNDEF)
                                            {
                                                if((nhi = btAllocVec(sizeof(struct BtHidItem))))
                                                {
                                                    nhi->nhi_Collection = nhc;
                                                    nhi->nhi_Type = itag;
                                                    nhi->nhi_Flags = udata;
                                                    nhi->nhi_Offset = bitpos;
                                                    nhi->nhi_Size = nhb->nhb_HidGlobal.nhg_ReportSize;

                                                    nhi->nhi_LogicalMin = nhb->nhb_HidGlobal.nhg_LogicalMin;
                                                    nhi->nhi_LogicalMax = nhb->nhb_HidGlobal.nhg_LogicalMax;
                                                    nhi->nhi_IsSigned = (nhi->nhi_LogicalMin < 0) || (nhi->nhi_LogicalMax < 0);
                                                    nhi->nhi_PhysicalMin = nhb->nhb_HidGlobal.nhg_PhysicalMin;
                                                    nhi->nhi_PhysicalMax = nhb->nhb_HidGlobal.nhg_PhysicalMax;
                                                    nhi->nhi_UnitExp = nhb->nhb_HidGlobal.nhg_UnitExp;
                                                    nhi->nhi_Unit = nhb->nhb_HidGlobal.nhg_Unit;
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
                                            bitpos += nhb->nhb_HidGlobal.nhg_ReportSize;
                                        }
                                    } else {
                                        nhu = (struct BtHidUsage *) nhb->nhb_HidUsages.lh_Head;
                                        if(nhu->nhu_Node.ln_Succ)
                                        {
                                            if((nhi = btAllocVec(sizeof(struct BtHidItem))))
                                            {
                                                nhi->nhi_Collection = nhc;
                                                nhi->nhi_Type = itag;
                                                nhi->nhi_Flags = udata;
                                                nhi->nhi_Offset = bitpos;
                                                nhi->nhi_Size = nhb->nhb_HidGlobal.nhg_ReportSize;
                                                nhi->nhi_Count = nhb->nhb_HidGlobal.nhg_ReportCount;

                                                nhi->nhi_LogicalMin = nhb->nhb_HidGlobal.nhg_LogicalMin;
                                                nhi->nhi_LogicalMax = nhb->nhb_HidGlobal.nhg_LogicalMax;

                                                if((nhi->nhi_LogicalMin > nhi->nhi_LogicalMax) || ((nhi->nhi_LogicalMax - nhi->nhi_LogicalMin) > 65535))
                                                {
                                                    KPRINTF(10, ("Invalid HID logical range %ld..%ld, capping\n", nhi->nhi_LogicalMin, nhi->nhi_LogicalMax));
                                                    nhi->nhi_LogicalMax = nhi->nhi_LogicalMin;
                                                }
                                                nhi->nhi_MapSize = (nhi->nhi_LogicalMax - nhi->nhi_LogicalMin)+1;
                                                nhi->nhi_UsageMap = btAllocVec(sizeof(ULONG) * nhi->nhi_MapSize);
                                                nhi->nhi_ActionMap = btAllocVec(sizeof(struct List) * nhi->nhi_MapSize);
                                                if(nhi->nhi_Count > 65535) nhi->nhi_Count = 1;
                                                nhi->nhi_Buffer = btAllocVec(2 * sizeof(LONG) * nhi->nhi_Count);
                                                nhi->nhi_PhysicalMin = nhb->nhb_HidGlobal.nhg_PhysicalMin;
                                                nhi->nhi_PhysicalMax = nhb->nhb_HidGlobal.nhg_PhysicalMax;
                                                nhi->nhi_UnitExp = nhb->nhb_HidGlobal.nhg_UnitExp;
                                                nhi->nhi_Unit = nhb->nhb_HidGlobal.nhg_Unit;

                                                usageptr = nhi->nhi_UsageMap;
                                                alistptr = nhi->nhi_ActionMap;
                                                if(!(usageptr && alistptr && nhi->nhi_Buffer))
                                                {
                                                    btFreeVec(usageptr);
                                                    btFreeVec(alistptr);
                                                    btFreeVec(nhi->nhi_Buffer);
                                                    btFreeVec(nhi);
                                                    KPRINTF(10, ("Out of memory\n"));
                                                    rptr = NULL;
                                                    break;
                                                }

                                                nhi->nhi_OldBuffer = &nhi->nhi_Buffer[nhi->nhi_Count];
                                                nhi->nhi_Usage = nhi->nhi_DesignIndex = nhi->nhi_StringIndex = HID_PARAM_UNDEF;
                                                NewList(&nhi->nhi_ActionList);

                                                KPRINTF(1, ("New Array(%ld*%ld) Item: %ld.%ld (%ld-%ld)\n",
                                                            nhi->nhi_Count, nhi->nhi_MapSize,
                                                            nhi->nhi_Offset>>3, nhi->nhi_Offset & 7,
                                                            nhi->nhi_LogicalMin, nhi->nhi_LogicalMax));

                                                nhi->nhi_SameUsages = TRUE;
                                                for (count = nhi->nhi_MapSize; count; count--)
                                                {
                                                    nhu = (struct BtHidUsage *) nhb->nhb_HidDesigns.lh_Head;
                                                    if(nhu->nhu_Node.ln_Succ)
                                                    {
                                                        designid = nhu->nhu_Usage++;
                                                        if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                        {
                                                            Remove(&nhu->nhu_Node);
                                                            btFreeVec(nhu);
                                                        }
                                                    }
                                                    nhu = (struct BtHidUsage *) nhb->nhb_HidStrings.lh_Head;
                                                    if(nhu->nhu_Node.ln_Succ)
                                                    {
                                                        stringid = nhu->nhu_Usage++;
                                                        if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                        {
                                                            Remove(&nhu->nhu_Node);
                                                            btFreeVec(nhu);
                                                        }
                                                    }
                                                    nhu = (struct BtHidUsage *) nhb->nhb_HidUsages.lh_Head;
                                                    if(nhu->nhu_Node.ln_Succ)
                                                    {
                                                        usageid = nhu->nhu_Usage++;
                                                        if(nhu->nhu_Usage > nhu->nhu_UsageMax)
                                                        {
                                                            KPRINTF(1, ("Removing usage %08lx\n", nhu));
                                                            Remove(&nhu->nhu_Node);
                                                            btFreeVec(nhu);
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
                                        bitpos += nhb->nhb_HidGlobal.nhg_ReportSize * nhb->nhb_HidGlobal.nhg_ReportCount;
                                    }
                                }
                                if(fakelogmin)
                                {
                                    nhb->nhb_HidGlobal.nhg_LogicalMin = HID_PARAM_UNDEF;
                                }
                                if(fakelogmax)
                                {
                                    nhb->nhb_HidGlobal.nhg_LogicalMax = HID_PARAM_UNDEF;
                                }
                            }
                            break;
                        }

                    case REPORT_MAIN_COLLECT:
                        {
                            struct BtHidUsage *nhu;
                            struct BtHidCollection *oldnhc = nhc;
                            KPRINTF(1, ("Collection(%lx)\n", udata));
                            if((nhc = btAllocVec(sizeof(struct BtHidCollection))))
                            {
                                NewList(&nhc->nhc_Items);
                                nhc->nhc_Parent = oldnhc;
                                nhu = (struct BtHidUsage *) nhb->nhb_HidUsages.lh_Head;
                                if(nhu->nhu_Node.ln_Succ)
                                {
                                    nhc->nhc_Usage = nhu->nhu_Usage;
                                    nhc->nhc_Name = bGetUsageName(nhb, nhu->nhu_Usage);
                                } else {
                                    if(nhc->nhc_Parent)
                                    {
                                        nhc->nhc_Usage = nhc->nhc_Parent->nhc_Usage;
                                        nhc->nhc_Name = btCopyStr(nhc->nhc_Parent->nhc_Name);
                                    } else {
                                        nhc->nhc_Name = btCopyStr("Argl!");
                                    }
                                }
                            }
                            node = nhb->nhb_HidUsages.lh_Head;
                            while(node->ln_Succ)
                            {
                                KPRINTF(1, ("Removing usage %08lx\n", node));
                                Remove(node);
                                btFreeVec(node);
                                node = nhb->nhb_HidUsages.lh_Head;
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
                                struct BtHidCollection *oldnhc = nhc;
                                KPRINTF(1, ("Deleting empty collection %lx (%s)\n", nhc, nhc->nhc_Name));
                                nhc = nhc->nhc_Parent;
                                btFreeVec(oldnhc->nhc_Name);
                                btFreeVec(oldnhc);
                            } else {
                                AddTail(&nhr->nhr_Collections, &nhc->nhc_Node);
                                nhc->nhc_Report = nhr;
                                nhc = nhc->nhc_Parent;
                            }
                        }
                        break;

                    default:
                        KPRINTF(1, ("Reserved_%02lx(%lx)\n", itag, data));
                        btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID Error at %04lx: Reserved_MAIN_%02lx(%lx)", reportpos, itag, data);
                        bDebugReport(nhb, nhr);
                        rptr = NULL;
                        break;
                }
                /* Remove remaining usages */
                node = nhb->nhb_HidUsages.lh_Head;
                while(node->ln_Succ)
                {
                    KPRINTF(1, ("MainEnd: Removing usage %08lx\n", node));
                    Remove(node);
                    btFreeVec(node);
                    node = nhb->nhb_HidUsages.lh_Head;
                }
                /* Remove remaining designators */
                node = nhb->nhb_HidDesigns.lh_Head;
                while(node->ln_Succ)
                {
                    KPRINTF(1, ("MainEnd: Removing designator %08lx\n", node));
                    Remove(node);
                    btFreeVec(node);
                    node = nhb->nhb_HidDesigns.lh_Head;
                }
                /* Remove remaining strings */
                node = nhb->nhb_HidStrings.lh_Head;
                while(node->ln_Succ)
                {
                    KPRINTF(1, ("MainEnd: Removing strings %08lx\n", node));
                    Remove(node);
                    btFreeVec(node);
                    node = nhb->nhb_HidStrings.lh_Head;
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
                        KPRINTF(1, ("Usage Page(%ld) %s\n", udata, bNumToStr(nhb, NTS_USAGEPAGE, udata, "unknown")));
                        nhb->nhb_HidGlobal.nhg_UsagePage = udata;
                        break;

                    case REPORT_GLOB_LOGMIN:
                        KPRINTF(1, ("LogMin(%ld)\n", data));
                        nhb->nhb_HidGlobal.nhg_LogicalMin = data;
                        break;

                    case REPORT_GLOB_LOGMAX:
                        /* Some devices (like usb-kbd in QEMU) have wrong descriptors
                         * Try to detect and correct this here which seems to be what other OSes do
                         * This only works if LogMin is defined before LogMax but that's likely common
			 */
                        nhb->nhb_HidGlobal.nhg_LogicalMax = (data < nhb->nhb_HidGlobal.nhg_LogicalMin ? udata : data);
                        KPRINTF(1, ("LogMax(%ld)\n", nhb->nhb_HidGlobal.nhg_LogicalMax));
                        break;

                    case REPORT_GLOB_PHYMIN:
                        KPRINTF(1, ("PhyMin(%ld)\n", data));
                        nhb->nhb_HidGlobal.nhg_PhysicalMin = data;
                        break;

                    case REPORT_GLOB_PHYMAX:
                        nhb->nhb_HidGlobal.nhg_PhysicalMax = (data < nhb->nhb_HidGlobal.nhg_PhysicalMin ? udata : data);
                        KPRINTF(1, ("PhyMax(%ld)\n", nhb->nhb_HidGlobal.nhg_PhysicalMax));
                        break;

                    case REPORT_GLOB_UNITEXP:
                        KPRINTF(1, ("UnitExp(%ld)\n", data));
                        nhb->nhb_HidGlobal.nhg_UnitExp = data;
                        break;

                    case REPORT_GLOB_UNIT:
                        KPRINTF(1, ("Unit(%ld)\n", data));
                        nhb->nhb_HidGlobal.nhg_Unit = data;
                        break;

                    case REPORT_GLOB_RPSIZE:
                        KPRINTF(1, ("ReportSize(%ld)\n", udata));
                        nhb->nhb_HidGlobal.nhg_ReportSize = udata;
                        break;

                    case REPORT_GLOB_RPID:
                        KPRINTF(1, ("ReportID(%ld)\n", udata));
                        nhb->nhb_HidGlobal.nhg_ReportID = udata;
                        break;

                    case REPORT_GLOB_RPCOUNT:
                        KPRINTF(1, ("ReportCount(%ld)\n", udata));
                        nhb->nhb_HidGlobal.nhg_ReportCount = udata;
                        break;

                    case REPORT_GLOB_PUSH:
                    {
                        struct BtHidGlobal *nhg = btAllocVec(sizeof(struct BtHidGlobal));
                        KPRINTF(1, ("Push(%ld)\n", data));
                        if(nhg)
                        {
                            *nhg = nhb->nhb_HidGlobal;
                            AddTail(&nhb->nhb_HidStack, &nhg->nhg_Node);
                        } else {
                            KPRINTF(10, ("Out of HID stack memory!\n"));
                            rptr = NULL;
                        }
                        break;
                    }

                    case REPORT_GLOB_POP:
                    {
                        struct BtHidGlobal *nhg = (struct BtHidGlobal *) nhb->nhb_HidStack.lh_Tail;
                        KPRINTF(1, ("Pop(%ld)\n", data));
                        if(nhg->nhg_Node.ln_Pred)
                        {
                            nhb->nhb_HidGlobal = *nhg;
                            Remove(&nhg->nhg_Node);
                            btFreeVec(nhg);
                        } else {
                            KPRINTF(10, ("Pop from HID stack without push!\n"));
                            btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                           "HID Error at %04lx: Pop from HID stack without push!", reportpos);
                            bDebugReport(nhb, nhr);
                            rptr = NULL;
                        }
                        break;
                    }

                    default:
                        KPRINTF(1, ("Reserved_%02lx(%lx)\n", itag, data));
                        btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID Error at %04lx: Reserved_GLOBAL_%02lx(%lx)", reportpos, itag, data);
                        bDebugReport(nhb, nhr);
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
                            udata |= nhb->nhb_HidGlobal.nhg_UsagePage<<16;
                        }
                        KPRINTF(1, ("Usage (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        if(!bAddUsage(nhb, &nhb->nhb_HidUsages, udata, udata))
                        {
                            rptr = NULL;
                        }
                        break;

                    case REPORT_LOCL_USEMIN:
                        if(len <= 2)
                        {
                            udata |= nhb->nhb_HidGlobal.nhg_UsagePage<<16;
                        }
                        KPRINTF(1, ("UsageMin (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        usagemin = udata;
                        if(usagemax != HID_PARAM_UNDEF)
                        {
                            if(!bAddUsage(nhb, &nhb->nhb_HidUsages, usagemin, usagemax))
                            {
                                rptr = NULL;
                            }
                            usagemin = usagemax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_USEMAX:
                        if(len <= 2)
                        {
                            udata |= nhb->nhb_HidGlobal.nhg_UsagePage<<16;
                        }
                        KPRINTF(1, ("UsageMax (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        usagemax = udata;
                        if(usagemin != HID_PARAM_UNDEF)
                        {
                            if(!bAddUsage(nhb, &nhb->nhb_HidUsages, usagemin, usagemax))
                            {
                                rptr = NULL;
                            }
                            usagemin = usagemax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_DESIDX:
                        KPRINTF(1, ("Designator index (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        if(!bAddUsage(nhb, &nhb->nhb_HidDesigns, udata, udata))
                        {
                            rptr = NULL;
                        }
                        break;

                    case REPORT_LOCL_DESMIN:
                        KPRINTF(1, ("DesignMin (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        designmin = udata;
                        if(designmax != HID_PARAM_UNDEF)
                        {
                            if(!bAddUsage(nhb, &nhb->nhb_HidDesigns, designmin, designmax))
                            {
                                rptr = NULL;
                            }
                            designmin = designmax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_DESMAX:
                        KPRINTF(1, ("DesignMax (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        designmin = udata;
                        if(designmax != HID_PARAM_UNDEF)
                        {
                            if(!bAddUsage(nhb, &nhb->nhb_HidDesigns, designmin, designmax))
                            {
                                rptr = NULL;
                            }
                            designmin = designmax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_STRIDX:
                        KPRINTF(1, ("String index (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        if(!bAddUsage(nhb, &nhb->nhb_HidStrings, udata, udata))
                        {
                            rptr = NULL;
                        }
                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID String Index %ld", udata);
                        break;

                    case REPORT_LOCL_STRMIN:
                        KPRINTF(1, ("StringMin (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        stringmin = udata;
                        if(stringmax != HID_PARAM_UNDEF)
                        {
                            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                           "HID String Index Range %ld - %ld", stringmin, stringmax);
                            if(!bAddUsage(nhb, &nhb->nhb_HidStrings, stringmin, stringmax))
                            {
                                rptr = NULL;
                            }
                            stringmin = stringmax = HID_PARAM_UNDEF;
                        }
                        break;

                    case REPORT_LOCL_STRMAX:
                        KPRINTF(1, ("StringMax (%lx) %s\n", udata, bNumToStr(nhb, NTS_USAGEID, udata, "unknown")));
                        stringmax = udata;
                        if(stringmin != HID_PARAM_UNDEF)
                        {
                            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                           "HID String Index Range %ld - %ld", stringmin, stringmax);
                            if(!bAddUsage(nhb, &nhb->nhb_HidStrings, stringmin, stringmax))
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
                        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "HID Report Delimiters not really supported!");
                        break;

                    default:
                        KPRINTF(1, ("Reserved_%02lx(%lx)\n", itag, udata));
                        btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                       "HID Error at %04lx: Reserved_LOCAL_%02lx(%lx)", reportpos, itag, udata);
                        bDebugReport(nhb, nhr);
                        rptr = NULL;
                        break;
                }
                break;

            default:
                KPRINTF(1, ("Reserved!\n"));
                btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                               "HID Error at %04lx: Reserved_%02lx", reportpos, itag);
                bDebugReport(nhb, nhr);
                rptr = NULL;
                break;
        }
    }
    KPRINTF(1, ("Parsing done, status %lx.\n", rptr));

    /* Remove remaining stack frame */
    node = nhb->nhb_HidStack.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing stack frame %08lx\n", node));
        Remove(node);
        btFreeVec(node);
        node = nhb->nhb_HidStack.lh_Head;
    }
    /* Remove remaining usages */
    node = nhb->nhb_HidUsages.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing usage %08lx\n", node));
        Remove(node);
        btFreeVec(node);
        node = nhb->nhb_HidUsages.lh_Head;
    }
    /* Remove remaining designators */
    node = nhb->nhb_HidDesigns.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing designator %08lx\n", node));
        Remove(node);
        btFreeVec(node);
        node = nhb->nhb_HidDesigns.lh_Head;
    }
    /* Remove remaining strings */
    node = nhb->nhb_HidStrings.lh_Head;
    while(node->ln_Succ)
    {
        KPRINTF(1, ("Removing strings %08lx\n", node));
        Remove(node);
        btFreeVec(node);
        node = nhb->nhb_HidStrings.lh_Head;
    }
    //bDebugReport(nhb, nhr);
    return((BOOL) (rptr ? TRUE : FALSE));
}
/* \\\ */

/* /// "bFindCollID()" */
BOOL bFindCollID(struct BTHidBinding *nhb, struct BtHidCollection *nhc, ULONG collidmin, ULONG collidmax)
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

/* /// "bFindItemID()" */
struct BtHidItem * bFindItemID(struct BTHidBinding *nhb, UWORD id, UWORD itype, ULONG *pos)
{
    struct BtHidReport *nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
    struct BtHidItem *nhi;
    struct BtHidItem **nhiptr;
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
        nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
    }
    return(NULL);
}
/* \\\ */

/* /// "bFindItemUsage()" */
UWORD bFindItemUsage(struct BTHidBinding *nhb, ULONG usage, UWORD itype)
{
    struct BtHidReport *nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
    struct BtHidItem *nhi;
    struct BtHidItem **nhiptr;
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
        nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
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

/* /// "bDetectDefaultAction()" */
BOOL bDetectDefaultAction(struct BTHidBinding *nhb,  struct BtHidItem *nhi, struct List *lst, struct BtHidCollection *nhc, ULONG uid)
{
    BOOL res = FALSE;
    UWORD usageid = uid;
    struct BtHidAction *nha;
    APTR wc; /* USB tablet init sequences are not generated here */
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

                btGetAttrs(BGA_DEVICE, nhb->nhb_Device,
                           BDA_VendorID, &vendid,
                           BDA_ProductID, &prodid,
                           TAG_END);
                if((vendid == 0x08ca) &&
                   ((prodid == 0x0001) || (prodid == 0x0010) ||
                    ((prodid >= 0x0020) && (prodid <= 0x0024))) &&
                   (!nhi->nhi_ActionList.lh_Head->ln_Succ))
                {
                    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Generating special AipTek USB Tablet init sequence.");
                    command = bFindItemUsage(nhb, 0x0D003F, REPORT_MAIN_FEATURE);
                    data = bFindItemUsage(nhb, 0x0D0040, REPORT_MAIN_FEATURE);
                    if(command && data)
                    {
                        /* set resolution to 500DPI */
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x18;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x04;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }

                        /* set absolute tablet mode */
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x10;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x01;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }

                        /* enable macro keys */
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x11;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x02;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }

                        /* enable auto gain */
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x12;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = data;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0xff;
                        }
                        /*if((nha = bAllocAction(nhb, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }*/
                    } else {
                        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "AipTek: No command (%ld) or data (%ld) feature!", command, data);
                    }
                }
                wc = NULL; /* USB tablets only */
                if(wc)
                {
                    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Generating special Wacom USB Tablet init sequence.");
                    command = bFindItemUsage(nhb, 0xff000001, REPORT_MAIN_FEATURE);
                    if(command)
                    {
                        /* activate returning of absolute tablet data */
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
                        {
                            nha->nha_FeatOp = HUAT_ASSIGN;
                            nha->nha_FeatItem = command;
                            nha->nha_ValEnable = TRUE;
                            nha->nha_ValVar = HUAT_CONST;
                            nha->nha_ValConst = 0x02;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_FEATURE|HUA_ANY)))
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
                    command = bFindItemUsage(nhb, 0xff000001, REPORT_MAIN_OUTPUT);
                    if(command)
                    {
                        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                       "Adding special HID2HCI Logitech Bluetooth init sequence, but deactivated!");
                        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "This init sequence would disable HID usage and enable pure Bluetooth use!");
                        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "If you want to use this dongle for Bluetooth, switch the NOP-Startup-Actions to type Output!");
                        // send three commands to output report 0x10
                        if((nha = bAllocAction(nhb, lst, HUA_NOP|HUA_ANY)))
                        {
                            nha->nha_OutItem = command;
                            strcpy(nha->nha_OutArray, "0xff, 0x80, 0x80, 0x01, 0x00, 0x00");
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_NOP|HUA_ANY)))
                        {
                            nha->nha_OutItem = command;
                            strcpy(nha->nha_OutArray, "0xff, 0x80, 0x00, 0x00, 0x30, 0x00");
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_MISC|HUA_ANY)))
                        {
                            nha->nha_MiscMode = HUAT_FLUSHEVENTS;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_NOP|HUA_ANY)))
                        {
                            nha->nha_OutItem = command;
                            strcpy(nha->nha_OutArray, "0xff, 0x81, 0x80, 0x00, 0x00, 0x00");
                        }
                    }
                }
            }

            if(bFindCollID(nhb, nhc, 0x010002, 0x010002) || /* Mice */
               bFindCollID(nhb, nhc, 0x0d0001, 0x0d000d)) /* Tablets */
            {
                switch(usageid)
                {
                    case 0x30: /* Mouse X */
                        if((nha = bAllocAction(nhb, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
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
                        if((nha = bAllocAction(nhb, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
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
                        if((nha = bAllocAction(nhb, lst, HUA_WHEEL|HUA_ALWAYS)))
                        {
                            nha->nha_WheelMode = HUAT_DELTAY;
                            nha->nha_WheelDist = 1;
                        }
                        break;
                }
            }

            if(bFindCollID(nhb, nhc, 0x010004, 0x010005)) /* Joystick, Game Pad */
            {
                switch(usageid)
                {
                    case 0x39: /* Hatswitch */
                        if((nha = bAllocAction(nhb, lst, HUA_DIGJOY|HUA_NAN)))
                        {
                            nha->nha_JoypadOp = HUAT_CLEAR;
                            nha->nha_JoypadFeat = HUAT_HATSWITCH;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_DIGJOY|HUA_ALWAYS)))
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
                        if((nha = bAllocAction(nhb, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_GT;
                            nha->nha_CCConst2 = minpos;
                            nha->nha_JoypadOp = HUAT_CLEAR;
                            nha->nha_JoypadFeat = negaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_LE;
                            nha->nha_CCConst2 = minpos;
                            nha->nha_JoypadOp = HUAT_SET;
                            nha->nha_JoypadFeat = negaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_LT;
                            nha->nha_CCConst2 = maxpos;
                            nha->nha_JoypadOp = HUAT_CLEAR;
                            nha->nha_JoypadFeat = posaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_DIGJOY|HUA_ANY)))
                        {
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCCond = HUAT_GE;
                            nha->nha_CCConst2 = maxpos;
                            nha->nha_JoypadOp = HUAT_SET;
                            nha->nha_JoypadFeat = posaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_ANALOGJOY|HUA_ANY)))
                        {
                            nha->nha_APadFeat = alogaxis;
                            nha->nha_JoypadPort = 1;
                        }
                        break;
                    }
                }
            }

            if(bFindCollID(nhb, nhc, 0x010008, 0x010008) && (nhi->nhi_Flags & RPF_MAIN_RELATIVE)) /* Multi-axis controller */
            {
                switch(usageid)
                {
                    case 0x30: /* Mouse X */
                        if((nha = bAllocAction(nhb, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
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
                        if((nha = bAllocAction(nhb, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
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
                        if((nha = bAllocAction(nhb, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
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
                        if((nha = bAllocAction(nhb, lst, HUA_MOUSEPOS|HUA_ALWAYS)))
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
                        UWORD led = bFindItemUsage(nhb, 0x08004B, REPORT_MAIN_OUTPUT); // blue LED
                        if(maxpos > 0)
                        {
                            if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
                            {
                                nha->nha_ButtonMode = HUAT_SET;
                                nha->nha_ButtonNo = 1;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_GT;
                                nha->nha_CCConst2 = maxpos;
                            }
                            if(led && ((nha = bAllocAction(nhb, lst, HUA_OUTPUT|HUA_ANY))))
                            {
                                nha->nha_OutOp = HUAT_SET;
                                nha->nha_OutItem = led;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_GT;
                                nha->nha_CCConst2 = maxpos;
                            }
                            if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
                            {
                                nha->nha_ButtonMode = HUAT_CLEAR;
                                nha->nha_ButtonNo = 1;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_LT;
                                nha->nha_CCConst2 = maxpos;
                            }
                            if(led && ((nha = bAllocAction(nhb, lst, HUA_OUTPUT|HUA_ANY))))
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
                            if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
                            {
                                nha->nha_ButtonMode = HUAT_SET;
                                nha->nha_ButtonNo = 2;
                                nha->nha_CCEnable = TRUE;
                                nha->nha_CCCond = HUAT_LT;
                                nha->nha_CCConst2 = minpos;
                            }
                            if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
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
                            if((nha = bAllocAction(nhb, lst, HUA_WHEEL|HUA_ANY)))
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
                            if((nha = bAllocAction(nhb, lst, HUA_WHEEL|HUA_ANY)))
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
            if(bFindCollID(nhb, nhc, 0x010002, 0x010002) || /* Mice */
               bFindCollID(nhb, nhc, 0x0d0001, 0x0d000d) || /* Tablets */
               bFindCollID(nhb, nhc, 0x010008, 0x010008)) /* Multi-Axis controller */
            {
                if(usageid && (usageid < 6))
                {
                    if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
                    {
                        nha->nha_ButtonMode = HUAT_ASSIGN;
                        nha->nha_ButtonNo = usageid;
                    }
                }
            }
            if(bFindCollID(nhb, nhc, 0x010004, 0x010005)) /* Joystick, Game Pad */
            {
                if(usageid && (usageid < 8))
                {
                    if((nha = bAllocAction(nhb, lst, HUA_DIGJOY|HUA_ANY)))
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
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_DOWNEVENT)))
                    {
                        nha->nha_QualMode = HUAT_TOGGLE;
                        nha->nha_Qualifier = IEQUALIFIERB_CAPSLOCK;
                    }
                    if((led = bFindItemUsage(nhb, 0x080002, REPORT_MAIN_OUTPUT)))
                    {
                        if((nha = bAllocAction(nhb, lst, HUA_OUTPUT|HUA_DOWNEVENT)))
                        {
                            nha->nha_OutOp = HUAT_SET;
                            nha->nha_OutItem = led;
                            nha->nha_CCEnable = TRUE;
                            nha->nha_CCVar1 = HUAT_QUALIFIERS;
                            nha->nha_CCCond = HUAT_BWAND;
                            nha->nha_CCVar2 = HUAT_CONST;
                            nha->nha_CCConst2 = IEQUALIFIER_CAPSLOCK;
                        }
                        if((nha = bAllocAction(nhb, lst, HUA_OUTPUT|HUA_DOWNEVENT)))
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
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_CONTROL;
                    }
                    break;

                case 0xe1: /* Left shift */
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_LSHIFT;
                    }
                    break;

                case 0xe2: /* Left alt */
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_LALT;
                    }
                    break;

                case 0xe3: /* Left GUI */
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_LCOMMAND;
                    }
                    break;

                case 0xe4: /* Right control */
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_CONTROL;
                    }
                    break;

                case 0xe5: /* Right shift */
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_RSHIFT;
                    }
                    break;

                case 0xe6: /* Right alt */
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_RALT;
                    }
                    break;

                case 0x45: /* F12 */
                case 0x65: /* Application */
                case 0xe7: /* Right GUI */
                    if((nha = bAllocAction(nhb, lst, HUA_QUALIFIER|HUA_ANY)))
                    {
                        nha->nha_QualMode = HUAT_ASSIGN;
                        nha->nha_Qualifier = IEQUALIFIERB_RCOMMAND;
                    }
                    break;
            }
            nha = bAllocAction(nhb, lst, HUA_KEYMAP|HUA_ANY);
            break;

        case 0x0d: /* digitizer/tablet page */
            switch(usageid)
            {
                case 0x30: /* Tip pressure */
                case 0x31: /* Barrel pressure */
                    if((nha = bAllocAction(nhb, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_PRESSURE;
                    }
                    break;

                case 0x32: /* In Range */
                case 0x37: /* Data Valid */
                    if((nha = bAllocAction(nhb, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_PROX;
                    }
                    break;

                case 0x3D: /* X Tilt */
                    if((nha = bAllocAction(nhb, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_XROT;
                    }
                    break;

                case 0x3E: /* Y Tilt */
                    if((nha = bAllocAction(nhb, lst, HUA_TABLET|HUA_ALWAYS)))
                    {
                        nha->nha_TabletAxis = HUAT_YROT;
                    }
                    break;

                case 0x42: /* Tip switch */
                    if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
                    {
                        nha->nha_ButtonMode = HUAT_ASSIGN;
                        nha->nha_ButtonNo = 1;
                    }
                    break;

                case 0x44: /* Barrel switch */
                    if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
                    {
                        nha->nha_ButtonMode = HUAT_ASSIGN;
                        nha->nha_ButtonNo = 2;
                    }
                    break;

                case 0x46: /* Tablet pick */
                    if((nha = bAllocAction(nhb, lst, HUA_BUTTONS|HUA_ANY)))
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
            if((nha = bAllocAction(nhb, lst, HUA_EXTRAWKEY|HUA_DOWNEVENT)))
            {
                nha->nha_RawKey = utp->utp_ExtCode;
            }
            if((nha = bAllocAction(nhb, lst, HUA_EXTRAWKEY|HUA_UPEVENT)))
            {
                nha->nha_RawKey = utp->utp_ExtCode|IECODE_UP_PREFIX;
            }
        }
        utp++;
    }
    return(res);
}
/* \\\ */

/* /// "bCheckForDefaultAction()" */
BOOL bCheckForDefaultAction(struct BTHidBinding *nhb,  struct BtHidItem *nhi, struct List *lst, struct BtHidCollection *nhc, ULONG uid)
{
    BOOL res = TRUE;
    struct BtHidAction *nha;
    struct BtHidAction *nhadef;
    struct List tmplist;

    NewList(&tmplist);
    bDetectDefaultAction(nhb, nhi, &tmplist, nhc, nhi->nhi_Usage);
    nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
    nhadef = (struct BtHidAction *) tmplist.lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        if(nhadef->nha_Node.ln_Succ)
        {
            nhadef->nha_IsDefault = nha->nha_IsDefault;
            if(memcmp(&nha->nha_Type, &nhadef->nha_Type, sizeof(struct BtHidAction)-offsetof(struct BtHidAction, nha_Type)))
            {
                nha->nha_IsDefault = FALSE;
                res = FALSE;
            } else {
                nha->nha_IsDefault = TRUE;
            }
            nhadef = (struct BtHidAction *) nhadef->nha_Node.ln_Succ;
        } else {
            res = FALSE;
            nha->nha_IsDefault = FALSE;
        }
        nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
    }
    nhadef = (struct BtHidAction *) tmplist.lh_Head;
    while(nhadef->nha_Node.ln_Succ)
    {
        Remove(&nhadef->nha_Node);
        btFreeVec(nhadef);
        nhadef = (struct BtHidAction *) tmplist.lh_Head;
    }
    return(res);
}
/* \\\ */

/* /// "bAllocAction()" */
struct BtHidAction * bAllocAction(struct BTHidBinding *nhb, struct List *lst, UWORD utype)
{
    struct BtHidAction *nha = btAllocVec(sizeof(struct BtHidAction));
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

/* /// "bAddUsage()" */
BOOL bAddUsage(struct BTHidBinding *nhb, struct List *list, ULONG umin, ULONG umax)
{
    struct BtHidUsage *nhu;
    if(umin > umax)
    {
        KPRINTF(10, ("UsageMin %lx > UsageMax %lx\n", umin, umax));
        return(FALSE);
    }
    if((nhu = btAllocVec(sizeof(struct BtHidUsage))))
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

/* /// "bGetUsageName()" */
STRPTR bGetUsageName(struct BTHidBinding *nhb, ULONG uid)
{
    STRPTR uname;

    uname = bNumToStr(nhb, NTS_USAGEID, uid, NULL);
    if(uname)
    {
        return(btCopyStr(uname));
    }
    uname = bNumToStr(nhb, NTS_USAGEPAGE, uid>>16, "unknown");
    return(btCopyStrFmt("%s (0x%lx)", uname, uid));
}
/* \\\ */

/* /// "bGenerateOutReport()" */
void bGenerateOutReport(struct BTHidBinding *nhb, struct BtHidReport *nhr, UBYTE *buf)
{
    struct BtHidItem **nhiptr = nhr->nhr_OutItemMap;
    ULONG cnt;

    for(cnt = 0; cnt < nhr->nhr_OutItemCount; cnt++)
    {
        bEncodeItemBuffer(nhb, *nhiptr++, buf);
    }
}
/* \\\ */

/* /// "bGenerateFeatReport()" */
void bGenerateFeatReport(struct BTHidBinding *nhb, struct BtHidReport *nhr, UBYTE *buf)
{
    struct BtHidItem **nhiptr = nhr->nhr_FeatItemMap;
    ULONG cnt;

    for(cnt = 0; cnt < nhr->nhr_FeatItemCount; cnt++)
    {
        bEncodeItemBuffer(nhb, *nhiptr++, buf);
    }
}
/* \\\ */

/* /// "bEncodeItemBuffer()" */
void bEncodeItemBuffer(struct BTHidBinding *nhb, struct BtHidItem *nhi, UBYTE *buf)
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

/* /// "bParseArrayString()" */
void bParseArrayString(struct BTHidBinding *nhb, struct BtHidItem *nhi, STRPTR str)
{
    UWORD acount;
    LONG *valptr = nhi->nhi_Buffer;
    UWORD mode = 0;
    BOOL issigned = FALSE;

    if(!*str)
    {
        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Empty array value string not allowed.");
        return;
    }
    acount = nhi->nhi_Count;
    if(!acount)
    {
        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Target array has no space!");
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
                btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S0!");
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
                btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S1!");
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
                btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S2!");
                return;

            case 3: // ascii character
                if(*str == '\'')
                {
                    *valptr = ch;
                    str++;
                    mode = 5;
                    break;
                }
                btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S3!");
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
                btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S5!");
                return;
        }
    }
    if(!(*str))
    {
        if(mode == 3)
        {
            btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Syntax error in S3!");
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
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Parse error: Excessive elements ignored!");
    }
}
/* \\\ */

/* /// "bProcessItem()" */
BOOL bProcessItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, UBYTE *buf)
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
    struct BtHidAction *nha;

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
                    nhb->nhb_LastUSBKey = nhi->nhi_Usage;
                }
                nhb->nhb_LastItem = nhi;
                nhb->nhb_LastItemAList = &nhi->nhi_ActionList;
            }

            if(nhb->nhb_HIntBase)
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
            nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                if((nhi->nhi_OldValue != value) || ((nha->nha_Type & HUA_TRIGMASK) == HUA_ALWAYS))
                {
                    bDoAction(nhb, nha, nhi, nhi->nhi_Usage, value, (nhi->nhi_OldValue < value) || ((nha->nha_Type & HUA_TRIGMASK) == HUA_ALWAYS));
                }
                nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
            }
            if(nhi->nhi_OldValue > value)
            {
                nhi->nhi_HoldSeconds = 0;
                nhi->nhi_HoldMicros = 0;
            }
            if((nhi->nhi_OldValue != value) && nhb->nhb_ReportValues &&
                nhb->nhb_GUICurrentItem && (nhb->nhb_GUICurrentItem->nhgi_Item == nhi))
            {
                nhb->nhb_ItemChanged = TRUE;
            }
            nhi->nhi_OldValue = value;
            res = TRUE;
        } else {
            nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
            while(nha->nha_Node.ln_Succ)
            {
                if((nha->nha_Type & HUA_TRIGMASK) == HUA_NAN)
                {
                    bDoAction(nhb, nha, nhi, nhi->nhi_Usage, nhi->nhi_OldValue, 0);
                }
                nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
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
                        nhb->nhb_LastItem = nhi;
                        nhb->nhb_LastItemAList = &nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin];
                    }
                    nha = (struct BtHidAction *) nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin].lh_Head;
                    if(!nha->nha_Node.ln_Succ) /* Use default, no override defined */
                    {
                        nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
                    }
                    nhi->nhi_OldValue = nhi->nhi_LogicalMin - 1;
                    while(nha->nha_Node.ln_Succ)
                    {
                        bDoAction(nhb, nha, nhi, usage, value, FALSE);
                        nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
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
                            nhb->nhb_LastUSBKey = usage;
                        }
                        nhb->nhb_LastItem = nhi;
                        nhb->nhb_LastItemAList = &nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin];
                    }

                    nha = (struct BtHidAction *) nhi->nhi_ActionMap[value - nhi->nhi_LogicalMin].lh_Head;
                    if(!nha->nha_Node.ln_Succ) /* Use default, no override defined */
                    {
                        nha = (struct BtHidAction *) nhi->nhi_ActionList.lh_Head;
                    }
                    nhi->nhi_OldValue = nhi->nhi_LogicalMin - 1;
                    while(nha->nha_Node.ln_Succ)
                    {
                        bDoAction(nhb, nha, nhi, usage, value, TRUE);
                        nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
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

/* /// "bDoAction()" */
BOOL bDoAction(struct BTHidBinding *nhb, struct BtHidAction *nha, struct BtHidItem *nhi, ULONG uid, LONG value, BOOL downevent)
{
    UWORD atype = nha->nha_Type & HUA_ATYPEMASK;
    BOOL res = FALSE;
    ULONG qualmask;
    LONG clipmin, clipmax;
    LONG origvalue = value;

    if(nhb->nhb_DisableActions)
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
                var1 = nhb->nhb_KeyQualifiers;
                break;

            case HUAT_ALLQUAL:
                var1 = nhb->nhb_KeyQualifiers|PeekQualifier();
                break;

            case HUAT_RANDOMBIT:
                var1 = _rand(nhb->nhb_ClsBase) & 1;
                break;

            case HUAT_RANDOMVAL:
                var1 = _rand(nhb->nhb_ClsBase);
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
                var1 = nhb->nhb_LocalVars[nha->nha_CCVar1 - HUAT_LOCALVAR1];
                break;

            case HUAT_GLOBVARA:
            case HUAT_GLOBVARB:
            case HUAT_GLOBVARC:
            case HUAT_GLOBVARD:
            case HUAT_GLOBVARE:
            case HUAT_GLOBVARF:
            case HUAT_GLOBVARG:
            case HUAT_GLOBVARH:
                var1 = nhb->nhb_ClsBase->nh_GlobalVars[nha->nha_CCVar1 - HUAT_GLOBVARA];
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
                var2 = nhb->nhb_KeyQualifiers;
                break;

            case HUAT_ALLQUAL:
                var2 = nhb->nhb_KeyQualifiers|PeekQualifier();
                break;

            case HUAT_RANDOMBIT:
                var2 = _rand(nhb->nhb_ClsBase) & 1;
                break;

            case HUAT_RANDOMVAL:
                var2 = _rand(nhb->nhb_ClsBase);
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
                var2 = nhb->nhb_LocalVars[nha->nha_CCVar2 - HUAT_LOCALVAR1];
                break;

            case HUAT_GLOBVARA:
            case HUAT_GLOBVARB:
            case HUAT_GLOBVARC:
            case HUAT_GLOBVARD:
            case HUAT_GLOBVARE:
            case HUAT_GLOBVARF:
            case HUAT_GLOBVARG:
            case HUAT_GLOBVARH:
                var2 = nhb->nhb_ClsBase->nh_GlobalVars[nha->nha_CCVar2 - HUAT_GLOBVARA];
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
                value = nhb->nhb_KeyQualifiers;
                break;

            case HUAT_ALLQUAL:
                value = nhb->nhb_KeyQualifiers|PeekQualifier();
                break;

            case HUAT_RANDOMBIT:
                value = _rand(nhb->nhb_ClsBase) & 1;
                break;

            case HUAT_RANDOMVAL:
                value = _rand(nhb->nhb_ClsBase);
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
                value = nhb->nhb_LocalVars[nha->nha_ValVar - HUAT_LOCALVAR1];
                break;

            case HUAT_GLOBVARA:
            case HUAT_GLOBVARB:
            case HUAT_GLOBVARC:
            case HUAT_GLOBVARD:
            case HUAT_GLOBVARE:
            case HUAT_GLOBVARF:
            case HUAT_GLOBVARG:
            case HUAT_GLOBVARH:
                value = nhb->nhb_ClsBase->nh_GlobalVars[nha->nha_ValVar - HUAT_GLOBVARA];
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
            if(!nhb->nhb_ClsBase->nh_DTaskMsgPort)
            {
                bInstallLastActionHero(nhb);
            }
            if(nhb->nhb_ClsBase->nh_DTaskMsgPort)
            {
                if((am = AllocVec(sizeof(struct ActionMsg), MEMF_PUBLIC)))
                {
                    am->am_Msg.mn_Length = sizeof(struct ActionMsg);
                    am->am_Msg.mn_ReplyPort = NULL;
                    am->am_NCH = nhb;
                    am->am_Action = nha;
                    PutMsg(nhb->nhb_ClsBase->nh_DTaskMsgPort, &am->am_Msg);
                }
            }
            break;
        }

        case HUA_RAWKEY:
            bSendRawKey(nhb, nha->nha_RawKey);
            break;

        case HUA_KEYMAP:
        {
            UWORD iecode;
            if((uid > 0x70000) && (uid < 0x700e8))
            {
                iecode = nhb->nhb_KeymapCfg.kmc_Keymap[uid & 0xff];
                KPRINTF(1,("Key %ld %s\n", iecode, downevent ? "DOWN" : "UP"));
                nhb->nhb_FakeEvent.ie_Class = IECLASS_RAWKEY;
                nhb->nhb_FakeEvent.ie_SubClass = 0;
                bSendRawKey(nhb, downevent ? iecode : iecode|IECODE_UP_PREFIX);
            }
            break;
        }

        case HUA_MOUSEPOS:
            switch(nha->nha_MouseAxis)
            {
                case HUAT_DELTAX:
                    nhb->nhb_NewMouseRel = TRUE;
                    nhb->nhb_MouseDeltaX += value;
                    break;

                case HUAT_DELTAY:
                    nhb->nhb_NewMouseRel = TRUE;
                    nhb->nhb_MouseDeltaY += value;
                    break;

                case HUAT_ABSX:
                    if(nhb->nhb_MouseRangeY)
                    {
                        nhb->nhb_NewMouseAbs = TRUE;
                    }
                    nhb->nhb_MouseAbsX = value - nhi->nhi_RealMin;
                    nhb->nhb_MouseRangeX = (nhi->nhi_RealMax - nhi->nhi_RealMin) + 1;
                    break;

                case HUAT_ABSY:
                    if(nhb->nhb_MouseRangeX)
                    {
                        nhb->nhb_NewMouseAbs = TRUE;
                    }
                    nhb->nhb_MouseAbsY = value - nhi->nhi_RealMin;
                    nhb->nhb_MouseRangeY = (nhi->nhi_RealMax - nhi->nhi_RealMin) + 1;
                    break;
            }
            break;

        case HUA_QUALIFIER:
            qualmask = 1<<nha->nha_Qualifier;
            switch(nha->nha_QualMode)
            {
                case HUAT_SET:
                    nhb->nhb_KeyQualifiers |= qualmask;
                    break;

                case HUAT_CLEAR:
                    nhb->nhb_KeyQualifiers &= ~qualmask;
                    break;

                case HUAT_TOGGLE:
                    nhb->nhb_KeyQualifiers ^= qualmask;
                    break;

                case HUAT_ASSIGN:
                    nhb->nhb_KeyQualifiers &= ~qualmask;
                    if(downevent)
                    {
                        nhb->nhb_KeyQualifiers |= qualmask;
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
                    if((!newmouse) && (nhb->nhb_KeyQualifiers & iequal))
                    {
                        iequal = 0;
                        iecode = IECODE_NOBUTTON;
                    } else {
                        nhb->nhb_KeyQualifiers |= iequal;
                        nhb->nhb_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                    }
                    break;

                case HUAT_CLEAR:
                    if(newmouse || (nhb->nhb_KeyQualifiers & iequal))
                    {
                        nhb->nhb_KeyQualifiers &= ~iequal;
                        nhb->nhb_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
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
                        if(nhb->nhb_MouseButtons & (1<<(nha->nha_ButtonNo-1)))
                        {
                            nhb->nhb_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
                            iecode |= IECODE_UP_PREFIX;
                        } else {
                            nhb->nhb_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                        }
                    } else {
                        if(nhb->nhb_KeyQualifiers & iequal)
                        {
                            nhb->nhb_KeyQualifiers &= ~iequal;
                            nhb->nhb_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
                            iequal = 0;
                            iecode |= IECODE_UP_PREFIX;
                        } else {
                            nhb->nhb_KeyQualifiers |= iequal;
                            nhb->nhb_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                        }
                    }
                    break;

                case HUAT_ASSIGN:
                    if(value)
                    {
                        if((!newmouse) && (nhb->nhb_KeyQualifiers & iequal))
                        {
                            iequal = 0;
                            iecode = IECODE_NOBUTTON;
                        } else {
                            nhb->nhb_KeyQualifiers |= iequal;
                            nhb->nhb_MouseButtons |= 1<<(nha->nha_ButtonNo-1);
                        }
                    } else {
                        if(newmouse || (nhb->nhb_KeyQualifiers & iequal))
                        {
                            nhb->nhb_KeyQualifiers &= ~iequal;
                            nhb->nhb_MouseButtons &= ~(1<<(nha->nha_ButtonNo-1));
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
                    bSendRawKey(nhb, iecode);

                    nhb->nhb_FakeEvent.ie_Class = IECLASS_NEWMOUSE;
                    nhb->nhb_FakeEvent.ie_SubClass = 0;
                    nhb->nhb_FakeEvent.ie_Code = iecode;
                    nhb->nhb_FakeEvent.ie_NextEvent = NULL;
                    nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers;
                    nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
                    nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
                    nhb->nhb_InpIOReq->io_Command = IND_WRITEEVENT;
                    DoIO((struct IORequest *) nhb->nhb_InpIOReq);
                } else {
                    nhb->nhb_FakeEvent.ie_X = nhb->nhb_MouseDeltaX;
                    nhb->nhb_FakeEvent.ie_Y = nhb->nhb_MouseDeltaY;
                    nhb->nhb_FakeEvent.ie_Class = IECLASS_RAWMOUSE;
                    nhb->nhb_FakeEvent.ie_SubClass = 0;
                    nhb->nhb_FakeEvent.ie_Code = iecode;
                    nhb->nhb_FakeEvent.ie_NextEvent = NULL;
                    nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers|IEQUALIFIER_RELATIVEMOUSE;
                    nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
                    nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
                    nhb->nhb_InpIOReq->io_Command = nhb->nhb_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
                    DoIO((struct IORequest *) nhb->nhb_InpIOReq);
                    nhb->nhb_NewMouseRel = FALSE;
                    nhb->nhb_MouseDeltaX = 0;
                    nhb->nhb_MouseDeltaY = 0;
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
                    nhb->nhb_MouseAbsZ = value - nhi->nhi_RealMin;
                    nhb->nhb_MouseRangeZ = (nhi->nhi_RealMax - nhi->nhi_RealMin) + 1;
                    break;

                case HUAT_PRESSURE:
                    if(nhi->nhi_RealMax)
                    {
                        nhb->nhb_TabPressure = ((0x1fffffff / nhi->nhi_RealMax)*value)<<2;
                        nhb->nhb_VldPressure = TRUE;
                    }
                    break;

                case HUAT_XROT:
                    if(nhi->nhi_RealMax)
                    {
                        nhb->nhb_TabRotX = ((0x3fffffff / nhi->nhi_RealMax)*value)<<2;
                        nhb->nhb_VldRotX = TRUE;
                    }
                    break;

                case HUAT_YROT:
                    if(nhi->nhi_RealMax)
                    {
                        nhb->nhb_TabRotY = ((0x3fffffff / nhi->nhi_RealMax)*value)<<2;
                        nhb->nhb_VldRotY = TRUE;
                    }
                    break;

                case HUAT_ZROT:
                    if(nhi->nhi_RealMax)
                    {
                        nhb->nhb_TabRotZ = ((0x3fffffff / nhi->nhi_RealMax)*value)<<2;
                        nhb->nhb_VldRotZ = TRUE;
                    }
                    break;

                case HUAT_PROX:
                    nhb->nhb_TabProx = value;
                    nhb->nhb_VldProx = TRUE;
                    break;
            }
            if(nhb->nhb_MouseRangeX && nhb->nhb_MouseRangeY)
            {
                nhb->nhb_NewMouseAbs = TRUE;
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
                        bSendRawKey(nhb, wheeliecode);
#if 0
                        nhb->nhb_FakeEvent.ie_Class = IECLASS_NEWMOUSE;
                        nhb->nhb_FakeEvent.ie_SubClass = 0;
                        nhb->nhb_FakeEvent.ie_Code = wheeliecode;
                        nhb->nhb_FakeEvent.ie_NextEvent = NULL;
                        nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers;
                        nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
                        nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
                        nhb->nhb_InpIOReq->io_Command = IND_WRITEEVENT;
                        DoIO((struct IORequest *) nhb->nhb_InpIOReq);
#endif
                    }

                    bSendRawKey(nhb, wheeliecode|IECODE_UP_PREFIX);
#if 0
                    nhb->nhb_FakeEvent.ie_Class = IECLASS_NEWMOUSE;
                    nhb->nhb_FakeEvent.ie_SubClass = 0;
                    nhb->nhb_FakeEvent.ie_Code = wheeliecode|IECODE_UP_PREFIX;
                    nhb->nhb_FakeEvent.ie_NextEvent = NULL;
                    nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers;
                    nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
                    nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
                    nhb->nhb_InpIOReq->io_Command = IND_WRITEEVENT;
                    DoIO((struct IORequest *) nhb->nhb_InpIOReq);
#endif
                }
            }
            break;
        }

        case HUA_DIGJOY:
        {
            ULONG mask = 0;
            /* ULONG oldval; */
            ULONG *stateptr = &nhb->nhb_LLPortState[nha->nha_JoypadPort];

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
                    stateptr = &nhb->nhb_LLHatswitch[nha->nha_JoypadPort];
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
            ULONG *stateptr = &nhb->nhb_LLAnalogue[nha->nha_JoypadPort];
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
            struct BtHidItem *nhi;

            nhi = bFindItemID(nhb, nha->nha_OutItem, REPORT_MAIN_OUTPUT, &pos);
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
                    bParseArrayString(nhb, nhi, nha->nha_OutArray);
                }
                nhi->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                nhb->nhb_OutFeatTouched = TRUE;
            }
            break;
        }

        case HUA_FEATURE:
        {
            ULONG pos;
            struct BtHidItem *nhi;

            nhi = bFindItemID(nhb, nha->nha_FeatItem, REPORT_MAIN_FEATURE, &pos);
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
                    bParseArrayString(nhb, nhi, nha->nha_OutArray);
                }
                nhi->nhi_Collection->nhc_Report->nhr_FeatTouched = TRUE;
                nhb->nhb_OutFeatTouched = TRUE;
            }
            break;
        }

        case HUA_MISC:
            nhb->nhb_FakeEvent.ie_Class = IECLASS_NULL;
            nhb->nhb_FakeEvent.ie_SubClass = 0;
            nhb->nhb_FakeEvent.ie_Code = IECODE_NOBUTTON;
            nhb->nhb_FakeEvent.ie_NextEvent = NULL;
            nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers;
            nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
            nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
            nhb->nhb_InpIOReq->io_Command = IND_WRITEEVENT;
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
                    if(!nhb->nhb_ClsBase->nh_DTaskMsgPort)
                    {
                        bInstallLastActionHero(nhb);
                    }
                    if(nhb->nhb_ClsBase->nh_DTaskMsgPort)
                    {
                        if((am = AllocVec(sizeof(struct ActionMsg), MEMF_PUBLIC)))
                        {
                            am->am_Msg.mn_Length = sizeof(struct ActionMsg);
                            am->am_Msg.mn_ReplyPort = NULL;
                            am->am_NCH = nhb;
                            am->am_Action = nha;
                            PutMsg(nhb->nhb_ClsBase->nh_DTaskMsgPort, &am->am_Msg);
                        }
                    }
                    break;
                }

                case HUAT_CLOSEWINDOW:
                    nhb->nhb_FakeEvent.ie_Class = IECLASS_CLOSEWINDOW;
                    DoIO((struct IORequest *) nhb->nhb_InpIOReq);
                    break;

                case HUAT_REBOOT:
                    nhb->nhb_CDC->cdc_EnableKBReset = TRUE;
                    nhb->nhb_KeyQualifiers |= IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND;
                    bCheckReset(nhb);
                    break;

                case HUAT_FLUSHEVENTS:
                    bFlushEvents(nhb);
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
                    tarvarptr = &nhb->nhb_LocalVars[nha->nha_TarVar - HUAT_LOCALVAR1];
                    break;

                case HUAT_GLOBVARA:
                case HUAT_GLOBVARB:
                case HUAT_GLOBVARC:
                case HUAT_GLOBVARD:
                case HUAT_GLOBVARE:
                case HUAT_GLOBVARF:
                case HUAT_GLOBVARG:
                case HUAT_GLOBVARH:
                    tarvarptr = &nhb->nhb_ClsBase->nh_GlobalVars[nha->nha_TarVar - HUAT_GLOBVARA];
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
            nhb->nhb_FakeEvent.ie_Class = IECLASS_EXTRAWKEY;
            nhb->nhb_FakeEvent.ie_SubClass = 0;
            nhb->nhb_FakeEvent.ie_Code = nha->nha_RawKey;
            // using ie.dead.ie_prev1DownCode instead of ie_extkey.ie_extkey1
            nhb->nhb_FakeEvent.ie_position.ie_dead.ie_prev1DownCode = 0xe0;
            nhb->nhb_FakeEvent.ie_position.ie_dead.ie_prev1DownQual = nha->nha_RawKey;
            nhb->nhb_FakeEvent.ie_NextEvent = NULL;
            nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers;
            nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
            nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
            nhb->nhb_InpIOReq->io_Command = IND_WRITEEVENT;
            DoIO((struct IORequest *) nhb->nhb_InpIOReq);
#endif
            break;

    }
    return(res);
}
/* \\\ */

/* /// "bFlushEvents()" */
void bFlushEvents(struct BTHidBinding *nhb)
{
    ULONG buflen;

    struct BtHidReport *nhr;

    /* output/feature reports */
    if(nhb->nhb_OutFeatTouched)
    {
        nhb->nhb_OutFeatTouched = FALSE;
        nhr = (struct BtHidReport *) nhb->nhb_HidReports.lh_Head;
        while(nhr->nhr_Node.ln_Succ)
        {
            if(nhr->nhr_OutTouched)
            {
                KPRINTF(1, ("Out Report %ld touched...\n", nhr->nhr_ReportID));
                buflen = (nhr->nhr_ReportOutSize+7)>>3;
                bGenerateOutReport(nhb, nhr, nhb->nhb_EPOutBuf);
                bSendReport(nhb, nhr->nhr_ReportID, 2, nhb->nhb_EPOutBuf, buflen);
                nhr->nhr_OutTouched = FALSE;
            }
            if(nhr->nhr_FeatTouched)
            {
                KPRINTF(1, ("Feat Report %ld touched...\n", nhr->nhr_ReportID));
                buflen = (nhr->nhr_ReportFeatSize+7)>>3;
                bGenerateFeatReport(nhb, nhr, nhb->nhb_EPOutBuf);
                bSendReport(nhb, nhr->nhr_ReportID, 3, nhb->nhb_EPOutBuf, buflen);
                nhr->nhr_FeatTouched = FALSE;
            }
            nhr = (struct BtHidReport *) nhr->nhr_Node.ln_Succ;
        }
    }
    if(nhb->nhb_NewMouseRel && (nhb->nhb_MouseDeltaX || nhb->nhb_MouseDeltaY))
    {
        KPRINTF(1, ("Mouse rel movement...\n"));
        nhb->nhb_FakeEvent.ie_X = nhb->nhb_MouseDeltaX;
        nhb->nhb_FakeEvent.ie_Y = nhb->nhb_MouseDeltaY;
        nhb->nhb_FakeEvent.ie_Class = IECLASS_RAWMOUSE;
        nhb->nhb_FakeEvent.ie_SubClass = 0;
        nhb->nhb_FakeEvent.ie_Code = IECODE_NOBUTTON;
        nhb->nhb_FakeEvent.ie_NextEvent = NULL;
        nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers|IEQUALIFIER_RELATIVEMOUSE;
        nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
        nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
        nhb->nhb_InpIOReq->io_Command = nhb->nhb_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
        DoIO((struct IORequest *) nhb->nhb_InpIOReq);
        nhb->nhb_NewMouseRel = FALSE;
        nhb->nhb_MouseDeltaX = 0;
        nhb->nhb_MouseDeltaY = 0;
    }
    if(nhb->nhb_NewMouseAbs)
    {
        ULONG *tagptr = nhb->nhb_TabTags;
        KPRINTF(1, ("Mouse abs movement...\n"));
        nhb->nhb_FakeEvent.ie_EventAddress = &nhb->nhb_TabletEvent;
        nhb->nhb_TabletEvent.ient_RangeX = nhb->nhb_MouseRangeX;
        nhb->nhb_TabletEvent.ient_RangeY = nhb->nhb_MouseRangeY;
        nhb->nhb_TabletEvent.ient_TabletX = nhb->nhb_MouseAbsX;
        nhb->nhb_TabletEvent.ient_TabletY = nhb->nhb_MouseAbsY;
        nhb->nhb_TabletEvent.ient_TagList = (struct TagItem *) tagptr;
        if(nhb->nhb_MouseRangeZ)
        {
            KPRINTF(1, ("AbsZ %ld\n", nhb->nhb_MouseAbsZ));
            *tagptr++ = TABLETA_TabletZ;
            *tagptr++ = nhb->nhb_MouseAbsZ;
            *tagptr++ = TABLETA_RangeZ;
            *tagptr++ = nhb->nhb_MouseRangeZ;
        }
        if(nhb->nhb_VldRotX)
        {
            KPRINTF(1, ("AngleX %ld\n", nhb->nhb_TabRotX));
            *tagptr++ = TABLETA_AngleX;
            *tagptr++ = nhb->nhb_TabRotX;
        }
        if(nhb->nhb_VldRotY)
        {
            KPRINTF(1, ("AngleY %ld\n", nhb->nhb_TabRotY));
            *tagptr++ = TABLETA_AngleY;
            *tagptr++ = nhb->nhb_TabRotY;
        }
        if(nhb->nhb_VldRotZ)
        {
            KPRINTF(1, ("AngleZ %ld\n", nhb->nhb_TabRotZ));
            *tagptr++ = TABLETA_AngleZ;
            *tagptr++ = nhb->nhb_TabRotZ;
        }
        if(nhb->nhb_VldPressure)
        {
            KPRINTF(1, ("Pressure %ld\n", nhb->nhb_TabPressure));
            *tagptr++ = TABLETA_Pressure;
            *tagptr++ = nhb->nhb_TabPressure;
        }
        *tagptr++ = TABLETA_ButtonBits;
        *tagptr++ = nhb->nhb_MouseButtons;
        if(nhb->nhb_VldProx)
        {
            KPRINTF(1, ("InProx %ld\n", nhb->nhb_TabProx));
            *tagptr++ = TABLETA_InProximity;
            *tagptr++ = nhb->nhb_TabProx;
        }
        *tagptr = TAG_END;
        nhb->nhb_FakeEvent.ie_Class = IECLASS_NEWPOINTERPOS;
        nhb->nhb_FakeEvent.ie_SubClass = IESUBCLASS_NEWTABLET;
        nhb->nhb_FakeEvent.ie_Code = IECODE_NOBUTTON;
        nhb->nhb_FakeEvent.ie_NextEvent = NULL;
        nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers;
        nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
        nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
        nhb->nhb_InpIOReq->io_Command = IND_WRITEEVENT;
        DoIO((struct IORequest *) nhb->nhb_InpIOReq);
        nhb->nhb_NewMouseAbs = FALSE;
    }
    bCheckReset(nhb);
}
/* \\\ */

/* /// "bSendRawKey()" */
void bSendRawKey(struct BTHidBinding *nhb, UWORD key)
{
    nhb->nhb_FakeEvent.ie_Class = IECLASS_RAWKEY;
    nhb->nhb_FakeEvent.ie_SubClass = 0;
    nhb->nhb_FakeEvent.ie_Code = key;
    nhb->nhb_FakeEvent.ie_NextEvent = NULL;
    nhb->nhb_FakeEvent.ie_Qualifier = nhb->nhb_KeyQualifiers;
    nhb->nhb_InpIOReq->io_Data = &nhb->nhb_FakeEvent;
    nhb->nhb_InpIOReq->io_Length = sizeof(struct InputEvent);
    nhb->nhb_InpIOReq->io_Command = nhb->nhb_OS4Hack ? IND_ADDEVENT : IND_WRITEEVENT;
    DoIO((struct IORequest *) nhb->nhb_InpIOReq);
}
/* \\\ */

/* /// "bCheckReset()" */
void bCheckReset(struct BTHidBinding *nhb)
{
    if(nhb->nhb_CDC->cdc_EnableKBReset &&
       (nhb->nhb_KeyQualifiers & (IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)) ==
                                 (IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND))
    {
        struct MsgPort *mp;
        struct IOStdReq *ioreq;
        struct Interrupt tempint;
        struct Node *node;
        struct List *listhead = NULL;
        BOOL haskbh = FALSE;
        KPRINTF(20, ("Reboot!\n"));

        if(nhb->nhb_CDC->cdc_EnableRH)
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
                                btDelayMS(nhb->nhb_CDC->cdc_ResetDelay*1000);
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
    else if(nhb->nhb_CDC->cdc_EnableKBReset &&
        (nhb->nhb_KeyQualifiers & IEQUALIFIER_CONTROL) &&
        (nhb->nhb_KeyQualifiers & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)) &&
        nhb->nhb_FakeEvent.ie_Code == RAWKEY_DELETE)
    {
        KPRINTF(20, ("Reboot!\n"));
        ShutdownA(SD_ACTION_COLDREBOOT);
    }
}
/* \\\ */

/* /// "bLoadItem()" */
BOOL bLoadItem(struct BTHidBinding *nhb, struct BtIFFContext *rppic, struct List *lst, UWORD idbase)
{
    struct BtIFFContext *itpic;
    struct BtIFFContext *actpic;
    struct BtHidAction *nha;
    ULONG newform[3];
    ULONG buf[2];
    struct BtHidActionChunk *nhac;
    STRPTR tmpstr;

    btSafeRawDoFmt((STRPTR) buf, 8, "I%03lx", GET_WTYPE(lst) - idbase + 1);
    newform[0] = AROS_LONG2BE(ID_FORM);
    newform[1] = AROS_LONG2BE(4);
    newform[2] = *buf;
    itpic = btFindCfgForm(rppic, AROS_LONG2BE(newform[2]));
    if(!itpic)
    {
        return(FALSE);
    }
    actpic = btFindCfgForm(itpic, MAKE_ID('A','C','T','N'));
    while(actpic)
    {
        if((nhac = btGetCfgChunk(actpic, MAKE_ID('A','C','D','F'))))
        {
            if((nha = bAllocAction(nhb, lst, 0)))
            {
                CopyMem(&nhac->nhac_Type, &nha->nha_Type, min(AROS_LONG2BE(nhac->nhac_Length), sizeof(struct BtHidActionChunk) - 8));
                if((tmpstr = btGetStringChunk(actpic, MAKE_ID('S','N','D','F'))))
                {
                    strncpy(nha->nha_SoundFile, tmpstr, 255);
                    btFreeVec(tmpstr);
                }
                if((tmpstr = btGetStringChunk(actpic, MAKE_ID('V','A','N','S'))))
                {
                    strncpy(nha->nha_VanillaString, tmpstr, 79);
                    btFreeVec(tmpstr);
                }
                if((tmpstr = btGetStringChunk(actpic, MAKE_ID('K','E','Y','S'))))
                {
                    strncpy(nha->nha_KeyString, tmpstr, 79);
                    btFreeVec(tmpstr);
                }
                if((tmpstr = btGetStringChunk(actpic, MAKE_ID('E','X','E','S'))))
                {
                    strncpy(nha->nha_ExeString, tmpstr, 79);
                    btFreeVec(tmpstr);
                }
                if((tmpstr = btGetStringChunk(actpic, MAKE_ID('O','A','R','R'))))
                {
                    strncpy(nha->nha_OutArray, tmpstr, 255);
                    btFreeVec(tmpstr);
                }
            }
            btFreeVec(nhac);
        }
        actpic = btNextCfgForm(actpic);
    }
    return(TRUE);
}
/* \\\ */

/* /// "bSaveItem()" */
struct BtIFFContext * bSaveItem(struct BTHidBinding *nhb, struct BtIFFContext *rppic, struct List *lst, UWORD idbase)
{
    struct BtIFFContext *itpic;
    struct BtIFFContext *actpic;
    struct BtHidAction *nha;
    ULONG newform[3];
    ULONG buf[2];
    struct BtHidActionChunk nhac;
    BOOL isdefault = TRUE;

    if(!lst->lh_Head->ln_Succ)
    {
        return(NULL);
    }
    nha = (struct BtHidAction *) lst->lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        if(!nha->nha_IsDefault)
        {
            isdefault = FALSE;
        }
        nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
    }
    if(isdefault)
    {
        return(NULL);
    }
    btSafeRawDoFmt((STRPTR) buf, 8, "I%03lx", GET_WTYPE(lst) - idbase + 1);
    newform[0] = AROS_LONG2BE(ID_FORM);
    newform[1] = AROS_LONG2BE(4);
    newform[2] = *buf;
    itpic = btFindCfgForm(rppic, AROS_LONG2BE(newform[2]));
    if(!itpic)
    {
        itpic = btAddCfgEntry(rppic, newform);
        if(!itpic)
        {
            return(NULL);
        }
    }
    nhac.nhac_ID = AROS_LONG2BE(MAKE_ID('A','C','D','F'));
    nhac.nhac_Length = AROS_LONG2BE(sizeof(struct BtHidActionChunk)-8);
    nha = (struct BtHidAction *) lst->lh_Head;
    while(nha->nha_Node.ln_Succ)
    {
        if(!nha->nha_IsDefault)
        {
            isdefault = FALSE;
        }
        newform[2] = AROS_LONG2BE(MAKE_ID('A','C','T','N'));
        actpic = btAddCfgEntry(itpic, newform);
        if(!actpic)
        {
            break;
        }
        CopyMem(&nha->nha_Type, &nhac.nhac_Type, sizeof(struct BtHidActionChunk)-8);
        btAddCfgEntry(actpic, &nhac);
        if(*nha->nha_SoundFile)
        {
            btAddStringChunk(actpic, MAKE_ID('S','N','D','F'), nha->nha_SoundFile);
        }
        if(*nha->nha_VanillaString)
        {
            btAddStringChunk(actpic, MAKE_ID('V','A','N','S'), nha->nha_VanillaString);
        }
        if(*nha->nha_KeyString)
        {
            btAddStringChunk(actpic, MAKE_ID('K','E','Y','S'), nha->nha_KeyString);
        }
        if(*nha->nha_ExeString)
        {
            btAddStringChunk(actpic, MAKE_ID('E','X','E','S'), nha->nha_ExeString);
        }
        if(*nha->nha_OutArray)
        {
            btAddStringChunk(actpic, MAKE_ID('O','A','R','R'), nha->nha_OutArray);
        }
        nha = (struct BtHidAction *) nha->nha_Node.ln_Succ;
    }
    return(itpic);
}
/* \\\ */

/* /// "bReadJoyPort()" */
AROS_LH1(ULONG, bReadJoyPort,
         AROS_LHA(ULONG, port, D0),
         struct Library *, LowLevelBase, 5, bthid)
{
    AROS_LIBFUNC_INIT

    struct BTHidBase *nh;
    struct BTHidBinding *nhb;
    ULONG result = JP_TYPE_NOTAVAIL;
    ULONG mode;

    if(!(nh = (struct BTHidBase *) FindName(&SysBase->LibList, GM_UNIQUENAME(libname))))
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
        nhb = (struct BTHidBinding *) nh->nh_Interfaces.lh_Head;
        while(nhb->nhb_Node.ln_Succ)
        {
            mode = nhb->nhb_CDC->cdc_LLPortMode[port];
            if(nh->nh_LLAnalogueOverride[port] && nhb->nhb_LLAnalogue[port])
            {
                mode = 4;
            }

            switch(mode)
            {
//                case 0: /* don't touch */
//                    break;

                case 1: /* overwrite with USB */
                    result = nhb->nhb_LLPortState[port] | nhb->nhb_LLHatswitch[port] | JP_TYPE_GAMECTLR;
                    break;

                case 2: /* merge with USB */
                    if(((result & JP_TYPE_MASK) == JP_TYPE_GAMECTLR) ||
                       ((result & JP_TYPE_MASK) == JP_TYPE_JOYSTK) ||
                       ((result & JP_TYPE_MASK) == JP_TYPE_UNKNOWN) ||
                       (!result))
                    {
                        result = (result & (JP_BUTTON_MASK|JP_DIRECTION_MASK)) | nhb->nhb_LLPortState[port] | nhb->nhb_LLHatswitch[port] | JP_TYPE_GAMECTLR;
                    }
                    break;

                case 3: /* disable */
                    result = JP_TYPE_NOTAVAIL;
                    break;

                case 4: /* overwrite with analogue USB */
                    if(nhb->nhb_LLAnalogue[port])
                    {
                        result = (nhb->nhb_LLPortState[port] & ~JP_DIRECTION_MASK) | (nhb->nhb_LLAnalogue[port] & JP_XYAXIS_MASK) | nhb->nhb_LLHatswitch[port] | JP_TYPE_ANALOGUE;
                    }
                    break;
            }
            nhb = (struct BTHidBinding *) nhb->nhb_Node.ln_Succ;
        }
        Permit();
    }
    if((port >= JP_ANALOGUE_PORT_MAGIC) && (port < (JP_ANALOGUE_PORT_MAGIC|4)))
    {
        port &= 3;
        Forbid();
        nhb = (struct BTHidBinding *) nh->nh_Interfaces.lh_Head;
        while(nhb->nhb_Node.ln_Succ)
        {
            if(nhb->nhb_LLAnalogue[port])
            {
                result = (nhb->nhb_LLPortState[port] & ~JP_DIRECTION_MASK) | (nhb->nhb_LLAnalogue[port] & JP_XYAXIS_MASK) | nhb->nhb_LLHatswitch[port] | JP_TYPE_ANALOGUE;
            }
            nhb = (struct BTHidBinding *) nhb->nhb_Node.ln_Succ;
        }
        Permit();
    }
    KPRINTF(40, ("And after %08lx\n", result));

    return(result);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bSetJoyPortAttrsA()" */
AROS_LH2(ULONG, bSetJoyPortAttrsA,
         AROS_LHA(ULONG, port, D0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct Library *, LowLevelBase, 22, bthid)
{
    AROS_LIBFUNC_INIT

    struct BTHidBase *nh;
    struct TagItem *typetag;
    struct TagItem *motortags[2];
    struct TagItem *motorofftag;
    ULONG result = FALSE;

    if(!(nh = (struct BTHidBase *) FindName(&SysBase->LibList, GM_UNIQUENAME(libname))))
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
            struct BtHidItem *nhi;
            struct BTHidBinding *nhb;
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
            nhb = (struct BTHidBinding *) nh->nh_Interfaces.lh_Head;
            while(nhb->nhb_Node.ln_Succ)
            {
                if(nhb->nhb_CDC->cdc_LLRumblePort == port+1)
                {
                    for(cnt = 0; cnt < 2; cnt++)
                    {
                        if(motortags[cnt] && ((nhi = nhb->nhb_RumbleMotors[cnt])))
                        {
                            if(nhi->nhi_OldValue != motortags[cnt]->ti_Data)
                            {
                                nhi->nhi_OldValue = motortags[cnt]->ti_Data;
                                nhi->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
                                nhb->nhb_OutFeatTouched = TRUE;
                                sigit = TRUE;
                            }
                        }
                    }
                    if(sigit)
                    {
                        Signal(nhb->nhb_Task, (1L<<nhb->nhb_TaskMsgPort->mp_SigBit));
                    }
                }

                nhb = (struct BTHidBinding *) nhb->nhb_Node.ln_Succ;
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

/* /// "bInstallLastActionHero()" */
void bInstallLastActionHero(struct BTHidBinding *nhb)
{
    struct BTHidBase *nh = nhb->nhb_ClsBase;

    if(!nh->nh_DispatcherTask)
    {
        nh->nh_ReadySignal = SIGB_SINGLE;
        nh->nh_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if(btSpawnSubTask("Last Action Hero", GM_UNIQUENAME(bDispatcherTask), nh))
        {
            Wait(1L<<nh->nh_ReadySignal);
            if(!nh->nh_DispatcherTask)
            {
                btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                               "It's your fault! I'm sure! Couldn't create subtask for launching!");
            } else {
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Last Action Hero successfully launched!");
            }
        } else {
            btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "It's your fault! I'm sure! Couldn't create subtask for launching!");
        }
        nh->nh_ReadySigTask = NULL;
        //FreeSignal(nh->nh_ReadySignal);
    }
}
/* \\\ */

/************************************************************************/

#undef BluetoothBase

/* /// "bDispatcherTask()" */
AROS_UFH0(void, GM_UNIQUENAME(bDispatcherTask))
{
    AROS_USERFUNC_INIT

    struct BTHidBase *nh;
    struct Task *thistask;
    struct ActionMsg *am;
    struct BtHidSound *nhs;

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

                                        bLastActionHero(nh);

                                        Forbid();
                                        while((am = (struct ActionMsg *) GetMsg(nh->nh_DTaskMsgPort)))
                                        {
                                            FreeVec(am);
                                        }
                                        nhs = (struct BtHidSound *) nh->nh_Sounds.lh_Head;
                                        while(nhs->nhs_Node.ln_Succ)
                                        {
                                            bFreeSound(nh, nhs);
                                            nhs = (struct BtHidSound *) nh->nh_Sounds.lh_Head;
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

/* /// "bLastActionHero()" */
void bLastActionHero(struct BTHidBase *nh)
{
    ULONG sigmask;
    ULONG sigs;
    struct ActionMsg *am;
    struct BtHidAction *nha;
    struct BTHidBinding *nhb;
    struct Window *actwindow;
    struct Screen *actscreen;
    char buf[128];
    ULONG intlock;
    sigmask = (1L<<nh->nh_DTaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
    do
    {
        while((am = (struct ActionMsg *) GetMsg(nh->nh_DTaskMsgPort)))
        {
            nhb = am->am_NCH;
            nha = am->am_Action;
            switch(nha->nha_Type & HUA_ATYPEMASK)
            {
                case HUA_VANILLA:
                    buf[0] = '<';
                    strcpy(&buf[1], nha->nha_VanillaString);
                    strcat(buf, ">");
                    if(!bSendKeyString(nh, buf))
                    {
                        bEasyRequest(nh, "'%s' is no good. Really.", "Argl!", nha->nha_VanillaString);
                    }
                    break;

                case HUA_KEYSTRING:
                    if(!bSendKeyString(nh, nha->nha_KeyString))
                    {
                        bEasyRequest(nh, "Don't you ever say\n'%s'\nto me again!", "Terribly sorry!", nha->nha_KeyString);
                    }
                    break;

                case HUA_SOUND:
                    bPlaySound(nh, nha);
                    break;

                case HUA_SHELL:
                {
                    BPTR fhandle;
                    LONG ioerr;
                    if((fhandle = Open(nhb->nhb_CDC->cdc_ShellCon, MODE_READWRITE)))
                    {
                        ioerr = SystemTags(nha->nha_ExeString,
                                           SYS_Input, fhandle,
                                           SYS_Output, NULL,
                                           SYS_Asynch, nha->nha_ShellAsync,
                                           NP_StackSize, nhb->nhb_CDC->cdc_ShellStack,
                                           TAG_END);
                        if(ioerr)
                        {
                            bEasyRequest(nh, "Offender survived execution of\n'%s'\nwith error code %ld.", "Get me a shotgun then!", nha->nha_ExeString, ioerr);
                        }
                        if((!nha->nha_ShellAsync) || ioerr)
                        {
                            Close(fhandle);
                        }
                    } else {
                        bEasyRequest(nh, "Do you really think\n'%s'\nis a nice and witty\nconsole window? Well, it's not!", "I'll use that string to hang myself!", nhb->nhb_CDC->cdc_ShellCon);
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

/* /// "bLoadSound()" */
struct BtHidSound * bLoadSound(struct BTHidBase *nh, STRPTR name)
{
    struct BtHidSound *nhs;
    if((nhs = AllocVec(sizeof(struct BtHidSound), MEMF_PUBLIC|MEMF_CLEAR)))
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
                bEasyRequest(nh, "Does '%s' make a sound\nin the woods, when it was chopped\nbut nobody was looking at it?", "No! Couldn't load it!", name);
            }
            return(nhs);
        }
        FreeVec(nhs);
    }
    return(NULL);
}
/* \\\ */

/* /// "bPlaySound()" */
BOOL bPlaySound(struct BTHidBase *nh, struct BtHidAction *nha)
{
    struct BtHidSound *nhs;
    struct dtTrigger playmsg;
    nhs = (struct BtHidSound *) FindName(&nh->nh_Sounds, nha->nha_SoundFile);
    if(!nhs)
    {
        nhs = bLoadSound(nh, nha->nha_SoundFile);
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

/* /// "bFreeSound()" */
void bFreeSound(struct BTHidBase *nh, struct BtHidSound *nhs)
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

/* /// "bInvertString()" */
struct InputEvent *bInvertString(struct BTHidBase *nh, STRPTR str, struct KeyMap *km)
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
        bFreeIEvents(nh, prior);
        return(NULL);
    }
    return(prior);
}
/* \\\ */

/* /// "bFreeIEvents()" */
void bFreeIEvents(struct BTHidBase *nh, struct InputEvent *event)
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

/* /// "bSendKeyString()" */
BOOL bSendKeyString(struct BTHidBase *nh, STRPTR str)
{
    struct InputEvent *events = bInvertString(nh, str, NULL);
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
    bFreeIEvents(nh, events);
    return(TRUE);
}
/* \\\ */

/* /// "bEasyRequest()" */
LONG bEasyRequest(struct BTHidBase *nh, STRPTR body, STRPTR gadgets, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(gadgets);
    ret = bEasyRequestA(nh, body, gadgets, AROS_SLOWSTACKFORMAT_ARG(gadgets));
    AROS_SLOWSTACKFORMAT_POST(gadgets);

    return ret;
}
/* \\\ */

/* /// "bEasyRequestA()" */
LONG bEasyRequestA(struct BTHidBase *nh, STRPTR body, STRPTR gadgets, RAWARG params)
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
