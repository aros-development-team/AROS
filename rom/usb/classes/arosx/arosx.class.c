/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gamepad (XInput) USB class driver
    Lang: English
*/

#include "debug.h"

#include "arosx.class.h"

/* /// "Lib Stuff" */
static int libInit(LIBBASETYPEPTR nh)
{
    mybug(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(!UtilityBase)
    {
        mybug(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        nh = NULL;
    }

    mybug(10, ("libInit: Ok\n"));
    return(nh ? TRUE : FALSE);
}

static int libOpen(LIBBASETYPEPTR nh)
{
    mybug(10, ("libOpen nh: 0x%08lx\n", nh));
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    mybug(10, ("libExpunge nh: 0x%08lx\n", nh));
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

    struct PsdConfig *pc;
    struct PsdDevice *pd;

    struct PsdDescriptor *pdd;

    mybug(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    IFA_Config, &pc,
                    TAG_DONE);

        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    DA_ConfigList, &pdd,
                    TAG_END);

        /*
            Check to see if it is believed to be an XInput Gamepad interface
             - We could extend this class to also house code for other XInput devices,
               but for now it's only XInput Gamepad
            TODO: Force usbForceInterfaceBinding to go through this too
        */
        pdd = psdFindDescriptor(pd, NULL, DDA_DescriptorType, 33, DDA_Interface, pif, TAG_END);

        CloseLibrary(ps);

        if(((ifclass == 255) && (subclass == 93) && (proto == 1) && (pdd != NULL)))
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

    UBYTE buf[64];
    struct Task *tmptask;

    mybug(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        if((nch = psdAllocVec(sizeof(struct NepClassHid))))
        {
            nch->nch_ClsBase = nh;
            nch->nch_Device = NULL;
            nch->nch_Interface = pif;

            psdSafeRawDoFmt(buf, 64, "arosx.class<%08lx>", nch);
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
                    psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &nch->nch_devname, TAG_END);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "Play it again, '%s'!",
                                   nch->nch_devname);

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

    mybug(1, ("nepHidReleaseInterfaceBinding(%08lx)\n", nch));

    /* Kill the nch_GUITask */
    if(nch->nch_GUITask)
    {
        Signal(nch->nch_GUITask, SIGBREAKF_CTRL_C);
    }

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

    mybug(1, ("nepHidGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "Gamepad (XInput)";
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
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

    mybug(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassHid *) methoddata[0]);
            return(TRUE);

        case UCM_OpenBindingCfgWindow:
            return(nOpenCfgWindow((struct NepClassHid *) methoddata[0]));

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/**************************************************************************/

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
        psdSendPipe(nch->nch_EPInPipe, buf, 20);
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
                        nParseMsg(nch, buf, len);
                    } else {
                        mybug(1, ("Int Pipe failed %ld\n", ioerr));
                        psdDelayMS(200);
                    }
                    psdSendPipe(nch->nch_EPInPipe, buf, 20);
                    break;
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        mybug(-1, ("Going down the river!\n"));
        psdAbortPipe(nch->nch_EPInPipe);
        psdWaitPipe(nch->nch_EPInPipe);
        nFreeHid(nch);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nParseMsg()" */
void nParseMsg(struct NepClassHid *nch, UBYTE *buf, ULONG len)
{
/* TODO: Store controller information and parse accordingly */

/*
    Ta-daa!!
    When Logitech Wireless Gamepad F710 goes to sleep we get this in our endpoint
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 84 00 00 00 0 0 00
        bit 4 on byte 14

    And this is the first message after it wakes on "A" button press
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 94 00 55 00 00 00
        followed by this, the "A" button msg
        Msg: 00 14 00 10 00 00 80 00 80 00 80 00 80 00 b4 00 55 00 00 00

    Mode LED on
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 9c 00 55 00 00 00

    Mode LED off
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 94 00 55 00 00 00

    Long vibration
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 b4 00 55 00 00 00

    Short vibration
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 94 00 55 00 00 00

    Taking the controller out of range and we get this (same as removing the battery)
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 a4 00 00 00 00 00

    Taking the battery out and the dongle soon sends this
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 a4 00 00 00 00 00

    Re-inserting the battery and we get this
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 b4 00 55 00 00 00

    Pressing "A" on Logitech (wired) Gamepad F310
        Msg: 00 14 00 10 00 00 80 00 80 00 80 00 80 00 00 00 00 00 00 00

    Toggling mode button on Logitech (wired) Gamepad F310 has no effect on the msg but we get one
        some bit from byte 14 and on could tell if it's a wireless or wired (bit 7 of byte 14)
        Byte 16 might be the battery level on Wireless F710?
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 00 00 00 00 00 00
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 00 00 00 00 00 00

    mybug(1, ("Msg: %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx\n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10],
                    buf[11], buf[12], buf[13], buf[14], buf[15], buf[16], buf[17], buf[18], buf[19]));
*/

    /*
        Just blindly map thumb sticks (They should be there for XBox360 and Logitech F710 and F310)
         - Needs a structure defined for them (there are different kinds of mappings)
        These are not actually UWORDS, see below how they are placed on the gauge objects
    */

    nch->stick_lx = (UWORD)AROS_WORD2LE((UWORD)((buf[6])  | (buf[7]<<8)));
    nch->stick_ly = (UWORD)AROS_WORD2LE((UWORD)((buf[8])  | (buf[9]<<8)));
    nch->stick_rx = (UWORD)AROS_WORD2LE((UWORD)((buf[10]) | (buf[11]<<8)));
    nch->stick_ry = (UWORD)AROS_WORD2LE((UWORD)((buf[12]) | (buf[13]<<8)));

    /*
    mybug(1, ("\n"))
    mybug(1, ("stick_lx: %04x\n", nch->stick_lx)); //0x8000 - 0x7fff
    mybug(1, ("stick_ly: %04x\n", nch->stick_ly));
    mybug(1, ("stick_rx: %04x\n", nch->stick_rx));
    mybug(1, ("stick_ry: %04x\n", nch->stick_ry));
    */

    if(nch->nch_GUITask)
    {
        Signal(nch->nch_GUITask, (ULONG) (1<<nch->nch_TrackingSignal));
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
                                       EA_TransferType, USEAF_INTERRUPT,
                                       TAG_END);
        if(!nch->nch_EPIn)
        {
            mybug(1, ("Ooops!?! No Endpoints defined?\n"));
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                           "No Interrupt-In Endpoint!");
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

/* /// "nOpenCfgWindow()" */
LONG nOpenCfgWindow(struct NepClassHid *nch)
{
    struct Library *ps;
    mybug(10, ("Opening GUI...\n"));
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

#undef ps
#undef MUIMasterBase
#define ps PsdBase
#define MUIMasterBase MUIBase

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepHidBase *nh;
    struct NepClassHid *nch;

    struct Library *MUIBase;
    struct Library *PsdBase;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;
    nh = nch->nch_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    if((MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        if((ps = OpenLibrary("poseidon.library", 4)))
        {

            nch->nch_App = ApplicationObject,
            MUIA_Application_Title      , (IPTR)libname,
            MUIA_Application_Version    , (IPTR)VERSION_STRING,
            MUIA_Application_Copyright  , (IPTR)"©2017 The AROS Development Team",
            MUIA_Application_Author     , (IPTR)"The AROS Development Team",
            MUIA_Application_Description, (IPTR)"Settings for the arosx.class",
            MUIA_Application_Base       , (IPTR)"AROSX",
            MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
            MUIA_Application_Menustrip  , (IPTR)MenustripObject,
                Child, (IPTR)MenuObjectT((IPTR)"Project"),
                    Child, (IPTR)(nch->nch_AboutMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"About...",
                        MUIA_Menuitem_Shortcut, (IPTR)"?",
                        End),
                    End,
                Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                    Child, (IPTR)(nch->nch_UseMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Save",
                        MUIA_Menuitem_Shortcut, (IPTR)"S",
                        End),
                    Child, (IPTR)MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                        End,
                    Child, (IPTR)(nch->nch_MUIPrefsMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                        MUIA_Menuitem_Shortcut, (IPTR)"M",
                        End),
                    End,
                End,

            SubWindow, (IPTR)(nch->nch_MainWindow = WindowObject,
                MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
                MUIA_Window_Title, (IPTR)nch->nch_devname,
                MUIA_HelpNode, (IPTR)libname,

                WindowContents, (IPTR)VGroup,
                    Child, (IPTR)ColGroup(2), GroupFrameT("Gamepad"),
                        Child, (IPTR)(nch->nch_GaugeObject_stick_lx = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xfffe,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),
                        Child, (IPTR)(nch->nch_GaugeObject_stick_ly = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xfffe,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),
                        Child, (IPTR)(nch->nch_GaugeObject_stick_rx = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xfffe,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),
                        Child, (IPTR)(nch->nch_GaugeObject_stick_ry = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xfffe,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),
                        End,
                    Child, (IPTR)VSpace(0),
                    Child, (IPTR)HGroup,
                        MUIA_Group_SameWidth, TRUE,
                        Child, (IPTR)(nch->nch_UseObj = TextObject, ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Save ",
                            End),
                        Child, (IPTR)(nch->nch_CloseObj = TextObject, ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Use ",
                            End),
                        End,
                    End,
                End),
            End;

            if(nch->nch_App) 
            {
                DoMethod(nch->nch_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                         nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                DoMethod(nch->nch_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                         nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                DoMethod(nch->nch_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                         nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

                DoMethod(nch->nch_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         nch->nch_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
                DoMethod(nch->nch_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                DoMethod(nch->nch_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         nch->nch_App, 2, MUIM_Application_OpenConfigWindow, 0);


                IPTR  isopen = 0;
                IPTR  iconify = 0;
                ULONG sigs;
                ULONG sigmask;
                LONG retid;

                get(nch->nch_App, MUIA_Application_Iconified, &iconify);
                set(nch->nch_MainWindow, MUIA_Window_Open, TRUE);
                get(nch->nch_MainWindow, MUIA_Window_Open, &isopen);

                if((isopen || (!iconify)))
                {
                    nch->nch_TrackingSignal = AllocSignal(-1);
                    sigmask = (1<<nch->nch_TrackingSignal);
                    do
                    {
                        retid = DoMethod(nch->nch_App, MUIM_Application_NewInput, &sigs);
                        switch(retid)
                        {
                            case ID_ABOUT:
                                MUI_RequestA(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Fabulous!", VERSION_STRING, NULL);
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

                            if(nch->stick_lx>=0x8000) {
                                set(nch->nch_GaugeObject_stick_lx, MUIA_Gauge_Current, (nch->stick_lx-0x8000));
                            } else {
                                set(nch->nch_GaugeObject_stick_lx, MUIA_Gauge_Current, (0x7fff+nch->stick_lx));
                            }

                            if(nch->stick_ly>=0x8000) {
                                set(nch->nch_GaugeObject_stick_ly, MUIA_Gauge_Current, (nch->stick_ly-0x8000));
                            } else {
                                set(nch->nch_GaugeObject_stick_ly, MUIA_Gauge_Current, (0x7fff+nch->stick_ly));
                            }

                            if(nch->stick_rx>=0x8000) {
                                set(nch->nch_GaugeObject_stick_rx, MUIA_Gauge_Current, (nch->stick_rx-0x8000));
                            } else {
                                set(nch->nch_GaugeObject_stick_rx, MUIA_Gauge_Current, (0x7fff+nch->stick_rx));
                            }

                            if(nch->stick_ry>=0x8000) {
                                set(nch->nch_GaugeObject_stick_ry, MUIA_Gauge_Current, (nch->stick_ry-0x8000));
                            } else {
                                set(nch->nch_GaugeObject_stick_ry, MUIA_Gauge_Current, (0x7fff+nch->stick_ry));
                            }

                        }
                    } while(TRUE);
                    set(nch->nch_MainWindow, MUIA_Window_Open, FALSE);
                }

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

                if(ps)
                {
                    CloseLibrary(ps);
                    ps = NULL;
                }
            }
        }
    }

    Forbid();
    FreeSignal(nch->nch_TrackingSignal);
    nch->nch_TrackingSignal = -1;
    nch->nch_GUITask = NULL;
    --nh->nh_Library.lib_OpenCnt;

    AROS_USERFUNC_EXIT
}
/* \\\ */

