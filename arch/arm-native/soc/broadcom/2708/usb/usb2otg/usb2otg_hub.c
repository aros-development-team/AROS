/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <devices/usb_hub.h>

#include <string.h>

#include "usb2otg_intern.h"
#include "usb2otg_hub.h"

WORD FNAME_ROOTHUB(cmdControlXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    UWORD rt = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);
    BOOL cmdgood;
    //ULONG cnt;

    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER(%ld:%ld)\n", rt, req));

#if defined(OTG_FORCEHOSTMODE)
    if (ioreq->iouh_Endpoint)
    {
        return (UHIOERR_STALL);
    }
#endif

    if (len != ioreq->iouh_Length)
    {
        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: IOReq Len mismatch! %ld != %ld\n", len, ioreq->iouh_Length));
        return (UHIOERR_STALL);
    }

    switch (rt)
    {
        case (URTF_STANDARD|URTF_DEVICE):
            switch (req)
            {
                case USR_SET_ADDRESS:
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Set Device Address to #%ld\n", val));
                    otg_Unit->hu_HubAddr = val;
                    ioreq->iouh_Actual = len;
                    return (0);

                case USR_SET_CONFIGURATION:
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Set Device Configuration to #%ld\n", val));
                    ioreq->iouh_Actual = len;
                    return (0);
            }
            break;

        case (URTF_IN|URTF_STANDARD|URTF_DEVICE):
            switch (req)
            {
                case USR_GET_STATUS:
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetStatus (%ld)\n", len));
                    if (len == 2)
                    {
                        UWORD *mptr = ioreq->iouh_Data;
                        *mptr++ = AROS_WORD2LE(U_GSF_SELF_POWERED);
                        return (0);
                    }
                    break;

                case USR_GET_DESCRIPTOR:
                    switch (val >> 8)
                    {
                        case UDT_DEVICE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetDeviceDescriptor (%ld)\n", len));
                            ioreq->iouh_Actual = (len > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : len;
                            CopyMem((APTR)&OTGRootHubDevDesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                            return (0);

                        case UDT_CONFIGURATION:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetConfigDescriptor (%ld)\n", len));
                            ioreq->iouh_Actual = (len > sizeof(struct OTGHubCfg)) ? sizeof(struct OTGHubCfg) : len;
                            CopyMem((APTR)&OTGRootHubCfg, ioreq->iouh_Data, ioreq->iouh_Actual);

                            return (0);

                        case UDT_STRING:
                            if (val & 0xFF) /* get lang array */
                            {
                                CONST_STRPTR source = NULL;
                                UWORD *mptr = ioreq->iouh_Data;
                                UWORD slen;
                                D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetString %04lx (%ld)\n", val, len));
                                if ((val & 0xFF) > 4) /* index too high? */
                                {
                                    return (UHIOERR_STALL);
                                }

                                source = OTGRootHubStrings[(val & 0xFF) - 1];
                                slen = strlen(source);

                                if (len > 1)
                                {
                                    ioreq->iouh_Actual = 2;
                                    *mptr++ = AROS_WORD2BE(((slen + 1) << 9)|UDT_STRING);
                                    /* "expand" string to utf16 */
                                    while ((ioreq->iouh_Actual + 1) < len)
                                    {
                                        *mptr++ = AROS_WORD2LE(*source++);
                                        ioreq->iouh_Actual += 2;
                                        if (!(*source))
                                        {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                UWORD *mptr = ioreq->iouh_Data;
                                D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetLangArray %04lx (%ld)\n", val, len));
                                if (len > 1)
                                {
                                   ioreq->iouh_Actual = 2;
                                   mptr[0] = AROS_WORD2BE((4 << 8)|UDT_STRING);
                                   if (len > 3)
                                   {
                                      ioreq->iouh_Actual += 2;
                                      mptr[1] = AROS_WORD2LE(0x0409);
                                   }
                                }
                            }
                            return (0);

                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Unsupported Descriptor %04lx\n", idx));
                            break;
                    }
                    break;

                case USR_GET_CONFIGURATION:
                    if (len == 1)
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetConfiguration\n"));
                        ((UBYTE *) ioreq->iouh_Data)[0] = 1; // TODO: Expose 3 configurations? 1 = Host + Device, 2 = Host Only, 3 = Device Only?
                        ioreq->iouh_Actual = len;
                        return (0);
                    }
                    break;
            }
            break;

        case (URTF_CLASS|URTF_OTHER):
        {
            switch (req)
            {
                case USR_SET_FEATURE:
                {
                    if ((!idx) || (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }

                    cmdgood = FALSE;

                    unsigned int otg_RegVal;

                    switch (val)
                    {
                        case UFS_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld enable requested although OTG cannot do that!\n", idx));
                            break;

                        case UFS_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Suspending Port #%ld\n", idx));

                            usb2otg_hostport_rmw(USB2OTG_HOSTPORT_PRTSUSP, 0);
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_RESET:
                            {
                                struct MsgPort *port = CreateMsgPort();
                                struct timerequest *req = (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest));
                                OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) req, 0);

                                D(bug("[USB2OTG:Hub] === PORT RESET START ===\n"));
                                D(bug("[USB2OTG:Hub] HOSTPORT before reset=%08x\n", rd32le(USB2OTG_HOSTPORT)));

                                req->tr_node.io_Command = TR_ADDREQUEST;
                                req->tr_time.tv_secs = 0;
                                req->tr_time.tv_micro = 10000;
                                DoIO((struct IORequest *)req);

                                D(bug("[USB2OTG:Hub] Reset: asserting PRTRST\n"));
                                usb2otg_hostport_rmw(USB2OTG_HOSTPORT_PRTRST, 0);
                                D(bug("[USB2OTG:Hub] Reset: asserted PRTRST, HOSTPORT now=%08x\n", rd32le(USB2OTG_HOSTPORT)));

                                req->tr_time.tv_secs = 0;
                                req->tr_time.tv_micro = 60000;
                                D(bug("[USB2OTG:Hub] Reset: before DoIO(60ms)\n"));
                                DoIO((struct IORequest *)req);
                                D(bug("[USB2OTG:Hub] Reset: after DoIO(60ms)\n"));

                                D(bug("[USB2OTG:Hub] Reset: deasserting PRTRST\n"));
                                usb2otg_hostport_rmw(0, USB2OTG_HOSTPORT_PRTRST);
                                D(bug("[USB2OTG:Hub] Reset: deasserted PRTRST, HOSTPORT now=%08x\n", rd32le(USB2OTG_HOSTPORT)));

                                /* 200 ms settle after PRTRST deassert (SMSC LAN7515 on Pi 3B+). */
                                req->tr_time.tv_secs = 0;
                                req->tr_time.tv_micro = 200000;
                                D(bug("[USB2OTG:Hub] Reset: before DoIO(200ms)\n"));
                                DoIO((struct IORequest *)req);
                                D(bug("[USB2OTG:Hub] Reset: after DoIO(200ms)\n"));

                                otg_RegVal = rd32le(USB2OTG_HOSTPORT);
                                D(bug("[USB2OTG:Hub] Port reset complete, HOSTPORT=%08x\n", otg_RegVal));

                                /*
                                 * Synthesize a root-hub status change so the hub
                                 * class wakes any queued INT xfer; the SW reset
                                 * path produces no port-change IRQ on its own.
                                 */
                                Disable();
#if defined(__AROSEXEC_SMP__)
                                KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
                                otg_Unit->hu_HubPortChanged = TRUE;
#if defined(__AROSEXEC_SMP__)
                                KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                                Enable();
                                D(bug("[USB2OTG:Hub] Reset: HubPortChanged=TRUE, causing pending INT\n"));
                                FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);
                                D(bug("[USB2OTG:Hub] Reset: Cause returned, cleaning up timer\n"));

                                CloseDevice((struct IORequest *)req);
                                DeleteIORequest((struct IORequest *)req);
                                DeleteMsgPort(port);
                                D(bug("[USB2OTG:Hub] Reset: cleanup done, returning success\n"));

                                cmdgood = TRUE;
                            }
                            break;

                        case UFS_PORT_POWER:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Powering Port #%ld\n", idx));

                            usb2otg_hostport_rmw(USB2OTG_HOSTPORT_PRTPWR, 0);
                            cmdgood = TRUE;

                            break;

                        /*
                            case UFS_PORT_LOW_SPEED:
                            case UFS_C_PORT_CONNECTION:
                            case UFS_C_PORT_OVER_CURRENT:
                        */
                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_SET_FEATURE - Unhandled feature %ld for Port #%ld\n", val, idx));
                            break;
                    }
                    if (cmdgood)
                    {
                        D(bug("[USB2OTG:Hub] SET_FEATURE: returning 0 (val=%ld)\n", val));
                        return (0);
                    }

                    break;
                }

                case USR_CLEAR_FEATURE:
                {
                    if ((!idx) || (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }

                    cmdgood = FALSE;

                    ULONG oldval = rd32le(USB2OTG_HOSTPORT) & ~USB2OTG_HOSTPORT_SC_BITS;
                    ULONG newval = oldval;

                    switch (val)
                    {
                        case UFS_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Enable\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTENA;
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Suspend\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTRES;
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_POWER:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Power\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTPWR;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_CONNECTION:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Connect Change\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTCONNDET;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Enable Change\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTENCHNG;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Suspend Change\n", idx));

//                            newval |= USB2OTG_HOSTPORT_PRTRES;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_OVER_CURRENT:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Over-Current Change\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTOVRCURRCHNG;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_RESET:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Reset Change\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTRST;
                            cmdgood = TRUE;

                            break;

                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE - Unhandled feature %ld for Port #%ld\n", val, idx));
                            break;
                    }
                    if (cmdgood)
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld CLEAR_FEATURE %04lx->%04lx\n", idx, oldval, newval));
                        wr32le(USB2OTG_HOSTPORT, newval);
                        ioreq->iouh_Actual = 0;

                        /*
                         * Only signal port change if HW change bits remain
                         * after clearing this one; otherwise the hub class
                         * spins polling.
                         */
                        {
                            ULONG postval = rd32le(USB2OTG_HOSTPORT);
                            Disable();
#if defined(__AROSEXEC_SMP__)
                            KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
                            if (postval & (USB2OTG_HOSTPORT_PRTCONNDET |
                                           USB2OTG_HOSTPORT_PRTENCHNG |
                                           USB2OTG_HOSTPORT_PRTOVRCURRCHNG))
                            {
                                otg_Unit->hu_HubPortChanged = TRUE;
                            }
#if defined(__AROSEXEC_SMP__)
                            KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                            Enable();
                        }

                        return (0);
                    }
                    break;
                }
            }
            break;
        }

        case (URTF_IN|URTF_CLASS|URTF_OTHER):
        {
            switch (req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Get Port #%ld Status..\n", idx));

                    if (len != sizeof(struct UsbPortStatus))
                    {
                        return (UHIOERR_STALL);
                    }
                    if ((!idx) || (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }

                    ULONG oldval = rd32le(USB2OTG_HOSTPORT);

                    D(bug("[USB2OTG:Hub] GET_PORT_STATUS: HOSTPORT=%08x\n", oldval));

                    *mptr = 0;
                    if (oldval & USB2OTG_HOSTPORT_PRTPWR)        *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
                    if (oldval & USB2OTG_HOSTPORT_PRTENA)        *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                    if (oldval & USB2OTG_HOSTPORT_PRTCONNSTS)    *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                    if (oldval & USB2OTG_HOSTPORT_PRTRST)        *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
                    if (oldval & USB2OTG_HOSTPORT_PRTSUSP)       *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

                    switch (USB2OTG_HOSTPORT_PRTSPD(oldval))
                    {
                        case USB2OTG_HOSTPORT_PRTSPD_HS_VAL:
                            *mptr |= AROS_WORD2LE(UPSF_PORT_HIGH_SPEED);
                            break;
                        case USB2OTG_HOSTPORT_PRTSPD_LS_VAL:
                            *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                            break;
                        default:
                            break;
                    }

                    D(bug("[USB2OTG:Hub] Port #%ld Status word=%04lx (%s)\n", idx, AROS_LE2WORD(*mptr),
                        USB2OTG_HOSTPORT_PRTSPD(oldval) == USB2OTG_HOSTPORT_PRTSPD_HS_VAL ? "HIGH" :
                        USB2OTG_HOSTPORT_PRTSPD(oldval) == USB2OTG_HOSTPORT_PRTSPD_FS_VAL ? "FULL" : "LOW"));

                    mptr++;
                    *mptr = 0;
                    if (oldval & USB2OTG_HOSTPORT_PRTENCHNG)    *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                    if (oldval & USB2OTG_HOSTPORT_PRTCONNDET)   *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                    if (oldval & USB2OTG_HOSTPORT_PRTRES)       *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE);

                    D(bug("[USB2OTG:Hub] GET_PORT_STATUS: status=%04x change=%04x\n",
                        AROS_LE2WORD(*(mptr-1)), AROS_LE2WORD(*mptr)));

                    ioreq->iouh_Actual = sizeof(struct UsbPortStatus);
                    return (0);
                }
            }
            break;
        }

        case (URTF_IN|URTF_CLASS|URTF_DEVICE):
        {
            switch (req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetHubStatus (%ld)\n", len));

                    if (len < sizeof(struct UsbHubStatus))
                    {
                        return(UHIOERR_STALL);
                    }

                    *mptr++ = 0;
                    *mptr++ = 0;
                    ioreq->iouh_Actual = 4;
                    return (0);
                }

                case USR_GET_DESCRIPTOR:
                {
                    switch (val >> 8)
                    {
                        case UDT_HUB:
                        {
                            ULONG hubdesclen = 9;
                            //ULONG powergood = 1;

                            struct UsbHubDesc *uhd = (struct UsbHubDesc *) ioreq->iouh_Data;
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetHubDescriptor (%ld)\n", len));

                            ioreq->iouh_Actual = (len > hubdesclen) ? hubdesclen : len;
                            CopyMem((APTR)&OTGRootHubDesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                            if (ioreq->iouh_Length)
                            {
                                uhd->bLength = hubdesclen;
                            }

                            if (ioreq->iouh_Length >= hubdesclen)
                            {
                                if (hubdesclen == 9)
                                {
                                    uhd->DeviceRemovable = 0;
                                    uhd->PortPwrCtrlMask = (1 << 3) - 2;
                                } else {
                                    // each field is 16 bits wide
                                    uhd->DeviceRemovable = 0;
                                    uhd->PortPwrCtrlMask = 0;
                                    ((UBYTE *)ioreq->iouh_Data)[9] = (1 << 3) - 2;
                                    ((UBYTE *)ioreq->iouh_Data)[10] = ((1 << 3) - 2) >> 8;
                                }
                            }
                            return (0);
                        }

                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Unsupported Descriptor %04lx\n", idx));
                            break;
                    }
                    break;
                }
            }
        }
    }

    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Unsupported command %02lx %02lx %04lx %04lx %04lx!\n", rt, req, idx, val, len));

    return (UHIOERR_STALL);
}

