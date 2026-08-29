/*
    Copyright (C) 2014-2026, The AROS Development Team. All rights reserved.

    Desc: SuperSpeed USB3.0 hub for Poseidon (based upon hub.class.c by Chris Hodges <chrisly@platon42.de>)
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <aros/debug.h>

#include <proto/poseidon.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/usbhardware.h>
#include <libraries/usbclass.h>

#include "hubss_class.h"

#include LC_LIBDEFS_FILE

struct NepClassHubSS * GM_UNIQUENAME(usbAttemptDeviceBinding)(struct NepHubSSBase *nh, struct PsdDevice *pd);
struct NepClassHubSS * GM_UNIQUENAME(usbForceDeviceBinding)(struct NepHubSSBase * nh, struct PsdDevice *pd);
void GM_UNIQUENAME(usbReleaseDeviceBinding)(struct NepHubSSBase *nh, struct NepClassHubSS *nch);

struct NepClassHubSS * GM_UNIQUENAME(nAllocHub)(void);
void GM_UNIQUENAME(nFreeHub)(struct NepClassHubSS *nch);
struct PsdDevice * GM_UNIQUENAME(nConfigurePort)(struct NepClassHubSS *nch, UWORD port);
LONG GM_UNIQUENAME(nClearPortStatus)(struct NepClassHubSS *nch, UWORD port);
BOOL GM_UNIQUENAME(nHubSuspendDevice)(struct NepClassHubSS *nch, struct PsdDevice *pd);
BOOL GM_UNIQUENAME(nHubResumeDevice)(struct NepClassHubSS *nch, struct PsdDevice *pd);
void GM_UNIQUENAME(nHandleHubMethod)(struct NepClassHubSS *nch, struct NepHubSSMsg *nhm);
AROS_UFP0(void, GM_UNIQUENAME(nHubssTask));

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh) {
    KPRINTF(1, ("%s()\n", __func__));
    NEWLIST(&nh->nh_Bindings);
    InitSemaphore(&nh->nh_Adr0Sema);

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)

/* \\\ */

/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

/* /// "usbAttemptDeviceBinding()" */
struct NepClassHubSS * GM_UNIQUENAME(usbAttemptDeviceBinding)(struct NepHubSSBase *nh, struct PsdDevice *pd) {
    struct Library *ps;
    IPTR devclass;
    IPTR issuperspeed = 0;

    KPRINTF(1, ("%s(0x%p)\n", __func__, pd));

    if((ps = OpenLibrary("poseidon.library", 4))) {
        psdGetAttrs(PGA_DEVICE, pd, DA_Class, &devclass, DA_IsSuperspeed, &issuperspeed, TAG_DONE);
        CloseLibrary(ps);

        if((devclass == HUB_CLASSCODE) && (issuperspeed)) {
            return(GM_UNIQUENAME(usbForceDeviceBinding)(nh, pd));
        }
    }
    return(NULL);
}

/* /// "usbForceDeviceBinding()" */
struct NepClassHubSS * GM_UNIQUENAME(usbForceDeviceBinding)(struct NepHubSSBase * nh, struct PsdDevice *pd) {
    struct Library *ps;
    struct NepClassHubSS *nch;
    STRPTR devname;
    char buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("%s(0x%p)\n", __func__, pd));

    if((ps = OpenLibrary("poseidon.library", 4))) {
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_DONE);

        if((nch = psdAllocVec(sizeof(struct NepClassHubSS)))) {
            nch->nch_HubBase = nh;
            nch->nch_Device = pd;
            psdSafeRawDoFmt(buf, 64, "hubss.class<%p>", nch);

            nch->nch_ReadySignal = SIGB_SINGLE;
            nch->nch_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);

            if((tmptask = psdSpawnSubTask(buf, GM_UNIQUENAME(nHubssTask), nch))) {
                psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);

                if(nch->nch_Task) {
                    nch->nch_ReadySigTask = NULL;
                    //FreeSignal(nch->nch_ReadySignal);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "I'm in love with superspeed hub '%s'.", devname);

                    Forbid();
                    AddTail(&nh->nh_Bindings, &nch->nch_Node);
                    Permit();
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

/* /// "usbReleaseDeviceBinding()" */
void GM_UNIQUENAME(usbReleaseDeviceBinding)(struct NepHubSSBase *nh, struct NepClassHubSS *nch) {
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("%s(0x%p, 0x%p)\n", __func__, nh, nch));

    if((ps = OpenLibrary("poseidon.library", 4))) {

        Forbid();
        nch->nch_ReadySignal = SIGB_SINGLE;
        nch->nch_ReadySigTask = FindTask(NULL);
        if(nch->nch_Task) {
            KPRINTF(1, ("Sending Break\n"));
            Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
        }
        Permit();

        while(nch->nch_Task) {
            psdBorrowLocksWait(nch->nch_Task, 1UL<<nch->nch_ReadySignal);
        }
        KPRINTF(1, ("Task gone\n"));

        //FreeSignal(nch->nch_ReadySignal);
        psdGetAttrs(PGA_DEVICE, nch->nch_Device, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Time to get rid of '%s'!", devname);

        Forbid();
        Remove(&nch->nch_Node);
        Permit();

        psdFreeVec(nch);
        CloseLibrary(ps);
    }
}

/* /// "usbGetAttrsA()" */
AROS_LH3(LONG, usbGetAttrsA, AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, usbstruct, A0), AROS_LHA(struct TagItem *, taglist, A1), LIBBASETYPEPTR, nh, 5, hubss) {
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    KPRINTF(1, ("%s(%ld, 0x%p, 0x%p)\n", __func__, type, usbstruct, taglist));

    switch(type) {
        case UGA_CLASS:
            while((ti = LibNextTagItem(&taglist)) != NULL) {
                switch (ti->ti_Tag) {
                    case UCCA_Priority:
                        *((SIPTR *) ti->ti_Data) = 1;
                        count++;
                        break;
                    case UCCA_Description:
                        *((STRPTR *) ti->ti_Data) = "Root/external superspeed hub base class";
                        count++;
                        break;
                    case UCCA_HasClassCfgGUI:
                        *((IPTR *) ti->ti_Data) = FALSE;
                        count++;
                        break;
                    case UCCA_HasBindingCfgGUI:
                        *((IPTR *) ti->ti_Data) = FALSE;
                        count++;
                        break;
                    case UCCA_AfterDOSRestart:
                        *((IPTR *) ti->ti_Data) = FALSE;
                        count++;
                        break;
                    case UCCA_UsingDefaultCfg:
                        *((IPTR *) ti->ti_Data) = TRUE;
                        count++;
                        break;
                    case UCCA_SupportsSuspend:
                        *((IPTR *) ti->ti_Data) = TRUE;
                        count++;
                        break;
                } /* switch (ti->ti_Tag) */
            }; /* while((ti = LibNextTagItem(&taglist)) != NULL) */
            break;

         case UGA_BINDING:
             if((ti = LibFindTagItem(UCBA_UsingDefaultCfg, taglist))) {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             break;
    }

    return(count);
    AROS_LIBFUNC_EXIT
}

/* /// "usbSetAttrsA()" */
AROS_LH3(LONG, usbSetAttrsA, AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, usbstruct, A0), AROS_LHA(struct TagItem *, tags, A1), LIBBASETYPEPTR, nh, 6, hubss) {
    AROS_LIBFUNC_INIT

    KPRINTF(1, ("%s(%ld, 0x%p, 0x%p)\n", __func__, type, usbstruct, tags));

    return(0);

    AROS_LIBFUNC_EXIT
}

