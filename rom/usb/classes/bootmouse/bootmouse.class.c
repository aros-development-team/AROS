/*
 *----------------------------------------------------------------------------
 *                         bootmouse class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "bootmouse.class.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        struct NepClassHid *nch;
        NewList(&nh->nh_Bindings);
        nch = &nh->nh_DummyNCH;
        nch->nch_ClsBase = nh;
        nch->nch_Interface = NULL;
        nch->nch_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
        if(!nch->nch_CDC)
        {
            return FALSE;
        }
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        return FALSE;
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return TRUE;
}

static int libOpen(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    nLoadClassConfig(nh);

    return TRUE;
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    CloseLibrary(UtilityBase);
    nh->nh_UtilityBase = NULL;
    FreeVec(nh->nh_DummyNCH.nch_CDC);

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

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassHid * usbAttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    ULONG ifclass;
    ULONG subclass;
    ULONG proto;

    KPRINTF(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, (ULONG) &ifclass,
                    IFA_SubClass, (ULONG) &subclass,
                    IFA_Protocol, (ULONG) &proto,
                    TAG_DONE);
        CloseLibrary(ps);

        if((ifclass == HID_CLASSCODE) && (subclass == HID_BOOT_SUBCLASS) && (proto == HID_PROTO_MOUSE))
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassHid * usbForceInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif)
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

            nLoadBindingConfig(nch);

            psdSafeRawDoFmt(buf, 64, "bootmouse.class<%08lx>", nch);
            nch->nch_ReadySignal = SIGB_SINGLE;
            nch->nch_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            if((tmptask = psdSpawnSubTask(buf, nHidTask, nch)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);
                if(nch->nch_Task)
                {
                    nch->nch_ReadySigTask = NULL;
                    //FreeSignal(nch->nch_ReadySignal);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "I'm pleased to introduce a mouse alliance to '%s'!",
                                   devname);

                    Forbid();
                    AddTail(&nh->nh_Bindings, &nch->nch_Node);
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
void usbReleaseInterfaceBinding(struct NepHidBase *nh, struct NepClassHid *nch)
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
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface, IFA_Config, (ULONG) &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, (ULONG) &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, (ULONG) &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "A cat ate my mouse '%s'!",
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
                 *((IPTR *) ti->ti_Data) = -100;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Support for mice/tablets in boot protocol mode";
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
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nh->nh_DummyNCH.nch_UsingDefaultCfg;
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

    struct NepClassHid *nch;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassHid *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenBindingCfgWindow(nh, &nh->nh_DummyNCH));

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassHid *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            Forbid();
            nch = (struct NepClassHid *) nh->nh_Bindings.lh_Head;
            while(nch->nch_Node.ln_Succ)
            {
                nLoadBindingConfig(nch);
                nch = (struct NepClassHid *) nch->nch_Node.ln_Succ;
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
BOOL nLoadClassConfig(struct NepHidBase *nh)
{
    struct NepClassHid *nch = &nh->nh_DummyNCH;
    struct Library *ps;
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
    nch->nch_CDC->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('B','M','S','E'));
    nch->nch_CDC->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    nch->nch_CDC->cdc_Wheelmouse = FALSE;
    nch->nch_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = psdGetClsCfg(libname);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, nch->nch_CDC->cdc_ChunkID);
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nch->nch_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nch->nch_CDC->cdc_Length)));
            psdFreeVec(cdc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nLoadBindingConfig()" */