WORD FNAME_ROOTHUB(cmdIntXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    ULONG cpu = KrnGetCPUNumber();

    D(bug("[USB2OTG:Hub] INTXFER: ep=%d len=%d changed=%d cpu=%lu\n",
        ioreq->iouh_Endpoint, ioreq->iouh_Length, otg_Unit->hu_HubPortChanged, cpu));
    if ((ioreq->iouh_Endpoint != 1) || (!ioreq->iouh_Length))
    {
        return UHIOERR_STALL;
    }

    /* PendingIO softint also touches these; serialize with Disable + lock. */
    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    if (otg_Unit->hu_HubPortChanged)
    {
        D(bug("[USB2OTG:Hub] INTXFER: Immediate Portchange reply\n"));

        /* Bit 1 = port 1 status changed (bit 0 would be hub status) */
        if (ioreq->iouh_Length == 1)
        {
            *((UBYTE *) ioreq->iouh_Data) = 2;
            ioreq->iouh_Actual = 1;
        }
        else
        {
            ((UBYTE *) ioreq->iouh_Data)[0] = 2;
            ((UBYTE *) ioreq->iouh_Data)[1] = 0;
            ioreq->iouh_Actual = 2;
        }
        otg_Unit->hu_HubPortChanged = FALSE;
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
        Enable();

        return 0;
    }

    D(bug("[USB2OTG:Hub] INTXFER: Queueing request\n"));

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    AddTail(&otg_Unit->hu_IOPendingQueue, (struct Node *)ioreq);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
    Enable();

    return RC_DONTREPLY;
}

void FNAME_ROOTHUB(PendingIO)(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    struct IOUsbHWReq *ioreq;

    for (;;)
    {
        Disable();
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
        if (!(otg_Unit->hu_HubPortChanged && otg_Unit->hu_IOPendingQueue.lh_Head->ln_Succ))
        {
#if defined(__AROSEXEC_SMP__)
            KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
            Enable();
            break;
        }
        D(bug("[USB2OTG:Hub] PendingIO: replying to queued INT because port changed\n"));
        ioreq = (struct IOUsbHWReq *) otg_Unit->hu_IOPendingQueue.lh_Head;
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        if (!otg_Unit->hu_IOPendingQueue.lh_Head->ln_Succ)
        {
            otg_Unit->hu_HubPortChanged = FALSE;
        }
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
        Enable();

        /* Bit 1 = port 1 status changed (bit 0 would be hub status) */
        if (ioreq->iouh_Length == 1)
        {
            *((UBYTE *) ioreq->iouh_Data) = 2;
            ioreq->iouh_Actual = 1;
        } else {
            ((UBYTE *) ioreq->iouh_Data)[0] = 2;
            ((UBYTE *) ioreq->iouh_Data)[1] = 0;
            ioreq->iouh_Actual = 2;
        }
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}
