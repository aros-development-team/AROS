/*
 *----------------------------------------------------------------------------
 *                         dfu class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "dfu.class.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    struct NepDFUBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        ret = nh;
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return(ret ? TRUE : FALSE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));
    CloseLibrary((struct Library *) UtilityBase);
    return(TRUE);
}

ADD2INITLIB(libInit, 0)
ADD2EXPUNGELIB(libExpunge, 0)
/* \\\ */

/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassDFU * usbAttemptInterfaceBinding(struct NepDFUBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    KPRINTF(1, ("nepDFUAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    TAG_DONE);
        CloseLibrary(ps);

        if((ifclass == FWUPGRADE_CLASSCODE) && (subclass == FWUPGRADE_STD_SUBCLASS))
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassDFU * usbForceInterfaceBinding(struct NepDFUBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassDFU *nch;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    struct PsdDevice *hubpd;
    struct PsdDescriptor *pdd;
    IPTR hubport;
    STRPTR devname;
    STRPTR ifidstr;
    STRPTR devidstr;
    IPTR ifnum;
    IPTR devclass;
    IPTR prodid;
    IPTR ifproto;
    struct UsbDFUDesc *dfudesc = NULL;

    KPRINTF(1, ("nepDFUForceInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Config, &pc,
                    IFA_IDString, &ifidstr,
                    IFA_InterfaceNum, &ifnum,
                    IFA_Protocol, &ifproto,
                    TAG_DONE);
        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    TAG_END);
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductName, &devname,
                    DA_Class, &devclass,
                    DA_ProductID, &prodid,
                    DA_IDString, &devidstr,
                    DA_HubDevice, &hubpd,
                    DA_AtHubPortNumber, &hubport,
                    TAG_END);
        pdd = psdFindDescriptor(pd, NULL,
                                DDA_DescriptorType, UDT_DFU,
                                DDA_Interface, pif,
                                TAG_END);
        if(pdd)
        {
            if((nch = psdAllocVec(sizeof(struct NepClassDFU))))
            {
                nch->nch_ClsBase = nh;
                nch->nch_Device = pd;
                nch->nch_Hub = hubpd;
                nch->nch_HubPort = hubport;
                nch->nch_Interface = pif;
                nch->nch_DevIDString = devidstr;
                nch->nch_IfIDString = ifidstr;
                nch->nch_IfNum = ifnum;

                psdGetAttrs(PGA_DESCRIPTOR, pdd,
                            DDA_DescriptorData, &dfudesc,
                            TAG_END);
                nch->nch_WillDetach = dfudesc->bmAttributes & UDDAF_WILL_DETACH;
                nch->nch_CanUpgrade = dfudesc->bmAttributes & UDDAF_DOWNLOADABLE;
                nch->nch_CanRetrieve = dfudesc->bmAttributes & UDDAF_UPLOADABLE;
                nch->nch_NoManifestReset = dfudesc->bmAttributes & UDDAF_NO_MANIFEST_RST;

                nch->nch_DetachTimeOut = dfudesc->wDetachTimeOut0|(dfudesc->wDetachTimeOut1<<8);
                nch->nch_TransferSize = dfudesc->wTransferSize0|(dfudesc->wTransferSize1<<8);

                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Firmware %s available for '%s' (through config GUI)!",
                               ((nch->nch_CanUpgrade && nch->nch_CanRetrieve) ? "Download/Upgrade" :
                                (nch->nch_CanRetrieve ? "Download" :
                                 (nch->nch_CanUpgrade ? "Upgrade" : "Cage"))),
                               devname);
                // auto open window
                if((prodid == 0xffff) || (devclass == FWUPGRADE_CLASSCODE) || (ifproto == FWUPGRADE_PROTO_DFU))
                {
                    nOpenBindingCfgWindow(nh, nch);
                }
                CloseLibrary(ps);
                return(nch);
            }
        } else {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                           "Could not find DFU functional descriptor for '%s'!",
                           devname);
        }
        CloseLibrary(ps);
    }
    return(NULL);

}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepDFUBase *nh, struct NepClassDFU *nch)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    KPRINTF(1, ("nepDFUReleaseInterfaceBinding(%08lx)\n", nch));
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

        //FreeSignal(nch->nch_ReadySignal);
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "Lost firm grip on '%s'!",
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

    KPRINTF(1, ("nepDFUGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "Firmware Upgrading/Downloading";
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
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             break;

         case UGA_BINDING:
             if((ti = FindTagItem(UCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
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
            usbReleaseInterfaceBinding(nh, (struct NepClassDFU *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(FALSE);

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassDFU *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            return(FALSE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "nOpenBindingCfgWindow()" */
LONG nOpenBindingCfgWindow(struct NepDFUBase *nh, struct NepClassDFU *nch)
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

const STRPTR DFUErrors[] =
{
    "No error condition is present.",
    "File is not targeted for use by this device.",
    "File is for this device but fails some vendor-specific verification test.",
    "Device is unable to write memory.",
    "Memory erase function failed.",
    "Memory erase check failed.",
    "Program memory function failed.",
    "Programmed memory failed verification.",
    "Cannot program memory due to received address that is out of range.",
    "Received DFU_DNLOAD with wLength = 0, but device does not think it has all of the data yet.",
    "Device's firmware is corrupt. It cannot return to run-time (non-DFU) operations.",
    "iString indicates a vendor-specific error.",
    "Device detected unexpected USB reset signaling.",
    "Device detected unexpected power on reset.",
    "Something went wrong, but the device does not know what it was.",
    "Device stalled an unexpected request."
};

const STRPTR DFUStates[] =
{
    "Device is running its normal application.",
    "Transition from normal to DFU mode.",
    "Device is operating in the DFU mode.",
    "Device has received a block, waiting for GetStatus.",
    "Device is programming a block into its memories.",
    "Device is processing a download operation.",
    "Device has received the final block of firmware.",
    "Device is in the Manifestation phase.",
    "Device is waiting for reset.",
    "Device is processing an upload operation.",
    "An error has occurred."
};

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepDFUBase *nh;
    struct NepClassDFU *nch;
    BOOL initokay = FALSE;
    STRPTR infomsg;

    thistask = FindTask(NULL);
#undef ps
#define ps nch->nch_PsdBase
#undef IntuitionBase
#define IntuitionBase nch->nch_IntBase
#undef MUIMasterBase
#define MUIMasterBase nch->nch_MUIBase
#undef DOSBase
#define DOSBase nch->nch_DOSBase

    nch = thistask->tc_UserData;
    nh = nch->nch_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    do
    {
        if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
        {
            KPRINTF(10, ("Couldn't open muimaster.library.\n"));
            break;
        }

        if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
        {
            KPRINTF(10, ("Couldn't open intuition.library.\n"));
            break;
        }
        if(!(DOSBase = OpenLibrary("dos.library", 39)))
        {
            KPRINTF(10, ("Couldn't open dos.library.\n"));
            break;
        }
        if(!(ps = OpenLibrary("poseidon.library", 4)))
        {
            KPRINTF(10, ("Couldn't open poseidon.library.\n"));
            break;
        }
        if(!(nch->nch_TaskMsgPort = CreateMsgPort()))
        {
            break;
        }
        if(!(nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
        {
            break;
        }
        psdSetAttrs(PGA_PIPE, nch->nch_EP0Pipe,
                    PPA_NakTimeout, TRUE,
                    PPA_NakTimeoutTime, 15000,
                    TAG_END);

        if(!(nch->nch_Buffer = psdAllocVec(nch->nch_TransferSize)))
        {
            break;
        }
        initokay = TRUE;
    } while(FALSE);
    if(!initokay)
    {
        nGUITaskCleanup(nch);
        return;
    }

    /* obtain status */
    infomsg = nGetStatus(nch);

    nch->nch_App = ApplicationObject,
        MUIA_Application_Title      , libname,
        MUIA_Application_Version    , VERSION_STRING,
        MUIA_Application_Copyright  , "©2005-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the dfu.class",
        MUIA_Application_Base       , "DFU",
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, nch->nch_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                End,
            Child, MenuObjectT("Settings"),
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
                Child, VGroup, GroupFrameT("Firmware Upgrade/Download"),
                    Child, HGroup,
                        MUIA_ShowMe, nch->nch_DFUStatus.bState >= STATE_DFU_IDLE,
                        Child, Label((ULONG) "Firmware file:"),
                        Child, PopaslObject,
                            MUIA_Popstring_String, nch->nch_FWFileObj = StringObject,
                                StringFrame,
                                MUIA_CycleChain, 1,
                                MUIA_String_AdvanceOnCR, TRUE,
                                MUIA_String_MaxLen, 256,
                                End,
                            MUIA_Popstring_Button, PopButton(MUII_PopFile),
                            ASLFR_TitleText, "Select a firmware binary file...",
                            ASLFR_InitialPattern, "#?.dfu",
                            ASLFR_DoPatterns, TRUE,
                            End,
                        End,
                    Child, nch->nch_GaugeObj = GaugeObject,
                        MUIA_Frame, MUIV_Frame_ReadList,
                        MUIA_Gauge_Horiz, TRUE,
                        MUIA_Gauge_InfoText, infomsg,
                        MUIA_Gauge_Current, 0,
                        MUIA_Gauge_Max, 100,
                        End,
                    Child, HGroup,
                        MUIA_Group_SameWidth, TRUE,
                        Child, nch->nch_DetachObj = TextObject, ButtonFrame,
                            MUIA_ShowMe, !nch->nch_DFUStatus.bState,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, "\33c Enter DFU Mode ",
                            End,
                        Child, nch->nch_DownloadObj = TextObject, ButtonFrame,
                            MUIA_ShowMe, nch->nch_DFUStatus.bState >= STATE_DFU_IDLE,
                            MUIA_Disabled, !nch->nch_CanRetrieve,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, "\33c Download (Read) ",
                            End,
                        Child, nch->nch_UploadObj = TextObject, ButtonFrame,
                            MUIA_ShowMe, nch->nch_DFUStatus.bState >= STATE_DFU_IDLE,
                            MUIA_Disabled, !nch->nch_CanUpgrade,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, "\33c Upgrade (Write) ",
                            End,
                        End,
                    End,
                Child, VSpace(0),
                Child, nch->nch_CloseObj = TextObject, ButtonFrame,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_CycleChain, 1,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_Text_Contents, "\33c Close ",
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
    DoMethod(nch->nch_DetachObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_DETACH);
    DoMethod(nch->nch_DownloadObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_DOWNLOAD);
    DoMethod(nch->nch_UploadObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPLOAD);
    DoMethod(nch->nch_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(nch->nch_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(nch->nch_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_OpenConfigWindow, 0);
    {
        ULONG isopen;
        ULONG iconify;
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
                case ID_DOWNLOAD:
                    nFWDownload(nch);
                    break;

                case ID_UPLOAD:
                    nFWUpload(nch);
                    break;

                case ID_DETACH:
                    nDetach(nch);
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

/* /// "nDetach()" */
void nDetach(struct NepClassDFU *nch)
{
    ULONG delay = (nch->nch_DetachTimeOut < 2000) ? nch->nch_DetachTimeOut : 2000;
    LONG ioerr;

    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                 UDFUR_DETACH, delay, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "Detaching failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return;
    }
    set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, nGetStatus(nch));
    if(!nch->nch_WillDetach)
    {
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "Sending USB Reset to device...");

        psdDelayMS(delay>>1);
        psdDoHubMethod(nch->nch_Device, UCM_HubPowerCyclePort, nch->nch_Hub, nch->nch_HubPort);
    } else {
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "Waiting for device to detach automatically...");
    }
}
/* \\\ */

/* /// "nFWDownload()" */
void nFWDownload(struct NepClassDFU *nch)
{
    UWORD blocknum = 0;
    STRPTR file;
    LONG ioerr;
    ULONG len;

    get(nch->nch_FWFileObj, MUIA_String_Contents, &file);
    if(!*file)
    {
        set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Give filename for writing!");
        return;
    }
    if((nch->nch_InOutFile = Open(file, MODE_NEWFILE)))
    {
        set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Starting download...");
        do
        {
            psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                         UDFUR_UPLOAD, (ULONG) blocknum, nch->nch_IfNum);
            ioerr = psdDoPipe(nch->nch_EP0Pipe, nch->nch_Buffer, nch->nch_TransferSize);
            len = psdGetPipeActual(nch->nch_EP0Pipe);
            if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
            {
                /*psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                               "%ld bytes", len);*/
                Write(nch->nch_InOutFile, nch->nch_Buffer, len);
            } else {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                               "Download at block %ld failed: %s (%ld), %ld read",
                               blocknum,
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr, len);

                set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, nGetStatus(nch));
                break;
            }
            blocknum++;
            set(nch->nch_GaugeObj, MUIA_Gauge_Current, blocknum % 100);
        } while(!ioerr);
        set(nch->nch_GaugeObj, MUIA_Gauge_Current, 0);
        if(ioerr == UHIOERR_RUNTPACKET)
        {
            set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Finished downloading.");
        }
        Close(nch->nch_InOutFile);
    } else {
        set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Error opening file!");
    }
}
/* \\\ */

/* /// "nFWUpload()" */
void nFWUpload(struct NepClassDFU *nch)
{
    UWORD blocknum = 0;
    STRPTR file;
    LONG ioerr;
    LONG len;
    LONG totallen;
    LONG pos = 0;
    LONG choice;
    STRPTR infomsg;
    ULONG delay;

    get(nch->nch_FWFileObj, MUIA_String_Contents, &file);
    if(!*file)
    {
        set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Give firmware file!");
        return;
    }
    if((nch->nch_InOutFile = Open(file, MODE_OLDFILE)))
    {
        Seek(nch->nch_InOutFile, 0, OFFSET_END);
        totallen = Seek(nch->nch_InOutFile, 0, OFFSET_BEGINNING);
        choice = MUI_Request(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Continue|Cancel",
                             "Are you sure you want to reflash the firmware of this device?\n\n"
                             "\33bDO NOT REMOVE THE DEVICE OR REBOOT DURING THIS PROCESS!\33n\n"
                             "This might render the device useless!\n", NULL);
        if(choice == 1)
        {
            set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Starting upload...");
            set(nch->nch_GaugeObj, MUIA_Gauge_Max, totallen);
            do
            {
                KPRINTF(1, ("Reading %ld/%ld: %ld...\n", pos, totallen, nch->nch_TransferSize));
                len = Read(nch->nch_InOutFile, nch->nch_Buffer, nch->nch_TransferSize);
                if(len < 0)
                {
                    len = 0;
                }
                pos += len;
                set(nch->nch_GaugeObj, MUIA_Gauge_Current, pos);
                psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                             UDFUR_DNLOAD, (ULONG) blocknum, nch->nch_IfNum);
                KPRINTF(1, ("Sending %ld\n", len));
                ioerr = psdDoPipe(nch->nch_EP0Pipe, nch->nch_Buffer, len);
                KPRINTF(1, ("IOErr = %ld\n", ioerr));
                if(ioerr)
                {
                    infomsg = nGetStatus(nch);
                    set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, infomsg);
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                                   "Upload at block %ld failed: %s (%ld), %s",
                                   blocknum,
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr, infomsg);
                    break;
                } else {
                    infomsg = nGetStatus(nch);
                    set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, infomsg);
                    if(nch->nch_DFUStatus.bState == STATE_DFU_ERROR)
                    {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                                       "Upload at block %ld failed: %s",
                                       blocknum,
                                       infomsg);
                        break;
                    } else {
                        delay = nch->nch_DFUStatus.bwPollTimeout0|(nch->nch_DFUStatus.bwPollTimeout1<<8)|(nch->nch_DFUStatus.bwPollTimeout2<<16);
                        KPRINTF(1, ("Delaying %ldms\n", delay));
                        //psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Delaying %ldms", delay);
                        psdDelayMS(delay);
                    }
                }
                blocknum++;
            } while((!ioerr) && len);
            KPRINTF(1, ("Done\n"));
            set(nch->nch_GaugeObj, MUIA_Gauge_Current, 0);
            set(nch->nch_GaugeObj, MUIA_Gauge_Max, 100);

            if(!len)
            {
                set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Finished uploading, manifesting.");
                if(nch->nch_NoManifestReset)
                {
                    psdDelayMS(5000);
                    set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, nGetStatus(nch));
                } else {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "Sending USB Reset to device in 5 seconds...");

                    psdDelayMS(5000);
                    //psdDoHubMethod(nch->nch_Device, UCM_HubPowerCyclePort, nch->nch_Hub, nch->nch_HubPort);
                    set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, nGetStatus(nch));
                }
            }
        } else {
            set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Upgrade aborted by user.");
        }
        Close(nch->nch_InOutFile);
    } else {
        set(nch->nch_GaugeObj, MUIA_Gauge_InfoText, "Error opening file!");
    }
}
/* \\\ */