/* /// "usbDoMethodA()" */
AROS_LH2(IPTR, usbDoMethodA, AROS_LHA(ULONG, methodid, D0), AROS_LHA(IPTR *, methoddata, A1), LIBBASETYPEPTR, nh, 7, hubss) {
    AROS_LIBFUNC_INIT

    struct NepClassHubSS *nch;

    KPRINTF(1, ("%s(%ld)\n", __func__, methodid));

    switch(methodid) {
        case UCM_AttemptDeviceBinding:
            return((IPTR) GM_UNIQUENAME(usbAttemptDeviceBinding)(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ForceDeviceBinding:
            return((IPTR) GM_UNIQUENAME(usbForceDeviceBinding)(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ReleaseDeviceBinding:
            GM_UNIQUENAME(usbReleaseDeviceBinding)(nh, (struct NepClassHubSS *) methoddata[0]);
            return(TRUE);

        case UCM_HubPowerCyclePort:
        case UCM_HubDisablePort: {
            struct PsdDevice *pd = (struct PsdDevice *) methoddata[0];
            ULONG port = (ULONG) methoddata[1];

            if(!(pd && port)) {
                KPRINTF(20, ("HubPowerCycle/DisablePort Params Null!\n"));
                return(FALSE);
            }

            Forbid();
            nch = (struct NepClassHubSS *) nh->nh_Bindings.lh_Head;
            while(nch->nch_Node.ln_Succ) {
                if(nch->nch_Device == pd) {
                    KPRINTF(20, ("HubPowerCycle/DisablePort Dev found (port %ld)!\n", port));
                    if(port <= nch->nch_NumPorts) {
                        nch->nch_DisablePort |= 1UL<<port;
                        if(methodid == UCM_HubPowerCyclePort) {
                            nch->nch_PowerCycle |= 1UL<<port;
                        }
                        if(nch->nch_Task) {
                            Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
                        }
                        Permit();
                        return(TRUE);
                    }
                    break;
                }
                nch = (struct NepClassHubSS *) nch->nch_Node.ln_Succ;
            }
            Permit();

            return(FALSE);
            } /* case UCM_HubDisablePort */

        case UCM_HubClassScan: {
            nch = (struct NepClassHubSS *) methoddata[0];

            Forbid();
            nch->nch_ClassScan = TRUE;
            if(nch->nch_Task) {
                Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
            }
            Permit();

            return(TRUE);
            } /* case UCM_HubClassScan */

        case UCM_AttemptSuspendDevice:
        case UCM_AttemptResumeDevice:
        case UCM_HubClaimAppBinding:
        case UCM_HubReleaseIfBinding:
        case UCM_HubReleaseDevBinding:
        case UCM_HubSuspendDevice:
        case UCM_HubResumeDevice: {
            struct NepHubSSMsg nhm;
            struct Library *ps;
            nch = (struct NepClassHubSS *) methoddata[0];
            nhm.nhm_Result = (IPTR) NULL;
            nhm.nhm_MethodID = methodid;
            nhm.nhm_Params = methoddata;

            if((ps = OpenLibrary("poseidon.library", 4))) {
                if(nch->nch_Task == FindTask(NULL)) {
                    // if we would send the message to ourself, we would deadlock, so handle this directly
                    GM_UNIQUENAME(nHandleHubMethod)(nch, &nhm);
                } else {
                    nhm.nhm_Msg.mn_ReplyPort = CreateMsgPort();
                    nhm.nhm_Msg.mn_Length = sizeof(struct NepHubSSMsg);

                    Forbid();
                    if(nch->nch_Task && nhm.nhm_Msg.mn_ReplyPort) {
                        PutMsg(nch->nch_CtrlMsgPort, &nhm.nhm_Msg);
                        Permit();

                        while(!GetMsg(nhm.nhm_Msg.mn_ReplyPort)) {
                            psdBorrowLocksWait(nch->nch_Task, 1UL<<nhm.nhm_Msg.mn_ReplyPort->mp_SigBit);
                        }
                    } else {
                        Permit();
                    }
                    if(nhm.nhm_Msg.mn_ReplyPort)
                    {
                        DeleteMsgPort(nhm.nhm_Msg.mn_ReplyPort);
                    }
                }
                CloseLibrary(ps);
            }

            return(nhm.nhm_Result);
            }/* case UCM_HubResumeDevice */

        default:
            break;
    }

    return(0);
    AROS_LIBFUNC_EXIT
}

#undef ps
#define ps nch->nch_Base

/* /// "nHubssTask()" */
AROS_UFH0(void, GM_UNIQUENAME(nHubssTask)) {
    AROS_USERFUNC_INIT

    struct NepClassHubSS *nch;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    UWORD num;
    LONG ioerr;
    struct UsbPortStatus uhps;
    struct UsbHubStatus uhhs;
    ULONG count;
    struct PsdDevice *pd;
    STRPTR devname;
    struct NepHubSSMsg *nhm;

    KPRINTF(1, ("%s()\n", __func__));

    if((nch = GM_UNIQUENAME(nAllocHub)())) {
        Forbid();
        if(nch->nch_ReadySigTask) {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        count = 0;
        for(num = 1; num <= nch->nch_NumPorts; num++) {
            if(((nch->nch_Downstream)[num-1] = pd = GM_UNIQUENAME(nConfigurePort)(nch, num))) {
                psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Detected device '%s' at port %ld. I like it.",
                               devname, num);
                count++;
            }
        }
        if(count) {
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                           "Hub has added %ld device(s). That'll be fun!",
                           count);
        }
        // do a class scan
        for(num = 1; num <= nch->nch_NumPorts; num++) {
            if((pd = (nch->nch_Downstream)[num-1])) {
                psdHubClassScan(pd);
            }
        }
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|(1L<<nch->nch_CtrlMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        nch->nch_Running = TRUE;
        nch->nch_IOStarted = FALSE;
        do {
            if(nch->nch_Running && (!nch->nch_IOStarted)) {
                psdSendPipe(nch->nch_EP1Pipe, nch->nch_PortChanges, (nch->nch_NumPorts+8)>>3);
                nch->nch_IOStarted = TRUE;
            }
            sigs = Wait(sigmask);

            while((nhm = (struct NepHubSSMsg *) GetMsg(nch->nch_CtrlMsgPort))) {
                GM_UNIQUENAME(nHandleHubMethod)(nch, nhm);
                ReplyMsg((struct Message *) nhm);
            }

            if(nch->nch_DisablePort) {
                for(num = 1; num <= nch->nch_NumPorts; num++) {
                    if((nch->nch_DisablePort) & (1L<<num)) {
                        nch->nch_DisablePort &= ~(1L<<num);
                        /* Remove device */
                        if((pd = (nch->nch_Downstream)[num-1])) {
                            psdSetAttrs(PGA_DEVICE, pd, DA_IsConnected, FALSE, TAG_END);
                            psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                           "Zapping device '%s' at port %ld!",
                                           devname, num);
                            psdFreeDevice(pd);
                            psdSendEvent(EHMB_REMDEVICE, pd, NULL);
                            (nch->nch_Downstream)[num-1] = NULL;
                            pd = NULL;
                            /*
                             * Disable the port so a device that has gone
                             * does not hold the bus. PORT_ENABLE is not a
                             * feature a SuperSpeed hub accepts - it stalls
                             * it, which halts the hub's EP0 in the middle
                             * of this cleanup - and a SuperSpeed link needs
                             * no such help: the port follows the link down
                             * by itself when the device leaves.
                             */
                            if(!nch->nch_SSPortProto) {
                                KPRINTF(1, ("hubss: USR_CLEAR_FEATURE:UFS_PORT_ENABLE for removed device..\n"));

                                psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                             USR_CLEAR_FEATURE, UFS_PORT_ENABLE, (ULONG) num);
                                ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                if(ioerr) {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                   "CLEAR_PORT_ENABLE failed: %s (%ld)",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    KPRINTF(1, ("CLEAR_PORT_ENABLE failed %ld.\n", ioerr));
                                }
                            }
                        }
                        if(nch->nch_PowerCycle & (1<<num)) {
                            KPRINTF(2, ("Powercycle request for port %lu\n", num));
                            nch->nch_PowerCycle &= ~(1L<<num);

                            /* Wait for device to settle */
                            psdDelayMS(250);
                            if(((nch->nch_Downstream)[num-1] = pd = GM_UNIQUENAME(nConfigurePort)(nch, num))) {
                                psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                               "Device '%s' returned. Happy happy joy joy.",
                                               devname);
                                psdHubClassScan(pd);
                            }
                        }
                    }
                }
            }

            if(nch->nch_ClassScan) {
                nch->nch_ClassScan = FALSE;
                for(num = 1; num <= nch->nch_NumPorts; num++)
                {
                    if((pd = (nch->nch_Downstream)[num-1]))
                    {
                        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                        psdHubClassScan(pd);
                    }
                }
            }
            while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort))) {
                if(pp == nch->nch_EP1Pipe) {
                    nch->nch_IOStarted = FALSE;
                    ioerr = psdGetPipeError(nch->nch_EP1Pipe);
                    if(ioerr == UHIOERR_TIMEOUT) {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "Hub involuntarily gone! Disconnecting...");
                        psdSetAttrs(PGA_DEVICE, nch->nch_Device,
                                    DA_IsConnected, FALSE,
                                    TAG_END);
                        nch->nch_PortChanges[0] = 0xff;
                        nch->nch_PortChanges[1] = 0xff;
                        nch->nch_PortChanges[2] = 0xff;
                        nch->nch_PortChanges[3] = 0xff;
                        sigs |= SIGBREAKF_CTRL_C;
                    }
                    if((!ioerr) || (ioerr == UHIOERR_TIMEOUT)) {
                        KPRINTF(2, ("Port changed at %p, Numports=%ld!\n", nch->nch_PortChanges[0], nch->nch_NumPorts));

                        if(nch->nch_PortChanges[0] & 1) {
                            psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_DEVICE,
                                         USR_GET_STATUS, 0, 0);
                            ioerr = psdDoPipe(nch->nch_EP0Pipe, &uhhs, sizeof(struct UsbHubStatus));
                            uhhs.wHubStatus = AROS_WORD2LE(uhhs.wHubStatus);
                            uhhs.wHubChange = AROS_WORD2LE(uhhs.wHubChange);
                            if(!ioerr)
                            {
                                if(uhhs.wHubStatus & UHSF_OVER_CURRENT)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                   "Hub over-current situation detected! Unpowering ALL ports!");
                                    for(num = 1; num <= nch->nch_NumPorts; num++)
                                    {
                                        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                                     USR_CLEAR_FEATURE, UFS_PORT_POWER, (ULONG) num);
                                        ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                        if(ioerr)
                                        {
                                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                           "PORT_POWER for port %ld failed: %s (%ld)",
                                                           num, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                            KPRINTF(1, ("PORT_POWER for port %ld failed %ld!\n", num, ioerr));
                                        }

                                        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                                     USR_CLEAR_FEATURE, UFS_C_PORT_OVER_CURRENT, (ULONG) num);
                                        psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                    }
                                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_DEVICE,
                                                 USR_CLEAR_FEATURE, UFS_C_HUB_OVER_CURRENT, 0);
                                    psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                }
                                if(uhhs.wHubChange & UHSF_LOCAL_POWER_LOST)
                                {
                                    struct PsdConfig *pc = NULL;
                                    struct PsdHardware *phw = NULL;
                                    psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                                                DA_Config, &pc,
                                                DA_Hardware, &phw,
                                                TAG_END);
                                    if(uhhs.wHubStatus & UHSF_LOCAL_POWER_LOST)
                                    {
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                       "Hub is no longer self-powered! Low power conditions may occur.");

                                        if(pc && phw)
                                        {
                                            psdSetAttrs(PGA_CONFIG, pc, CA_SelfPowered, FALSE, TAG_END);
                                            psdCalculatePower(phw);
                                        }
                                    } else {
                                        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                                       "Hub is now self-powered! Yay!");
                                        if(pc && phw)
                                        {
                                            psdSetAttrs(PGA_CONFIG, pc, CA_SelfPowered, TRUE, TAG_END);
                                            psdCalculatePower(phw);
                                        }
                                    }
                                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_DEVICE,
                                                 USR_CLEAR_FEATURE, UFS_C_HUB_LOCAL_POWER, 0);
                                    psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                }
                            }
                        }

                        for(num = 1; num <= nch->nch_NumPorts; num++)
                        {
                            if(nch->nch_PortChanges[num>>3] & (1L<<(num & 7)))
                            {
                                psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_OTHER,
                                             USR_GET_STATUS, 0, (ULONG) num);
                                ioerr = psdDoPipe(nch->nch_EP0Pipe, &uhps, sizeof(struct UsbPortStatus));
                                uhps.wPortStatus = AROS_WORD2LE(uhps.wPortStatus);
                                uhps.wPortChange = AROS_WORD2LE(uhps.wPortChange);
                                if(ioerr == UHIOERR_TIMEOUT)
                                {
                                    uhps.wPortStatus = 0;
                                    uhps.wPortChange = 0xffff;
                                    ioerr = 0;
                                } else {
                                    GM_UNIQUENAME(nClearPortStatus)(nch, num);
                                }
                                if(!ioerr)
                                {
                                    pd = (nch->nch_Downstream)[num-1];
                                    if(uhps.wPortStatus & UPSF_PORT_OVER_CURRENT)
                                    {
                                        if(pd)
                                        {
                                            psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                                        } else {
                                            devname = "a ghost";
                                        }
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                       "Over-current situation detected with %s at port %ld! Unpowering port!",
                                                       devname, num);
                                        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                                     USR_CLEAR_FEATURE, UFS_PORT_POWER, (ULONG) num);
                                        ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                        if(ioerr)
                                        {
                                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                           "PORT_POWER for port %ld failed: %s (%ld)",
                                                           num, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                            KPRINTF(1, ("PORT_POWER for port %ld failed %ld!\n", num, ioerr));
                                        }

                                        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                                     USR_CLEAR_FEATURE, UFS_C_PORT_OVER_CURRENT, (ULONG) num);
                                        psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                                    }
                                    /* A SuperSpeed hub has no suspend change
                                       bit; bit 2 is reserved there and the
                                       link state carries U1/U2/U3 instead. */
                                    if(!nch->nch_SSPortProto &&
                                       (uhps.wPortChange & UPSF_PORT_SUSPEND))
                                    {
                                        if((!(uhps.wPortStatus & UPSF_PORT_SUSPEND)) && pd)
                                        {
                                            IPTR oldsusp = 0;
                                            psdGetAttrs(PGA_DEVICE, pd, DA_IsSuspended, &oldsusp, TAG_END);
                                            psdSetAttrs(PGA_DEVICE, pd, DA_IsSuspended, FALSE, TAG_END);
                                            psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                                            if(oldsusp)
                                            {
                                                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                                               "Device '%s' at port %ld resumed from remote!",
                                                               devname, num);
                                                psdSendEvent(EHMB_DEVRESUMED, pd, NULL);
                                                psdResumeBindings(pd);
                                            }
                                        }
                                        else if((uhps.wPortStatus & UPSF_PORT_SUSPEND) && pd)
                                        {
                                            psdSetAttrs(PGA_DEVICE, pd, DA_IsSuspended, FALSE, TAG_END);
                                            psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                                            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                                           "Device '%s' at port %ld suspended!",
                                                           devname, num);
                                        } else {
                                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                           "Bogus suspend/resume change on port %ld.",
                                                           num);
                                        }
                                    }
                                    if(uhps.wPortChange & UPSF_PORT_CONNECTION)
                                    {
                                        /* Remove device */
                                        if((!(uhps.wPortStatus & UPSF_PORT_CONNECTION)) && pd)
                                        {
                                            psdSetAttrs(PGA_DEVICE, pd, DA_IsConnected, FALSE, TAG_END);
                                            psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                                            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                                           "Device '%s' at port %ld is gone!",
                                                           devname, num);
                                            psdFreeDevice(pd);
                                            psdSendEvent(EHMB_REMDEVICE, pd, NULL);
                                            (nch->nch_Downstream)[num-1] = NULL;
                                            pd = NULL;
                                        }
                                        /* add new device */
                                        if((uhps.wPortStatus & UPSF_PORT_CONNECTION) && (!pd))
                                        {
                                            /* Wait for device to settle */
                                            psdDelayMS(100);
                                            if(((nch->nch_Downstream)[num-1] = pd = GM_UNIQUENAME(nConfigurePort)(nch, num)))
                                            {
                                                psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
                                                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                                               "New device '%s' at port %ld. Very nice.",
                                                               devname, num);
                                                psdClassScan();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        /* Bail out on time out. */
                        if(nch->nch_PortChanges[0] == 0xff)
                        {
                            break;
                        }
                        psdDelayMS(50);
                    } else {
                        if(ioerr != IOERR_ABORTED)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "Something weird happened to the status packet, it failed: %s (%ld)",
                                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            psdDelayMS(200);
                        }
                    }
                    break;
                } else {
                    KPRINTF(20, ("Bogus message received!\n"));
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        KPRINTF(20, ("Going down the river!\n"));
        if(nch->nch_IOStarted)
        {
            psdAbortPipe(nch->nch_EP1Pipe);
            psdWaitPipe(nch->nch_EP1Pipe);
        }
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Oh no! I've been shot! Arrggghh...");
        GM_UNIQUENAME(nFreeHub)(nch);
    }
    AROS_USERFUNC_EXIT
}