BOOL nLoadBindingConfig(struct NepClassHid *nch)
{
    struct NepHidBase *nh = nch->nch_ClsBase;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Binding Config...\n"));
    if(nch->nch_GUITask)
    {
        return(FALSE);
    }
    //nLoadClassConfig(nh);
    *nch->nch_CDC = *nh->nh_DummyNCH.nch_CDC;
    nch->nch_UsingDefaultCfg = TRUE;

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, nch->nch_CDC->cdc_ChunkID);
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nch->nch_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nch->nch_CDC->cdc_Length)));
            psdFreeVec(cdc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nOpenBindingCfgWindow()" */
LONG nOpenBindingCfgWindow(struct NepHidBase *nh, struct NepClassHid *nch)
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
        if((nch->nch_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", nGUITask, nch)))
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

#undef ps
#define ps nch->nch_Base

/* /// "nHidTask()" */
AROS_UFH0(void, nHidTask)
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    UWORD iecode;
    UWORD qualifier;
    UWORD buts;
    UWORD oldbuts = 0;
    WORD wheel = 0;
    WORD oldwheel = 0;
    UWORD wheeliecode;
    UWORD wheeldist;
    BOOL newmouse;
    UBYTE *buf;
    UBYTE *bufreal;
    LONG ioerr;
    BOOL firstpkt = TRUE;

    if((nch = nAllocHid()))
    {
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        bufreal = buf = nch->nch_EP1Buf;
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
                        if(firstpkt)
                        {
                            if(*buf == 0x01)
                            {
                                bufreal++;
                            }
                            firstpkt = 0;
                        }
                        KPRINTF(1, ("Data: %08lx %08lx\n", (*(ULONG *) bufreal), ((ULONG *) bufreal)[1]));
                        newmouse = FALSE;
                        qualifier = IEQUALIFIER_RELATIVEMOUSE;
                        buts = bufreal[0];
                        iecode = wheeliecode = IECODE_NOBUTTON;
                        wheeldist = 0;
                        if(buts & 1)
                        {
                            qualifier |= IEQUALIFIER_LEFTBUTTON;
                        }
                        if(buts & 2)
                        {
                            qualifier |= IEQUALIFIER_RBUTTON;
                        }
                        if(buts & 4)
                        {
                            qualifier |= IEQUALIFIER_MIDBUTTON;
                        }
                        if(nch->nch_CDC->cdc_Wheelmouse)
                        {
                            wheel = ((BYTE *) bufreal)[3];
                            if(wheel != oldwheel)
                            {
                                if(oldwheel > 0)
                                {
                                    wheeliecode = RAWKEY_NM_WHEEL_UP|IECODE_UP_PREFIX;
                                    newmouse = TRUE;
                                }
                                else if(oldwheel < 0)
                                {
                                    wheeliecode = RAWKEY_NM_WHEEL_DOWN|IECODE_UP_PREFIX;
                                    newmouse = TRUE;
                                }
                                oldwheel = wheel;
                            }
                            if(wheel > 0)
                            {
                                wheeliecode = RAWKEY_NM_WHEEL_UP;
                                wheeldist = wheel;
                                newmouse = TRUE;
                            }
                            else if(wheel < 0)
                            {
                                wheeliecode = RAWKEY_NM_WHEEL_DOWN;
                                wheeldist = -wheel;
                                newmouse = TRUE;
                            }
                        }

                        if((buts^oldbuts) & 1)
                        {
                            iecode = (buts & 1) ? IECODE_LBUTTON : IECODE_LBUTTON|IECODE_UP_PREFIX;
                            oldbuts ^= 1;
                        }
                        else if((buts^oldbuts) & 2)
                        {
                            iecode = (buts & 2) ? IECODE_RBUTTON : IECODE_RBUTTON|IECODE_UP_PREFIX;
                            oldbuts ^= 2;
                        }
                        else if((buts^oldbuts) & 4)
                        {
                            iecode = (buts & 4) ? IECODE_MBUTTON : IECODE_MBUTTON|IECODE_UP_PREFIX;
                            oldbuts ^= 4;
                        }
                        nch->nch_FakeEvent.ie_X = ((BYTE *) bufreal)[1];
                        nch->nch_FakeEvent.ie_Y = ((BYTE *) bufreal)[2];
                        nch->nch_FakeEvent.ie_Class = IECLASS_RAWMOUSE;
                        nch->nch_FakeEvent.ie_SubClass = 0;
                        nch->nch_FakeEvent.ie_Code = iecode;
                        nch->nch_FakeEvent.ie_NextEvent = NULL;
                        nch->nch_FakeEvent.ie_Qualifier = qualifier;
                        nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                        nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                        nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                        DoIO((struct IORequest *) nch->nch_InpIOReq);
                        if(newmouse)
                        {
                            while(wheeldist--)
                            {
                                KPRINTF(1, ("Doing wheel %ld\n", wheel));
                                nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
                                nch->nch_FakeEvent.ie_SubClass = 0;
                                nch->nch_FakeEvent.ie_Code = wheeliecode;
                                nch->nch_FakeEvent.ie_NextEvent = NULL;
                                nch->nch_FakeEvent.ie_Qualifier = qualifier;
                                nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                                nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                                nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                                DoIO((struct IORequest *) nch->nch_InpIOReq);

                                nch->nch_FakeEvent.ie_Class = IECLASS_NEWMOUSE;
                                nch->nch_FakeEvent.ie_SubClass = 0;
                                nch->nch_FakeEvent.ie_Code = wheeliecode;
                                nch->nch_FakeEvent.ie_NextEvent = NULL;
                                nch->nch_FakeEvent.ie_Qualifier = qualifier;
                                nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                                nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                                nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                                DoIO((struct IORequest *) nch->nch_InpIOReq);
                                }
                        }
                    } else {
                        KPRINTF(1, ("Int Pipe failed %ld\n", ioerr));
                        psdDelayMS(20);
                    }
                    psdSendPipe(nch->nch_EP1Pipe, buf, nch->nch_EP1PktSize);
                    break;
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        KPRINTF(20, ("Going down the river!\n"));
        psdAbortPipe(nch->nch_EP1Pipe);
        psdWaitPipe(nch->nch_EP1Pipe);
        nFreeHid(nch);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocHid()" */
struct NepClassHid * nAllocHid(void)
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
                    EA_MaxPktSize, (ULONG) &nch->nch_EP1PktSize,
                    TAG_END);
        if((nch->nch_InpMsgPort = CreateMsgPort()))
        {
            if((nch->nch_InpIOReq = (struct IOStdReq *) CreateIORequest(nch->nch_InpMsgPort, sizeof(struct IOStdReq))))
            {
                if(!OpenDevice("input.device", 0, (struct IORequest *) nch->nch_InpIOReq, 0))
                {
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
                                        psdSetAttrs(PGA_PIPE, nch->nch_EP1Pipe,
                                                    PPA_AllowRuntPackets, TRUE,
                                                    TAG_END);
                                        nch->nch_Task = thistask;
                                        return(nch);
                                    }
                                } else {
                                    psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                                                   "SET_PROTOCOL=BOOT failed: %s (%ld)!",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
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

/* /// "nFreeHid()" */
void nFreeHid(struct NepClassHid *nch)
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
#define ps nch->nch_PsdBase
#undef IntuitionBase
#define IntuitionBase nch->nch_IntBase
#undef MUIMasterBase
#define MUIMasterBase nch->nch_MUIBase

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepHidBase *nh;
    struct NepClassHid *nch;
    APTR pic;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;
    nh = nch->nch_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        nGUITaskCleanup(nch);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        nGUITaskCleanup(nch);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        nGUITaskCleanup(nch);
        return;
    }

    nch->nch_App = ApplicationObject,
        MUIA_Application_Title      , libname,
        MUIA_Application_Version    , VERSION_STRING,
        MUIA_Application_Copyright  , "©2002-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the bootmouse.class",
        MUIA_Application_Base       , "BOOTMOUSE",
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, nch->nch_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                End,
            Child, MenuObjectT("Settings"),
                Child, nch->nch_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save",
                    MUIA_Menuitem_Shortcut, "S",
                    End,
                Child, nch->nch_SetDefaultMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save as Default",
                    MUIA_Menuitem_Shortcut, "D",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, nch->nch_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, "MUI Settings",
                    MUIA_Menuitem_Shortcut, "M",
                    End,
                End,
            End,

        SubWindow, nch->nch_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, libname,
            MUIA_HelpNode, libname,

            WindowContents, VGroup,
                Child, HGroup, GroupFrameT(nch->nch_Interface ? "Device Settings" : "Default Device Settings"),
                    Child, HSpace(0),
                    Child, ColGroup(2),
                        Child, Label((IPTR) "Experimental Wheelmouse support:"),
                        Child, HGroup,
                            Child, nch->nch_WheelmouseObj = ImageObject, ImageButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_Toggle,
                                MUIA_Image_Spec, MUII_CheckMark,
                                MUIA_Image_FreeVert, TRUE,
                                MUIA_Selected, nch->nch_CDC->cdc_Wheelmouse,
                                MUIA_ShowSelState, FALSE,
                                End,
                            Child, HSpace(0),
                            End,
                        End,
                    Child, HSpace(0),
                    End,
                Child, VSpace(0),
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, nch->nch_UseObj = TextObject, ButtonFrame,
                        MUIA_ShowMe, nch->nch_Interface,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Save ",
                        End,
                    Child, nch->nch_SetDefaultObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, nch->nch_Interface ? "\33c Save as Default " : "\33c Save Defaults ",
                        End,
                    Child, nch->nch_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Use ",
                        End,
                    End,
                End,
            End,
        End;

    if(!nch->nch_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        nGUITaskCleanup(nch);
        return;
    }

    DoMethod(nch->nch_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(nch->nch_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nch->nch_SetDefaultObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(nch->nch_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(nch->nch_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(nch->nch_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nch->nch_SetDefaultMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(nch->nch_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_OpenConfigWindow, 0);
    {
        IPTR isopen;
        IPTR iconify;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(nch->nch_App, MUIA_Application_Iconified, &iconify);
        set(nch->nch_MainWindow, MUIA_Window_Open, TRUE);
        get(nch->nch_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            nGUITaskCleanup(nch);
            return;
        }
        sigmask = 0;
        do
        {
            retid = DoMethod(nch->nch_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_DEF_CONFIG:
                case ID_STORE_CONFIG:
                case MUIV_Application_ReturnID_Quit:
                    get(nch->nch_WheelmouseObj, MUIA_Selected, &nch->nch_CDC->cdc_Wheelmouse);

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
                            if(psdAddCfgEntry(pic, nch->nch_CDC))
                            {
                                psdSaveCfgToDisk(NULL, FALSE);
                            }
                        }
                    }
                    if(nch->nch_Interface)
                    {
                        pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString);
                        if(!pic)
                        {
                            psdSetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString, NULL);
                            pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString);
                        }
                        if(pic)
                        {
                            if(psdAddCfgEntry(pic, nch->nch_CDC))
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
                    MUI_RequestA(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Blimey!", VERSION_STRING, NULL);
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
        set(nch->nch_MainWindow, MUIA_Window_Open, FALSE);
    }
    nGUITaskCleanup(nch);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void nGUITaskCleanup(struct NepClassHid *nch)
{
    if(nch->nch_App)
    {
        MUI_DisposeObject(nch->nch_App);
        nch->nch_App = NULL;
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
    nch->nch_GUIBinding = NULL;
    nch->nch_GUITask = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    --nch->nch_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */

