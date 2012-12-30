/*
 *----------------------------------------------------------------------------
 *                         bootkeyboard class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "bootkeyboard.class.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(!UtilityBase)
    {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        return FALSE;
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return TRUE;
}

static int libOpen(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    bootkbd_LoadClassConfig(nh);

    return TRUE;
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    CloseLibrary(UtilityBase);
    nh->nh_UtilityBase = NULL;

    return TRUE;
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

/* /// "bootkbd_AttemptInterfaceBinding()" */
struct NepClassHid * bootkbd_AttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    KPRINTF(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    TAG_DONE);
        CloseLibrary(ps);
        if((ifclass == HID_CLASSCODE) && (subclass == HID_BOOT_SUBCLASS) && (proto == HID_PROTO_KEYBOARD))
        {
            return(bootkbd_ForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "bootkbd_ForceInterfaceBinding()" */
struct NepClassHid * bootkbd_ForceInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassHid *nch;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        if((nch = psdAllocVec(sizeof(struct NepClassHid))))
        {
            nch->nch_ClsBase = nh;
            nch->nch_Device = NULL;
            nch->nch_Interface = pif;

            bootkbd_LoadClassConfig(nh);

            psdSafeRawDoFmt(buf, 64, "bootkeyboard.class<%08lx>", nch);
            nch->nch_ReadySignal = SIGB_SINGLE;
            nch->nch_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            if((tmptask = psdSpawnSubTask(buf, bootkbd_HidTask, nch)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);
                if(nch->nch_Task)
                {
                    nch->nch_ReadySigTask = NULL;
                    //FreeSignal(nch->nch_ReadySignal);
                    psdGetAttrs(PGA_INTERFACE, pif, IFA_Config, &pc, TAG_END);
                    psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
                    psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "I've got my fingers on '%s'!",
                                   devname);

                    CloseLibrary(ps);
                    return(nch);
                }
            }
            nch->nch_ReadySigTask = NULL;
            //FreeSignal(nch->nch_ReadySignal);
            psdFreeVec(nch);
        }
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "bootkbd_ReleaseInterfaceBinding()" */
void bootkbd_ReleaseInterfaceBinding(struct NepHidBase *nh, struct NepClassHid *nch)
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
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "I lost my keys to '%s'!",
                       devname);
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
         LIBBASETYPEPTR, nh, 5, bootkbd)
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
                 *((IPTR *) ti->ti_Data) = -100;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Support for keyboards in boot protocol mode";
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
                 *((IPTR *) ti->ti_Data) = TRUE;
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
         LIBBASETYPEPTR, nh, 6, bootkbd)
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
         LIBBASETYPEPTR, nh, 7, bootkbd)
{
    AROS_LIBFUNC_INIT

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) bootkbd_AttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) bootkbd_ForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            bootkbd_ReleaseInterfaceBinding(nh, (struct NepClassHid *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenCfgWindow(nh));

        case UCM_ConfigChangedEvent:
            bootkbd_LoadClassConfig(nh);
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "bootkbd_LoadClassConfig()" */
BOOL bootkbd_LoadClassConfig(struct NepHidBase *nh)
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
    nh->nh_CurrentCGC.cgc_ChunkID = AROS_LONG2BE(MAKE_ID('B','K','E','Y'));
    nh->nh_CurrentCGC.cgc_Length = AROS_LONG2BE(sizeof(struct ClsGlobalCfg)-8);
    nh->nh_CurrentCGC.cgc_RHEnable = TRUE;
    nh->nh_CurrentCGC.cgc_ResetDelay = 10;
    nh->nh_CurrentCGC.cgc_CapsLock = FALSE;
    nh->nh_CurrentCGC.cgc_ISAMap = FALSE;
    nh->nh_CurrentCGC.cgc_ExtraEmulDisable = FALSE;
    nh->nh_UsingDefaultCfg = TRUE;
    pic = psdGetClsCfg(libname);
    if(pic)
    {
        if((cgc = psdGetCfgChunk(pic, nh->nh_CurrentCGC.cgc_ChunkID)))
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
LONG nOpenCfgWindow(struct NepHidBase *nh)
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
        if((nh->nh_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", bootkbd_GUITask, nh)))
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
static const UBYTE usbkeymap[] =
{
    0xff, 0xff, 0xff, 0xff, 0x20, 0x35, 0x33, 0x22,  /* 0x00 */
    0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28,  /* 0x08 */
    0x37, 0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14,  /* 0x10 */
    0x16, 0x34, 0x11, 0x32, 0x15, 0x31, 0x01, 0x02,  /* 0x18 */
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,  /* 0x20 */
    0x44, 0x45, 0x41, 0x42, 0x40, 0x0b, 0x0c, 0x1a,  /* 0x28 */
    0x1b, 0x0d, 0x2b, 0x29, 0x2a, 0x00, 0x38, 0x39,  /* 0x30 */
    0x3a, 0x62, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,  /* 0x38 */
    0x56, 0x57, 0x58, 0x59, 0xff, 0x5f, 0xff, 0xff,  /* 0x40 */
    0xff, 0xff, 0xff, 0xff, 0x46, 0xff, 0xff, 0x4e,  /* 0x48 */
    0x4f, 0x4d, 0x4c, 0xff, 0x5c, 0x5d, 0x4a, 0x5e,  /* 0x50 */
    0x43, 0x1d, 0x1e, 0x1f, 0x2d, 0x2e, 0x2f, 0x3d,  /* 0x58 */
    0x3e, 0x3f, 0x0f, 0x3c, 0x30, 0xff, 0xff, 0xff,  /* 0x60 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x68 F13-F24 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x70 */
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

static const UBYTE usbisakeymap[] =
{
    0xff, 0xff, 0xff, 0xff, 0x20, 0x35, 0x33, 0x22,  /* 0x00 */
    0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28,  /* 0x08 */
    0x37, 0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14,  /* 0x10 */
    0x16, 0x34, 0x11, 0x32, 0x15, 0x31, 0x01, 0x02,  /* 0x18 */
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,  /* 0x20 */
    0x44, 0x45, 0x41, 0x42, 0x40, 0x0b, 0x0c, 0x1a,  /* 0x28 */
    0x1b, 0x0d, 0x2b, 0x29, 0x2a, 0x00, 0x38, 0x39,  /* 0x30 */
    0x3a, 0x62, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,  /* 0x38 */
    0x56, 0x57, 0x58, 0x59, 0x4b, 0x6f, 0xff, 0xff,  /* 0x40 */
    0x6e, 0x47, 0x70, 0x48, 0x46, 0x71, 0x49, 0x4e,  /* 0x48 */
    0x4f, 0x4d, 0x4c, 0xff, 0x5c, 0x5d, 0x4a, 0x5e,  /* 0x50 */
    0x43, 0x1d, 0x1e, 0x1f, 0x2d, 0x2e, 0x2f, 0x3d,  /* 0x58 */
    0x3e, 0x3f, 0x0f, 0x3c, 0x30, 0xff, 0xff, 0xff,  /* 0x60 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x68 F13-F24 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* 0x70 */
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

#undef ps
#define ps nch->nch_Base

/* /// "bootkbd_HidTask()" */
AROS_UFH0(void, bootkbd_HidTask)
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    UBYTE *buf;
    LONG ioerr;

    if((nch = bootkbd_AllocHid()))
    {
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        buf = nch->nch_EP1Buf;
        psdSendPipe(nch->nch_EP1Pipe, buf, nch->nch_EP1PktSize);
        do
        {
            sigs = Wait(sigmask);
            while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort)))
            {
                if(pp == nch->nch_EP1Pipe)
                {
                    if(!(ioerr = psdGetPipeError(pp)))
                    {
                        nParseKeys(nch, buf);
                    } else {
                        KPRINTF(1, ("Int Pipe failed %ld\n", ioerr));
                        psdDelayMS(200);
                    }
                    psdSendPipe(nch->nch_EP1Pipe, buf, nch->nch_EP1PktSize);
                    break;
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        KPRINTF(20, ("Going down the river!\n"));
        memset(buf, 0, 8);
        nParseKeys(nch, buf);

        psdAbortPipe(nch->nch_EP1Pipe);
        psdWaitPipe(nch->nch_EP1Pipe);
        bootkbd_FreeHid(nch);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nParseKeys()" */
void nParseKeys(struct NepClassHid *nch, UBYTE *buf)
{
    UWORD iecode;
    UWORD qualcode = 0;
    ULONG oldqual;
    ULONG qualifier = 0;
    UWORD keyqual = buf[0];
    UWORD nkey, nkey2;
    UWORD keycode;
    BOOL keydown;
    BOOL keyup;
    BOOL bonuskey;
    BOOL sentkey = FALSE;

    if(keyqual & 0x11) qualifier |= IEQUALIFIER_CONTROL;
    if(keyqual & 0x02) qualifier |= IEQUALIFIER_LSHIFT;
    if(keyqual & 0x04) qualifier |= IEQUALIFIER_LALT;
    if(keyqual & 0x08) qualifier |= IEQUALIFIER_LCOMMAND;
    if(keyqual & 0x20) qualifier |= IEQUALIFIER_RSHIFT;
    if(keyqual & 0x40) qualifier |= IEQUALIFIER_RALT;
    if(keyqual & 0x80) qualifier |= IEQUALIFIER_RCOMMAND;
    if(!nch->nch_ClsBase->nh_CurrentCGC.cgc_CapsLock)
    {
        if(qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))
        {
            nch->nch_CapsLock = FALSE;
        }
    }
    if(nch->nch_CapsLock) qualifier |= IEQUALIFIER_CAPSLOCK;
    if((qualifier & (IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)) ==
                    (IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND))
    {
        struct MsgPort *mp;
        struct IOStdReq *ioreq;
        struct Interrupt tempint;
        struct Node *node;
        struct List *listhead = NULL;
        KPRINTF(20, ("Reboot!\n"));

        if(nch->nch_ClsBase->nh_CurrentCGC.cgc_RHEnable)
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
                        tempint.is_Node.ln_Pri = 32;
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
                                node = node->ln_Succ;
                            }
                            KPRINTF(20, ("Done... awaiting doom\n"));
                            psdDelayMS(nch->nch_ClsBase->nh_CurrentCGC.cgc_ResetDelay*1000);
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
    if(qualifier != nch->nch_OldQualifier)
    {
        switch(qualifier^nch->nch_OldQualifier)
        {
            case IEQUALIFIER_LSHIFT:
                qualcode = 0x60;
                break;
            case IEQUALIFIER_RSHIFT:
                qualcode = 0x61;
                break;
            case IEQUALIFIER_CAPSLOCK:
                qualcode = 0x62;
                break;
            case IEQUALIFIER_CONTROL:
                qualcode = 0x63;
                break;
            case IEQUALIFIER_LALT:
                qualcode = 0x64;
                break;
            case IEQUALIFIER_RALT:
                qualcode = 0x65;
                break;
            case IEQUALIFIER_LCOMMAND:
                qualcode = 0x66;
                break;
            case IEQUALIFIER_RCOMMAND:
                qualcode = 0x67;
                break;
            default:
                qualcode = 0xFF;
                break;
        }
    }

    KPRINTF(1, ("Qualifier %08lx\n", qualifier));
    for(nkey = 2; nkey < 8; nkey++)
    {
        keycode = nch->nch_OldKeyArray[nkey];
        if(!keycode)
        {
            continue;
        }
        /* Check, if key is still there */
        keyup = TRUE;
        for(nkey2 = 2; nkey2 < 8; nkey2++)
        {
            if(buf[nkey2] == keycode)
            {
                keyup = FALSE;
                break;
            }
        }
        if(keyup)
        {
            KPRINTF(1, ("Key up: %08lx\n", keycode));
            iecode = nch->nch_ClsBase->nh_CurrentCGC.cgc_ISAMap ? usbisakeymap[keycode] : usbkeymap[keycode];
            oldqual = qualifier;
            if(!nch->nch_ClsBase->nh_CurrentCGC.cgc_ExtraEmulDisable)
            {
                bonuskey = TRUE;
                switch(keycode)
                {
                    case 0x49: /* Insert */
                        iecode = usbkeymap[0x19];
                        qualifier |= IEQUALIFIER_RCOMMAND;
                        qualcode = 0x67;
                        break;
                    case 0x4A: /* Pos 1 */
                        iecode = usbkeymap[0x50];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        qualcode = 0x60;
                        break;
                    case 0x4D: /* End */
                        iecode = usbkeymap[0x4F];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        qualcode = 0x60;
                        break;
                    case 0x4B: /* Page Up */
                        iecode = usbkeymap[0x52];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        qualcode = 0x60;
                        break;
                    case 0x4E: /* Page Down */
                        iecode = usbkeymap[0x51];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        qualcode = 0x60;
                        break;
                    case 0x48: /* Pause / Break */
                        /* *** FIXME *** should be a vanilla key for keymapping compatibility */
                        iecode = usbkeymap[0x06];
                        qualifier |= IEQUALIFIER_CONTROL;
                        qualcode = 0x63;
                        break;
                    default:
                        bonuskey = FALSE;
                }
            } else {
                bonuskey = FALSE;
            }
            nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
            nch->nch_FakeEvent.ie_SubClass = 0;
            nch->nch_FakeEvent.ie_Code = iecode|IECODE_UP_PREFIX;
            nch->nch_FakeEvent.ie_NextEvent = NULL;
            nch->nch_FakeEvent.ie_Qualifier = qualifier;
            nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
            nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
            nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
            DoIO((struct IORequest *) nch->nch_InpIOReq);
            if(bonuskey)
            {
                qualifier = oldqual;
                nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
                nch->nch_FakeEvent.ie_SubClass = 0;
                nch->nch_FakeEvent.ie_Code = qualcode|IECODE_UP_PREFIX;
                nch->nch_FakeEvent.ie_NextEvent = NULL;
                nch->nch_FakeEvent.ie_Qualifier = qualifier;
                nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                DoIO((struct IORequest *) nch->nch_InpIOReq);
            }
            sentkey = TRUE;
        }

    }

    for(nkey = 2; nkey < 8; nkey++)
    {
        keycode = buf[nkey];
        if(!keycode)
        {
            continue;
        }
        KPRINTF(1, ("Key down: %08lx\n", keycode));
        /* Check, if key was there previously */
        keydown = TRUE;
        for(nkey2 = 2; nkey2 < 8; nkey2++)
        {
            if(nch->nch_OldKeyArray[nkey2] == keycode)
            {
                keydown = FALSE;
                break;
            }
        }
        if(keydown)
        {
            iecode = nch->nch_ClsBase->nh_CurrentCGC.cgc_ISAMap ? usbisakeymap[keycode] : usbkeymap[keycode];
            if(!nch->nch_ClsBase->nh_CurrentCGC.cgc_ExtraEmulDisable)
            {
                switch(keycode)
                {
                    case 0x49: /* Insert */
                        iecode = usbkeymap[0x19];
                        qualifier |= IEQUALIFIER_RCOMMAND;
                        break;
                    case 0x4A: /* Pos 1 */
                        iecode = usbkeymap[0x50];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        break;
                    case 0x4D: /* End */
                        iecode = usbkeymap[0x4F];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        break;
                    case 0x4B: /* Page Up */
                        iecode = usbkeymap[0x52];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        break;
                    case 0x4E: /* Page Down */
                        iecode = usbkeymap[0x51];
                        qualifier |= IEQUALIFIER_LSHIFT;
                        break;
                    case 0x48: /* Pause / Break */
                        /* *** FIXME *** should be a vanilla key for keymapping compatibility */
                        iecode = usbkeymap[0x06];
                        qualifier |= IEQUALIFIER_CONTROL;
                        break;
                }
            }
            if(keycode == 0x39) /* Caps Lock */
            {
                if(nch->nch_ClsBase->nh_CurrentCGC.cgc_CapsLock)
                {
                    nch->nch_CapsLock = !nch->nch_CapsLock;
                } else {
                    nch->nch_CapsLock = TRUE;
                }
                if(nch->nch_CapsLock)
                {
                    qualifier |= IEQUALIFIER_CAPSLOCK;
                }
            }

            nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
            nch->nch_FakeEvent.ie_SubClass = 0;
            nch->nch_FakeEvent.ie_Code = iecode;
            nch->nch_FakeEvent.ie_NextEvent = NULL;
            nch->nch_FakeEvent.ie_Qualifier = qualifier;
            nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
            nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
            nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
            DoIO((struct IORequest *) nch->nch_InpIOReq);
            nch->nch_FakeEvent.ie_position.ie_dead.ie_prev2DownCode = nch->nch_FakeEvent.ie_position.ie_dead.ie_prev1DownCode;
            nch->nch_FakeEvent.ie_position.ie_dead.ie_prev2DownQual = nch->nch_FakeEvent.ie_position.ie_dead.ie_prev1DownQual;
            nch->nch_FakeEvent.ie_position.ie_dead.ie_prev1DownCode = iecode;
            nch->nch_FakeEvent.ie_position.ie_dead.ie_prev1DownQual = qualifier;
            sentkey = TRUE;
            break;
        }
    }
    /* copy old keymap */
    for(nkey = 2; nkey < nch->nch_EP1PktSize; nkey++)
    {
        nch->nch_OldKeyArray[nkey] = buf[nkey];
    }
    if((!sentkey) && (nch->nch_OldQualifier != qualifier))
    {
        nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
        nch->nch_FakeEvent.ie_SubClass = 0;
        nch->nch_FakeEvent.ie_Code = qualcode;
        nch->nch_FakeEvent.ie_NextEvent = NULL;
        nch->nch_FakeEvent.ie_Qualifier = qualifier;
        nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
        nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
        nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
        DoIO((struct IORequest *) nch->nch_InpIOReq);
    }
    nch->nch_OldQualifier = qualifier;

    /* Reboot machine upon Ctrl-Alt-Del */
    if((qualifier & IEQUALIFIER_CONTROL) &&
        (qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)) &&
        nch->nch_FakeEvent.ie_Code == RAWKEY_DELETE)
    {
        KPRINTF(20, ("Reboot!\n"));
        ShutdownA(SD_ACTION_COLDREBOOT);
    }
}
/* \\\ */

/* /// "bootkbd_AllocHid()" */
struct NepClassHid * bootkbd_AllocHid(void)
{
    struct Task *thistask;
    struct NepClassHid *nch;
    LONG ioerr;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;
    do
    {
        if(!(nch->nch_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                    IFA_Config, &nch->nch_Config,
                    IFA_InterfaceNum, &nch->nch_IfNum,
                    TAG_END);
        psdGetAttrs(PGA_CONFIG, nch->nch_Config,
                    CA_Device, &nch->nch_Device,
                    TAG_END);

        nch->nch_EP1 = psdFindEndpoint(nch->nch_Interface, NULL,
                                       EA_IsIn, TRUE,
                                       EA_TransferType, USEAF_INTERRUPT,
                                       TAG_END);
        if(!nch->nch_EP1)
        {
            KPRINTF(1, ("Ooops!?! No Endpoints defined?\n"));
            break;
        }
        psdGetAttrs(PGA_ENDPOINT, nch->nch_EP1,
                    EA_MaxPktSize, &nch->nch_EP1PktSize,
                    TAG_END);
        if((nch->nch_InpMsgPort = CreateMsgPort()))
        {
            if((nch->nch_InpIOReq = (struct IOStdReq *) CreateIORequest(nch->nch_InpMsgPort, sizeof(struct IOStdReq))))
            {
                if(!OpenDevice("input.device", 0, (struct IORequest *) nch->nch_InpIOReq, 0))
                {
                    nch->nch_InputBase = (struct Library *) nch->nch_InpIOReq->io_Device;
                    if((nch->nch_TaskMsgPort = CreateMsgPort()))
                    {
                        if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
                        {
                            if((nch->nch_EP1Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EP1)))
                            {
                                psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                                             UHR_SET_PROTOCOL, HID_PROTO_BOOT, nch->nch_IfNum);
                                ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                if(!ioerr)
                                {
                                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                                                 UHR_SET_IDLE, 0, nch->nch_IfNum);
                                    ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                    if(ioerr)
                                    {
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                       "SET_IDLE=0 failed: %s (%ld)!",
                                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    }
                                    if((nch->nch_EP1Buf = psdAllocVec(nch->nch_EP1PktSize)))
                                    {
                                        nch->nch_Task = thistask;
                                        return(nch);
                                    }
                                } else {
                                    psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                                                   "SET_PROTOCOL=BOOT failed: %s (%ld)!",
                                                   (APTR) psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                }
                                psdFreePipe(nch->nch_EP1Pipe);
                            }
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

/* /// "bootkbd_FreeHid()" */
void bootkbd_FreeHid(struct NepClassHid *nch)
{
    psdFreeVec(nch->nch_EP1Buf);
    psdFreePipe(nch->nch_EP1Pipe);
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    CloseDevice((struct IORequest *) nch->nch_InpIOReq);
    DeleteIORequest((struct IORequest *) nch->nch_InpIOReq);
    DeleteMsgPort(nch->nch_InpMsgPort);
    CloseLibrary(nch->nch_Base);
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
}
/* \\\ */

/**************************************************************************/

#undef ps
#define ps nh->nh_PsdBase
#undef IntuitionBase
#define IntuitionBase nh->nh_IntBase
#undef MUIMasterBase
#define MUIMasterBase nh->nh_MUIBase

/* /// "bootkbd_GUITask()" */
AROS_UFH0(void, bootkbd_GUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepHidBase *nh;
    APTR pic;

    thistask = FindTask(NULL);

    nh = thistask->tc_UserData;
    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        bootkbd_GUITaskCleanup(nh);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        bootkbd_GUITaskCleanup(nh);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        bootkbd_GUITaskCleanup(nh);
        return;
    }

    nh->nh_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)libname,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)"©2002-2009 Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the bootkeyboard.class",
        MUIA_Application_Base       , (IPTR)"BOOTKEYBOARD",
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
                Child, (IPTR)ColGroup(2), GroupFrameT((IPTR)"Global Settings"),
                    Child, (IPTR)Label((IPTR) "Hijack ResetHandlers:"),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(nh->nh_RHEnableObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nh->nh_CurrentCGC.cgc_RHEnable,
                            MUIA_ShowSelState, FALSE,
                            End),
                        Child, (IPTR)HSpace(0),
                        End,
                    Child, (IPTR)Label((IPTR) "Reset delay:"),
                    Child, (IPTR)(nh->nh_ResetDelayObj = SliderObject, SliderFrame,
                        MUIA_CycleChain, 1,
                        MUIA_Numeric_Min, 0,
                        MUIA_Numeric_Max, 60,
                        MUIA_Numeric_Value, nh->nh_CurrentCGC.cgc_ResetDelay,
                        MUIA_Numeric_Format, (IPTR)"%ldsec",
                        End),
                    Child, (IPTR)Label((IPTR) "Amiga CapsLock behaviour:"),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(nh->nh_CapsLockObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nh->nh_CurrentCGC.cgc_CapsLock,
                            MUIA_ShowSelState, FALSE,
                            End),
                        Child, (IPTR)HSpace(0),
                        End,
                    Child, (IPTR)Label((IPTR) "Use standard ISA mapping:"),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(nh->nh_ISAMapObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nh->nh_CurrentCGC.cgc_ISAMap,
                            MUIA_ShowSelState, FALSE,
                            End),
                        Child, (IPTR)HSpace(0),
                        End,
                    Child, (IPTR)Label((IPTR) "Disable extra key emulation:"),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(nh->nh_ExtraEmulObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nh->nh_CurrentCGC.cgc_ExtraEmulDisable,
                            MUIA_ShowSelState, FALSE,
                            End),
                        Child, (IPTR)HSpace(0),
                        End,
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
        bootkbd_GUITaskCleanup(nh);
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
    DoMethod(nh->nh_RestoreDefMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_RESTORE_DEF);
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
            bootkbd_GUITaskCleanup(nh);
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
                    get(nh->nh_RHEnableObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_RHEnable);
                    get(nh->nh_ResetDelayObj, MUIA_Numeric_Value, &nh->nh_CurrentCGC.cgc_ResetDelay);
                    get(nh->nh_CapsLockObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_CapsLock);
                    get(nh->nh_ISAMapObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_ISAMap);
                    get(nh->nh_ExtraEmulObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_ExtraEmulDisable);
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
                    MUI_RequestA(nh->nh_App, nh->nh_MainWindow, 0, NULL, "Amazing!", VERSION_STRING, NULL);
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
    bootkbd_GUITaskCleanup(nh);

    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "bootkbd_GUITaskCleanup()" */
void bootkbd_GUITaskCleanup(struct NepHidBase *nh)
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