/* /// "nAllocHub()" */
struct NepClassHubSS * GM_UNIQUENAME(nAllocHub)(void) {
    struct UsbSSHubDesc *usshd;
    //struct UsbStdBOSDesc *usbosd;
    struct Task *thistask;
    struct NepClassHubSS *nch;
    struct UsbHubStatus uhhs;
    APTR parenthub;
    LONG ioerr;
    ULONG len;
    UWORD num;
    UBYTE buf[2];
    IPTR issuperspeed = 0;
    IPTR prodid;
    IPTR vendid;
    BOOL overcurrent = FALSE;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;

    KPRINTF(1, ("%s()\n", __func__));

    do {
        if(!(nch->nch_Base = OpenLibrary("poseidon.library", 4))) {
            Alert(AG_OpenLib);
            break;
        }

        psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                    DA_Hardware, &nch->nch_Hardware,
                    DA_IsSuperspeed, &issuperspeed,
                    DA_ProductID, &prodid,
                    DA_VendorID, &vendid,
                    DA_HubDevice, &parenthub,
                    TAG_END);

        nch->nch_IsRootHub = (parenthub ? FALSE : TRUE);
        nch->nch_IsUSB30 = issuperspeed;

        /*
         * A real SuperSpeed hub answers GET_PORT_STATUS in the USB 3 format
         * and stalls the USB 2.0 port features. The root hub is emulated by
         * the host controller driver, which reports the USB 2.0 layout with
         * a synthetic superspeed flag and covers both its USB 2 and USB 3
         * ports, so it keeps the older decoding.
         */
        nch->nch_SSPortProto = (issuperspeed && !nch->nch_IsRootHub);

        if(!nch->nch_Interface) {
            nch->nch_Interface = psdFindInterface(nch->nch_Device, NULL, IFA_Class, HUB_CLASSCODE, TAG_END);
        }

        if(!nch->nch_Interface) {
            KPRINTF(1, ("Ooops!?! No interfaces defined?\n"));
            break;
        }

        nch->nch_EP1 = psdFindEndpoint(nch->nch_Interface, NULL, EA_IsIn, TRUE, EA_TransferType, USEAF_INTERRUPT, TAG_END);

        if(!nch->nch_EP1) {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Ooops!?! No endpoints defined?");
            KPRINTF(1, ("Ooops!?! No Endpoints defined?\n"));
            break;
        }

        /*
            Device descriptor tree:

            Device descriptor
            Config descriptor
            ...String descriptor
            BOS descriptor
            ...SS capability descriptor
            ......USB2.0 LPM descriptor
            Interface descriptor
            Endpoint descriptor 0
            ...Endpoint descriptor 1
            ......Endpoint descriptor n

            USB 3.0 enumeration:

            Set address
            Get device descriptor
            Get BOS descriptor
            Get config descriptor
                .
                .
                .

            see: http://youtu.be/5ChWxMLKzOs
        */


        if((nch->nch_CtrlMsgPort = CreateMsgPort())) {
            if((nch->nch_TaskMsgPort = CreateMsgPort())) {
                KPRINTF(2, ("Allocating EP0 pipe..\n"));
                if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL))) {
                    KPRINTF(2, ("EP0 pipe @ 0x%p\n", nch->nch_EP0Pipe));
                    psdSetAttrs(PGA_PIPE, nch->nch_EP0Pipe, PPA_NakTimeout, TRUE, PPA_NakTimeoutTime, 1000, TAG_END);
                    psdSetAltInterface(nch->nch_EP0Pipe, nch->nch_Interface);

                    if((nch->nch_EP1Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EP1))) {

                        psdSetAttrs(PGA_PIPE, nch->nch_EP1Pipe, PPA_AllowRuntPackets, TRUE, TAG_END);
                        psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_DEVICE, USR_GET_DESCRIPTOR, UDT_SSHUB<<8, 0);

                        ioerr = psdDoPipe(nch->nch_EP0Pipe, &buf, 2);

                        if(buf[1] == UDT_SSHUB) {

                            if((!ioerr) || (ioerr == UHIOERR_OVERFLOW)) {
                                len = buf[0];

                                if((usshd = psdAllocVec(len))) {
                                    ioerr = psdDoPipe(nch->nch_EP0Pipe, usshd, len);

                                    if(!ioerr) {
                                        nch->nch_NumPorts     = (UWORD)usshd->bNbrPorts;
                                        nch->nch_HubAttr      = (UWORD)AROS_WORD2LE(usshd->wHubCharacteristics);
                                        nch->nch_PwrGoodTime  = (UWORD)usshd->bPwrOn2PwrGood<<1;
                                        nch->nch_HubCurrent   = (UWORD)usshd->bHubContrCurrent;
                                        nch->nch_HubHdrDecLat = (UWORD)usshd->bHubHdrDecLat;
                                        nch->nch_HubDelay     = (UWORD)usshd->wHubDelay;
                                        nch->nch_Removable    = (UWORD)usshd->DeviceRemovable;

                                        if(nch->nch_HubAttr & UHCM_THINK_TIME) {
                                            psdSetAttrs(PGA_DEVICE, nch->nch_Device, DA_HubThinkTime, (nch->nch_HubAttr & UHCM_THINK_TIME)>>UHCS_THINK_TIME, TAG_END);
                                        }

                                        KPRINTF(2, ("Parsed SSHub descriptor\n"
                                                    "  nch_NumPorts     = %d\n"
                                                    "  nch_HubAttr      = 0x%04x\n"
                                                    "  nch_PwrGoodTime  = %d\n"
                                                    "  nch_HubCurrent   = %d\n"
                                                    "  nch_HubHdrDecLat = %d\n"
                                                    "  nch_HubDelay     = %d\n"
                                                    "  nch_Removable    = 0x%04x\n\n",
                                                    (ULONG)nch->nch_NumPorts,
                                                    (ULONG)nch->nch_HubAttr,
                                                    (ULONG)nch->nch_PwrGoodTime,
                                                    (ULONG)nch->nch_HubCurrent,
                                                    (ULONG)nch->nch_HubHdrDecLat,
                                                    (ULONG)nch->nch_HubDelay,
                                                    (ULONG)nch->nch_Removable));

                                        psdFreeVec(usshd);

                                        /*
                                         * A SuperSpeed hub needs to be told how
                                         * far down the tree it sits before it
                                         * can work out which nibble of a route
                                         * string names one of its own ports.
                                         * Depth counts the hubs between it and
                                         * the root hub, so a hub on a root port
                                         * is depth zero.
                                         */
                                        if(nch->nch_SSPortProto) {
                                            struct PsdDevice *uphub = (struct PsdDevice *)parenthub;
                                            ULONG hubdepth = 0;

                                            while(uphub) {
                                                IPTR next = (IPTR)NULL;

                                                psdGetAttrs(PGA_DEVICE, uphub,
                                                            DA_HubDevice, &next, TAG_END);
                                                if(!next)
                                                    break;

                                                hubdepth++;
                                                uphub = (struct PsdDevice *)next;
                                            }

                                            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_DEVICE,
                                                         USR_SET_HUB_DEPTH, hubdepth, 0);
                                            if((ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0))) {
                                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                               "SET_HUB_DEPTH(%ld) failed: %s (%ld)",
                                                               hubdepth,
                                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"),
                                                               ioerr);
                                            }
                                            KPRINTF(2, ("SET_HUB_DEPTH(%ld) -> %ld\n", hubdepth, ioerr));
                                        }

                                        psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_DEVICE, USR_GET_STATUS, 0, 0);
                                        ioerr = psdDoPipe(nch->nch_EP0Pipe, &uhhs, sizeof(struct UsbHubStatus));

                                        uhhs.wHubStatus = AROS_WORD2LE(uhhs.wHubStatus);
                                        uhhs.wHubChange = AROS_WORD2LE(uhhs.wHubChange);
                                        if(!ioerr)
                                        {
                                            struct PsdConfig *pc = NULL;
                                            struct PsdHardware *phw = NULL;
                                            if(uhhs.wHubStatus & UHSF_OVER_CURRENT)
                                            {
                                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                                               "Hub over-current situation detected! Resolve this first!");
                                                //overcurrent = TRUE;
                                            }

                                            psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                                                        DA_Config, &pc,
                                                        DA_Hardware, &phw,
                                                        TAG_END);
                                            if(uhhs.wHubStatus & UHSF_LOCAL_POWER_LOST)
                                            {
                                                if(pc && phw)
                                                {
                                                    psdSetAttrs(PGA_CONFIG, pc, CA_SelfPowered, FALSE, TAG_END);
                                                    psdCalculatePower(phw);
                                                }
                                            } else {
                                                if(pc && phw)
                                                {
                                                    psdSetAttrs(PGA_CONFIG, pc, CA_SelfPowered, TRUE, TAG_END);
                                                    psdCalculatePower(phw);
                                                }
                                            }
                                        }
                                        if(!overcurrent) {
                                            if((nch->nch_Downstream = psdAllocVec((ULONG) nch->nch_NumPorts*sizeof(APTR)))) {
                                                /*for(num = 1; num <= nch->nch_NumPorts; num++)
                                                {
                                                    GM_UNIQUENAME(nClearPortStatus)(nch, num);
                                                }
                                                psdDelayMS(20);*/

                                                KPRINTF(2, ("Powering up ports...\n\n"));

                                                for(num = 1; num <= nch->nch_NumPorts; num++) {
                                                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER, USR_SET_FEATURE, UFS_PORT_POWER, (ULONG) num);
                                                    ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);

                                                    if(ioerr) {
                                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "PORT_POWER for port %ld failed: %s (%ld)", num, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                                        KPRINTF(1, ("PORT_POWER for port %ld failed %ld!\n", num, ioerr));
                                                    }
                                                }
                                                psdDelayMS((ULONG) nch->nch_PwrGoodTime + 15);

                                                psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Hub with %ld ports successfully configured.", nch->nch_NumPorts);

                                                KPRINTF(10, ("%s ready!\n", thistask->tc_Node.ln_Name));
                                                nch->nch_Task = thistask;

                                                return(nch);
                                            } else {
                                                KPRINTF(1, ("No downstream port array memory!\n"));
                                            }
                                        }
                                    } else {
                                        psdFreeVec(usshd);
                                        psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "GET_HUB_DESCRIPTOR (%ld) failed: %s (%ld)", len, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                        KPRINTF(1, ("GET_HUB_DESCRIPTOR (%ld) failed %ld!\n", len, ioerr));
                                    }

                                } else {
                                    KPRINTF(1, ("No Hub Descriptor memory!\n"));
                                }
                            } else {
                                psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "GET_HUB_DESCRIPTOR (%ld) failed: %s (%ld)", 1, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                KPRINTF(1, ("GET_HUB_DESCRIPTOR (1) failed %ld!\n", ioerr));
                            }

                        }

                        psdFreePipe(nch->nch_EP1Pipe);
                    }
                    psdFreePipe(nch->nch_EP0Pipe);
                }
                DeleteMsgPort(nch->nch_TaskMsgPort);
            }
            DeleteMsgPort(nch->nch_CtrlMsgPort);
        }
    } while(FALSE);

    CloseLibrary(nch->nch_Base);

    Forbid();
    nch->nch_Task = NULL;

    if(nch->nch_ReadySigTask) {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }

    return(NULL);
}

/* /// "nFreeHub()" */
void GM_UNIQUENAME(nFreeHub)(struct NepClassHubSS *nch) {
    UWORD num;
    LONG ioerr;
    struct PsdDevice *pd;
    STRPTR devname;
    IPTR isconnected;
    struct Message *msg;

    KPRINTF(1, ("%s(0x%p)\n", __func__, nch));

    psdGetAttrs(PGA_DEVICE, nch->nch_Device, DA_IsConnected, &isconnected, TAG_END);
    for(num = 1; num <= nch->nch_NumPorts; num++) {
        KPRINTF(1, ("Iterating Port %ld\n", num));
        /* Remove downstream device */
        pd = (nch->nch_Downstream)[num-1];
        if(pd) {
            if(!isconnected) {
                psdSetAttrs(PGA_DEVICE, pd, DA_IsConnected, FALSE, TAG_END);
            }
            psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "My death killed device '%s' at port %ld!", devname, num);
            KPRINTF(1, ("FreeDevice %p\n", pd));
            psdFreeDevice(pd);
            psdSendEvent(EHMB_REMDEVICE, pd, NULL);
            (nch->nch_Downstream)[num-1] = NULL;
        }
        /* There's no sense trying to send out commands if the hub is already gone! */
        if(isconnected) {
             /* power down for port */
             psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER, USR_CLEAR_FEATURE, UFS_PORT_POWER, (ULONG) num);
             ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
             if(ioerr) {
                 psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "PORT_POWER for port %ld failed: %s (%ld)", num, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                 KPRINTF(1, ("PORT_POWER for port %ld failed %ld!\n", num, ioerr));
             }
        }
    }

    KPRINTF(1, ("FreePipes\n"));
    psdFreePipe(nch->nch_EP1Pipe);
    psdFreePipe(nch->nch_EP0Pipe);
    psdFreeVec(nch->nch_Downstream);

    KPRINTF(1, ("Entering Forbid\n"));
    Forbid();
    // clear queue
    while((msg = GetMsg(nch->nch_CtrlMsgPort))) {
        ReplyMsg(msg);
    }

    DeleteMsgPort(nch->nch_TaskMsgPort);
    DeleteMsgPort(nch->nch_CtrlMsgPort);
    CloseLibrary(nch->nch_Base);
    nch->nch_Task = NULL;

    if(nch->nch_ReadySigTask) {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }

    KPRINTF(1, ("Really gone now!\n"));
}

/* *** HUBSS Class *** */

/* /// "nClearPortStatus()" */
LONG GM_UNIQUENAME(nClearPortStatus)(struct NepClassHubSS *nch, UWORD port)
{
    LONG ioerr;
    LONG firsterr = 0;

    KPRINTF(1, ("%s(0x%p, %ld)\n", __func__, nch, port));

    /* Best-effort: try to clear all relevant change bits. Do not abort early,
       otherwise we may leave sticky change flags behind and re-trigger events. */

    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                 USR_CLEAR_FEATURE, UFS_C_PORT_CONNECTION, (ULONG)port);
    if((ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0))) {
        psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                       "CLEAR_PORT_FEATURE (C_PORT_CONNECTION) failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        KPRINTF(10, ("error occurred clearing UFS_C_PORT_CONNECTION!\n"));
        if(!firsterr) firsterr = ioerr;
    }

    /*
     * C_PORT_ENABLE and C_PORT_SUSPEND do not exist on a SuperSpeed hub and
     * are stalled by it; the link state change bits take their place.
     */
    if(nch->nch_SSPortProto) {
        static const UWORD ssfeatures[] = {
            UFS_C_BH_PORT_RESET, UFS_C_PORT_LINK_STATE, UFS_C_PORT_CONFIG_ERROR
        };
        UWORD i;

        for(i = 0; i < (UWORD)(sizeof(ssfeatures) / sizeof(ssfeatures[0])); i++) {
            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                         USR_CLEAR_FEATURE, ssfeatures[i], (ULONG)port);
            if((ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0))) {
                KPRINTF(10, ("error occurred clearing SS change feature %ld!\n",
                             (ULONG)ssfeatures[i]));
                if(!firsterr) firsterr = ioerr;
            }
        }
    } else {
        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                     USR_CLEAR_FEATURE, UFS_C_PORT_ENABLE, (ULONG)port);
        if((ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0))) {
            psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                           "CLEAR_PORT_FEATURE (C_PORT_ENABLE) failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            KPRINTF(10, ("error occurred clearing UFS_C_PORT_ENABLE!\n"));
            if(!firsterr) firsterr = ioerr;
        }

        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                     USR_CLEAR_FEATURE, UFS_C_PORT_SUSPEND, (ULONG)port);
        if((ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0))) {
            psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                           "CLEAR_PORT_FEATURE (C_PORT_SUSPEND) failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            KPRINTF(10, ("error occurred clearing UFS_C_PORT_SUSPEND!\n"));
            if(!firsterr) firsterr = ioerr;
        }
    }

    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                 USR_CLEAR_FEATURE, UFS_C_PORT_OVER_CURRENT, (ULONG)port);
    if((ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0))) {
        KPRINTF(10, ("error occurred clearing UFS_C_PORT_OVER_CURRENT!\n"));
        if(!firsterr) firsterr = ioerr;
    }

    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                 USR_CLEAR_FEATURE, UFS_C_PORT_RESET, (ULONG)port);
    if((ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0))) {
        psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                       "CLEAR_PORT_FEATURE (C_PORT_RESET) failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        KPRINTF(10, ("error occurred clearing UFS_C_PORT_RESET!\n"));
        if(!firsterr) firsterr = ioerr;
    }

    return firsterr;
}


/* /// "nDecodePortStatus()" */
/*
 * wPortStatus means different things on a USB 2.0 hub and on a SuperSpeed
 * one - power moves from bit 8 to bit 9, the link state replaces the suspend
 * bit and the speed becomes a field. Decode it once here so the port logic
 * does not have to care which kind of hub answered.
 */
struct HubSSPortState {
    BOOL  hps_Connected;
    BOOL  hps_Enabled;
    BOOL  hps_Powered;
    BOOL  hps_InReset;
    BOOL  hps_OverCurrent;
    BOOL  hps_SuperSpeed;
    BOOL  hps_HighSpeed;
    BOOL  hps_LowSpeed;
    UWORD hps_LinkState;
};

static void GM_UNIQUENAME(nDecodePortStatus)(struct NepClassHubSS *nch,
                                             UWORD status,
                                             struct HubSSPortState *hps)
{
    hps->hps_HighSpeed = FALSE;
    hps->hps_LowSpeed  = FALSE;

    if(nch->nch_SSPortProto) {
        hps->hps_Connected   = (status & UPS3F_PORT_CONNECTION) ? TRUE : FALSE;
        hps->hps_Enabled     = (status & UPS3F_PORT_ENABLE) ? TRUE : FALSE;
        hps->hps_Powered     = (status & UPS3F_PORT_POWER) ? TRUE : FALSE;
        hps->hps_InReset     = (status & UPS3F_PORT_RESET) ? TRUE : FALSE;
        hps->hps_OverCurrent = (status & UPS3F_PORT_OVER_CURRENT) ? TRUE : FALSE;
        hps->hps_LinkState   = (status & UPS3M_PORT_LINK_STATE) >> UPS3S_PORT_LINK_STATE;

        /* Every downstream port of a SuperSpeed hub runs at SuperSpeed;
           anything slower is attached to the companion USB 2.0 hub. */
        hps->hps_SuperSpeed  = TRUE;
    } else {
        hps->hps_Connected   = (status & UPSF_PORT_CONNECTION) ? TRUE : FALSE;
        hps->hps_Enabled     = (status & UPSF_PORT_ENABLE) ? TRUE : FALSE;
        hps->hps_Powered     = (status & UPSF_PORT_POWER) ? TRUE : FALSE;
        hps->hps_InReset     = (status & UPSF_PORT_RESET) ? TRUE : FALSE;
        hps->hps_OverCurrent = (status & UPSF_PORT_OVER_CURRENT) ? TRUE : FALSE;
        hps->hps_SuperSpeed  = (status & UPSF_PORT_SUPER_SPEED) ? TRUE : FALSE;
        hps->hps_HighSpeed   = (status & UPSF_PORT_HIGH_SPEED) ? TRUE : FALSE;
        hps->hps_LowSpeed    = (status & UPSF_PORT_LOW_SPEED) ? TRUE : FALSE;
        hps->hps_LinkState   = (status & UPSF_PORT_SUSPEND) ? UPS3_PLS_U3 : UPS3_PLS_U0;
    }
}
/* \\\ */

/* /// "nConfigurePort()" */
struct PsdDevice * GM_UNIQUENAME(nConfigurePort)(struct NepClassHubSS *nch, UWORD port)
{
    struct HubSSPortState hps;

    LONG ioerr;
    LONG delayretries;
    LONG resetretries;
    ULONG delaytime = 10;
    struct UsbPortStatus uhps;
    struct PsdDevice *pd;
    struct PsdPipe *pp;
    BOOL washighspeed = FALSE;
    BOOL islowspeed = FALSE;

    KPRINTF(1, ("%s(0x%p, %ld)\n", __func__, nch, port));

    uhps.wPortStatus = 0xDEAD;
    uhps.wPortChange = 0xDA1A;

    /* HUB class GET_STATUS: wValue must be 0, wIndex is the port number. */
    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_OTHER,
                 USR_GET_STATUS, 0, (ULONG)port);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, &uhps, sizeof(struct UsbPortStatus));

    uhps.wPortStatus = AROS_WORD2LE(uhps.wPortStatus);
    uhps.wPortChange = AROS_WORD2LE(uhps.wPortChange);

    if(!ioerr) {
        KPRINTF(2, ("Status 0x%04x, change 0x%04x\n", uhps.wPortStatus, uhps.wPortChange));

        GM_UNIQUENAME(nDecodePortStatus)(nch, uhps.wPortStatus, &hps);

        /*
         * An enabled USB2 port is disabled first so the device is addressed
         * from a known state. A SuperSpeed link cannot be treated that way:
         * disabling the port puts the link into Disabled, and a port reset
         * does not bring it back - the device stops answering entirely. A
         * port already reporting connected and enabled at SuperSpeed has
         * trained its own link and is ready to address as it stands, and
         * PORT_ENABLE is not even a feature such a hub accepts.
         */
        if(hps.hps_Enabled && !hps.hps_SuperSpeed) {
            KPRINTF(2, ("Disabling port %u\n", port));

            KPRINTF(1, ("%s: USR_CLEAR_FEATURE:UFS_PORT_ENABLE\n", __func__));
            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                         USR_CLEAR_FEATURE, UFS_PORT_ENABLE, (ULONG)port);
            ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
            if(ioerr) {
                psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                               "CLEAR_PORT_ENABLE failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                KPRINTF(1, ("CLEAR_PORT_ENABLE failed %ld.\n", ioerr));
            } else {
                psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                               "Disabling port %ld.", port);
            }
        }

        if(hps.hps_Connected) {
            KPRINTF(2, ("There's something at port %ld!\n", port));

            Forbid();
            if((pd = psdAllocDevice(nch->nch_Hardware))) {
                psdLockWriteDevice(pd);
                Permit();

                /* Hub reference */
                psdSetAttrs(PGA_DEVICE, pd,
                            DA_HubDevice, nch->nch_Device,
                            DA_IsConnected, TRUE,
                            DA_AtHubPortNumber, port,
                            TAG_END);

                if(hps.hps_LowSpeed) {
                    psdSetAttrs(PGA_DEVICE, pd, DA_IsLowspeed, TRUE, TAG_END);
                    KPRINTF(2, ("    It's a lowspeed device!\n"));
                    islowspeed = TRUE;
                }
                if(hps.hps_SuperSpeed) {
                    psdSetAttrs(PGA_DEVICE, pd, DA_IsSuperspeed, TRUE, TAG_END);
                    KPRINTF(2, ("    It's a superspeed device!\n"));
                }

                ObtainSemaphore(&nch->nch_HubBase->nh_Adr0Sema);

                /*
                 * A SuperSpeed port that already reads connected and enabled
                 * with its link in U0 has trained itself and left the device
                 * ready to address. The reset below belongs to USB2
                 * enumeration, where it is what makes a device answer at all;
                 * asking for it here only drives the port into reset, which
                 * it never leaves.
                 */
                if(hps.hps_SuperSpeed && hps.hps_Enabled &&
                   (hps.hps_LinkState == UPS3_PLS_U0))
                {
                    KPRINTF(2, ("    SuperSpeed link already up, skipping reset\n"));

                    GM_UNIQUENAME(nClearPortStatus)(nch, port);
                    psdDelayMS(100);

                    if((pp = psdAllocPipe(pd, nch->nch_TaskMsgPort, NULL))) {
                        if(psdEnumerateDevice(pp)) {
                            KPRINTF(2, ("  Device successfully added!\n"));
                            psdFreePipe(pp);
                            psdUnlockDevice(pd);
                            psdSendEvent(EHMB_ADDDEVICE, pd, NULL);
                            ReleaseSemaphore(&nch->nch_HubBase->nh_Adr0Sema);
                            return pd;
                        }
                        psdFreePipe(pp);
                    }
                    resetretries = 3;
                } else
                for(resetretries = 0; resetretries < 3; resetretries++) {
                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                 USR_SET_FEATURE, UFS_PORT_RESET, (ULONG)port);
                    ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);

                    if(ioerr) {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                                       "PORT_RESET for port %ld failed: %s (%ld)",
                                       port, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        KPRINTF(1, ("PORT_RESET failed %ld.\n", ioerr));
                        break;
                    }

                    if(nch->nch_IsRootHub) {
                        /* Root hubs need 50ms minimum delay */
                        psdDelayMS(50);
                    }

                    for(delayretries = 0; delayretries < 500; delayretries += delaytime) {
                        psdDelayMS(delaytime);

                        /* HUB class GET_STATUS: wValue must be 0, wIndex is the port number. */
                        psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_OTHER,
                                     USR_GET_STATUS, 0, (ULONG)port);
                        ioerr = psdDoPipe(nch->nch_EP0Pipe, &uhps, sizeof(struct UsbPortStatus));

                        uhps.wPortStatus = AROS_WORD2LE(uhps.wPortStatus);
                        uhps.wPortChange = AROS_WORD2LE(uhps.wPortChange);

                        if(ioerr) {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                                           "GET_PORT_STATUS for port %ld failed: %s (%ld)",
                                           port, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            KPRINTF(1, ("GET_PORT_STATUS failed %ld.\n", ioerr));
                            break;
                        }

                        KPRINTF(2, ("After reset: status 0x%04x, change 0x%04x\n",
                                    uhps.wPortStatus, uhps.wPortChange));

                        GM_UNIQUENAME(nDecodePortStatus)(nch, uhps.wPortStatus, &hps);

                        if(!hps.hps_Connected) {
                            break;
                        }

                        if(hps.hps_Connected && hps.hps_Enabled && hps.hps_Powered &&
                           !hps.hps_InReset && !hps.hps_OverCurrent &&
                           (!hps.hps_SuperSpeed || (hps.hps_LinkState == UPS3_PLS_U0)))
                        {
                            if(hps.hps_SuperSpeed) {
                                psdSetAttrs(PGA_DEVICE, pd, DA_IsSuperspeed, TRUE, TAG_END);
                                KPRINTF(2, ("    It's a superspeed device!\n"));
                            } else if(hps.hps_HighSpeed || washighspeed) {
                                psdSetAttrs(PGA_DEVICE, pd, DA_IsHighspeed, TRUE, TAG_END);
                                washighspeed = TRUE;
                                KPRINTF(2, ("    It's a highspeed device!\n"));
                            } else {
                                IPTR needssplit = 0;

                                /* Some hubs report speed correctly only after reset */
                                if(hps.hps_LowSpeed) {
                                    psdSetAttrs(PGA_DEVICE, pd, DA_IsLowspeed, TRUE, TAG_END);
                                    KPRINTF(2, ("    It's a lowspeed device!\n"));
                                    islowspeed = TRUE;
                                }

                                /* inherit needs split from hub */
                                psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                                            DA_NeedsSplitTrans, &needssplit, TAG_END);
                                KPRINTF(2, ("    Needs split transfers: %ld\n", needssplit));

                                psdSetAttrs(PGA_DEVICE, pd, DA_NeedsSplitTrans, needssplit, TAG_END);
                            }

                            GM_UNIQUENAME(nClearPortStatus)(nch, port);
                            psdDelayMS((ULONG)(islowspeed ? 1000 : 100));

                            if((pp = psdAllocPipe(pd, nch->nch_TaskMsgPort, NULL))) {
                                if(psdEnumerateDevice(pp)) {
                                    KPRINTF(2, ("  Device successfully added!\n"));
                                    psdFreePipe(pp);
                                    psdUnlockDevice(pd);
                                    psdSendEvent(EHMB_ADDDEVICE, pd, NULL);
                                    ReleaseSemaphore(&nch->nch_HubBase->nh_Adr0Sema);
                                    return pd;
                                }
                                psdFreePipe(pp);
                            }
                            break;
                        } else {
                            if(!hps.hps_InReset) {
                                psdAddErrorMsg(RETURN_ERROR, (STRPTR)libname,
                                               "Wrong port status %04lx for port %ld!",
                                               uhps.wPortStatus, port);
                                KPRINTF(2, ("Wrong port status %04lx for port %ld.\n",
                                            uhps.wPortStatus, port));
                            }
                        }

                        if(delayretries > 20) {
                            delaytime = 300;
                        }
                    }

                    /* A low speed device that will not come up is worth a
                       power cycle. The test reads USB 2.0 status bits and
                       clears PORT_ENABLE, neither of which a SuperSpeed hub
                       has, so it stays on the USB 2.0 path. */
                    if(!nch->nch_SSPortProto &&
                       (uhps.wPortStatus &
                        (UPSF_PORT_RESET|UPSF_PORT_CONNECTION|UPSF_PORT_ENABLE|
                         UPSF_PORT_POWER|UPSF_PORT_OVER_CURRENT|UPSF_PORT_LOW_SPEED))
                       == (UPSF_PORT_CONNECTION|UPSF_PORT_POWER|UPSF_PORT_LOW_SPEED))
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                                       "Strange port response, power-cycling port %ld", port);

                        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                     USR_CLEAR_FEATURE, UFS_PORT_ENABLE, (ULONG)port);
                        ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                        if(ioerr) {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                                           "CLEAR_PORT_ENABLE for port %ld failed: %s (%ld)",
                                           port, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            KPRINTF(1, ("CLEAR_PORT_ENABLE for port %ld failed %ld!\n", port, ioerr));
                        }
                        psdDelayMS(50);

                        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                     USR_CLEAR_FEATURE, UFS_PORT_POWER, (ULONG)port);
                        ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                        if(ioerr) {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                                           "CLEAR_PORT_POWER for port %ld failed: %s (%ld)",
                                           port, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            KPRINTF(1, ("CLEAR_PORT_POWER for port %ld failed %ld!\n", port, ioerr));
                        }
                        psdDelayMS(50);

                        psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                     USR_SET_FEATURE, UFS_PORT_POWER, (ULONG)port);
                        ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                        if(ioerr) {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                                           "SET_PORT_POWER for port %ld failed: %s (%ld)",
                                           port, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            KPRINTF(1, ("SET_PORT_POWER for port %ld failed %ld!\n", port, ioerr));
                        }
                        psdDelayMS((ULONG)nch->nch_PwrGoodTime + 15);
                    }

                    delaytime = 200;
                }

                psdUnlockDevice(pd);
                psdFreeDevice(pd);

                /*
                 * Disable port: keep misbehaving devices from keeping the
                 * bus wedged. Not on SuperSpeed - the hub stalls
                 * PORT_ENABLE and the stall halts its EP0, so the port
                 * status left behind by this failure never gets cleared
                 * and the hub stops reporting the port at all.
                 */
                if(!nch->nch_SSPortProto) {
                    KPRINTF(1, ("%s: USR_CLEAR_FEATURE:UFS_PORT_ENABLE for bad device\n", __func__));

                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                                 USR_CLEAR_FEATURE, UFS_PORT_ENABLE, (ULONG)port);
                    ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);
                    if(ioerr) {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                                       "CLEAR_PORT_ENABLE failed: %s (%ld)",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        KPRINTF(1, ("CLEAR_PORT_ENABLE failed %ld.\n", ioerr));
                    }
                }

                ReleaseSemaphore(&nch->nch_HubBase->nh_Adr0Sema);
                GM_UNIQUENAME(nClearPortStatus)(nch, port);
            } else {
                Permit();
                KPRINTF(1, ("AllocDevice() failed.\n"));
            }
        }
    } else {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR)libname,
                       "GET_PORT_STATUS failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        KPRINTF(1, ("GET_PORT_STATUS for port %d failed %ld.\n", port, ioerr));
    }

    return NULL;
}


