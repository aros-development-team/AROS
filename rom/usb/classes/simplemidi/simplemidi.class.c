/*
 *----------------------------------------------------------------------------
 *                         simplemidi class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "simplemidi.class.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        /* Create default config */
        nh->nh_CurrentCGC.cgc_ChunkID = AROS_LONG2BE(MAKE_ID('M','I','D','I'));
        nh->nh_CurrentCGC.cgc_Length = AROS_LONG2BE(sizeof(struct ClsGlobalCfg)-8);
        nh->nh_CurrentCGC.cgc_MidiMinOctave = 2;
        nh->nh_CurrentCGC.cgc_KeyMaxOctave = 5;
        nh->nh_CurrentCGC.cgc_AutoKeyUp = TRUE;
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        nh = NULL;
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return(nh ? TRUE : FALSE);
}

static int libOpen(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    nLoadClassConfig(nh);
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));
    CloseLibrary((struct Library *) UtilityBase);
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

#define ps ps

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassHid * usbAttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif)
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
        if((ifclass == AUDIO_CLASSCODE) && (subclass == AUDIO_MIDI_SUBCLASS))
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

            nLoadClassConfig(nh);

            psdSafeRawDoFmt(buf, 64, "simplemidi.class<%08lx>", nch);
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
                    psdGetAttrs(PGA_INTERFACE, pif, IFA_Config, &pc, TAG_END);
                    psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
                    psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "Play it again, '%s'!",
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
                       "'%s' fell silent!",
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
                 *((SIPTR *) ti->ti_Data) = -100;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Simple MIDI-IN Stream class mapping to keyboard";
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
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
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
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassHid *) methoddata[0]);
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

/**************************************************************************/

static const UBYTE lowoctave[] =
{
    0x31, 0x21, 0x32, 0x22, 0x33, 0x34, 0x24, 0x35, 0x25, 0x36, 0x26, 0x37, /* C-1 - B-1 */
    0x38, 0x28, 0x39, 0x29, 0x3a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  /* C-2 - E-2 */
};

static const UBYTE highoctave[] =
{
    0x10, 0x02, 0x11, 0x03, 0x12, 0x13, 0x05, 0x14, 0x06, 0x15, 0x07, 0x16, /* C-2 - B-2 */
    0x17, 0x09, 0x18, 0x0a, 0x19, 0x1a, 0x0b, 0x1b, 0xff, 0xff, 0xff, 0xff  /* C-3 - F-3 */
};

#undef  ps
#define ps nch->nch_Base