/* /// "nGetStatus()" */
STRPTR nGetStatus(struct NepClassDFU *nch)
{
    STRPTR infomsg;
    LONG ioerr;

    KPRINTF(1, ("GetStatus\n"));
    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                 UDFUR_GETSTATUS, 0, nch->nch_IfNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, &nch->nch_DFUStatus, sizeof(nch->nch_DFUStatus));
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "UDFUR_GETSTATUS failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        nch->nch_DFUStatus.bState = STATE_DFU_ERROR;
        nch->nch_DFUStatus.bStatus = STATUS_ERR_UNKNOWN;
    } else {
        KPRINTF(1, ("State %ld, Status %ld\n", nch->nch_DFUStatus.bState, nch->nch_DFUStatus.bStatus));
        if(nch->nch_DFUStatus.bState > STATE_DFU_ERROR)
        {
            nch->nch_DFUStatus.bState = STATE_DFU_ERROR;
        }
        if(nch->nch_DFUStatus.bStatus > STATUS_ERR_STALLEDPKT)
        {
            nch->nch_DFUStatus.bStatus = STATUS_ERR_UNKNOWN;
        }
    }
    if(nch->nch_DFUStatus.bState == STATE_DFU_ERROR)
    {
        infomsg = DFUErrors[nch->nch_DFUStatus.bStatus];
        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                     UDFUR_CLRSTATUS, 0, nch->nch_IfNum);
        ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                           "UDFUR_CLRSTATUS failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }
    } else {
        infomsg = DFUStates[nch->nch_DFUStatus.bState];
    }
    return(infomsg);
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void nGUITaskCleanup(struct NepClassDFU *nch)
{
    if(nch->nch_App)
    {
        MUI_DisposeObject(nch->nch_App);
        nch->nch_App = NULL;
    }
    if(nch->nch_Buffer)
    {
        psdFreeVec(nch->nch_Buffer);
        nch->nch_Buffer = NULL;
    }
    if(nch->nch_EP0Pipe)
    {
        psdFreePipe(nch->nch_EP0Pipe);
        nch->nch_EP0Pipe = NULL;
    }
    if(nch->nch_TaskMsgPort)
    {
        DeleteMsgPort(nch->nch_TaskMsgPort);
        nch->nch_TaskMsgPort = NULL;
    }
    if(MUIMasterBase)
    {
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
    }
    if(DOSBase)
    {
        CloseLibrary(DOSBase);
        DOSBase = NULL;
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