/* /// "nHandleHubMethod()" */
void GM_UNIQUENAME(nHandleHubMethod)(struct NepClassHubSS *nch, struct NepHubSSMsg *nhm)
{
    ULONG num;
    struct PsdDevice *pd;

    nhm->nhm_Result = 0;

    switch(nhm->nhm_MethodID) {
        case UCM_HubClaimAppBinding:
            nhm->nhm_Result = (IPTR)psdHubClaimAppBindingA((struct TagItem *)nhm->nhm_Params[1]);
            break;

        case UCM_HubReleaseIfBinding:
            psdHubReleaseIfBinding((struct PsdInterface *)nhm->nhm_Params[1]);
            break;

        case UCM_HubReleaseDevBinding:
            psdHubReleaseDevBinding((struct PsdDevice *)nhm->nhm_Params[1]);
            break;

        case UCM_AttemptSuspendDevice: {
            BOOL res = TRUE;

            for(num = 1; num <= nch->nch_NumPorts; num++) {
                if((pd = (nch->nch_Downstream)[num-1])) {
                    res &= psdSuspendDevice(pd);
                }
            }

            if(res) {
                /* Suspending all downstream devices succeeded; stop hub activity too. */
                psdAbortPipe(nch->nch_EP1Pipe);
                nch->nch_Running = FALSE;
                nhm->nhm_Result = TRUE;
            }
            break;
        }

        case UCM_AttemptResumeDevice:
            if(!nch->nch_Running) {
                psdWaitPipe(nch->nch_EP1Pipe);
                psdSendPipe(nch->nch_EP1Pipe, nch->nch_PortChanges, (nch->nch_NumPorts+8)>>3);
                nch->nch_Running = TRUE;
            }

            nhm->nhm_Result = TRUE;

            for(num = 1; num <= nch->nch_NumPorts; num++) {
                if((pd = (nch->nch_Downstream)[num-1])) {
                    psdResumeDevice(pd);
                }
            }
            break;

        case UCM_HubSuspendDevice:
            nhm->nhm_Result = GM_UNIQUENAME(nHubSuspendDevice)(nch, (struct PsdDevice *)nhm->nhm_Params[1]);
            break;

        case UCM_HubResumeDevice:
            nhm->nhm_Result = GM_UNIQUENAME(nHubResumeDevice)(nch, (struct PsdDevice *)nhm->nhm_Params[1]);
            break;

        default:
            /* Unknown/unsupported method */
            nhm->nhm_Result = 0;
            break;
    }
}


/* /// "nHubSuspendDevice()" */
BOOL GM_UNIQUENAME(nHubSuspendDevice)(struct NepClassHubSS *nch, struct PsdDevice *pd)
{
    ULONG num;
    BOOL result = FALSE;
    LONG ioerr;

    /* Binding info is not used here; avoid unused-variable warnings. */
    {
        APTR binding = NULL;
        APTR puc = NULL;
        psdGetAttrs(PGA_DEVICE, pd, DA_Binding, &binding, DA_BindingClass, &puc, TAG_END);
        (void)binding;
        (void)puc;
    }

    for(num = 1; num <= nch->nch_NumPorts; num++) {
        if(pd == (nch->nch_Downstream)[num-1]) {
            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                         USR_SET_FEATURE, UFS_PORT_SUSPEND, (ULONG)num);
            ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);

            if(ioerr) {
                psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                               "SET_PORT_SUSPEND failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                KPRINTF(1, ("SET_PORT_SUSPEND failed %ld.\n", ioerr));
            } else {
                result = TRUE;
                psdSetAttrs(PGA_DEVICE, pd, DA_IsSuspended, TRUE, TAG_END);
                psdSendEvent(EHMB_DEVSUSPENDED, pd, NULL);
            }
            break; /* done */
        }
    }

    return result;
}


/* /// "nHubResumeDevice()" */
BOOL GM_UNIQUENAME(nHubResumeDevice)(struct NepClassHubSS *nch, struct PsdDevice *pd)
{
    ULONG num;
    BOOL result = FALSE;
    LONG ioerr;

    for(num = 1; num <= nch->nch_NumPorts; num++) {
        if(pd == (nch->nch_Downstream)[num-1]) {
            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_OTHER,
                         USR_CLEAR_FEATURE, UFS_PORT_SUSPEND, (ULONG)num);
            ioerr = psdDoPipe(nch->nch_EP0Pipe, NULL, 0);

            if(ioerr) {
                psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                               "CLEAR_PORT_SUSPEND failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                KPRINTF(1, ("CLEAR_PORT_SUSPEND failed %ld.\n", ioerr));
            } else {
                psdSetAttrs(PGA_DEVICE, pd, DA_IsSuspended, FALSE, TAG_END);
                psdSendEvent(EHMB_DEVRESUMED, pd, NULL);
                result = TRUE;
                psdDelayMS(30);
            }
            break; /* done */
        }
    }

    return result;
}