/* /// "nHidTask()" */
AROS_UFH0(void, nHidTask)
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    UBYTE *buf;
    LONG ioerr;
    ULONG len;

    if((nch = nAllocHid()))
    {
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        buf = nch->nch_EPInBuf;
        psdSendPipe(nch->nch_EPInPipe, buf, 512);
        do
        {
            sigs = Wait(sigmask);
            while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort)))
            {
                if(pp == nch->nch_EPInPipe)
                {
                    if(!(ioerr = psdGetPipeError(pp)))
                    {
                        len = psdGetPipeActual(pp);
                        nParseMidi(nch, buf, len);
                    } else {
                        KPRINTF(1, ("Int Pipe failed %ld\n", ioerr));
                        psdDelayMS(200);
                    }
                    psdSendPipe(nch->nch_EPInPipe, buf, 512);
                    break;
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        KPRINTF(20, ("Going down the river!\n"));
        psdAbortPipe(nch->nch_EPInPipe);
        psdWaitPipe(nch->nch_EPInPipe);
        nFreeHid(nch);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nSendKey()" */
void nSendKey(struct NepClassHid *nch, UWORD code)
{
    nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
    nch->nch_FakeEvent.ie_SubClass = 0;
    nch->nch_FakeEvent.ie_Code = code;
    nch->nch_FakeEvent.ie_NextEvent = NULL;
    nch->nch_FakeEvent.ie_Qualifier = 0;
    nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
    nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
    nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
    DoIO((struct IORequest *) nch->nch_InpIOReq);
}
/* \\\ */

/* /// "nParseMidi()" */
void nParseMidi(struct NepClassHid *nch, UBYTE *buf, ULONG len)
{
    UWORD iecode = 0xff;
    UWORD chan;
    UWORD note;
    UWORD velocity;
    UWORD octave;
    UWORD cmd;

    struct ClsGlobalCfg *cgc = &nch->nch_ClsBase->nh_CurrentCGC;

    while(len > 3)
    {
        KPRINTF(1, ("Msg: %02lx %02lx %02lx %02lx\n", buf[0], buf[1], buf[2], buf[3]));
        chan = buf[0] >> 4;
        cmd = buf[0] & 0x0f;
        switch(cmd)
        {
            case 0x0: // Miscellaneous function codes. Reserved for future extensions.
            case 0x1: // Cable events. Reserved for future expansion.
            case 0x2: // Two-byte System Common messages like MTC, SongSelect, etc.
            case 0x3: // Three-byte System Common messages like SPP, etc.
            case 0x4: // SysEx starts or continues
            case 0x5: // Single-byte System Common Message or SysEx ends with following single byte.
            case 0x6: // SysEx ends with following two bytes.
            case 0x7: // SysEx ends with following three bytes.
                break;

            case 0x8: // Note-off
                KPRINTF(1, ("Note %ld up with speed %ld\n", note, velocity));
                // fall through

            case 0x9: // Note-on
                note = buf[2];
                if(cmd == 0x9)
                {
                    KPRINTF(1, ("Note %ld down with speed %ld\n", note, velocity));
                    velocity = buf[3];
                } else {
                    velocity = 0;
                }
                if(note < 12 * cgc->cgc_MidiMinOctave)
                {
                    octave = 0;
                } else {
                    octave = (note - (12 * cgc->cgc_MidiMinOctave)) / 12;
                }
                if(octave > cgc->cgc_KeyMaxOctave)
                {
                    octave = cgc->cgc_KeyMaxOctave;
                }
                if((octave != nch->nch_LastOctave) && (octave != nch->nch_LastOctave+1))
                {
                    nch->nch_LastOctave = octave; /* go to right octave */
                    if(nch->nch_LastOctave > 0)
                    {
                        nch->nch_LastOctave--; // stay at low octave
                    }
                    /* Send F-key */
                    nSendKey(nch, 0x50+nch->nch_LastOctave);
                    nSendKey(nch, (IECODE_UP_PREFIX|0x50)+nch->nch_LastOctave);
                }
                if(octave == nch->nch_LastOctave)
                {
                    iecode = lowoctave[note % 12];
                }
                if(octave > nch->nch_LastOctave)
                {
                    iecode = highoctave[note % 12];
                }
                if(iecode != 0xff)
                {
                    if(!velocity)
                    {
                        iecode |= IECODE_UP_PREFIX;
                    }
                    nSendKey(nch, iecode);
                    if(cgc->cgc_AutoKeyUp && (!(iecode & IECODE_UP_PREFIX)))
                    {
                        iecode |= IECODE_UP_PREFIX;
                        nSendKey(nch, iecode);
                    }
                }
                break;

            case 0xA: // Poly-KeyPress
            case 0xB: // Control Change
            case 0xC: // Program Change
            case 0xD: // Channel Pressure
            case 0xE: // PitchBend Change
            case 0xF: // Single Byte
                break;

        }
        /*if(keyup)
        {
            KPRINTF(1, ("Key up: %08lx\n", keycode));
            iecode = usbkeymap[keycode];
            oldqual = qualifier;
            nch->nch_FakeEvent.ie_Class = IECLASS_RAWKEY;
            nch->nch_FakeEvent.ie_SubClass = 0;
            nch->nch_FakeEvent.ie_Code = iecode|IECODE_UP_PREFIX;
            nch->nch_FakeEvent.ie_NextEvent = NULL;
            nch->nch_FakeEvent.ie_Qualifier = qualifier;
            nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
            nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
            nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
            DoIO((struct IORequest *) nch->nch_InpIOReq);
        }*/
        buf += 4;
        len -= 4;
    }
}
/* \\\ */

/* /// "nAllocHid()" */
struct NepClassHid * nAllocHid(void)
{
    struct Task *thistask;
    struct NepClassHid *nch;

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

        nch->nch_EPIn = psdFindEndpoint(nch->nch_Interface, NULL,
                                       EA_IsIn, TRUE,
                                       EA_TransferType, USEAF_BULK,
                                       TAG_END);
        if(!nch->nch_EPIn)
        {
            KPRINTF(1, ("Ooops!?! No Endpoints defined?\n"));
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                           "No Bulk-In Endpoint!");
            break;
        }
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
                            if((nch->nch_EPInPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPIn)))
                            {
                                psdSetAttrs(PGA_PIPE, nch->nch_EPInPipe,
                                            PPA_NakTimeout, FALSE,
                                            PPA_AllowRuntPackets, TRUE,
                                            TAG_END);

                                if((nch->nch_EPInBuf = psdAllocVec(1024)))
                                {
                                    nch->nch_Task = thistask;
                                    return(nch);
                                }
                                psdFreePipe(nch->nch_EPInPipe);
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
    psdFreeVec(nch->nch_EPInBuf);
    psdFreePipe(nch->nch_EPInPipe);
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

/* /// "nLoadClassConfig()" */
BOOL nLoadClassConfig(struct NepHidBase *nh)
{
    struct Library *ps;
    struct ClsGlobalCfg *cgc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
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

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepHidBase *nh;
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
        MUIA_Application_Title      , libname,
        MUIA_Application_Version    , VERSION_STRING,
        MUIA_Application_Copyright  , "©2005-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the simplemidi.class",
        MUIA_Application_Base       , "SIMPLEMIDI",
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, nh->nh_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                End,
            Child, MenuObjectT("Settings"),
                Child, nh->nh_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save",
                    MUIA_Menuitem_Shortcut, "S",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, nh->nh_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, "MUI Settings",
                    MUIA_Menuitem_Shortcut, "M",
                    End,
                End,
            End,

        SubWindow, nh->nh_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, libname,
            MUIA_HelpNode, libname,

            WindowContents, VGroup,
/*                Child, actionobj = NewObject(ActionClass->mcc_Class, 0,
                    MUIA_ShowMe, FALSE,
                    End, */
                Child, ColGroup(2), GroupFrameT("Global Settings"),
                    Child, Label((ULONG) "Midi Minimum Octave:"),
                    Child, nh->nh_MidiMinOctaveObj = SliderObject, SliderFrame,
                        MUIA_CycleChain, 1,
                        MUIA_Numeric_Min, 0,
                        MUIA_Numeric_Max, 10,
                        MUIA_Numeric_Value, nh->nh_CurrentCGC.cgc_MidiMinOctave,
                        End,
                    Child, Label((ULONG) "Maximum Octaves on Keyboard:"),
                    Child, nh->nh_KeyMaxOctaveObj = SliderObject, SliderFrame,
                        MUIA_CycleChain, 1,
                        MUIA_Numeric_Min, 2,
                        MUIA_Numeric_Max, 10,
                        MUIA_Numeric_Value, nh->nh_CurrentCGC.cgc_KeyMaxOctave,
                        End,
                    Child, Label((ULONG) "Automatic KeyUp Event:"),
                    Child, HGroup,
                        Child, nh->nh_AutoKeyUpObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nh->nh_CurrentCGC.cgc_AutoKeyUp,
                            MUIA_ShowSelState, FALSE,
                            End,
                        Child, HSpace(0),
                        End,
                    End,
                Child, VSpace(0),
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, nh->nh_UseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Save ",
                        End,
                    Child, nh->nh_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Use ",
                        End,
                    End,
                End,
            End,
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
        ULONG isopen;
        ULONG iconify;
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
                    get(nh->nh_MidiMinOctaveObj, MUIA_Numeric_Value, &nh->nh_CurrentCGC.cgc_MidiMinOctave);
                    get(nh->nh_KeyMaxOctaveObj, MUIA_Numeric_Value, &nh->nh_CurrentCGC.cgc_KeyMaxOctave);
                    get(nh->nh_AutoKeyUpObj, MUIA_Selected, &nh->nh_CurrentCGC.cgc_AutoKeyUp);

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
                    MUI_RequestA(nh->nh_App, nh->nh_MainWindow, 0, NULL, "Fabulous!", VERSION_STRING, NULL);
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
void nGUITaskCleanup(struct NepHidBase *nh)
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

