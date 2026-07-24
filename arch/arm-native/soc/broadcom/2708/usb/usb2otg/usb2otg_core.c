/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "usb2otg_intern.h"

static const UWORD __suported_cmds[] =
{
    CMD_FLUSH,
    CMD_RESET,
    UHCMD_QUERYDEVICE,
    UHCMD_USBRESET,
    UHCMD_USBRESUME,
    UHCMD_USBSUSPEND,
    UHCMD_USBOPER,
    UHCMD_CONTROLXFER,
    UHCMD_ISOXFER,
    UHCMD_INTXFER,
    UHCMD_BULKXFER,
    NSCMD_DEVICEQUERY,
    0
};

struct Unit * FNAME_DEV(OpenUnit)(struct IOUsbHWReq *ioreq,
                        LONG unitnr,
                        LIBBASETYPEPTR USB2OTGBase)
{
    struct USB2OTGUnit *otg_Unit = NULL;
    volatile unsigned int        otg_RegVal;
    unsigned int chan, ns;

    D(bug("[USB2OTG] %s(ioreq:0x%p, unit#%d)\n",
                __PRETTY_FUNCTION__, ioreq, unitnr));

    // We only support a single unit presently
    if ((unitnr == 0) && (USB2OTGBase->hd_Unit))
    {
        otg_Unit = USB2OTGBase->hd_Unit;
        if (!(otg_Unit->hu_UnitAllocated))
        {
            otg_Unit->hu_UnitAllocated = TRUE;

            D(bug("[USB2OTG] %s: Enabling Power ..\n", __PRETTY_FUNCTION__));
            wr32le(USB2OTG_POWER, 0);

            /* Save firmware GUSBCFG before reset to restore PHY settings. */
            ULONG saved_gusbcfg = rd32le(USB2OTG_USB);

            /* Step 1: Core soft reset (wait AHB IDLE -> CSFTRST -> wait clear). */
            bug("[USB2OTG] Init: Core soft reset\n");
            usb2otg_wait_ahb_idle(100000, "before core soft reset");
            wr32le(USB2OTG_RESET, USB2OTG_RESET_CORESOFT);
            usb2otg_wait_reset_bit_clear(USB2OTG_RESET_CORESOFT, 1000000,
                "Init: Core soft reset");
            /* Wait for internal logic to settle after core reset */
            {
                volatile int i;
                for (i = 0; i < 100000; i++)
                    asm volatile("yield\n");
            }

            /*
             * Step 2: Re-enable DMA + global IRQs (cleared by core reset).
             * NPTxFEmpLvl=1 → NPTXFEMP only fires when fully drained, so
             * OUT-arming code doesn't queue while stale data is in FIFO.
             */
            otg_RegVal = rd32le(USB2OTG_AHB);
            otg_RegVal |= USB2OTG_AHB_DMAENABLE
                        | USB2OTG_AHB_AXIBURSTLENGTH
                        | USB2OTG_AHB_INTENABLE
                        | USB2OTG_AHB_TRANSFEREMPTYLEVEL;  /* NPTxFEmpLvl = fully empty */
            wr32le(USB2OTG_AHB, otg_RegVal);

            /* Step 3: Restore GUSBCFG with force-host bit set. */
            saved_gusbcfg &= ~USB2OTG_USB_FORCE_DEV_MODE;
            saved_gusbcfg |= USB2OTG_USB_FORCE_HOST_MODE;
            wr32le(USB2OTG_USB, saved_gusbcfg);
            /* 25 ms for mode switch — ~30M NOPs on 1.2 GHz Cortex-A53. */
            {
                volatile int i;
                for (i = 0; i < 30000000; i++)
                    asm volatile("yield\n");
            }

            /* Step 4: Configure HOSTCFG (FSLSPCLKSEL) after core reset. */
            otg_RegVal = rd32le(USB2OTG_HARDWARE2);
            if (USB2OTG_HW2_FSPHY_TYPE(otg_RegVal) == USB2OTG_HW2_FSPHY_TYPE_ULPI &&
                USB2OTG_HW2_HSPHY_TYPE(otg_RegVal) == USB2OTG_HW2_HSPHY_TYPE_UTMI)
            {
                otg_RegVal = rd32le(USB2OTG_USB);
                if (otg_RegVal & USB2OTG_USB_ULPIFSLS)
                {
                    bug("[USB2OTG] Host clock: 48Mhz\n");
                    otg_RegVal = rd32le(USB2OTG_HOSTCFG);
                    otg_RegVal &= ~3;
                    otg_RegVal |= 1;
                    wr32le(USB2OTG_HOSTCFG, otg_RegVal);
                }
                else
                {
                    bug("[USB2OTG] Host clock: 30-60Mhz (ULPI)\n");
                    otg_RegVal = rd32le(USB2OTG_HOSTCFG);
                    otg_RegVal &= ~3;
                    wr32le(USB2OTG_HOSTCFG, otg_RegVal);
                }
            } else {
                bug("[USB2OTG] Host clock: 30-60Mhz (default)\n");
                otg_RegVal = rd32le(USB2OTG_HOSTCFG);
                otg_RegVal &= ~3;
                wr32le(USB2OTG_HOSTCFG, otg_RegVal);
            }

            /* Step 5: HFIR = 60000 for 30-60 MHz PHY clock. */
            {
                ULONG hfir = rd32le(USB2OTG_HOSTFRAMEINTERV);
                hfir &= ~0xffff;
                hfir |= 60000;
                wr32le(USB2OTG_HOSTFRAMEINTERV, hfir);
            }

            /*
             * Step 6: Enable SOF + HOSTCHANNEL IRQs (GINTMSK cleared by
             * reset). SOF drives frame scheduling; HOSTCHANNEL drives
             * per-channel completion/error.
             */
            wr32le(USB2OTG_INTR, 0xffffffff); /* clear pending */
            otg_RegVal = USB2OTG_INTRCORE_DMASTARTOFFRAME |
                         USB2OTG_INTRCORE_HOSTCHANNEL;
            wr32le(USB2OTG_INTRMASK, otg_RegVal);

            /*
             * BCM2835/2837 DWC OTG SPRAM layout: RXFIFO 774 DW @ 0,
             * NPTXFIFO 256 DW @ 774, PTXFIFO 512 DW @ 1030.
             */
            wr32le(USB2OTG_RCVSIZE, 774);
            wr32le(USB2OTG_NONPERIFIFOSIZE, (256 << 16) | 774);
            wr32le(USB2OTG_PERIFIFOSIZE, (512 << 16) | (774 + 256));

            usb2otg_wait_ahb_idle(100000, "before FIFO flush");

            D(bug("[USB2OTG] %s: Flushing Tx Fifo's...\n", __PRETTY_FUNCTION__));
            wr32le(USB2OTG_RESET, USB2OTG_RESET_TXFIFOFLUSH | (0x10 << 6));
            usb2otg_wait_reset_bit_clear(USB2OTG_RESET_TXFIFOFLUSH, 100000,
                "Init: Tx Flush");

            D(bug("[USB2OTG] %s: Flushing Rx Fifo's...\n", __PRETTY_FUNCTION__));
            wr32le(USB2OTG_RESET, USB2OTG_RESET_RXFIFOFLUSH);
            usb2otg_wait_reset_bit_clear(USB2OTG_RESET_RXFIFOFLUSH, 100000,
                "Init: Rx Flush");

            otg_RegVal = rd32le(USB2OTG_HARDWARE2);
            D(bug("[USB2OTG] %s: Queue Depths:\n",
                        __PRETTY_FUNCTION__));
            D(bug("[USB2OTG] %s:      Periodic Transmit: 0x%0x\n",
                        __PRETTY_FUNCTION__, USB2OTG_HW2_PTXQDEPTH(otg_RegVal)));
            D(bug("[USB2OTG] %s:      Non-Periodic Transmit: 0x%0x\n",
                        __PRETTY_FUNCTION__, USB2OTG_HW2_NPTXQDEPTH(otg_RegVal)));

            {
                ULONG hw3 = rd32le(USB2OTG_HARDWARE3);
                /* HW3[31:16] = total FIFO depth in DWORDs; *4 for bytes. */
                D(bug("[USB2OTG] %s:      FIFO: %ld bytes\n",
                            __PRETTY_FUNCTION__, ((hw3 >> 16) & 0xFFFF) << 2));
            }

            otg_RegVal = rd32le(USB2OTG_HARDWARE2);
            if ((otg_Unit->hu_HostChans = USB2OTG_HW2_HOSTCHANS(otg_RegVal)) > EPSCHANS_MAX)
                otg_Unit->hu_HostChans = EPSCHANS_MAX;

            otg_RegVal = rd32le(USB2OTG_HOSTCFG);
            if ((otg_RegVal & (1 << 23)) == 0)
            {
                D(bug("[USB2OTG] %s: Host Channels: %d\n",
                            __PRETTY_FUNCTION__, otg_Unit->hu_HostChans));

                /*
                 * Two-phase channel halt per Synopsys DWC OTG Programming
                 * Guide: 1) disable active channels, 2) force-halt all.
                 */
                bug("[USB2OTG] Init: Halting %d host channels\n", otg_Unit->hu_HostChans);

                /* Phase 1: Disable channels */
                for (chan = 0; chan < otg_Unit->hu_HostChans; chan++) {
                    otg_RegVal = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                    /* Clear enable, clear direction, set disable */
                    otg_RegVal &= ~(USB2OTG_HOSTCHAR_ENABLE | USB2OTG_HOSTCHAR_EPDIR(1));
                    otg_RegVal |= USB2OTG_HOSTCHAR_DISABLE;
                    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), otg_RegVal);
                }

                /* Phase 2: Force-halt all channels */
                for (chan = 0; chan < otg_Unit->hu_HostChans; chan++) {
                    otg_RegVal = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                    otg_RegVal |= USB2OTG_HOSTCHAR_ENABLE | USB2OTG_HOSTCHAR_DISABLE;
                    otg_RegVal &= ~USB2OTG_HOSTCHAR_EPDIR(1);
                    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), otg_RegVal);

                    /* Wait for channel to halt (ENABLE bit clears) */
                    {
                        int timeout = 100000;
                        while ((rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE) && --timeout > 0)
                            asm volatile("yield\n");
                        if (timeout == 0)
                            bug("[USB2OTG] Init: Channel #%d halt timed out!\n", chan);
                    }

                    /* Clear all pending interrupts on this channel */
                    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);
                }

            }


            {
                struct MsgPort *port = CreateMsgPort();
                struct timerequest *req = (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest));
                OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) req, 0);

                otg_RegVal = rd32le(USB2OTG_HOSTPORT);
                if (!(otg_RegVal & USB2OTG_HOSTPORT_PRTPWR))
                {
                    bug("[USB2OTG] Init: Powering On Host Port\n");
                    otg_RegVal &= ~USB2OTG_HOSTPORT_SC_BITS;
                    otg_RegVal |= USB2OTG_HOSTPORT_PRTPWR;
                    wr32le(USB2OTG_HOSTPORT, otg_RegVal);
                }

                req->tr_node.io_Command = TR_ADDREQUEST;
                req->tr_time.tv_secs = 0;
                req->tr_time.tv_micro = 100000;
                DoIO((struct IORequest *)req);

                otg_RegVal = rd32le(USB2OTG_HOSTPORT);
                if (otg_RegVal & USB2OTG_HOSTPORT_PRTCONNSTS)
                {
                    bug("[USB2OTG] Init: Device connected, resetting port\n");

                    req->tr_time.tv_secs = 0;
                    req->tr_time.tv_micro = 100000;
                    DoIO((struct IORequest *)req);

                    otg_RegVal = rd32le(USB2OTG_HOSTPORT);
                    otg_RegVal &= ~USB2OTG_HOSTPORT_SC_BITS;
                    otg_RegVal |= USB2OTG_HOSTPORT_PRTRST;
                    wr32le(USB2OTG_HOSTPORT, otg_RegVal);

                    req->tr_time.tv_secs = 0;
                    req->tr_time.tv_micro = 60000;
                    DoIO((struct IORequest *)req);

                    otg_RegVal = rd32le(USB2OTG_HOSTPORT);
                    otg_RegVal &= ~(USB2OTG_HOSTPORT_SC_BITS | USB2OTG_HOSTPORT_PRTRST);
                    wr32le(USB2OTG_HOSTPORT, otg_RegVal);

                    req->tr_time.tv_secs = 0;
                    req->tr_time.tv_micro = 20000;
                    DoIO((struct IORequest *)req);

                    otg_RegVal = rd32le(USB2OTG_HOSTPORT);

                    /* Signal port change so hub class INT EP returns immediately. */
                    if (otg_RegVal & USB2OTG_HOSTPORT_PRTCONNSTS)
                    {
                        otg_Unit->hu_HubPortChanged = TRUE;
                        D(bug("[USB2OTG] Init: HubPortChanged set to TRUE\n"));
                    }
                }
                else
                {
                    bug("[USB2OTG] Init: No device connected on port!\n");
                }

                CloseDevice((struct IORequest *)req);
                DeleteIORequest((struct IORequest *)req);
                DeleteMsgPort(port);
            }
            /* Don't double-start NakTimeoutInt — creates overlapping chains. */
            // Cause(&otg_Unit->hu_NakTimeoutInt);
        }

        otg_RegVal = rd32le(USB2OTG_OTGCTRL);
        D(bug("[USB2OTG] %s: OTG Control: %08x\n",
                    __PRETTY_FUNCTION__, otg_RegVal));
        return (&otg_Unit->hu_Unit);
    }

    return (NULL);
}

void FNAME_DEV(CloseUnit)(struct IOUsbHWReq *ioreq, struct USB2OTGUnit *otg_Unit, LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] %s(unit:0x%p, ioreq:0x%p)\n",
                __PRETTY_FUNCTION__, otg_Unit, ioreq));

    otg_Unit->hu_UnitAllocated = FALSE;
}

void FNAME_DEV(TermIO)(struct IOUsbHWReq *ioreq,
            LIBBASETYPEPTR USB2OTGBase)
{
    /*
     * USB §9.4.5: CLEAR_FEATURE(ENDPOINT_HALT) resets device toggle to
     * DATA0; host must mirror or every subsequent xfer hits DATATGLERR.
     * MSC BOT reset-recovery depends on this.
     */
    if (ioreq->iouh_Req.io_Error == 0 &&
        ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
        ioreq->iouh_SetupData.bmRequestType ==
            (URTF_OUT | URTF_STANDARD | URTF_ENDPOINT) &&
        ioreq->iouh_SetupData.bRequest == USR_CLEAR_FEATURE &&
        AROS_LE2WORD(ioreq->iouh_SetupData.wValue) == UFS_ENDPOINT_HALT)
    {
        struct USB2OTGUnit *otg_Unit = USB2OTGBase->hd_Unit;
        UWORD widx = AROS_LE2WORD(ioreq->iouh_SetupData.wIndex);
        UBYTE ep = (UBYTE)(widx & 0x0f);
        UBYTE dev = ioreq->iouh_DevAddr;

        if (otg_Unit != NULL)
        {
            Disable();
#if defined(__AROSEXEC_SMP__)
            KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
            otg_Unit->hu_PIDBits[dev & 0x7f] &= ~(3UL << (2 * ep));
            otg_Unit->hu_NakGate[dev & 0x7f] = USB2OTG_NAK_GATE_NONE;
            /* Also clear PING flow bit; stale PING state survives BOT
             * Reset Recovery and re-wedges on the next CBW. */
            otg_Unit->hu_PingBits[dev & 0x7f] &= ~(1UL << ep);
#if defined(__AROSEXEC_SMP__)
            KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
            Enable();
        }
    }

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    if (!(ioreq->iouh_Req.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}

WORD FNAME_DEV(cmdNSDeviceQuery)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    struct USBNSDeviceQueryResult *query = (struct USBNSDeviceQueryResult *)ioreq->io_Data;

    D(bug("[USB2OTG] NSCMD_DEVICEQUERY(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if ((!query) ||
       (ioreq->io_Length < sizeof(struct USBNSDeviceQueryResult)) ||
       (query->DevQueryFormat != 0) ||
       (query->SizeAvailable != 0))
    {
        ioreq->io_Error = IOERR_NOCMD;
        FNAME_DEV(TermIO)((struct IOUsbHWReq *) ioreq, USB2OTGBase);

        return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct USBNSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_USBHARDWARE;
    query->DeviceSubType     = 0;
    query->SupportedCommands = __suported_cmds;

    return RC_OK;
}

WORD FNAME_DEV(cmdQueryDevice)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    D(bug("[USB2OTG] UHCMD_QUERYDEVICE(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (((tag = FindTagItem(UHA_State, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = (IPTR)UHSF_OPERATIONAL;
        count++;
    }

    if (((tag = FindTagItem(UHA_Manufacturer, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "The AROS Dev Team";
        count++;
    }

    if (((tag = FindTagItem(UHA_ProductName, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "HS USB 2.0 OTG Controller";
        count++;
    }

    if (((tag = FindTagItem(UHA_Description, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "Synopsys/DesignWare USB 2.0 OTG Controller";
        count++;
    }

    if (((tag = FindTagItem(UHA_Copyright, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) ="\xA9""2013-2023 The AROS Dev Team";
        count++;
    }

    if (((tag = FindTagItem(UHA_Version, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = VERSION_NUMBER;
        count++;
    }

    if (((tag = FindTagItem(UHA_Revision, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = REVISION_NUMBER;
        count++;
    }

    if (((tag = FindTagItem(UHA_DriverVersion, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = 0x100;
        count++;
    }

    if (((tag = FindTagItem(UHA_Capabilities, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = UHCF_USB20;
        count++;
    }
    ioreq->iouh_Actual = count;

    return RC_OK;
}

WORD FNAME_DEV(cmdReset)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] CMD_RESET(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdFlush)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] CMD_FLUSH(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return RC_OK;
}

WORD FNAME_DEV(cmdUsbReset)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBRESET(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbResume)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBRESUME(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbSuspend)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBSUSPEND(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_SUSPENDED)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbOper)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBOPER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdControlXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_CONTROLXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));


    if (ioreq->iouh_DevAddr == otg_Unit->hu_HubAddr)
    {
        D(bug("[USB2OTG] CTRL-RH: dev=%ld hubaddr=%ld req=%02lx val=%04lx len=%ld\n",
            (LONG)ioreq->iouh_DevAddr,
            (LONG)otg_Unit->hu_HubAddr,
            (ULONG)ioreq->iouh_SetupData.bRequest,
            (ULONG)AROS_LE2WORD(ioreq->iouh_SetupData.wValue),
            (LONG)ioreq->iouh_Length));
        return (FNAME_ROOTHUB(cmdControlXFer)(ioreq, otg_Unit, USB2OTGBase));
    }

    D(bug("[USB2OTG] CTRL-HW: dev=%ld hubaddr=%ld req=%02lx val=%04lx len=%ld\n",
        (LONG)ioreq->iouh_DevAddr,
        (LONG)otg_Unit->hu_HubAddr,
        (ULONG)ioreq->iouh_SetupData.bRequest,
        (ULONG)AROS_LE2WORD(ioreq->iouh_SetupData.wValue),
        (LONG)ioreq->iouh_Length));

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;
    usb2otg_reset_retry_state(ioreq);

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    AddTail(&otg_Unit->hu_CtrlXFerQueue, (struct Node *) ioreq);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
    Enable();
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    return (RC_DONTREPLY);
}

WORD FNAME_DEV(cmdBulkXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_BULKXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));


    if (ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;
    usb2otg_reset_retry_state(ioreq);

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    {
        int chan;
        for (chan = 0; chan < 8; chan++)
        {
            if (otg_Unit->hu_Channel[chan].hc_Request == ioreq &&
                otg_Unit->hu_Channel[chan].hc_SplitCSplitPending)
            {
#if defined(__AROSEXEC_SMP__)
                KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                Enable();
                return RC_DONTREPLY;
            }
        }
    }
    AddTail(&otg_Unit->hu_BulkXFerQueue, (struct Node *) ioreq);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
    Enable();
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    D(bug("[USB2OTG] UHCMD_BULKXFER: handled ioreq @ 0x%p\n", ioreq));

    return (RC_DONTREPLY);
}

WORD FNAME_DEV(cmdIntXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    if (ioreq->iouh_DevAddr == otg_Unit->hu_HubAddr)
    {
        D(bug("[USB2OTG] INT-RH: ep=%ld len=%ld changed=%d\n",
            (LONG)ioreq->iouh_Endpoint, (LONG)ioreq->iouh_Length,
            otg_Unit->hu_HubPortChanged));
        return (FNAME_ROOTHUB(cmdIntXFer)(ioreq, otg_Unit, USB2OTGBase));
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    /* Calculate "last time handled" and "next time to be handled" frame numbers */
    ULONG next_to_handle = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;
    ULONG last_handled = (next_to_handle - ioreq->iouh_Interval) & 0x7ff;
    ioreq->iouh_DriverPrivate1 = (APTR)((last_handled << 16) | next_to_handle);

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    AddTail(&otg_Unit->hu_IntXFerQueue, (struct Node *) ioreq);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
    Enable();
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    D(bug("[USB2OTG] INT-Q: dev=%d ep=%d len=%d interval=%d next=%d\n",
        ioreq->iouh_DevAddr, ioreq->iouh_Endpoint,
        ioreq->iouh_Length, ioreq->iouh_Interval, next_to_handle));

    return (RC_DONTREPLY);
}

WORD FNAME_DEV(cmdIsoXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_ISOXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if(ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    AddTail(&otg_Unit->hu_IsoXFerQueue, (struct Node *) ioreq);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
    Enable();
    //FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    D(bug("[USB2OTG] UHCMD_ISOXFER: handled ioreq @ 0x%p\n", ioreq));
    return(RC_DONTREPLY);
}
