/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/atomic.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/mbox.h>

#include <hardware/videocore.h>

#include "usb2otg_intern.h"

/*
 * INT-pipe activity counters per dev addr (0..7). poll: ScheduleIntTDs
 * assignment; nak: IRQ requeue; comp: completion. Dumped every ~3 s.
 */

/* Quarantine/blackout instrumentation — see usb2otg_intern.h. */

/* Control-pipe lifecycle: arms (channel), finishes, errors. */

/* Control failure-mode counters — which requeue path ctrl reqs take. */

/*
 * Log every channel released between prev_mask and the current
 * hu_DeadChannels, with how long it was dark (150 ms ticks).
 */
static void usb2otg_quar_log_release(struct USB2OTGUnit *unit,
    UBYTE prev_mask, const char *where)
{
    int c;

    for (c = 0; c < 8; c++)
    {
        if ((prev_mask & (1 << c)) && !(unit->hu_DeadChannels & (1 << c)))
        {
            /* Release is as important to see as the quarantine. */
            D(ULONG dark = unit->hu_WdTicks - unit->hu_QuarSetTick[c];)
            D(bug("[USB2OTG:QUAR] chan=%d released (%s) after %lu ticks (~%lu ms)%s\n",
                c, where, (unsigned long)dark, (unsigned long)(dark * 150),
                (c == CHAN_CTRL) ? " — CTRL BLACKOUT ENDS" : "");)
            (void)where;
        }
    }
}

static inline BOOL usb2otg_require_cpu0(struct USB2OTGDevice *USB2OTGBase, const char *where)
{
#if defined(__AROSEXEC_SMP__)
    ULONG cpu = KrnGetCPUNumber();
    if (cpu != 0)
    {
        bug("[USB2OTG:SMP] %s running on CPU %u, expected CPU 0\n", where, (unsigned)cpu);
        return FALSE;
    }
#else
    (void)where;
#endif
    return TRUE;
}

static void DumpChannelRegs(int channel)
{
    D(bug("Regs for channel %d: CHAR=%08x SPLIT=%08x INTR=%08x TSIZE=%08x DMA=%08x\n",
        channel,
        rd32le(USB2OTG_CHANNEL_REG(channel, CHARBASE)),
        rd32le(USB2OTG_CHANNEL_REG(channel, SPLITCTRL)),
        rd32le(USB2OTG_CHANNEL_REG(channel, INTR)),
        rd32le(USB2OTG_CHANNEL_REG(channel, TRANSSIZE)),
        rd32le(USB2OTG_CHANNEL_REG(channel, DMAADDR))));
}

/* Halt-sequence trace; set 1 for per-step register snapshots. */
#define USB2OTG_HALT_TRACE 0

#if USB2OTG_HALT_TRACE
static void usb2otg_halt_snapshot(int chan, const char *label)
{
    bug("[USB2OTG:HALT %s] chan=%d CHAR=%08x INTR=%08x GINTS=%08x HAINT=%08x"
        " NPTX=%08x HFIFO=%08x HPRT=%08x HFNUM=%08x GRST=%08x\n",
        label, chan,
        rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
        rd32le(USB2OTG_CHANNEL_REG(chan, INTR)),
        rd32le(USB2OTG_INTR),
        rd32le(USB2OTG_HOSTINTR),
        rd32le(USB2OTG_NONPERIFIFOSTATUS),
        rd32le(USB2OTG_HOSTFIFOSTATUS),
        rd32le(USB2OTG_HOSTPORT),
        rd32le(USB2OTG_HOSTFRAMENO),
        rd32le(USB2OTG_RESET));
}
#define HALT_SNAP(chan, label) usb2otg_halt_snapshot((chan), (label))
#else
#define HALT_SNAP(chan, label) do { } while (0)
#endif

static void usb2otg_halt_channel_preserve_char(int chan)
{
    ULONG hcchar = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));

    if (hcchar & USB2OTG_HOSTCHAR_ENABLE)
    {
        HALT_SNAP(chan, "entry");

        hcchar |= USB2OTG_HOSTCHAR_ENABLE | USB2OTG_HOSTCHAR_DISABLE;
        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), hcchar);

        HALT_SNAP(chan, "post-CHDIS");

        /*
         * Wait for halt completion (CHENA→0). Without this, async halt
         * races a subsequent StartChannel and leaves the channel for
         * the watchdog to clean. Halt can take a full microframe.
         */
        int timeout = 200000;
        while ((rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE)
               && --timeout > 0)
            asm volatile("yield\n");
        if (timeout <= 0)
        {
            HALT_SNAP(chan, "timeout-pre-flush");

            bug("[USB2OTG] halt-channel timeout chan=%d CHAR=%08x INTR=%08x\n",
                chan,
                rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
                rd32le(USB2OTG_CHANNEL_REG(chan, INTR)));

            /*
             * CHENA refuses to drop — usually a stale TX FIFO packet
             * the device isn't ACKing. Flush ALL TX FIFOs (TxFNum=16
             * per DWC2 spec); NPTXFIFO-only flush is insufficient on
             * Pi 3B+ after flash-busy waits. Collateral: other non-periodic
             * channels lose in-flight packets — acceptable on a
             * give-up path; class drivers retry.
             */
            wr32le(USB2OTG_RESET, USB2OTG_RESET_TXFIFOFLUSH | (0x10 << 6));
            {
                int t = 10000;
                while ((rd32le(USB2OTG_RESET) & USB2OTG_RESET_TXFIFOFLUSH)
                       && --t > 0)
                    asm volatile("yield\n");
                if (t <= 0)
                    HALT_SNAP(chan, "flush-self-timeout");
            }

            HALT_SNAP(chan, "post-flush");

            timeout = 200000;
            while ((rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE)
                   && --timeout > 0)
                asm volatile("yield\n");
            if (timeout <= 0)
            {
                HALT_SNAP(chan, "stuck-after-flush");

                bug("[USB2OTG] halt-channel still stuck after ALL-TX-FIFO flush chan=%d CHAR=%08x INTR=%08x\n",
                    chan,
                    rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
                    rd32le(USB2OTG_CHANNEL_REG(chan, INTR)));
            }
            else
            {
                HALT_SNAP(chan, "recovered");
                bug("[USB2OTG] halt-channel recovered after ALL-TX-FIFO flush chan=%d\n", chan);
            }
        }
    }
    else if (hcchar != 0)
    {
        hcchar |= USB2OTG_HOSTCHAR_DISABLE;
        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), hcchar);
    }
    else
    {
        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), USB2OTG_HOSTCHAR_DISABLE);
    }
}

static inline BOOL usb2otg_trace_bulk_ep2(int chan, struct IOUsbHWReq *req)
{
    return req != NULL &&
           chan == 1 &&
           req->iouh_Req.io_Command == UHCMD_BULKXFER &&
           req->iouh_Endpoint == 2;
}

/*
 * Post-halt cleanup before re-arming HS bulk-OUT: NPTxFIFO flush +
 * ~1 ms settle so any half-written OUT data clears and the device's
 * BBB endpoint SM resyncs. Mirrors FreeBSD dwc_otg.c:765-767.
 * Does NOT clear NPTxQ — wedge persistence escalates to HCLKSOFT.
 */
static inline void usb2otg_post_halt_bulk_out_settle(void)
{
    /*
     * Flush only with every channel idle — a flush while any other
     * channel is enabled corrupts the core's request-queue pointers
     * and wedges those channels (CHENA stuck, INTR=0). The settle
     * delay below is unconditional.
     */
    if (!usb2otg_any_channel_enabled())
    {
        wr32le(USB2OTG_RESET, USB2OTG_RESET_TXFIFOFLUSH | (0 << 6));
        {
            int t = 10000;
            while ((rd32le(USB2OTG_RESET) & USB2OTG_RESET_TXFIFOFLUSH)
                   && --t > 0)
                asm volatile("yield\n");
        }
    }
    {
        int t = 1200000;
        while (--t > 0)
            asm volatile("yield\n");
    }
}


/*
 * Drop a pending delayed CSPLIT re-arm. Several paths free a channel
 * (requeue/timeout/wedge) without cancelling its delay; if the channel
 * is then reassigned, the SOF handler would re-write CHENA on the new
 * transfer and the core deschedules it instantly (bare CHHLTD).
 */
void usb2otg_clear_delayed_channel(struct USB2OTGUnit *unit, int chan)
{
    if (chan >= 0 && chan < 8)
        unit->hu_DelayedChannel[chan] = 0;
}

/* TRUE if the periodic request queue holds entries (HPTXSTS.QSpcAvail
 * below the HW depth). In a wedge context (INT channel silent for the
 * whole watchdog period) pending entries are orphans — a CSPLIT for a
 * halted channel jams the queue head and starves every INT channel. */
static BOOL usb2otg_periodic_queue_jammed(void)
{
    ULONG depth = 2 << USB2OTG_HW2_PTXQDEPTH(rd32le(USB2OTG_HARDWARE2));
    ULONG qspc = (rd32le(USB2OTG_HOSTFIFOSTATUS) >> 16) & 0xff;

    return qspc < depth;
}

/*
 * Periodic request-queue jam recovery: halt every periodic channel,
 * requeue their requests, then flush the periodic TxFIFO/queue
 * (TxFNum=1). The flush is legal because all periodic channels are
 * halted first; bulk/ctrl use the non-periodic FIFO and are untouched.
 * Caller must hold hu_Lock.
 */
static void usb2otg_recover_periodic_queue(struct USB2OTGUnit *USBUnit)
{
    ULONG hptx_before = rd32le(USB2OTG_HOSTFIFOSTATUS);
    ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;
    int chan;

    bug("[USB2OTG] PTXQ-RECOVERY: HPTX=%08x — halting periodic channels, flushing PTX queue\n",
        hptx_before);

    for (chan = CHAN_INT1; chan <= CHAN_INT_LAST; chan++)
    {
        struct IOUsbHWReq *req = USBUnit->hu_Channel[chan].hc_Request;

        if (rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE)
            usb2otg_halt_channel_preserve_char(chan);
        usb2otg_exorcise_channel(chan);
        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
        USBUnit->hu_DelayedChannel[chan] = 0;
        USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
        USBUnit->hu_Channel[chan].hc_SplitState = USB2OTG_SPLIT_IDLE;
        USBUnit->hu_Channel[chan].hc_WatchdogCount = 0;

        if (req != NULL)
        {
            ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);

            if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                interval = 2;
            req->iouh_DriverPrivate1 =
                (APTR)(IPTR)((frnm << 16) | ((frnm + interval) & 0x7ff));
            ADDTAIL(&USBUnit->hu_IntXFerQueue, (struct Node *)req);
            USBUnit->hu_Channel[chan].hc_Request = NULL;
        }
    }

    usb2otg_wait_ahb_idle(100000, "periodic queue flush");
    wr32le(USB2OTG_RESET, USB2OTG_RESET_TXFIFOFLUSH | (1 << 6));
    {
        int t = 10000;
        while ((rd32le(USB2OTG_RESET) & USB2OTG_RESET_TXFIFOFLUSH) && --t > 0)
            asm volatile("yield\n");
    }

    bug("[USB2OTG] PTXQ-RECOVERY: done HPTX %08x -> %08x\n",
        hptx_before, rd32le(USB2OTG_HOSTFIFOSTATUS));
}

/*
 * Full core soft reset + host re-init, mirroring OpenUnit's bring-up
 * minus timer.device (this may run in softint context, so all waits
 * are yield spins — including the ~25 ms mode-switch settle, which is
 * acceptable on this catastrophic give-up path). Port power is
 * re-asserted afterwards; the resulting connect change makes Poseidon
 * re-enumerate the tree. Returns TRUE if wedged_chan's CHENA cleared.
 */
static BOOL usb2otg_core_reset_recover(struct USB2OTGUnit *USBUnit, int wedged_chan)
{
    ULONG regval;
    /*
     * Use the boot-time GUSBCFG snapshot: after a power cycle the
     * live register holds power-on defaults (wrong USBTrdTim etc),
     * and "restoring" those leaves the PHY interface misconfigured.
     */
    ULONG saved_gusbcfg = USBUnit->hu_BootGusbCfg != 0
                              ? USBUnit->hu_BootGusbCfg
                              : rd32le(USB2OTG_USB);
    unsigned int nchans = USBUnit->hu_HostChans ? USBUnit->hu_HostChans : 8;
    unsigned int chan;

    bug("[USB2OTG] WEDGE-RECOVERY: core soft reset\n");

    usb2otg_wait_ahb_idle(100000, "wedge-recovery core reset");
    wr32le(USB2OTG_RESET, USB2OTG_RESET_CORESOFT);
    usb2otg_wait_reset_bit_clear(USB2OTG_RESET_CORESOFT, 1000000,
        "WEDGE-RECOVERY: core soft reset");
    {
        volatile int i;
        for (i = 0; i < 100000; i++)
            asm volatile("yield\n");
    }

    /* AHB: DMA + global IRQs (cleared by core reset). */
    regval = rd32le(USB2OTG_AHB);
    regval |= USB2OTG_AHB_DMAENABLE
            | USB2OTG_AHB_AXIBURSTLENGTH
            | USB2OTG_AHB_INTENABLE
            | USB2OTG_AHB_TRANSFEREMPTYLEVEL;
    wr32le(USB2OTG_AHB, regval);

    /* Restore PHY config with force-host. */
    saved_gusbcfg &= ~USB2OTG_USB_FORCE_DEV_MODE;
    saved_gusbcfg |= USB2OTG_USB_FORCE_HOST_MODE;
    wr32le(USB2OTG_USB, saved_gusbcfg);
    {
        volatile int i;
        for (i = 0; i < 30000000; i++)   /* ~25 ms mode switch */
            asm volatile("yield\n");
    }

    /* Host clock select — same decision tree as OpenUnit. */
    regval = rd32le(USB2OTG_HARDWARE2);
    {
        ULONG hostcfg = rd32le(USB2OTG_HOSTCFG) & ~3;
        if (USB2OTG_HW2_FSPHY_TYPE(regval) == USB2OTG_HW2_FSPHY_TYPE_ULPI &&
            USB2OTG_HW2_HSPHY_TYPE(regval) == USB2OTG_HW2_HSPHY_TYPE_UTMI &&
            (rd32le(USB2OTG_USB) & USB2OTG_USB_ULPIFSLS))
            hostcfg |= 1;
        wr32le(USB2OTG_HOSTCFG, hostcfg);
    }

    {
        ULONG hfir = rd32le(USB2OTG_HOSTFRAMEINTERV);
        hfir &= ~0xffff;
        hfir |= 60000;
        wr32le(USB2OTG_HOSTFRAMEINTERV, hfir);
    }

    wr32le(USB2OTG_INTR, 0xffffffff);
    wr32le(USB2OTG_INTRMASK, USB2OTG_INTRCORE_DMASTARTOFFRAME |
                             USB2OTG_INTRCORE_HOSTCHANNEL);

    /* FIFO layout — must match OpenUnit. */
    wr32le(USB2OTG_RCVSIZE, 774);
    wr32le(USB2OTG_NONPERIFIFOSIZE, (256 << 16) | 774);
    wr32le(USB2OTG_PERIFIFOSIZE, (512 << 16) | (774 + 256));

    usb2otg_wait_ahb_idle(100000, "wedge-recovery FIFO flush");
    wr32le(USB2OTG_RESET, USB2OTG_RESET_TXFIFOFLUSH | (0x10 << 6));
    usb2otg_wait_reset_bit_clear(USB2OTG_RESET_TXFIFOFLUSH, 100000,
        "WEDGE-RECOVERY: Tx flush");
    wr32le(USB2OTG_RESET, USB2OTG_RESET_RXFIFOFLUSH);
    usb2otg_wait_reset_bit_clear(USB2OTG_RESET_RXFIFOFLUSH, 100000,
        "WEDGE-RECOVERY: Rx flush");

    /* Two-phase halt of all channels — fresh SMs after reset. */
    for (chan = 0; chan < nchans; chan++)
    {
        regval = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
        regval &= ~(USB2OTG_HOSTCHAR_ENABLE | USB2OTG_HOSTCHAR_EPDIR(1));
        regval |= USB2OTG_HOSTCHAR_DISABLE;
        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), regval);
    }
    for (chan = 0; chan < nchans; chan++)
    {
        int timeout = 100000;

        regval = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
        regval |= USB2OTG_HOSTCHAR_ENABLE | USB2OTG_HOSTCHAR_DISABLE;
        regval &= ~USB2OTG_HOSTCHAR_EPDIR(1);
        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), regval);
        while ((rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE)
               && --timeout > 0)
            asm volatile("yield\n");
        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
    }

    /*
     * The whole bus re-enumerates after the power cycle and device
     * addresses get reused — drop stale per-device state so fresh
     * pipes start with DATA0 toggles, no PING flow and no NAK gates.
     */
    {
        unsigned int dev;

        for (dev = 0; dev < 128; dev++)
        {
            USBUnit->hu_PIDBits[dev] = 0;
            USBUnit->hu_NakGate[dev][0] = USB2OTG_NAK_GATE_NONE;
            USBUnit->hu_NakGate[dev][1] = USB2OTG_NAK_GATE_NONE;
            USBUnit->hu_BulkGiveupStreak[dev] = 0;
        }
        USBUnit->hu_BulkOwnerDev[0] = 0;
        USBUnit->hu_BulkOwnerDev[1] = 0;
    }

    /* Re-power the port; the connect change drives re-enumeration. */
    usb2otg_hostport_rmw(USB2OTG_HOSTPORT_PRTPWR, 0);
    USBUnit->hu_HubPortChanged = TRUE;

    return !(rd32le(USB2OTG_CHANNEL_REG(wedged_chan, CHARBASE))
             & USB2OTG_HOSTCHAR_ENABLE);
}

/*
 * Last-resort wedge recovery: GRSTCTL.HCLKSOFT (host-clock domain
 * soft reset — resets all 8 channel SMs and host FIFO queue pointers,
 * preserves port state), escalating to a full core soft reset via
 * usb2otg_core_reset_recover() if CHENA still won't clear.
 * Side effect: other channels lose HW state — their requests are
 * re-queued to the class-command queue REGARDLESS of outcome (the
 * reset pulse disturbs them even when the wedged channel stays stuck).
 * wedged_chan is NOT re-queued (caller finishes it with
 * UHIOERR_TIMEOUT). Must be called holding hu_Lock; does not take it.
 */
static void usb2otg_escalate_hw_reset(struct USB2OTGUnit *USBUnit, int wedged_chan)
{
    ULONG char_before, char_after;
    UBYTE quar_before = USBUnit->hu_DeadChannels;
    int i;
    int t;
    int requeued = 0;

    char_before = rd32le(USB2OTG_CHANNEL_REG(wedged_chan, CHARBASE));
    if (!(char_before & USB2OTG_HOSTCHAR_ENABLE))
    {
        /* Channel actually cleared since caller's last check — nothing to do. */
        return;
    }

    bug("[USB2OTG] WEDGE-RECOVERY: escalating to HCLKSOFT chan=%d CHAR=%08x\n",
        wedged_chan, char_before);

    /* Wait for AHB master to idle before asserting reset, per DWC2 spec. */
    t = 100000;
    while (!(rd32le(USB2OTG_RESET) & (1UL << 31)) && --t > 0)
        asm volatile("yield\n");
    if (t <= 0)
        bug("[USB2OTG] WEDGE-RECOVERY: AHB never idled before HCLKSOFT RESET=%08x\n",
            rd32le(USB2OTG_RESET));

    /* Issue HCLKSOFT. HW auto-clears the bit when reset completes. */
    wr32le(USB2OTG_RESET, USB2OTG_RESET_HCLKSOFT);

    t = 200000;
    while ((rd32le(USB2OTG_RESET) & USB2OTG_RESET_HCLKSOFT) && --t > 0)
        asm volatile("yield\n");

    char_after = rd32le(USB2OTG_CHANNEL_REG(wedged_chan, CHARBASE));
    if (char_after & USB2OTG_HOSTCHAR_ENABLE)
    {
        bug("[USB2OTG] WEDGE-RECOVERY: HCLKSOFT did NOT clear CHENA chan=%d CHAR=%08x — escalating to core soft reset\n",
            wedged_chan, char_after);

        if (usb2otg_core_reset_recover(USBUnit, wedged_chan))
        {
            bug("[USB2OTG] WEDGE-RECOVERY: core soft reset cleared CHENA chan=%d\n",
                wedged_chan);
            USBUnit->hu_DeadChannels = 0;
        }
        else
        {
            bug("[USB2OTG] WEDGE-RECOVERY: core soft reset did NOT clear CHENA chan=%d — channel quarantined\n",
                wedged_chan);
            if (!(USBUnit->hu_DeadChannels & (1 << wedged_chan)))
                USBUnit->hu_QuarSetTick[wedged_chan] = USBUnit->hu_WdTicks;
            USBUnit->hu_DeadChannels |= (1 << wedged_chan);
        }
    }
    else
    {
        bug("[USB2OTG] WEDGE-RECOVERY: HCLKSOFT cleared CHENA chan=%d (CHAR=%08x)\n",
            wedged_chan, char_after);
        USBUnit->hu_DeadChannels = 0;
    }

    /*
     * Re-queue in-flight requests on other channels unconditionally:
     * the reset pulse disturbed their HW state even if the wedged
     * channel did not recover.
     */
    for (i = 0; i < 8; i++)
    {
        struct IOUsbHWReq *stuck_req;

        if (i == wedged_chan)
            continue;

        stuck_req = USBUnit->hu_Channel[i].hc_Request;
        if (stuck_req == NULL)
            continue;

        USBUnit->hu_Channel[i].hc_Request = NULL;
        USBUnit->hu_Channel[i].hc_SplitCSplitPending = 0;
        USBUnit->hu_Channel[i].hc_WatchdogCount = 0;
        USBUnit->hu_Channel[i].hc_DeferCount = 0;

        switch (stuck_req->iouh_Req.io_Command)
        {
            case UHCMD_BULKXFER:
                ADDHEAD(&USBUnit->hu_BulkXFerQueue, (struct Node *)stuck_req);
                break;
            case UHCMD_CONTROLXFER:
                ADDHEAD(&USBUnit->hu_CtrlXFerQueue, (struct Node *)stuck_req);
                break;
            case UHCMD_INTXFER:
                ADDHEAD(&USBUnit->hu_IntXFerQueue, (struct Node *)stuck_req);
                break;
            default:
                /* Unknown cmd — drop; class driver retries via timeout. */
                continue;
        }
        requeued++;

        bug("[USB2OTG] WEDGE-RECOVERY: re-queued chan=%d cmd=%lu dev=%d ep=%d dir=%s act=%lu/%lu\n",
            i,
            (unsigned long)stuck_req->iouh_Req.io_Command,
            (int)stuck_req->iouh_DevAddr,
            (int)stuck_req->iouh_Endpoint,
            stuck_req->iouh_Dir == UHDIR_IN ? "IN" : "OUT",
            (unsigned long)stuck_req->iouh_Actual,
            (unsigned long)stuck_req->iouh_Length);
    }

    bug("[USB2OTG] WEDGE-RECOVERY: done, %d in-flight request(s) re-queued\n", requeued);

    usb2otg_quar_log_release(USBUnit, quar_before, "hw-reset");
}

/*
 * Set the VideoCore power state of the USB HCD block. Mirrors the
 * boot-time power-on in usb2otg_device.c. Task context only (MBoxCall
 * busy-waits on the firmware).
 */
static BOOL usb2otg_set_usb_power(BOOL on)
{
    void *MBoxBase = OpenResource("mbox.resource");
    ULONG *raw, *msg;
    BOOL ok = FALSE;

    if (MBoxBase == NULL)
        return FALSE;

    /* MBoxCall invalidates the reply a whole cache line at a time, so the
     * message must own its 64-byte line exclusively. */
    raw = AllocVec(64 + 63, MEMF_CLEAR);
    if (raw == NULL)
        return FALSE;
    msg = (ULONG *)(((IPTR)raw + 63) & ~63);

    msg[0] = AROS_LE2LONG(8 * sizeof(ULONG));
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_SETPOWER);
    msg[3] = AROS_LE2LONG(8);
    msg[4] = AROS_LE2LONG(0);
    msg[5] = AROS_LE2LONG(VCPOWER_USBHCD);
    msg[6] = AROS_LE2LONG((on ? VCPOWER_STATE_ON : 0) | VCPOWER_STATE_WAIT);
    msg[7] = 0;

    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, msg) == msg)
        ok = ((AROS_LE2LONG(msg[6]) & 1) == (ULONG)(on ? 1 : 0));

    FreeVec(raw);
    return ok;
}

/*
 * Recover from wedged core split state: after a force-halted SSPLIT
 * (unplug episode), every later split-ctrl arm bare-CHHLTDs on ANY
 * channel with identical registers/uframe as arms that worked before
 * the episode — global core state, surviving both HCCHAR=0 and
 * HCLKSOFT. Full core soft reset + re-init is the remaining hammer;
 * it re-powers the port so the whole bus re-enumerates (the known-
 * good cold-boot path). Only with the bus idle, max once per ~5 s.
 * Caller is in IRQ context on CPU 0 — the ~25 ms re-init stall is
 * accepted on this catastrophic path.
 */
static BOOL usb2otg_core_reset_recover(struct USB2OTGUnit *USBUnit, int wedged_chan);

static void usb2otg_split_fsm_recover(struct USB2OTGUnit *USBUnit, int chan)
{
    int i;

#if !USB2OTG_ENABLE_CORE_RESET_RECOVERY
    (void)i; (void)chan; (void)USBUnit;
    return;     /* proven harmful — see the define */
#else
    if (usb2otg_any_channel_enabled())
    {
        bug("[USB2OTG:RECOVER] skip core reset (chan=%d): channels enabled\n", chan);
        return;
    }
    for (i = 0; i < 8; i++)
    {
        if (i != chan && USBUnit->hu_Channel[i].hc_Request != NULL)
        {
            bug("[USB2OTG:RECOVER] skip core reset (chan=%d): chan %d owned\n", chan, i);
            return;
        }
    }
    if (USBUnit->hu_LastResetTick != 0 &&
        (USBUnit->hu_WdTicks - USBUnit->hu_LastResetTick) < 34)
    {
        bug("[USB2OTG:RECOVER] skip core reset (chan=%d): cooldown\n", chan);
        return;
    }
    USBUnit->hu_LastResetTick = USBUnit->hu_WdTicks;

    /*
     * Register-level reinit alone does NOT revive the core (round-13
     * dumps: post-recover register state identical to the boot
     * baseline, yet zero transactions execute). The one asymmetry
     * left vs cold boot is the VideoCore power domain, so recovery is
     * a full power-cycle + reinit — done by the watchdog SM in task
     * context (mailbox calls busy-wait on firmware).
     */
    bug("[USB2OTG:RECOVER] scheduling USB power-cycle recovery (chan=%d)\n", chan);
    USBUnit->hu_PortRecoverState = 10;
    USBUnit->hu_PortRecoverTick = 0;
#endif
}

/*
 * Reset a channel's split engine with a forced disable cycle.
 *
 * A split sequence that ends in anything but a clean completion
 * leaves per-channel state inside the core that no amount of
 * reprogramming reaches: every later SSPLIT on that channel is
 * descheduled in its arming microframe with a bare CHHLTD, and
 * neither a core soft reset nor a port power cycle brings it back.
 * Driving CHENA|CHDIS even though the channel is already halted —
 * the way FreeBSD's dwc_otg frees a channel — takes the core through
 * its own disable path and clears that state. Must be followed by a
 * full reprogram, so HCSPLT/HCCHAR are scrubbed here.
 */
void usb2otg_exorcise_channel(int chan)
{
    ULONG hcchar = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
    int spin = 200000;

    /*
     * Recorded, not printed. This runs on the IRQ path, and the STALL
     * origin alone fires ~34 times per PsdDevLister round (devices
     * stall the string indices they do not implement) - serial output
     * at that rate moves split timing, which is what the ring exists
     * to avoid. The trace shows origin, channel and HCCHAR instead.
     */

    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE),
        hcchar | USB2OTG_HOSTCHAR_ENABLE | USB2OTG_HOSTCHAR_DISABLE);

    while ((rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) &
            USB2OTG_HOSTCHAR_ENABLE) && --spin > 0)
        asm volatile("yield\n");

    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0xffffffff);
    wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), 0);
    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0);

    /* Only the failure is worth the serial line. */
    if (spin <= 0)
        bug("[USB2OTG] split-engine reset on chan %d: halt timed out\n", chan);
}

/*
 * Recover a channel after a split transfer was given up on. Giving up
 * means tearing down a sequence the TT still has state for, which
 * poisons the channel, so reset its split engine every time. Retiring
 * the channel is only a backstop for one that keeps failing without a
 * single completed transfer in between.
 */
/*
 * engine_fault says what the give-up proves about the CHANNEL. Only a
 * bare-CHHLTD storm does (armed transactions the core never executes);
 * NAK and XactErr are received handshakes and wire errors - the channel
 * demonstrably ran its transactions, the problem is the device or the
 * bus. Hot-plugging a hub throws a burst of XactErr onto the interrupt
 * pipes of already-enumerated LS/FS devices, and counting those toward
 * the burn threshold retired both split-INT channels for good inside
 * 100 ms - after which no device behind any TT could interrupt-poll
 * until reboot (hub reinsertion "not detected").
 */
static void usb2otg_split_giveup_recover(struct USB2OTGUnit *USBUnit, int chan,
    BOOL engine_fault, UBYTE dev)
{
    struct USB2OTGChannel *hc = &USBUnit->hu_Channel[chan];

    usb2otg_exorcise_channel(chan);

    if (!engine_fault)
        return;

    /*
     * A streak of bare-CHHLTD give-ups all for ONE device address is
     * explained by that device vanishing: the TT refuses transactions
     * to a gone device with the same bare CHHLTD a poisoned engine
     * produces, and exorcise (above) is the right response. Burning
     * exists for the channel that hurts OTHERS - the historical case
     * was dead-device ctrl poisoning the channel for live devices'
     * ctrl - so the streak must span more than one device to count.
     * Without this, every hub unplug burned one channel for good.
     */
    if (hc->hc_GiveupStreak == 0)
    {
        hc->hc_GiveupDev = dev;
        hc->hc_GiveupCross = 0;
    }
    else if (hc->hc_GiveupDev != dev)
        hc->hc_GiveupCross = 1;

    if (hc->hc_GiveupStreak < 255)
        hc->hc_GiveupStreak++;

    if (hc->hc_GiveupStreak >= 4 && hc->hc_GiveupCross &&
        !(USBUnit->hu_BurnedChannels & (1 << chan)))
    {
        /*
         * Beyond revival — retire it, and if it carried split control
         * move that to the next candidate. INT5/INT4 come from the
         * direct-INT pool and are a last resort: taking one shrinks
         * the pool that serves hub status polling.
         */
        static const UBYTE candidates[] =
            { CHAN_INT3, CHAN_INT2, CHAN_INT1, CHAN_INT5, CHAN_INT4 };
        int i;

        USBUnit->hu_BurnedChannels |= (1 << chan);
        if (chan == USBUnit->hu_CtrlSplitChan)
        {
            for (i = 0; i < 5; i++)
            {
                if (!(USBUnit->hu_BurnedChannels & (1 << candidates[i])))
                {
                    USBUnit->hu_CtrlSplitChan = candidates[i];
                    break;
                }
            }
        }
        bug("[USB2OTG:BURN] chan %d burned after %d giveups (mask=%02x) ctrl-split now chan %d\n",
            chan, (int)hc->hc_GiveupStreak,
            (unsigned)USBUnit->hu_BurnedChannels,
            (int)USBUnit->hu_CtrlSplitChan);
    }
}

/*
 * Non-split bulk-OUT progress accounting for mid-burst halts.
 * Reads HCTSIZ.PktCnt, advances iouh_Actual by ACKed × MaxPktSize,
 * toggles host PID, resets transient retry budget on progress.
 * Callers MUST gate on HCINT.CHHLTD — TSIZE only settles after halt;
 * acting on pre-halt PktCnt over-advances Actual. No locking.
 */
static inline void usb2otg_bulk_out_advance_pktcnt(
    struct USB2OTGUnit *USBUnit, int chan,
    struct IOUsbHWReq *req, ULONG tsize_snapshot)
{
    struct USB2OTGChannel *hc;
    int initial_pktcnt;
    int remaining_pktcnt;
    int acked_pktcnt_raw;
    int acked_pktcnt;
    ULONG accepted;
    ULONG act_before;

    if (req->iouh_Req.io_Command != UHCMD_BULKXFER ||
        req->iouh_Dir != UHDIR_OUT ||
        (req->iouh_Flags & UHFF_SPLITTRANS))
        return;

    hc = &USBUnit->hu_Channel[chan];

    initial_pktcnt = (hc->hc_XferSize + req->iouh_MaxPktSize - 1)
        / (req->iouh_MaxPktSize ? req->iouh_MaxPktSize : 1);
    if (initial_pktcnt == 0)
        initial_pktcnt = 1;

    remaining_pktcnt = (tsize_snapshot >> 19) & 0x3FF;
    acked_pktcnt_raw = initial_pktcnt - remaining_pktcnt;
    if (acked_pktcnt_raw <= 0)
        return;

    /*
     * BCM2835 over-reports PktCnt on NYET (PktCnt=0 with only some
     * packets actually consumed). Cross-check against XferSize delta
     * and use the smaller of the two — under-credit re-sends a
     * duplicate (device discards by toggle); over-credit wedges BBB.
     */
    {
        ULONG xfer_remaining = tsize_snapshot & 0x7FFFF;
        ULONG bytes_from_xfersize;
        ULONG bytes_from_pktcnt;
        ULONG bytes_acked;

        if (xfer_remaining > hc->hc_XferSize)
            xfer_remaining = hc->hc_XferSize;  /* defensive */
        bytes_from_xfersize = hc->hc_XferSize - xfer_remaining;
        bytes_from_pktcnt = (ULONG)acked_pktcnt_raw * req->iouh_MaxPktSize;

        bytes_acked = bytes_from_pktcnt < bytes_from_xfersize
            ? bytes_from_pktcnt : bytes_from_xfersize;

        /* Round down to whole MaxPktSize — partial packets are not ACKed. */
        bytes_acked -= bytes_acked % req->iouh_MaxPktSize;

        accepted = bytes_acked;
        if (accepted > hc->hc_XferSize)
            accepted = hc->hc_XferSize;
        if (req->iouh_Actual + accepted > req->iouh_Length)
            accepted = req->iouh_Length - req->iouh_Actual;

        acked_pktcnt = accepted / req->iouh_MaxPktSize;
    }

    act_before = req->iouh_Actual;
    req->iouh_Actual += accepted;
    if (acked_pktcnt & 1)
        USBUnit->hu_PIDBits[req->iouh_DevAddr] ^=
            (USB2OTG_PID_DATA1 << (2 * req->iouh_Endpoint));
    req->iouh_DriverPrivate2 = (APTR)0;

}

static void usb2otg_remove_bulk_queue_duplicates(struct USB2OTGUnit *USBUnit,
    struct IOUsbHWReq *target, int chan)
{
    struct IOUsbHWReq *req = NULL, *next = NULL;

    ForeachNodeSafe(&USBUnit->hu_BulkXFerQueue, req, next)
    {
        if (req == target)
        {
            REMOVE(req);
            break;
        }
    }
}

static BOOL usb2otg_process_pending(struct USB2OTGUnit *otg_Unit);
static BOOL usb2otg_process_naktimeout(struct USB2OTGUnit *otg_Unit);

void FNAME_DEV(WorkerTask)(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    ULONG sigmask = 1UL << otg_Unit->hu_WorkerPort->mp_SigBit;

    for (;;)
    {
        struct Message *msg;
        ULONG flags;

        Wait(sigmask);

        while ((msg = GetMsg(otg_Unit->hu_WorkerPort)) != NULL)
        {
            if (msg == &otg_Unit->hu_NakTimeoutReq.tr_node.io_Message)
                usb2otg_process_naktimeout(otg_Unit);
        }

        Disable();
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
        flags = otg_Unit->hu_WorkFlags;
        otg_Unit->hu_WorkFlags = 0;
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
        Enable();
        if (flags & USB2OTG_WORK_NAKTIMEOUT)
            usb2otg_process_naktimeout(otg_Unit);
        if (flags & USB2OTG_WORK_PENDING)
            usb2otg_process_pending(otg_Unit);
    }
}

static void handle_SOF(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase, ULONG frnm_orig)
{
    ULONG frnm = frnm_orig >> 3;
    struct IOUsbHWReq *req = NULL, *next = NULL;
    struct USB2OTGDevice * USB2OTGBase = USBUnit->hu_USB2OTGBase;
    BOOL int_scheduled = FALSE;

    wr32le(USB2OTG_INTR, USB2OTG_INTRCORE_DMASTARTOFFRAME);

    if (frnm < USBUnit->hu_LastSOFFrame)
    {
        D(bug("[USB2OTG] SOF, frame wrap %d %d\n", frnm, USBUnit->hu_LastSOFFrame));
    }

    if (frnm != USBUnit->hu_LastSOFFrame)
    {
        {
#if defined(__AROSEXEC_SMP__)
            KrnSpinLock(&USBUnit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
            int walk_guard = 0;

            ForeachNodeSafe(&USBUnit->hu_IntXFerQueue, req, next)
            {
                ULONG last_handled = (ULONG)(IPTR)req->iouh_DriverPrivate1 >> 16;
                ULONG next_to_handle = (ULONG)(IPTR)req->iouh_DriverPrivate1 & 0x7ff;

                /* A cross-linked node turns this walk into a cycle (seen
                 * hanging the machine); bail loudly instead of spinning. */
                if (++walk_guard > 128)
                {
                    bug("[USB2OTG] SOF: IntXFerQueue walk runaway — list cycle, aborting walk\n");
                    break;
                }

                /*
                 * Due when the frames elapsed since last_handled reach
                 * the scheduled distance — wrap-safe modulo-2048. The
                 * old three-case comparison misfired whenever next
                 * wrapped past 2047 (next < last): every frame in
                 * [last..2047] promoted instantly, hammering long-
                 * interval pipes (hub status, interval 1024+) at SOF
                 * rate for up to interval ms per counter cycle.
                 */
                if (((frnm - last_handled) & 0x7ff) >=
                    ((next_to_handle - last_handled) & 0x7ff))
                {
                    D(bug("[USB2OTG] SOF: promoting INT dev=%ld ep=%ld frnm=%ld\n",
                        (LONG)req->iouh_DevAddr, (LONG)req->iouh_Endpoint, (LONG)frnm));
                    REMOVE(req);
                    ADDTAIL(&USBUnit->hu_IntXFerScheduled, req);
                    int_scheduled = TRUE;
                }
            }
#if defined(__AROSEXEC_SMP__)
            KrnSpinUnLock(&USBUnit->hu_Lock);
#endif
        }

        FNAME_DEV(Cause)(USB2OTGBase, &USBUnit->hu_PendingInt);

        /*
         * Do NOT arm INT channels from here: arming a direct periodic
         * channel inside the SOF interrupt (uframe-0 boundary, while
         * the core processes its periodic schedule) gets the transfer
         * descheduled instantly with bare CHHLTD — both ODDFRM
         * parities, 100% of attempts (verified on Pi 3B+; the hub
         * status pipes never executed a single poll). The PendingInt
         * softint Cause()d above runs ScheduleIntTDs shortly after,
         * outside the SOF window, which is the arming context that
         * works. Split periodic is unaffected either way (loose
         * frame-level scheduling) — its CSPLIT re-arms below stay.
         */
        USBUnit->hu_LastSOFFrame = frnm;


    }
    {
            int chan;
            for (chan=0; chan < 8; chan++)
            {
                if (USBUnit->hu_DelayedChannel[chan] != 0)
                {
                    USBUnit->hu_DelayedChannel[chan]--;
                    if (USBUnit->hu_DelayedChannel[chan] == 0)
                    {
                        /*
                         * Only re-arm if the channel still holds the
                         * split transfer this delay belongs to. A stale
                         * delay firing on a reassigned channel re-writes
                         * CHENA on a live transfer — the core kills it
                         * with bare CHHLTD (killed every hub status poll
                         * once a split device was present).
                         */
                        struct IOUsbHWReq *dreq =
                            USBUnit->hu_Channel[chan].hc_Request;

                        if (dreq != NULL && (dreq->iouh_Flags & UHFF_SPLITTRANS))
                            FNAME_DEV(StartChannel)(USBUnit, chan, 1);
                    }
                }
            }

        }

        /*
         * Rescue interrupt splits the core armed but never ran: CHENA
         * set, zero HCINT events, nothing in HAINT — no interrupt is
         * coming to end this transfer, and the watchdog only notices
         * after 900 ms (long enough to lose the report that releases
         * a mouse button). Guards vs the earlier, reverted attempt:
         * only SSPLIT state is touched (a CS-phase channel is mid-
         * transaction and healthy — hc_SplitSSUframe is not updated
         * past the SS arm, so CS must not be judged by it), and the
         * channel is exorcised after the halt (halting an armed
         * SSPLIT poisons the split engine; the forced disable cycle
         * is the proven cure).
         */
        {
            int chan;

            for (chan = CHAN_INT1; chan <= CHAN_INT_LAST; chan++)
            {
                struct USB2OTGChannel *hc = &USBUnit->hu_Channel[chan];
                struct IOUsbHWReq *sreq = hc->hc_Request;
                ULONG elapsed;

                if (sreq == NULL || USBUnit->hu_DelayedChannel[chan] != 0)
                    continue;
                if (sreq->iouh_Req.io_Command != UHCMD_INTXFER ||
                    !(sreq->iouh_Flags & UHFF_SPLITTRANS))
                    continue;
                if (hc->hc_SplitState != USB2OTG_SPLIT_SS)
                    continue;

                elapsed = (frnm_orig - hc->hc_SplitSSUframe) & 0x3fff;
                if (elapsed < USB2OTG_SPLIT_INT_STALL_UFRAMES)
                    continue;
                /* A pending interrupt gets to finish the transfer. */
                if (rd32le(USB2OTG_CHANNEL_REG(chan, INTR)) != 0)
                    continue;
                if (!(rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) &
                      USB2OTG_HOSTCHAR_ENABLE))
                    continue;

                {
                    ULONG interval = usb2otg_clamp_interval(sreq->iouh_Interval);

                    if (interval < 2)
                        interval = 2;

                    if (++USBUnit->hu_LogStallCount <= 5 || (USBUnit->hu_LogStallCount & 0xff) == 0)
                        bug("[USB2OTG] SOF: int split never ran, %lu uframes chan=%d dev=%d ep=%d — exorcise+requeue (#%lu)\n",
                            (unsigned long)elapsed, chan,
                            (int)sreq->iouh_DevAddr, (int)sreq->iouh_Endpoint,
                            (unsigned long)USBUnit->hu_LogStallCount);


                    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                    usb2otg_halt_channel_preserve_char(chan);
                    /* Heal the halt-on-armed-SSPLIT poison before reuse. */
                    usb2otg_exorcise_channel(chan);
                    hc->hc_SplitCSplitPending = 0;
                    hc->hc_SplitState = USB2OTG_SPLIT_IDLE;
                    hc->hc_CsplitRetry = 0;
                    hc->hc_WatchdogCount = 0;
                    sreq->iouh_DriverPrivate1 =
                        (APTR)((frnm << 16) | ((frnm + interval) & 0x7ff));
                    /* Helper re-checks the owner against the watchdog. */
                    usb2otg_irq_finish_or_requeue(USBUnit, chan, sreq,
                        &USBUnit->hu_IntXFerQueue, FALSE);
                }
            }
        }
}

void FNAME_DEV(GlobalIRQHandler)(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase)
{
    struct USB2OTGDevice * USB2OTGBase = USBUnit->hu_USB2OTGBase;
    if (!usb2otg_require_cpu0(USB2OTGBase, __PRETTY_FUNCTION__))
        return;

    /*
     * The handler body uses Disable()/Enable() pairs it shares with
     * task-context code. Running in IRQ context, an inner Enable()
     * would let a second USB interrupt in mid-handler, which then
     * completes the same channel again from stale registers and
     * replies the same request twice. Nest everything inside one
     * outer pair so those inner Enable()s never reach zero.
     */
    Disable();

    volatile unsigned int otg_RegVal;
    ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff);

    otg_RegVal = rd32le(USB2OTG_INTR);

    if (otg_RegVal & USB2OTG_INTRCORE_DMASTARTOFFRAME)
    {
        handle_SOF(USBUnit, SysBase, frnm);
    }

    /*
     * W1C all host-mode core IRQ bits we don't dispatch on. GINTSTS is
     * W1C — leaving IncompPeriodicXfer/FetchSusp latched starves
     * non-periodic arbitration. Mask SOF/HOSTCHANNEL (self-cleared)
     * and PORT (read by hub layer) before clearing the rest.
     */
    {
        ULONG clear_mask = otg_RegVal &
            ~(USB2OTG_INTRCORE_DMASTARTOFFRAME |
              USB2OTG_INTRCORE_HOSTCHANNEL |
              USB2OTG_INTRCORE_PORT);
        if (clear_mask != 0)
            wr32le(USB2OTG_INTR, clear_mask);
    }

    frnm >>= 3;

    if (otg_RegVal & USB2OTG_INTRCORE_HOSTCHANNEL)
    {
        unsigned int otg_ChanVal;
        int chan;
        otg_ChanVal = rd32le(USB2OTG_HOSTINTR);

        //D(bug("[USB2OTG] HOSTCHANNEL %x frnm %d\n", otg_ChanVal, frnm));

        for (chan = 0; chan < 8; chan++)
        {


            if (otg_ChanVal & (1 << chan))
            {
                struct IOUsbHWReq * req;
                uint32_t intr = rd32le(USB2OTG_CHANNEL_REG(chan, INTR));

                wr32le(USB2OTG_CHANNEL_REG(chan, INTR), intr);
                /*
                 * IRQ handler runs on CPU 0 with interrupts disabled —
                 * no other code can preempt us here. The lock is only
                 * needed at final completion to exclude the watchdog.
                 */
                req = USBUnit->hu_Channel[chan].hc_Request;

#if USB2OTG_DEBUG_FORCE_CTRL_NAK
                /* Synthesise the TT NAK QEMU never produces — see the define. */
                if (req != NULL && chan == USBUnit->hu_CtrlSplitChan &&
                    req->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
                    (req->iouh_Flags & UHFF_SPLITTRANS) &&
                    (intr & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) &&
                    (rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)) &
                     USB2OTG_HCSPLT_SPLITEN))
                {
                    static ULONG forced = 0;

                    if ((++forced % USB2OTG_DEBUG_FORCE_CTRL_NAK) == 0)
                    {
                        bug("[USB2OTG:FORCE] faking ctrl-split NAK #%lu chan=%d dev=%d\n",
                            (unsigned long)forced, chan, (int)req->iouh_DevAddr);
                        intr = USB2OTG_INTR_HALT_NAK;
                    }
                }
#endif

#if USB2OTG_DEBUG_FORCE_XACTERR_BURST
                /* Synthesise the hub-hot-plug XactErr burst — see the
                 * define. The ring records the faked intr as HCINT. */
                if (req != NULL &&
                    (req->iouh_Flags & UHFF_SPLITTRANS) &&
                    req->iouh_Req.io_Command == UHCMD_INTXFER &&
                    req->iouh_DevAddr >= 3 &&
                    (intr & USB2OTG_INTRCHAN_HALT))
                {
                    static ULONG seen = 0;

                    seen++;
                    if (seen > USB2OTG_DEBUG_FORCE_XACTERR_AFTER &&
                        seen <= USB2OTG_DEBUG_FORCE_XACTERR_AFTER +
                                USB2OTG_DEBUG_FORCE_XACTERR_BURST)
                    {
                        if (seen == USB2OTG_DEBUG_FORCE_XACTERR_AFTER + 1)
                            bug("[USB2OTG:FORCE] XactErr burst begins (%d events)\n",
                                USB2OTG_DEBUG_FORCE_XACTERR_BURST);
                        intr = USB2OTG_INTRCHAN_HALT |
                               USB2OTG_INTRCHAN_TRANSACTIONERROR;
                    }
                }
#endif


                /*
                 * DIAG: ctrl-split event trace. First 6 events of every
                 * submission plus the first 4 anomalous events (bare
                 * CHHLTD or error handshake) — shows the opening move
                 * of a 64-halt give-up storm: XactErr, NAK-then-bare,
                 * or clean progress that suddenly deschedules.
                 */
                if (req != NULL && chan == USBUnit->hu_CtrlSplitChan &&
                    req->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
                    (req->iouh_Flags & UHFF_SPLITTRANS))
                {
                    struct USB2OTGChannel *thc = &USBUnit->hu_Channel[chan];
                    BOOL anom = (intr == USB2OTG_INTRCHAN_HALT) ||
                        (intr & (USB2OTG_INTRCHAN_TRANSACTIONERROR |
                                 USB2OTG_INTRCHAN_DATATOGGLEERROR |
                                 USB2OTG_INTRCHAN_BABBLEERROR |
                                 USB2OTG_INTRCHAN_AHBERROR |
                                 USB2OTG_INTRCHAN_STALL)) != 0;

                    if (thc->hc_TraceCount < 0xff)
                        thc->hc_TraceCount++;
                    /*
                     * Anomalies always log. Clean events only under
                     * DEBUG: the ~9 ms UART line inside the IRQ acts
                     * as unintended pacing and masks the timing bug
                     * (that accident is what exposed it).
                     */
                    BOOL trace_this = (anom && thc->hc_TraceAnom < 4);
                    D(if (!trace_this && thc->hc_TraceCount <= 6)
                        trace_this = TRUE;)
                    if (trace_this)
                    {
                        if (anom)
                            thc->hc_TraceAnom++;
                        bug("[USB2OTG:CTRC] ev#%u%s chan=%d dev=%d bReq=%02x intr=%04x SPLT=%08x TSIZE=%08x HFNUM=%04x arm=%04x\n",
                            (unsigned)thc->hc_TraceCount,
                            anom ? "!" : "",
                            chan,
                            (int)req->iouh_DevAddr,
                            (unsigned)req->iouh_SetupData.bRequest,
                            (unsigned)intr,
                            rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)),
                            rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)),
                            (unsigned)(rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff),
                            (unsigned)(USBUnit->hu_Channel[chan].hc_StartHfnum & 0x3fff));
                    }
                }

                D(
                    if (chan == CHAN_CTRL)
                        bug("[USB2OTG:DBG] CTRL-IRQ intr=%04x dev=%d bReq=%02x wVal=%04x\n",
                            (unsigned)intr,
                            req ? (int)req->iouh_DevAddr : -1,
                            req ? (unsigned)req->iouh_SetupData.bRequest : 0,
                            req ? (unsigned)AROS_LE2WORD(req->iouh_SetupData.wValue) : 0);
                )

                if ((intr & USB2OTG_INTR_XFC_STALL) == USB2OTG_INTR_XFC_STALL)  /* XferComplete + STALL both present? */
                    bug("[USB2OTG] IRQ: chan=%d intr=%04x BOTH XferComplete+STALL!\n",
                        chan, intr);

            if (rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & 0x40000000)
            {

                D(bug("disable bit on channel %d is set!!!\n", chan));

                D(bug("req=%08x\n", req));

                DumpChannelRegs(chan);
                /*
                 * Do not zero HCCHAR here. QEMU's DWC2 model may still be
                 * consuming the channel state, and clearing HCCHAR wipes MPS
                 * and endpoint metadata underneath the emulator.
                 */
                D(bug("HOSTINTR=%08x\nHOSTINTRMASK=%08x\n", otg_ChanVal, rd32le(USB2OTG_HOSTINTRMASK)));


            }

                if (req)
                {
                    /* do_split: SSPLIT bit, optionally with CSPLIT bit */
                    int do_split = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)) & USB2OTG_HCSPLT_CSPLIT;
                    /* Channel closed with ACK on active split: issue CSPLIT. */
                    if (do_split == USB2OTG_HCSPLT_SSPLIT &&
                        ((intr == USB2OTG_INTR_HALT_ACK) ||
                         (req->iouh_Req.io_Command == UHCMD_BULKXFER &&
                          ((chan < CHAN_INT1) || (chan > CHAN_INT_LAST)) &&
                          (intr & (0x10 | USB2OTG_INTRCHAN_HALT)))))
                    {
                        uint32_t tmp = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                        tmp |= 1 << 16;   /* Set "do complete split" */
                        wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), tmp);
                        USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 1;
                        /* Fresh CSPLIT sequence — reset the NYET retry window. */
                        USBUnit->hu_Channel[chan].hc_CsplitRetry = 0;
                        USBUnit->hu_Channel[chan].hc_SplitState = USB2OTG_SPLIT_CS;

                        if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                            usb2otg_remove_bulk_queue_duplicates(USBUnit, req, chan);

                        D(bug("[USB2OTG] Completing split transaction in interrupt: chan=%d intr=%04x SPLITCTRL=%08x\n",
                              chan, intr, tmp));

                        /*
                         * Periodic INT split: issue the CSPLIT one microframe
                         * later (hu_DelayedChannel[] ticks per SOF) so it lands in
                         * the TT result window instead of hammering the same
                         * uframe. Bulk keeps the immediate re-arm. Split ctrl
                         * gets the full inter-transaction pace — see
                         * USB2OTG_CTRL_SPLIT_PACE_UFRAMES.
                         */
                        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                            USBUnit->hu_DelayedChannel[chan] = USB2OTG_CTRL_SPLIT_PACE_UFRAMES;
                        else if (chan >= CHAN_INT1 && chan <= CHAN_INT_LAST)
                            USBUnit->hu_DelayedChannel[chan] = 1;
                        else
                            FNAME_DEV(StartChannel)(USBUnit, chan, 1);
                    }
                    else if ((do_split == USB2OTG_HCSPLT_CSPLIT) &&
                             (req->iouh_Req.io_Command == UHCMD_BULKXFER) &&
                             ((chan < CHAN_INT1) || (chan > CHAN_INT_LAST)) &&
                             (intr & USB2OTG_INTRCHAN_NEGATIVEACKNOWLEDGE))
                    {
                        /* Bulk CSPLIT NAK: TT not done; requeue for fresh SSPLIT. */
                        D(
                            if (usb2otg_trace_bulk_ep2(chan, req))
                            {
                                bug("[USB2OTG:TRACE] IRQ-CSPLIT-NAK-REQUEUE chan=%d intr=%04x split=%08x char=%08x tsize=%08x\n",
                                    chan,
                                    (unsigned int)intr,
                                    rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)),
                                    rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
                                    rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)));
                            }
                        )

                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                        usb2otg_halt_channel_preserve_char(chan);
                        usb2otg_exorcise_channel(chan);

                        USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                        usb2otg_bulk_requeue(USBUnit, chan, req, intr, "split-csplit-nak");
                        /* Helper does TOCTOU vs watchdog. */
                        usb2otg_nak_gate_set(USBUnit, req->iouh_DevAddr,
                            usb2otg_gate_dir(req), USB2OTG_BULK_NAK_GATE_UFRAMES);
                        usb2otg_irq_finish_or_requeue(USBUnit, chan, req,
                            &USBUnit->hu_BulkXFerQueue, FALSE);
                        req = NULL;
                    }
                    else if (intr == USB2OTG_INTR_HALT_NYET && do_split == USB2OTG_HCSPLT_CSPLIT)
                    {
                        /*
                         * CHHLTD+ACK on a CSPLIT. If frame number passed
                         * half-interval, TT cache is stale → requeue with
                         * updated timing; otherwise re-arm/delay.
                         */
                        ULONG last = (ULONG)(IPTR)req->iouh_DriverPrivate1 >> 16;
                        ULONG thresh = (last + (ULONG)req->iouh_Interval / 2) % 2047;

                        if ((chan >= CHAN_INT1 && chan <= CHAN_INT_LAST) &&
                            req->iouh_Req.io_Command == UHCMD_INTXFER &&
                            ((frnm > thresh && frnm > last) ||
                             (frnm < thresh && frnm < last) ||
                             (frnm > thresh && frnm < last)))
                        {
                            D(bug("[USB2OTG] restarting transaction... %x, %x, %d\n", frnm, req->iouh_DriverPrivate1, req->iouh_DriverPrivate2));

                            /* Disable channel without clearing MPS/ep metadata */
                            usb2otg_halt_channel_preserve_char(chan);
                            usb2otg_exorcise_channel(chan);

                            /* Put transfer back into queue with updated timing */
                            if (req->iouh_Req.io_Command == UHCMD_INTXFER)
                            {
                                ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);
                                if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                                    interval = 2;
                                ULONG next = (frnm + interval) & 0x7ff;
                                req->iouh_DriverPrivate1 = (APTR)(IPTR)((frnm << 16) | next);
                                /* CSPLIT responded = liveness — reset wedge counter. */
                                req->iouh_DriverPrivate2 = (APTR)0;
                                ADDHEAD(&USBUnit->hu_IntXFerQueue, req);
                            }
                            else if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                                ADDHEAD(&USBUnit->hu_CtrlXFerQueue, req);
                            else if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                                ADDHEAD(&USBUnit->hu_BulkXFerQueue, req);

                            /* Mark channel free */
                            USBUnit->hu_Channel[chan].hc_Request = NULL;
                            req = NULL;
                        }
                        else
                        {
                            if (chan >= CHAN_INT1 && chan <= CHAN_INT_LAST)
                            {
                                /*
                                 * CSPLIT NYET = TT has not finished the
                                 * LS/FS transaction yet. Retry the CSPLIT
                                 * one microframe later (hu_DelayedChannel
                                 * ticks per SOF = per microframe in HS)
                                 * rather than re-arming in the same uframe.
                                 * Bound to ~8 uframes from the SSPLIT
                                 * (FreeBSD DWC_OTG_TT_SLOT_MAX); past that
                                 * the TT result window is gone, so requeue
                                 * for the next interval.
                                 */
                                if (USBUnit->hu_Channel[chan].hc_CsplitRetry < 8)
                                {
                                    USBUnit->hu_Channel[chan].hc_CsplitRetry++;
                                    USBUnit->hu_DelayedChannel[chan] = 1;
                                }
                                else if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                                {
                                    /*
                                     * Split ctrl: the TT result window is
                                     * gone, so go back to the SSPLIT of
                                     * the SAME transaction in place.
                                     * Requeueing would restart the
                                     * transfer from SETUP, and abandoning
                                     * an SSPLIT-ACKed transaction that way
                                     * wedges the channel's split engine.
                                     */
                                    D(bug("[USB2OTG] IRQ: ctrl csplit-nyet cap, retrying SSPLIT chan=%d dev=%d\n",
                                        chan, (int)req->iouh_DevAddr);)
                                    usb2otg_halt_channel_preserve_char(chan);
                                    {
                                        ULONG splt = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));

                                        splt &= ~USB2OTG_HCSPLT_COMPLSPLT;
                                        wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), splt);
                                    }
                                    USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                    USBUnit->hu_Channel[chan].hc_CsplitRetry = 0;
                                    USBUnit->hu_Channel[chan].hc_SplitState = USB2OTG_SPLIT_SS;
                                    USBUnit->hu_DelayedChannel[chan] = USB2OTG_CTRL_SPLIT_PACE_UFRAMES;
                                    req = NULL;
                                }
                                else
                                {
                                    ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);
                                    usb2otg_halt_channel_preserve_char(chan);
                                    if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                                        interval = 2;
                                    ULONG next = (frnm + interval) & 0x7ff;
                                    req->iouh_DriverPrivate1 = (APTR)(IPTR)((frnm << 16) | next);
                                    /* TT responded (NYETs) = liveness — reset wedge counter. */
                                    req->iouh_DriverPrivate2 = (APTR)0;
                                    ADDHEAD(&USBUnit->hu_IntXFerQueue, req);
                                    USBUnit->hu_Channel[chan].hc_Request = NULL;
                                    req = NULL;
                                }
                            }
                            else
                                USBUnit->hu_DelayedChannel[chan] = 16;
                        }
                    }
                    else if (intr == USB2OTG_INTRCHAN_HALT &&
                             do_split == USB2OTG_HCSPLT_SSPLIT &&
                             chan >= CHAN_INT1 && chan <= CHAN_INT_LAST &&
                             req->iouh_Req.io_Command == UHCMD_INTXFER)
                    {
                        /* Bare CHHLTD on SSPLIT INT: wrong OddFrame parity;
                         * restart, StartChannel re-predicts the frame. */
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                        FNAME_DEV(StartChannel)(USBUnit, chan, 1);
                        req = NULL;
                    }
                    else if ((intr == USB2OTG_INTR_HALT_NAK && do_split) &&
                        ((chan < CHAN_INT1 || chan > CHAN_INT_LAST) ||
                         (chan == USBUnit->hu_CtrlSplitChan &&
                          req->iouh_Req.io_Command == UHCMD_CONTROLXFER)))
                    {
                        D(
                            if (usb2otg_trace_bulk_ep2(chan, req))
                            {
                                bug("[USB2OTG:TRACE] REQUEUE-BEGIN chan=%d req=%p intr=%04x split=%08x\n",
                                    chan, req, (unsigned int)intr,
                                    rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)));
                            }
                        )
                        /* NAK on split — restart from the beginning. */
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                        usb2otg_halt_channel_preserve_char(chan);

                        /* Put transfer back into appropriate queue */
                        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                        {
                            /*
                             * TT NAK ≠ device alive: a dead port behind a
                             * TT is NAKed by the hub forever. Budget the
                             * retries and back off in wall time; ADDTAIL
                             * so other devices' control gets a turn
                             * (ADDHEAD monopolised CHAN_CTRL at ~1 kHz).
                             */
                            ULONG naks = (ULONG)(IPTR)req->iouh_DriverPrivate2 + 1;

                            USBUnit->hu_CtrlNakRequeues++;
                            if (naks > USB2OTG_CTRL_SPLIT_NAK_LIMIT)
                            {
                                bug("[USB2OTG] IRQ: ctrl split-NAK give-up dev=%d ep=%d bReq=%02x after %lu NAKs tt=%d.%d\n",
                                    (int)req->iouh_DevAddr,
                                    (int)req->iouh_Endpoint,
                                    (unsigned)req->iouh_SetupData.bRequest,
                                    (unsigned long)naks - 1,
                                    (int)req->iouh_SplitHubAddr,
                                    (int)req->iouh_SplitHubPort);
                                req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                                /*
                                 * Core reset only when the wedge blocks a
                                 * LIVE device: dev 0 = enumeration in
                                 * progress. Give-ups for assigned
                                 * addresses are unplug cleanup — error
                                 * delivery IS the right outcome there,
                                 * and a reset would nuke the whole bus
                                 * on every unplug.
                                 */
                                {
                                    BOOL was_dev0 = (req->iouh_DevAddr == 0);
                                    usb2otg_queue_clear_tt_buffer(USBUnit, req);
                                    usb2otg_irq_finish_or_requeue(USBUnit, chan, req,
                                        &USBUnit->hu_FinishedXfers, FALSE);
                                    if (was_dev0)
                                        usb2otg_split_fsm_recover(USBUnit, chan);
                                }
                                /* Give-up teardown poisons the channel. */
                                /* NAK = the TT answered; not an engine fault. */
                                usb2otg_split_giveup_recover(USBUnit, chan, FALSE, req->iouh_DevAddr);
                                req = NULL;
                            }
                            else
                            {
                                /*
                                 * A NAKed CSPLIT means re-issuing the
                                 * SSPLIT of the SAME transaction, on the
                                 * same channel: the channel registers
                                 * still hold the transaction and the data
                                 * toggle. Requeueing instead would
                                 * restart the transfer from SETUP and
                                 * abandon a transaction the TT has state
                                 * for, which wedges the split engine.
                                 * The retry budget still gives up above.
                                 */
                                D(bug("[USB2OTG] IRQ: ctrl split-NAK %lu, retrying SSPLIT chan=%d dev=%d\n",
                                    (unsigned long)naks, chan, (int)req->iouh_DevAddr);)
                                /*
                                 * Abandoning the NAKed CSPLIT leaves the
                                 * split engine in the same poisoned state
                                 * a STALL does: the re-issued SSPLIT is
                                 * descheduled with a bare CHHLTD in its
                                 * arming microframe, forever (a tester's
                                 * low-speed keyboard died here on every
                                 * enumeration, while its mouse - which
                                 * never NAKed a control transaction -
                                 * was fine). Reset the engine the way the
                                 * STALL path does, but around a save and
                                 * restore of the channel programming:
                                 * exorcising zeroes HCCHAR/HCSPLT, and
                                 * SetupChannel would restart a control
                                 * transfer from its SETUP packet rather
                                 * than resume this phase.
                                 */
                                {
                                    ULONG hcchar = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                                    ULONG hctsiz = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                                    ULONG hcsplt = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                                    ULONG hcdma  = rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR));

                                    usb2otg_exorcise_channel(chan);

                                    /* Next transaction is an SSPLIT, and
                                     * the channel must come back disarmed
                                     * for StartChannel to enable it. */
                                    hcsplt &= ~USB2OTG_HCSPLT_COMPLSPLT;
                                    hcchar &= ~(USB2OTG_HOSTCHAR_ENABLE |
                                                USB2OTG_HOSTCHAR_DISABLE);

                                    wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), hcdma);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE), hctsiz);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), hcsplt);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), hcchar);
                                }
                                USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                USBUnit->hu_Channel[chan].hc_CsplitRetry = 0;
                                USBUnit->hu_Channel[chan].hc_SplitState = USB2OTG_SPLIT_SS;
                                req->iouh_DriverPrivate2 = (APTR)(IPTR)naks;
                                /* SOF re-arms via StartChannel(quick=1)
                                 * while the req still owns the channel,
                                 * at the full pace — see the constant. */
                                USBUnit->hu_DelayedChannel[chan] = USB2OTG_CTRL_SPLIT_PACE_UFRAMES;
                                req = NULL;
                            }
                        }
                        else if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                        {
                            usb2otg_exorcise_channel(chan);
                            USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                            usb2otg_bulk_requeue(USBUnit, chan, req, intr, "split-begin-nak");
                            D(
                                if (usb2otg_trace_bulk_ep2(chan, req))
                                    bug("[USB2OTG:TRACE] REQUEUE-BEGIN-BULKQ chan=%d req=%p\n", chan, req);
                            )
                            ADDHEAD(&USBUnit->hu_BulkXFerQueue, req);
                        }

                        /* Mark channel free (ctrl path did it via helper). */
                        if (req != NULL)
                        {
                            USBUnit->hu_Channel[chan].hc_Request = NULL;
                            req = NULL;
                        }
                    }
                    else if ((intr == USB2OTG_INTRCHAN_HALT) &&
                             (do_split == USB2OTG_HCSPLT_CSPLIT) &&
                             (req->iouh_Req.io_Command == UHCMD_BULKXFER) &&
                             ((chan < CHAN_INT1) || (chan > CHAN_INT_LAST)))
                    {
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                        usb2otg_halt_channel_preserve_char(chan);
                        usb2otg_exorcise_channel(chan);

                        D(
                            if (usb2otg_trace_bulk_ep2(chan, req))
                                bug("[USB2OTG:TRACE] IRQ-CSPLIT-CHH-REQUEUE chan=%d req=%p\n", chan, req);
                        )

                        USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                        usb2otg_bulk_requeue(USBUnit, chan, req, intr, "split-csplit-halt");
                        usb2otg_nak_gate_set(USBUnit, req->iouh_DevAddr,
                            usb2otg_gate_dir(req), USB2OTG_BULK_NAK_GATE_UFRAMES);

                        usb2otg_irq_finish_or_requeue(USBUnit, chan, req,
                            &USBUnit->hu_BulkXFerQueue, FALSE);
                        req = NULL;
                    }
                    else
                    {
                        int cont = 0;

                        D(bug("[USB2OTG] IRQ: chan=%d intr=%04x dev=%d cmd=%d\n",
                            chan, intr, req->iouh_DevAddr, req->iouh_Req.io_Command));

                        /*
                         * XferCompl = success. ACK only fires for splits and
                         * must NOT be required. ChHltd always co-fires.
                         */
                        if ((intr & USB2OTG_INTRCHAN_TRANSFERCOMPLETE)
                            && !(intr & (USB2OTG_INTRCHAN_STALL |
                                         USB2OTG_INTRCHAN_AHBERROR |
                                         USB2OTG_INTRCHAN_TRANSACTIONERROR |
                                         USB2OTG_INTRCHAN_BABBLEERROR |
                                         USB2OTG_INTRCHAN_DATATOGGLEERROR |
                                         USB2OTG_INTRCHAN_BUFFERNOTAVAILABLE)))
                        {
                            req->iouh_Req.io_Error = 0;
                            /* Reset transient-error budget on completion. */
                            req->iouh_DriverPrivate2 = 0;
                            D(if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                                bug("[USB2OTG] BULK DONE: chan=%d actual=%ld/%ld\n",
                                    chan, (LONG)req->iouh_Actual, (LONG)req->iouh_Length));
                            D(bug("[USB2OTG] IRQ: XferComplete chan=%d, advancing\n", chan));

                            if (!FNAME_DEV(AdvanceChannel)(USBUnit, chan))
                            {
                                cont = 1;
                                D(bug("[USB2OTG] IRQ: Starting next phase chan=%d\n", chan));
                                /*
                                 * Split ctrl: pace the next phase's SSPLIT
                                 * instead of arming it in the completion
                                 * IRQ — an immediate arm risks the bare-
                                 * CHHLTD deschedule poison. AdvanceChannel
                                 * has programmed the registers; the SOF
                                 * handler arms via StartChannel(quick=1).
                                 */
                                if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
                                    (req->iouh_Flags & UHFF_SPLITTRANS))
                                    USBUnit->hu_DelayedChannel[chan] = USB2OTG_CTRL_SPLIT_PACE_UFRAMES;
                                else
                                    FNAME_DEV(StartChannel)(USBUnit, chan, 0);
                            }
                            else
                            {
                                D(bug("[USB2OTG] IRQ: Transfer fully done chan=%d actual=%d/%d\n",
                                    chan, req->iouh_Actual, req->iouh_Length));
                                /* Completion proves health — reset the
                                 * give-up streak (burn safety net). */
                                USBUnit->hu_Channel[chan].hc_GiveupStreak = 0;
                                /*
                                 * Hotplug diagnostic: a hub status-change
                                 * report is an INT completion with data.
                                 * Rate-limited; remove once the hotplug/
                                 * input-pump question is settled.
                                 */
                                if (req->iouh_Req.io_Command == UHCMD_INTXFER &&
                                    req->iouh_DevAddr < 8)
                                    USBUnit->hu_IntCompCount[req->iouh_DevAddr]++;
                                D(
                                    if (req->iouh_Req.io_Command == UHCMD_INTXFER &&
                                        req->iouh_Actual > 0)
                                    {
                                        static ULONG intc_count = 0;
                                        intc_count++;
                                        if (intc_count <= 10 || (intc_count & 0x3f) == 0)
                                            bug("[USB2OTG:INTC] dev=%d ep=%d len=%lu (#%lu)\n",
                                                (int)req->iouh_DevAddr,
                                                (int)req->iouh_Endpoint,
                                                (unsigned long)req->iouh_Actual,
                                                (unsigned long)intc_count);
                                    }
                                )
                            }
                        }
                        else if (intr & USB2OTG_INTRCHAN_AHBERROR)
                        {
                            /* AHB DMA fault — terminal (FreeBSD dwc_otg). */
                            bug("[USB2OTG] IRQ: AHBERR chan=%d dev=%d ep=%d intr=%04x\n",
                                chan, req->iouh_DevAddr, req->iouh_Endpoint, intr);
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            usb2otg_halt_channel_preserve_char(chan);
                            usb2otg_exorcise_channel(chan);
                            req->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        }
                        else if ((intr & (USB2OTG_INTRCHAN_TRANSACTIONERROR |
                                          USB2OTG_INTRCHAN_DATATOGGLEERROR |
                                          USB2OTG_INTRCHAN_BUFFERNOTAVAILABLE))
                                 && !(intr & (USB2OTG_INTRCHAN_STALL |
                                              USB2OTG_INTRCHAN_BABBLEERROR)))
                        {
                            /*
                             * Transient errors (XactErr/DataTglErr/BNA) —
                             * retry a few times. FreeBSD dwc_otg merges
                             * DATATGLERR into the XactErr retry path
                             * (dwc_otg.c:1422,1944). Halt-then-read TSIZE
                             * for settled PktCnt.
                             */
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            usb2otg_halt_channel_preserve_char(chan);

                            ULONG retries = (ULONG)(IPTR)req->iouh_DriverPrivate2;
                            if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
                                (req->iouh_Flags & UHFF_SPLITTRANS) &&
                                retries < USB2OTG_TRANSIENT_RETRY_LIMIT)
                            {
                                /*
                                 * Split ctrl: retry the SAME transaction
                                 * in place from its SSPLIT, like the NAK
                                 * and NYET paths. Requeueing restarts the
                                 * transfer from SETUP, and abandoning an
                                 * SSPLIT-ACKed transaction that way wedges
                                 * the channel's split engine. The channel
                                 * registers keep the transaction and the
                                 * data toggle.
                                 */
                                USBUnit->hu_CtrlXactRetries++;
                                D(bug("[USB2OTG] IRQ: ctrl split transient error %04x, retry %lu/%d chan=%d dev=%d\n",
                                    (unsigned)intr, (unsigned long)retries + 1,
                                    USB2OTG_TRANSIENT_RETRY_LIMIT, chan,
                                    (int)req->iouh_DevAddr);)
                                /* Reset-and-restore, exactly the proven
                                 * NAK-path medicine: the abandoned CSPLIT
                                 * poisons the engine the same way here. */
                                {
                                    ULONG hcchar = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                                    ULONG hctsiz = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                                    ULONG hcsplt = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                                    ULONG hcdma  = rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR));

                                    usb2otg_exorcise_channel(chan);

                                    hcsplt &= ~USB2OTG_HCSPLT_COMPLSPLT;
                                    hcchar &= ~(USB2OTG_HOSTCHAR_ENABLE |
                                                USB2OTG_HOSTCHAR_DISABLE);

                                    wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), hcdma);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE), hctsiz);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), hcsplt);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), hcchar);
                                }
                                USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                USBUnit->hu_Channel[chan].hc_CsplitRetry = 0;
                                USBUnit->hu_Channel[chan].hc_SplitState = USB2OTG_SPLIT_SS;
                                req->iouh_DriverPrivate2 = (APTR)(IPTR)(retries + 1);
                                USBUnit->hu_DelayedChannel[chan] = USB2OTG_CTRL_SPLIT_PACE_UFRAMES;
                                req = NULL;
                            }
                            else if ((req->iouh_Req.io_Command == UHCMD_CONTROLXFER ||
                                 req->iouh_Req.io_Command == UHCMD_BULKXFER) &&
                                retries < USB2OTG_TRANSIENT_RETRY_LIMIT)
                            {
                                const char *why =
                                    (intr & USB2OTG_INTRCHAN_DATATOGGLEERROR) ? "datatglerr" :
                                    (intr & USB2OTG_INTRCHAN_BUFFERNOTAVAILABLE) ? "bna" :
                                    "xacterr";
                                /* HCTSIZ settled by the halt above. */
                                ULONG tsize_snap =
                                    rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                                ULONG act_before_helper = req->iouh_Actual;
                                usb2otg_bulk_requeue(USBUnit, chan, req, intr, why);
                                D(bug("[USB2OTG] IRQ: %s chan=%d dev=%d intr=%04x, retry %ld/%d\n",
                                    why, chan, req->iouh_DevAddr, intr,
                                    retries + 1, USB2OTG_TRANSIENT_RETRY_LIMIT));

                                usb2otg_bulk_out_advance_pktcnt(USBUnit, chan, req, tsize_snap);
                                /* retry++ after helper (helper may reset budget on progress). */
                                req->iouh_DriverPrivate2 =
                                    (APTR)((IPTR)req->iouh_DriverPrivate2 + 1);
                                if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                                    USBUnit->hu_CtrlXactRetries++;

                                /*
                                 * XactErr-with-progress on HS direct bulk OUT:
                                 * device almost certainly flash-congested.
                                 * Apply a 500 ms NAK gate for bus-idle time.
                                 */
                                if (req != NULL &&
                                    req->iouh_Req.io_Command == UHCMD_BULKXFER &&
                                    req->iouh_Dir == UHDIR_OUT &&
                                    !(req->iouh_Flags & UHFF_SPLITTRANS) &&
                                    (req->iouh_Flags & UHFF_HIGHSPEED) &&
                                    req->iouh_Actual > act_before_helper)
                                {
                                    usb2otg_nak_gate_set(USBUnit,
                                        req->iouh_DevAddr, 0, 4000);
                                    usb2otg_post_halt_bulk_out_settle();
                                }

#if defined(__AROSEXEC_SMP__)
                                KrnSpinLock(&USBUnit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
                                if (USBUnit->hu_Channel[chan].hc_Request != req)
                                {
                                    KrnSpinUnLock(&USBUnit->hu_Lock);
                                }
                                else
                                {
#endif
                                /* Abandoning a ctrl transfer mid-phase
                                 * poisons the channel like on splits. */
                                if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                                    usb2otg_exorcise_channel(chan);
                                if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                                    ADDHEAD(&USBUnit->hu_CtrlXFerQueue, req);
                                else
                                    ADDHEAD(&USBUnit->hu_BulkXFerQueue, req);
                                USBUnit->hu_Channel[chan].hc_Request = NULL;
#if defined(__AROSEXEC_SMP__)
                                KrnSpinUnLock(&USBUnit->hu_Lock);
                                }
#endif
                                req = NULL;
                            }
                            else if (req->iouh_Req.io_Command == UHCMD_INTXFER &&
                                     retries < USB2OTG_TRANSIENT_RETRY_LIMIT)
                            {
                                /*
                                 * INT transient retry. The CONTROL/BULK
                                 * gate above used to drop INT straight to
                                 * TIMEOUT, so a single XactErr killed the
                                 * poll pipe and the class treated the
                                 * device as unplugged. Requeue with the
                                 * poll interval like the NAK path; the
                                 * budget still fails it after 3
                                 * consecutive errors.
                                 */
                                ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);
                                if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                                    interval = 2;
                                ULONG next = (frnm + interval) & 0x7ff;

                                D(bug("[USB2OTG] IRQ: int transient error %04x, retry %lu/%d chan=%d dev=%d ep=%d\n",
                                    (unsigned)intr, (unsigned long)retries + 1,
                                    USB2OTG_TRANSIENT_RETRY_LIMIT, chan,
                                    (int)req->iouh_DevAddr, (int)req->iouh_Endpoint);)

                                req->iouh_DriverPrivate1 = (APTR)((frnm << 16) | next);
                                req->iouh_DriverPrivate2 = (APTR)((ULONG)req->iouh_DriverPrivate2 + 1);

#if defined(__AROSEXEC_SMP__)
                                KrnSpinLock(&USBUnit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
                                if (USBUnit->hu_Channel[chan].hc_Request != req)
                                {
                                    /* Watchdog already handled this request */
                                    KrnSpinUnLock(&USBUnit->hu_Lock);
                                }
                                else
                                {
#endif
                                ADDTAIL(&USBUnit->hu_IntXFerQueue, req);
                                USBUnit->hu_Channel[chan].hc_Request = NULL;
#if defined(__AROSEXEC_SMP__)
                                KrnSpinUnLock(&USBUnit->hu_Lock);
                                }
#endif
                                req = NULL;
                            }
                            else
                            {
                                req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                                bug("[USB2OTG] IRQ: TIMEOUT chan=%d dev=%d cmd=%d\n", chan, req->iouh_DevAddr, req->iouh_Req.io_Command);
                                /*
                                 * Retry budget exhausted on a split — this
                                 * is where an unplugged device's periodic
                                 * split is abandoned, so unjam its TT
                                 * buffer here too (the ctrl give-up sites
                                 * only cover the control endpoint).
                                 */
                                usb2otg_queue_clear_tt_buffer(USBUnit, req);
                                /* Give-up teardown poisons the channel;
                                 * XactErr is a wire error, not an engine
                                 * fault - do not count it toward burning.
                                 * Non-split channels need the same reset:
                                 * ctrl to a vanished HS hub poisoned ch0
                                 * and every later arm to the (soldered!)
                                 * internal hub bare-CHHLTDed - hub.class
                                 * lost port management and tore down every
                                 * device on the bus. */
                                if (req->iouh_Flags & UHFF_SPLITTRANS)
                                    usb2otg_split_giveup_recover(USBUnit, chan, FALSE, req->iouh_DevAddr);
                                else
                                    usb2otg_exorcise_channel(chan);
                            }
                        }
                        else if (intr & USB2OTG_INTRCHAN_STALL)
                        {
                            /* STALL is terminal — USB "Request Error". */
                            D(bug("[USB2OTG] IRQ: STALL chan=%d dev=%d ep=%d HOSTPORT=%08x\n",
                                chan, req->iouh_DevAddr, req->iouh_Endpoint, rd32le(USB2OTG_HOSTPORT)));
                            /* Clear channel state after STALL */
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            usb2otg_halt_channel_preserve_char(chan);
                            req->iouh_Req.io_Error = UHIOERR_STALL;
                            /*
                             * A split sequence ending in STALL poisons the
                             * channel's split engine like any other unclean
                             * ending — every later SSPLIT is descheduled in
                             * its arming microframe with a bare CHHLTD.
                             * Reset it here, but keep this off the give-up
                             * streak: STALL is the device answering (e.g.
                             * an unsupported HID SET_IDLE), not a driver
                             * failure, and must not burn the channel.
                             */
                            if (req->iouh_Flags & UHFF_SPLITTRANS)
                                usb2otg_exorcise_channel(chan);
                        }
                        else if (intr & USB2OTG_INTRCHAN_NEGATIVEACKNOWLEDGE)
                        {
                            /* INT NAK: silently requeue so SOF re-promotes after interval. */
                            if (chan >= CHAN_INT1 && chan <= CHAN_INT_LAST)
                            {
                                if (req->iouh_DevAddr < 8)
                                    USBUnit->hu_IntNakCount[req->iouh_DevAddr]++;
                                /* Clear interrupt flags */
                                wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);

                                /* Disable channel */
                                usb2otg_halt_channel_preserve_char(chan);

                                /* Update DP1 so SOF waits the full interval; else tight poll. */
                                {
                                    ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);
                                    if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                                        interval = 2;
                                    ULONG next = (frnm + interval) & 0x7ff;
                                    req->iouh_DriverPrivate1 = (APTR)(IPTR)((frnm << 16) | next);
                                }
                                /* NAK = liveness — reset watchdog wedge counter. */
                                req->iouh_DriverPrivate2 = (APTR)0;

#if defined(__AROSEXEC_SMP__)
                                KrnSpinLock(&USBUnit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
                                if (USBUnit->hu_Channel[chan].hc_Request != req)
                                {
                                    /* Watchdog already handled this request */
                                    KrnSpinUnLock(&USBUnit->hu_Lock);
                                }
                                else
                                {
#endif
                                ADDTAIL(&USBUnit->hu_IntXFerQueue, req);
                                USBUnit->hu_Channel[chan].hc_Request = NULL;
#if defined(__AROSEXEC_SMP__)
                                KrnSpinUnLock(&USBUnit->hu_Lock);
                                }
#endif
                                req = NULL;
                            }
                            else if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                            {
                                if (USBUnit->hu_Channel[chan].hc_SplitCSplitPending)
                                {
                                    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                                    usb2otg_halt_channel_preserve_char(chan);
                                    D(
                                        if (usb2otg_trace_bulk_ep2(chan, req))
                                            bug("[USB2OTG:TRACE] IRQ-CSPLIT-NAK-REQUEUE chan=%d req=%p\n", chan, req);
                                    )
                                    USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                    usb2otg_bulk_requeue(USBUnit, chan, req, intr, "bulk-csplit-nak");
                                    /* 1 ms gate; prevents NAK-storm pegging CPU 0. */
                                    usb2otg_nak_gate_set(USBUnit, req->iouh_DevAddr,
                                                         usb2otg_gate_dir(req),
                                                         USB2OTG_BULK_NAK_GATE_UFRAMES);
                                    req->iouh_DriverPrivate1 = (APTR)0;
                                    /* NAK = forward progress → reset transient-error budget. */
                                    req->iouh_DriverPrivate2 = (APTR)0;
                                    usb2otg_irq_finish_or_requeue(USBUnit, chan, req,
                                        &USBUnit->hu_BulkXFerQueue, FALSE);
                                    req = NULL;
                                }
                                else
                                {
                                /* Bulk NAK: device busy; requeue. */
                                D(bug("[USB2OTG] BULK NAK: chan=%d dev=%d ep=%d\n",
                                    chan, req->iouh_DevAddr, req->iouh_Endpoint));
                                D(
                                    if (usb2otg_trace_bulk_ep2(chan, req))
                                        bug("[USB2OTG:TRACE] REQUEUE-BULK-NAK chan=%d req=%p\n", chan, req);
                                )
                                /* Snapshot HCTSIZ before halting for bulk-OUT progress. */
                                ULONG tsize_snap =
                                    rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                                wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                                usb2otg_halt_channel_preserve_char(chan);
                                usb2otg_bulk_out_advance_pktcnt(USBUnit, chan, req, tsize_snap);
                                USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                usb2otg_bulk_requeue(USBUnit, chan, req, intr, "bulk-nak");
                                usb2otg_nak_gate_set(USBUnit, req->iouh_DevAddr,
                                                     usb2otg_gate_dir(req),
                                                     USB2OTG_BULK_NAK_GATE_UFRAMES);
                                req->iouh_DriverPrivate1 = (APTR)0;
                                /* NAK = forward progress — reset transient-error budget */
                                req->iouh_DriverPrivate2 = (APTR)0;

                                usb2otg_irq_finish_or_requeue(USBUnit, chan, req,
                                    &USBUnit->hu_BulkXFerQueue, FALSE);
                                req = NULL;
                                }
                            }
                            else
                            {
                                D(bug("0x10: "));
                                req->iouh_Req.io_Error = UHIOERR_NAK;
                                D(DumpChannelRegs(chan));
                            }
                        }
                        else if (intr & USB2OTG_INTRCHAN_BABBLEERROR)
                        {
                            bug("[USB2OTG] IRQ: BABBLE chan=%d dev=%d ep=%d\n",
                                chan, req->iouh_DevAddr, req->iouh_Endpoint);
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            usb2otg_halt_channel_preserve_char(chan);
                            usb2otg_exorcise_channel(chan);
                            req->iouh_Req.io_Error = UHIOERR_BABBLE;
                        }
                        else if (intr & USB2OTG_INTRCHAN_FRAMEOVERRUN)
                        {
                            /* Frame overrun: requeue with updated timing. */
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            usb2otg_halt_channel_preserve_char(chan);
                            usb2otg_exorcise_channel(chan);
#if defined(__AROSEXEC_SMP__)
                            KrnSpinLock(&USBUnit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
                            if (USBUnit->hu_Channel[chan].hc_Request != req)
                            {
                                /* Watchdog already handled this request */
                                KrnSpinUnLock(&USBUnit->hu_Lock);
                            }
                            else
                            {
#endif
                            if (req->iouh_Req.io_Command == UHCMD_INTXFER)
                            {
                                ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);
                                if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                                    interval = 2;
                                ULONG next = (frnm + interval) & 0x7ff;
                                req->iouh_DriverPrivate1 = (APTR)(IPTR)((frnm << 16) | next);
                                /* Channel got scheduled = liveness — reset wedge counter. */
                                req->iouh_DriverPrivate2 = (APTR)0;
                                ADDHEAD(&USBUnit->hu_IntXFerQueue, req);
                            }
                            else if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                                ADDHEAD(&USBUnit->hu_CtrlXFerQueue, req);
                            else if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                            {
                                usb2otg_bulk_requeue(USBUnit, chan, req, intr, "frame-overrun");
                                ADDHEAD(&USBUnit->hu_BulkXFerQueue, req);
                            }
                            USBUnit->hu_Channel[chan].hc_Request = NULL;
#if defined(__AROSEXEC_SMP__)
                            KrnSpinUnLock(&USBUnit->hu_Lock);
                            }
#endif
                            req = NULL;
                        }
                        else if ((intr & USB2OTG_INTRCHAN_HALT) && !(intr & USB2OTG_INTRCHAN_TRANSFERCOMPLETE))
                        {
                            /*
                             * Bare CHHLTD (no XferCompl, no error). In
                             * Buffer DMA mode bulk NAK arrives this way;
                             * for INT it's a missed microframe. Treat as
                             * NAK with short retry. Snapshot regs before
                             * clearing.
                             */
                            ULONG chhltd_tsize = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                            /* Set when a retry keeps the channel's owner. */
                            BOOL keep_channel = FALSE;
                            D(ULONG chhltd_char = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));)
                            D(ULONG chhltd_dma = rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR));)
                            D(ULONG chhltd_nptx = rd32le(USB2OTG_NONPERIFIFOSTATUS);)
                            D(ULONG chhltd_hfnum = rd32le(USB2OTG_HOSTFRAMENO);)
                            D(ULONG chhltd_port = rd32le(USB2OTG_HOSTPORT);)

                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            usb2otg_halt_channel_preserve_char(chan);

#if defined(__AROSEXEC_SMP__)
                            KrnSpinLock(&USBUnit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
                            if (USBUnit->hu_Channel[chan].hc_Request != req)
                            {
                                KrnSpinUnLock(&USBUnit->hu_Lock);
                            }
                            else
                            {
#endif
                            if (chan >= CHAN_INT1 && chan <= CHAN_INT_LAST &&
                                req->iouh_Req.io_Command == UHCMD_INTXFER)
                            {
                                if (req->iouh_DevAddr < 8)
                                {
                                    USBUnit->hu_IntChhCount[req->iouh_DevAddr]++;
                                    USBUnit->hu_IntChhLastIntr[req->iouh_DevAddr] = intr;
                                    USBUnit->hu_IntChhSeenIntr[req->iouh_DevAddr] |= intr;
                                    USBUnit->hu_IntHltHfnum[req->iouh_DevAddr] =
                                        rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff;
                                }
                                /*
                                 * Pure CHHLTD without any handshake bit on
                                 * a direct periodic channel = the core
                                 * descheduled the transaction (never went
                                 * on the bus) — most likely wrong ODDFRM
                                 * parity. Flip the choice for next arm
                                 * (self-calibrating).
                                 */
                                if (!(req->iouh_Flags & UHFF_SPLITTRANS) &&
                                    (intr & USB2OTG_INTR_CLEAR_ALL) == USB2OTG_INTRCHAN_HALT)
                                    USBUnit->hu_Channel[chan].hc_OddfrmFlip ^= 1;

                                /*
                                 * Deschedule context capture: what were the
                                 * other channels doing when this direct INT
                                 * (hub status poll) was killed? Suspicion:
                                 * concurrent split activity. First 5 + every
                                 * 64th per device.
                                 */
                                D(
                                    if (!(req->iouh_Flags & UHFF_SPLITTRANS) &&
                                        req->iouh_DevAddr < 8)
                                    {
                                        ULONG cc = USBUnit->hu_IntChhCount[req->iouh_DevAddr];

                                        if (cc <= 5 || (cc & 0x3f) == 0)
                                        {
                                            int sc;

                                            bug("[USB2OTG:CHH] deschedule dev=%d chan=%d n=%lu hfnum=%04lx flip=%d chans:",
                                                (int)req->iouh_DevAddr, chan,
                                                (unsigned long)cc,
                                                (unsigned long)(rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff),
                                                (int)USBUnit->hu_Channel[chan].hc_OddfrmFlip);
                                            for (sc = 0; sc < 8; sc++)
                                            {
                                                ULONG sc_char = rd32le(USB2OTG_CHANNEL_REG(sc, CHARBASE));
                                                struct IOUsbHWReq *sc_req =
                                                    USBUnit->hu_Channel[sc].hc_Request;

                                                if ((sc_char & USB2OTG_HOSTCHAR_ENABLE) ||
                                                    sc_req != NULL || USBUnit->hu_DelayedChannel[sc] != 0)
                                                    bug(" %d:%08lx/s=%08lx/d%d%s", sc,
                                                        (unsigned long)sc_char,
                                                        (unsigned long)rd32le(USB2OTG_CHANNEL_REG(sc, SPLITCTRL)),
                                                        sc_req ? (int)sc_req->iouh_DevAddr : -1,
                                                        USBUnit->hu_DelayedChannel[sc] ? "*" : "");
                                            }
                                            bug("\n");
                                        }
                                    }
                                )
                                ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);
                                if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                                    interval = 2;
                                ULONG next = (frnm + interval) & 0x7ff;
                                req->iouh_DriverPrivate1 = (APTR)(IPTR)((frnm << 16) | next);
                                /* Bare CHHLTD = liveness — reset wedge counter. */
                                req->iouh_DriverPrivate2 = (APTR)0;
                                ADDTAIL(&USBUnit->hu_IntXFerQueue, req);
                            }
                            else if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                            {
                                /*
                                 * Same give-up budget as the split-NAK
                                 * path: an unbudgeted bare-CHHLTD requeue
                                 * kept dead-device ctrl reqs alive forever
                                 * (DP2 only resets on XFERCOMPL).
                                 */
                                ULONG chhs = (ULONG)(IPTR)req->iouh_DriverPrivate2 + 1;

                                USBUnit->hu_CtrlChhRequeues++;
                                if (chhs > USB2OTG_CTRL_SPLIT_NAK_LIMIT)
                                {
                                    bug("[USB2OTG] IRQ: ctrl bare-CHHLTD give-up dev=%d ep=%d bReq=%02x after %lu halts tt=%d.%d chan=%d\n",
                                        (int)req->iouh_DevAddr,
                                        (int)req->iouh_Endpoint,
                                        (unsigned)req->iouh_SetupData.bRequest,
                                        (unsigned long)chhs - 1,
                                        (int)req->iouh_SplitHubAddr,
                                        (int)req->iouh_SplitHubPort,
                                        chan);
                                    D(bug("[USB2OTG] IRQ: give-up chan=%d CHAR=%08x SPLT=%08x TSIZE=%08x INTR=%04x HFNUM=%04x\n",
                                        chan,
                                        rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
                                        rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)),
                                        rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)),
                                        (unsigned)intr,
                                        (unsigned)(rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff));)
                                    /* Port/core state at failure + arm
                                     * uframe (deschedule latency). */
                                    D(bug("[USB2OTG] IRQ: give-up HPRT=%08x GINTSTS=%08x HAINT=%08x HCFG=%08x HFNUM=%08x NPTXSTS=%08x\n",
                                        rd32le(USB2OTG_HOSTPORT),
                                        rd32le(USB2OTG_INTR),
                                        rd32le(USB2OTG_HOSTINTR),
                                        rd32le(USB2OTG_HOSTCFG),
                                        rd32le(USB2OTG_HOSTFRAMENO),
                                        rd32le(USB2OTG_NONPERIFIFOSTATUS));)
                                    D(bug("[USB2OTG] IRQ: give-up armHFN=%04x DMA=%08x\n",
                                        (unsigned)(USBUnit->hu_Channel[chan].hc_StartHfnum & 0x3fff),
                                        rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR)));)
                                    req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                                    ADDTAIL(&USBUnit->hu_FinishedXfers, (struct Node *)req);
                                    /*
                                     * Abandoned split → the TT buffer may
                                     * be holding the orphaned transaction.
                                     * Clear it so the next attempt (and any
                                     * other device on this TT) isn't
                                     * refused with bare CHHLTD.
                                     */
                                    usb2otg_queue_clear_tt_buffer(USBUnit, req);
                                    /* Reset only for blocked enumeration — see split-NAK site. */
                                    if (req->iouh_DevAddr == 0)
                                        usb2otg_split_fsm_recover(USBUnit, chan);
                                    /* Bare CHHLTD = the engine never ran
                                     * the transaction; this one counts. */
                                    if (req->iouh_Flags & UHFF_SPLITTRANS)
                                        usb2otg_split_giveup_recover(USBUnit, chan, TRUE, req->iouh_DevAddr);
                                    else
                                        usb2otg_exorcise_channel(chan);
                                }
                                else if (req->iouh_Flags & UHFF_SPLITTRANS)
                                {
                                    /*
                                     * Split control: retry the SAME
                                     * transaction in place, exactly like
                                     * the NAK and NYET-cap paths. Requeueing
                                     * restarts the transfer from its SETUP
                                     * stage and abandons a transaction the
                                     * transaction translator still has state
                                     * for, which wedges the channel's split
                                     * engine — every arm after that halts
                                     * bare in its arming microframe, so the
                                     * first stray halt turned into all 64.
                                     */
                                    /* Reset-and-restore, the proven NAK-
                                     * path medicine - COMPLSPLT-clear alone
                                     * left the engine poisoned, which is
                                     * why one stray halt became all 64. */
                                    {
                                    ULONG hcchar = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                                    ULONG hctsiz = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                                    ULONG hcsplt = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                                    ULONG hcdma  = rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR));

                                    usb2otg_exorcise_channel(chan);

                                    hcsplt &= ~USB2OTG_HCSPLT_COMPLSPLT;
                                    hcchar &= ~(USB2OTG_HOSTCHAR_ENABLE |
                                                USB2OTG_HOSTCHAR_DISABLE);

                                    wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), hcdma);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE), hctsiz);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), hcsplt);
                                    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), hcchar);
                                }

                                    D(bug("[USB2OTG] IRQ: ctrl bare-CHHLTD %lu, retrying SSPLIT chan=%d dev=%d\n",
                                        (unsigned long)chhs, chan, (int)req->iouh_DevAddr);)

                                    USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                    USBUnit->hu_Channel[chan].hc_CsplitRetry = 0;
                                    USBUnit->hu_Channel[chan].hc_SplitState = USB2OTG_SPLIT_SS;
                                    req->iouh_DriverPrivate2 = (APTR)(IPTR)chhs;
                                    /* SOF re-arms via StartChannel(quick=1)
                                     * while the req still owns the channel,
                                     * at the full pace — see the constant. */
                                    USBUnit->hu_DelayedChannel[chan] = USB2OTG_CTRL_SPLIT_PACE_UFRAMES;
                                    keep_channel = TRUE;
                                }
                                else
                                {
                                    /*
                                     * Reset the engine before the retry:
                                     * a bare CHHLTD leaves the channel FSM
                                     * poisoned exactly as on the split
                                     * channels, and SetupChannel's HCCHAR
                                     * zeroing alone does not clear it. The
                                     * requeue re-programs everything, so
                                     * nothing needs preserving.
                                     */
                                    usb2otg_exorcise_channel(chan);
                                    req->iouh_DriverPrivate2 = (APTR)(IPTR)chhs;
                                    /* Timed backoff + tail: no retry storm/HOL. */
                                    req->iouh_DriverPrivate1 =
                                        usb2otg_ctrl_backoff(USBUnit, frnm);
                                    ADDTAIL(&USBUnit->hu_CtrlXFerQueue, req);
                                }
                            }
                            else if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                            {
                                /* Multi-packet bulk-OUT progress accounting. */
                                usb2otg_bulk_out_advance_pktcnt(USBUnit, chan, req, chhltd_tsize);

                                usb2otg_bulk_requeue(USBUnit, chan, req, intr,
                                    "bare-chhltd");

                                {
                                    struct USB2OTGChannel *hc = &USBUnit->hu_Channel[chan];
                                    UBYTE np = hc->hc_BulkNoProgressCount;

                                    /* Absolute cap: not reset by np-reset below. */
                                    if (hc->hc_BareChhltdTotal < 0xFFFF)
                                        hc->hc_BareChhltdTotal++;

                                    D(
                                        {
                                            static ULONG bare_chhltd_total = 0;
                                            bare_chhltd_total++;

                                            if (np <= 3 || np == 100 || np == 200 ||
                                                (bare_chhltd_total & 0xffff) == 0)
                                            {
                                                ULONG hcint_now =
                                                    rd32le(USB2OTG_CHANNEL_REG(chan, INTR));
                                                ULONG hfnum_now =
                                                    rd32le(USB2OTG_HOSTFRAMENO);
                                                ULONG core_intr =
                                                    rd32le(USB2OTG_INTR);
                                                ULONG host_intr =
                                                    rd32le(USB2OTG_HOSTINTR);
                                                ULONG start_hfnum =
                                                    hc->hc_StartHfnum;
                                                LONG uframe_delta =
                                                    (start_hfnum != 0)
                                                        ? (LONG)((hfnum_now & 0x3fff) -
                                                                 (start_hfnum & 0x3fff)) & 0x3fff
                                                        : -1;
                                                ULONG pidbits =
                                                    (USBUnit->hu_PIDBits[req->iouh_DevAddr] >>
                                                     (2 * req->iouh_Endpoint)) & 3;
                                                bug("[USB2OTG:MSS] BARE-CHHLTD np=%u rq=%u total=%u chan=%d dev=%d ep=%d dir=%s act=%lu/%lu\n"
                                                    "  CHAR=%08x TSIZE=%08x DMA=%08x INTR_raw=%04x HCINT_now=%08x\n"
                                                    "  NPTXSTS=%08x HFNUM=%08x HOSTPORT=%08x\n"
                                                    "  start_hfnum=%08x ufdelta=%d coreINTR=%08x HAINT=%08x PID=%u\n",
                                                    (unsigned)np,
                                                    (unsigned)hc->hc_BulkRequeueCount,
                                                    bare_chhltd_total,
                                                    chan,
                                                    (int)req->iouh_DevAddr,
                                                    (int)req->iouh_Endpoint,
                                                    req->iouh_Dir == UHDIR_IN ? "IN" : "OUT",
                                                    (unsigned long)req->iouh_Actual,
                                                    (unsigned long)req->iouh_Length,
                                                    chhltd_char,
                                                    chhltd_tsize,
                                                    chhltd_dma,
                                                    (unsigned int)intr,
                                                    hcint_now,
                                                    chhltd_nptx,
                                                    chhltd_hfnum,
                                                    chhltd_port,
                                                    start_hfnum,
                                                    uframe_delta,
                                                    core_intr,
                                                    host_intr,
                                                    (unsigned)pidbits);
                                            }
                                        }
                                    )

                                    /*
                                     * Idle bulk-IN bypasses the np-counter
                                     * giveup (legit NAK pattern). Absolute
                                     * cap still applies via
                                     * hc_BareChhltdTotal (1500 × ~1 ms).
                                     * After 2 consecutive no-progress
                                     * giveups the device is likely gone
                                     * (unplug = bare-CHHLTD storm) — fail
                                     * fast so the class-retry loop doesn't
                                     * hog the bus and starve the hub's
                                     * disconnect report.
                                     */
                                    BOOL skip_np_giveup =
                                        req->iouh_Dir == UHDIR_IN &&
                                        req->iouh_Actual == 0;
                                    ULONG np_limit =
                                        USBUnit->hu_BulkGiveupStreak[req->iouh_DevAddr] >= 2
                                            ? 25 : 250;
                                    if ((np >= np_limit && !skip_np_giveup) ||
                                        hc->hc_BareChhltdTotal >= 1500)
                                    {
                                        if (req->iouh_Actual == 0 &&
                                            USBUnit->hu_BulkGiveupStreak[req->iouh_DevAddr] < 0xFF)
                                            USBUnit->hu_BulkGiveupStreak[req->iouh_DevAddr]++;
                                        {
                                            USBUnit->hu_LogGiveupCount++;
                                            if (usb2otg_log_rate(USBUnit->hu_LogGiveupCount))
                                                bug("[USB2OTG:MSS] BARE-CHHLTD giving up after %u no-progress chan=%d dev=%d ep=%d dir=%s act=%lu/%lu streak=%u (#%lu)\n",
                                                    (unsigned)np, chan,
                                                    (int)req->iouh_DevAddr,
                                                    (int)req->iouh_Endpoint,
                                                    req->iouh_Dir == UHDIR_IN ? "IN" : "OUT",
                                                    (unsigned long)req->iouh_Actual,
                                                    (unsigned long)req->iouh_Length,
                                                    (unsigned)USBUnit->hu_BulkGiveupStreak[req->iouh_DevAddr],
                                                    (unsigned long)USBUnit->hu_LogGiveupCount);
                                        }
                                        req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                                        /* Flush NPTxFIFO so next CBW lands clean —
                                         * only with all channels idle (flush with an
                                         * active channel corrupts the request queue). */
                                        if (req->iouh_Dir == UHDIR_OUT &&
                                            !usb2otg_any_channel_enabled())
                                        {
                                            wr32le(USB2OTG_RESET,
                                                USB2OTG_RESET_TXFIFOFLUSH | (0 << 6));
                                            {
                                                int timeout = 10000;
                                                while ((rd32le(USB2OTG_RESET)
                                                        & USB2OTG_RESET_TXFIFOFLUSH)
                                                       && --timeout > 0)
                                                    asm volatile("yield\n");
                                            }
                                            {
                                                USBUnit->hu_LogFlushCount++;
                                                if (usb2otg_log_rate(USBUnit->hu_LogFlushCount))
                                                    bug("[USB2OTG:MSS] BARE-CHHLTD NP-TX-FIFO-FLUSH after giveup chan=%d dev=%d ep=%d (#%lu)\n",
                                                        chan,
                                                        (int)req->iouh_DevAddr,
                                                        (int)req->iouh_Endpoint,
                                                        (unsigned long)USBUnit->hu_LogFlushCount);
                                            }
                                        }
                                        /* Do NOT reset PID toggle: would desync
                                         * a marginal device. Canonical recovery
                                         * is BOT Reset + CLEAR_HALT, handled
                                         * in TermIO. */
                                        usb2otg_bulk_finish(USBUnit, chan, req);
                                        USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                        USBUnit->hu_Channel[chan].hc_Request = NULL;
                                        ADDTAIL(&USBUnit->hu_FinishedXfers, (struct Node *)req);
                                    }
                                    else
                                    {
                                        USBUnit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                                        if (req->iouh_Dir == UHDIR_IN && req->iouh_Actual == 0 && np >= 250)
                                        {
                                            /* IN-idle long backoff. */
                                            usb2otg_nak_gate_set(USBUnit,
                                                req->iouh_DevAddr, 1, 200);
                                            hc->hc_BulkNoProgressCount = 0;
                                        }
                                        else
                                        {
                                            usb2otg_nak_gate_set(USBUnit,
                                                req->iouh_DevAddr,
                                                usb2otg_gate_dir(req),
                                                USB2OTG_BULK_NAK_GATE_UFRAMES);
                                        }
                                        ADDTAIL(&USBUnit->hu_BulkXFerQueue, req);
                                    }
                                }
                            }
                            if (!keep_channel)
                                USBUnit->hu_Channel[chan].hc_Request = NULL;
#if defined(__AROSEXEC_SMP__)
                            KrnSpinUnLock(&USBUnit->hu_Lock);
                            }
#endif
                            req = NULL;
                        }
                        else
                        {
                            D(bug("[USB2OTG] IRQ: Unknown %04x chan=%d dev=%d ep=%d\n",
                                intr, chan, req->iouh_DevAddr, req->iouh_Endpoint));
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            usb2otg_halt_channel_preserve_char(chan);
                            usb2otg_exorcise_channel(chan);
                        }


                        /* req==NULL means already requeued (NAK/frame-overrun/split-NYET). */
                        if (cont == 0 && req != NULL)
                        {
                            usb2otg_bulk_finish(USBUnit, chan, req);
                            usb2otg_irq_finish_or_requeue(USBUnit, chan, req,
                                &USBUnit->hu_FinishedXfers, FALSE);
                        }
                    }
                }
//intr...
            }
        }

        /* Trigger PendingInt so FinishedXfers run on IRQ return. */
        {
            /* IRQ context — no lock needed, nothing can preempt */
            if (!IsListEmpty(&USBUnit->hu_FinishedXfers))
                FNAME_DEV(Cause)(USBUnit->hu_USB2OTGBase, &USBUnit->hu_PendingInt);
        }
    }

    /* Matches the outer Disable() at entry — see comment there. */
    Enable();
}


static BOOL usb2otg_process_pending(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;

    /* **************** PROCESS DONE TRANSFERS **************** */

    FNAME_ROOTHUB(PendingIO)(otg_Unit);

//    FNAME_DEV(DoFinishedTDs)(otg_Unit);

    struct IOUsbHWReq * req = NULL;
    struct IOUsbHWReq * prev_req = NULL;
    ULONG drain_count = 0;
    ULONG same_count = 0;

    for (;;)
    {
        Disable();
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
        req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_FinishedXfers);
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
        Enable();
        if (!req)
            break;
        /*
         * Adding one request to the list twice self-links the node,
         * so REMHEAD keeps handing back the same one and this loop
         * never ends — a silent hang of all USB. Cheap insurance:
         * complain and drop out instead.
         */
        if (req == prev_req)
        {
            if (++same_count > 8)
            {
    bug("[USB2OTG] FinishedXfers self-linked req=%p cmd=%d dev=%d, dropping\n",
                    req, (int)req->iouh_Req.io_Command,
                    (int)req->iouh_DevAddr);
                break;
            }
        }
        else
            same_count = 0;
        prev_req = req;
        if (++drain_count > 2048)
        {
            bug("[USB2OTG] FinishedXfers drain runaway, last req=%p\n", req);
            break;
        }
        FNAME_DEV(TermIO)(req, otg_Unit->hu_USB2OTGBase);
    }

    if (!IsListEmpty(&otg_Unit->hu_CtrlXFerQueue))
        FNAME_DEV(ScheduleCtrlTDs)(otg_Unit);

    if (!IsListEmpty(&otg_Unit->hu_IntXFerScheduled))
        FNAME_DEV(ScheduleIntTDs)(otg_Unit);

    if (!IsListEmpty(&otg_Unit->hu_BulkXFerQueue))
        FNAME_DEV(ScheduleBulkTDs)(otg_Unit);

    //D(bug("[USB2OTG] [0x%p:PEND] finished\n", otg_Unit));

    return FALSE;
}

AROS_INTH1(FNAME_DEV(PendingInt), struct USB2OTGUnit *, otg_Unit)
{
    AROS_INTFUNC_INIT

    struct USB2OTGDevice * USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    BOOL ret;

    if (!usb2otg_require_cpu0(USB2OTGBase, __PRETTY_FUNCTION__))
        return FALSE;

    /* Softints run with IRQs enabled; the shared transfer queues are
     * otherwise juggled concurrently with GlobalIRQHandler (list
     * cross-link -> the SOF promotion walk spins forever). */
    Disable();
    ret = usb2otg_process_pending(otg_Unit);
    Enable();

    return ret;

    AROS_INTFUNC_EXIT
}

static BOOL usb2otg_process_naktimeout(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;

    otg_Unit->hu_WdTicks++;

    /*
     * Channel watchdog: detect stuck channels (lost completion IRQ,
     * lost split, DWC2 wedge). CHENA=0 with hc_Request set → force
     * timeout; CHENA=1 too long → halt+fail with HCLKSOFT escalation.
     */
#if 1
    BOOL watchdog_finished = FALSE;

    {
        int chan;
        struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;

        for (chan = 0; chan < 8; chan++)
        {
            struct IOUsbHWReq *req;

            Disable();
#if defined(__AROSEXEC_SMP__)
            KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
            req = otg_Unit->hu_Channel[chan].hc_Request;
            if (req == NULL)
            {
                /*
                 * Quarantined channel with no owner: the watchdog is its
                 * only recovery driver (everything else skips channels
                 * without hc_Request). Retry halt/escalate with a backoff
                 * that grows per failed attempt (7..200 ticks ≈ 1..30 s)
                 * so a truly dead channel doesn't core-reset every second.
                 */
                if (otg_Unit->hu_DeadChannels & (1 << chan))
                {
                    struct USB2OTGChannel *hc = &otg_Unit->hu_Channel[chan];
                    ULONG required = 7 * ((ULONG)hc->hc_DeferCount + 1);

                    if (required > 200)
                        required = 200;

                    if (++hc->hc_WatchdogCount >= required)
                    {
                        hc->hc_WatchdogCount = 0;

                        if (!(rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE))
                              & USB2OTG_HOSTCHAR_ENABLE))
                        {
                            /* CHENA cleared (e.g. by a prior reset) — release. */
                            UBYTE quar_prev = otg_Unit->hu_DeadChannels;
                            otg_Unit->hu_DeadChannels &= ~(1 << chan);
                            hc->hc_DeferCount = 0;
                            usb2otg_quar_log_release(otg_Unit, quar_prev,
                                "wd-chena-clear");
                        }
                        else
                        {
                            bug("[USB2OTG] Watchdog: recovering quarantined chan=%d (attempt %u)\n",
                                chan, (unsigned)hc->hc_DeferCount + 1);

                            usb2otg_halt_channel_preserve_char(chan);
                            if (rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE))
                                & USB2OTG_HOSTCHAR_ENABLE)
                            {
                                /* Clears/re-sets hu_DeadChannels itself. */
                                usb2otg_escalate_hw_reset(otg_Unit, chan);
                            }
                            else
                            {
                                UBYTE quar_prev = otg_Unit->hu_DeadChannels;
                                otg_Unit->hu_DeadChannels &= ~(1 << chan);
                                usb2otg_quar_log_release(otg_Unit, quar_prev,
                                    "wd-halt-ok");
                            }
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR),
                                   USB2OTG_INTR_CLEAR_ALL);

                            if (otg_Unit->hu_DeadChannels & (1 << chan))
                            {
                                if (hc->hc_DeferCount < 28)
                                    hc->hc_DeferCount++;
                            }
                            else
                                hc->hc_DeferCount = 0;
                        }
                    }
                }
                else
                    otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
#if defined(__AROSEXEC_SMP__)
                KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                Enable();
                continue;
            }
            /* Parked-NAK channel idles for the gate; not stuck. */
            if (otg_Unit->hu_Channel[chan].hc_NakParked)
            {
#if defined(__AROSEXEC_SMP__)
                KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                Enable();
                otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                continue;
            }

            /*
             * SOF scheduler owns active periodic-split channels: a CSPLIT
             * re-arm is pending within microframes (hu_DelayedChannel != 0),
             * so the channel is not stuck. Defer — the watchdog's lost-IRQ
             * recovery would otherwise race the SS->CS handoff and corrupt
             * CompSplt. The sequencer's bounded retry/requeue is the backstop.
             */
            if (chan >= CHAN_INT1 && chan <= CHAN_INT_LAST &&
                otg_Unit->hu_Channel[chan].hc_SplitState != USB2OTG_SPLIT_IDLE &&
                otg_Unit->hu_DelayedChannel[chan] != 0)
            {
#if defined(__AROSEXEC_SMP__)
                KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                Enable();
                otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                continue;
            }

            ULONG charbase = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));

            if (!(charbase & USB2OTG_HOSTCHAR_ENABLE))
            {
                uint32_t intr = rd32le(USB2OTG_CHANNEL_REG(chan, INTR));

                /*
                 * Lost-IRQ recovery: CHENA=0 + XFERCOMPL + no errors =
                 * HW completed, IRQ not yet processed. Run the normal
                 * AdvanceChannel path; applies to all channel types.
                 */
                if ((intr & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) &&
                    !(intr & (USB2OTG_INTRCHAN_STALL |
                              USB2OTG_INTRCHAN_TRANSACTIONERROR |
                              USB2OTG_INTRCHAN_BABBLEERROR)))
                {
                    D(bug("[USB2OTG] Watchdog: chan=%d lost IRQ, completing normally for dev=%d ep=%d intr=%04x\n",
                        chan, req->iouh_DevAddr, req->iouh_Endpoint, intr));

                    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                    req->iouh_Req.io_Error = 0;

                    if (FNAME_DEV(AdvanceChannel)(otg_Unit, chan))
                    {
                        otg_Unit->hu_Channel[chan].hc_Request = NULL;
                        otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                        ADDTAIL(&otg_Unit->hu_FinishedXfers, (struct Node *)req);
#if defined(__AROSEXEC_SMP__)
                        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                        Enable();
                        watchdog_finished = TRUE;
                    }
                    else
                    {
#if defined(__AROSEXEC_SMP__)
                        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                        Enable();
                        otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                        FNAME_DEV(StartChannel)(otg_Unit, chan, 0);
                    }
                }
                else if (intr == USB2OTG_INTR_HALT_ACK &&
                         (rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)) & USB2OTG_HCSPLT_CSPLIT) == USB2OTG_HCSPLT_SSPLIT)
                {
                    /* Lost-IRQ recovery: SSPLIT ACKed, arm CSPLIT. */
                    D(bug("[USB2OTG] Watchdog: chan=%d lost IRQ split-ACK, arming CSPLIT for dev=%d ep=%d intr=%04x\n",
                        chan, req->iouh_DevAddr, req->iouh_Endpoint, intr));
                    uint32_t tmp = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                    tmp |= 1 << 16;
                    wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), tmp);
                    otg_Unit->hu_Channel[chan].hc_SplitCSplitPending = 1;
                    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                    otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
#if defined(__AROSEXEC_SMP__)
                    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                    Enable();
                    FNAME_DEV(StartChannel)(otg_Unit, chan, 1);
                }
                else
                {
                    /* CHENA=0 with unrecognized INTR — wait for IRQ to catch up. */
                    UBYTE wd_thresh_off = usb2otg_watchdog_ticks(req);
                    otg_Unit->hu_Channel[chan].hc_WatchdogCount++;
                    if (otg_Unit->hu_Channel[chan].hc_WatchdogCount < wd_thresh_off)
                    {
#if defined(__AROSEXEC_SMP__)
                        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                        Enable();
                    }
                    else if (chan >= CHAN_INT1 && chan <= CHAN_INT_LAST &&
                             (intr & (0x10 | 0x40)) /* NAK or NYET */)
                    {
                        /* INT NAK missed by IRQ — requeue, don't time out. */
                        ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;
                        ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);
                        ULONG next = (frnm + interval) & 0x7ff;

                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);

                        req->iouh_DriverPrivate1 = (APTR)(IPTR)((frnm << 16) | next);
                        /* NAK = liveness — reset watchdog wedge counter. */
                        req->iouh_DriverPrivate2 = (APTR)0;
                        ADDHEAD(&otg_Unit->hu_IntXFerQueue, req);
                        otg_Unit->hu_Channel[chan].hc_Request = NULL;
                        otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
#if defined(__AROSEXEC_SMP__)
                        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                        Enable();
                    }
                    else if (intr == 0 &&
                             otg_Unit->hu_Channel[chan].hc_WatchdogCount < 16)
                    {
                        /*
                         * INTR=0 + CHENA=0 + hc_Request set = IRQ cleared
                         * INTR but not yet updated hc_Request. Transient,
                         * not stuck. Wait up to 16 ticks (2.4 s).
                         */
                        otg_Unit->hu_Channel[chan].hc_WatchdogCount++;
#if defined(__AROSEXEC_SMP__)
                        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                        Enable();
                    }
                    else
                    {
                        {
                            ULONG hcchar  = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                            ULONG hcsplt  = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                            ULONG hctsiz  = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                            ULONG hfnum   = rd32le(USB2OTG_HOSTFRAMENO);
                            ULONG started = otg_Unit->hu_Channel[chan].hc_StartHfnum;

                            bug("[USB2OTG] Watchdog: chan=%d stuck (CHENA=0), forcing timeout for dev=%d ep=%d intr=%04x\n",
                                chan, req->iouh_DevAddr, req->iouh_Endpoint, intr);
                            bug("[USB2OTG:STUCK] HCCHAR=%08lx HCSPLT=%08lx HCTSIZ=%08lx HFNUM=%08lx (armed@%08lx)\n",
                                (unsigned long)hcchar, (unsigned long)hcsplt, (unsigned long)hctsiz,
                                (unsigned long)hfnum, (unsigned long)started);
                            bug("[USB2OTG:STUCK] req: cmd=%d flags=%08lx mps=%u dev=%u ep=%u dir=%u splithub=%u splitport=%u csplit_pending=%u\n",
                                (int)req->iouh_Req.io_Command,
                                (unsigned long)req->iouh_Flags,
                                (unsigned)req->iouh_MaxPktSize,
                                (unsigned)req->iouh_DevAddr,
                                (unsigned)req->iouh_Endpoint,
                                (unsigned)req->iouh_Dir,
                                (unsigned)req->iouh_SplitHubAddr,
                                (unsigned)req->iouh_SplitHubPort,
                                (unsigned)otg_Unit->hu_Channel[chan].hc_SplitCSplitPending);

                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);

                            req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            usb2otg_bulk_finish(otg_Unit, chan, req);
                            otg_Unit->hu_Channel[chan].hc_Request = NULL;
                            otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                            /* Force-halting a live split is where the TT
                             * jam is created (unplug). Clear it here. */
                            usb2otg_queue_clear_tt_buffer(otg_Unit, req);
                            ADDTAIL(&otg_Unit->hu_FinishedXfers, (struct Node *)req);
#if defined(__AROSEXEC_SMP__)
                            KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                            Enable();
                            watchdog_finished = TRUE;
                        }
                    }
                }
            }
            else
            {
#if defined(__AROSEXEC_SMP__)
                KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                Enable();
                otg_Unit->hu_Channel[chan].hc_WatchdogCount++;
                /* WD threshold per-transfer-type — see usb2otg_watchdog_ticks(). */
                {
                UBYTE wd_thresh = usb2otg_watchdog_ticks(req);
                if (otg_Unit->hu_Channel[chan].hc_WatchdogCount >= wd_thresh)
                {
                    ULONG w_char = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                    ULONG w_intr = rd32le(USB2OTG_CHANNEL_REG(chan, INTR));
                    ULONG w_split = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                    ULONG w_tsize = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));

                    if (w_intr & (USB2OTG_INTRCHAN_TRANSFERCOMPLETE |
                                  USB2OTG_INTRCHAN_HALT))
                    {
                        /* Completion bits latched, IRQ behind — defer 16 ticks. */
                        if (otg_Unit->hu_Channel[chan].hc_DeferCount < 16)
                        {
                            otg_Unit->hu_Channel[chan].hc_DeferCount++;
                            /* Log only at streak start to avoid UART flood. */
                            D(
                                if (usb2otg_trace_bulk_ep2(chan, req) &&
                                    otg_Unit->hu_Channel[chan].hc_DeferCount == 1)
                                {
                                    bug("[USB2OTG:TRACE] WATCHDOG-COMPLETION-DEFER chan=%d intr=%04x split=%08x char=%08x tsize=%08x\n",
                                        chan, (unsigned int)w_intr, w_split, w_char, w_tsize);
                                }
                            )
                            otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                            continue;
                        }

                        bug("[USB2OTG] Watchdog: chan=%d COMPLETION-DEFER cap hit (defer=%u) — giving up\n",
                            chan,
                            (unsigned)otg_Unit->hu_Channel[chan].hc_DeferCount);
                        otg_Unit->hu_Channel[chan].hc_DeferCount = 0;
                        /* fall through to active-too-long halt+fail */
                    }

                    if ((req->iouh_Req.io_Command == UHCMD_BULKXFER) &&
                        (req->iouh_Flags & UHFF_SPLITTRANS) &&
                        (w_split & USB2OTG_HCSPLT_SPLITEN) &&
                        (w_intr & USB2OTG_INTRCHAN_NEGATIVEACKNOWLEDGE))
                    {
                        if ((w_split & USB2OTG_HCSPLT_COMPLSPLT) == 0)
                        {
                            ULONG tmp = w_split | USB2OTG_HCSPLT_COMPLSPLT;

                            wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), tmp);
                            otg_Unit->hu_Channel[chan].hc_SplitCSplitPending = 1;

                            D(
                                if (usb2otg_trace_bulk_ep2(chan, req))
                                {
                                    bug("[USB2OTG:TRACE] WATCHDOG-PROMOTE chan=%d intr=%04x split=%08x -> %08x char=%08x tsize=%08x\n",
                                        chan, (unsigned int)w_intr, w_split, tmp, w_char, w_tsize);
                                }
                            )

                            FNAME_DEV(StartChannel)(otg_Unit, chan, 1);
                            otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                            continue;
                        }

                        /* CSPLIT+NAK = TT busy; defer up to 16 ticks. */
                        if (otg_Unit->hu_Channel[chan].hc_DeferCount < 16)
                        {
                            otg_Unit->hu_Channel[chan].hc_DeferCount++;
                            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                            otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;

                            /* Log only at streak start (avoid UART flood). */
                            D(
                                if (usb2otg_trace_bulk_ep2(chan, req) &&
                                    otg_Unit->hu_Channel[chan].hc_DeferCount == 1)
                                {
                                    bug("[USB2OTG:TRACE] WATCHDOG-DEFER-CSPLIT chan=%d intr=%04x split=%08x char=%08x tsize=%08x\n",
                                        chan, (unsigned int)w_intr, w_split, w_char, w_tsize);
                                }
                            )
                            continue;
                        }

                        bug("[USB2OTG] Watchdog: chan=%d CSPLIT-DEFER cap hit dev=%d ep=%d — TT not responding, failing transfer\n",
                            chan, req->iouh_DevAddr, req->iouh_Endpoint);
                        otg_Unit->hu_Channel[chan].hc_DeferCount = 0;
                        /* fall through to active-too-long halt+fail */
                    }

                    /*
                     * Direct INT still NAKing (CHENA=1, NAK latched):
                     * the device just has nothing to report and the
                     * BULK-typed channel retries internally. Not a
                     * wedge — halt and requeue for the next interval.
                     * Backstop for a lost NAK interrupt; the IRQ path
                     * normally handles this within microseconds.
                     */
                    if ((req->iouh_Req.io_Command == UHCMD_INTXFER) &&
                        !(req->iouh_Flags & UHFF_SPLITTRANS) &&
                        (w_intr & USB2OTG_INTRCHAN_NEGATIVEACKNOWLEDGE))
                    {
                        ULONG frnm_now = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;
                        ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);

                        Disable();
#if defined(__AROSEXEC_SMP__)
                        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
                        if (otg_Unit->hu_Channel[chan].hc_Request != req)
                        {
                            KrnSpinUnLock(&otg_Unit->hu_Lock);
                            Enable();
                            otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                            continue;
                        }
#endif
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);
                        usb2otg_halt_channel_preserve_char(chan);
                        usb2otg_exorcise_channel(chan);

                        if (req->iouh_DevAddr < 8)
                            otg_Unit->hu_IntNakCount[req->iouh_DevAddr]++;
                        req->iouh_DriverPrivate1 =
                            (APTR)(IPTR)((frnm_now << 16) |
                                   ((frnm_now + interval) & 0x7ff));
                        req->iouh_DriverPrivate2 = (APTR)0;
                        ADDTAIL(&otg_Unit->hu_IntXFerQueue, (struct Node *)req);
                        otg_Unit->hu_Channel[chan].hc_Request = NULL;
                        otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
#if defined(__AROSEXEC_SMP__)
                        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                        Enable();
                        continue;
                    }

                    /*
                     * Direct bulk OUT silent in-data-phase (CHENA=1,
                     * INTR=0): the buffer-DMA core is driving NAK/NYET/
                     * PING flow control itself while the device is
                     * flash-busy. Aborting violates MSC BOT §6.6.1 and
                     * wedges the device. Defer up to FLASH_BUSY_DEFER_CAP
                     * (or forever — see cap branch below).
                     */
                    if ((req->iouh_Req.io_Command == UHCMD_BULKXFER) &&
                        (req->iouh_Dir == UHDIR_OUT) &&
                        !(req->iouh_Flags & UHFF_SPLITTRANS) &&
                        (w_char & USB2OTG_HOSTCHAR_ENABLE) &&
                        /* Benign NAK/NYET/ACK bits may latch during the
                         * core's own flow control even though masked —
                         * only real errors/completions end the wait. */
                        ((w_intr & USB2OTG_INTR_CLEAR_ALL &
                          ~(USB2OTG_INTRCHAN_NEGATIVEACKNOWLEDGE |
                            USB2OTG_INTRCHAN_NOTREADY |
                            USB2OTG_INTRCHAN_ACKNOWLEDGE)) == 0))
                    {
                        /*
                         * Distinguish flash-busy from a wedged channel: an
                         * actively retrying channel keeps entries in the
                         * non-periodic request queue, while every observed
                         * wedge shows queue AND FIFO completely free
                         * (NPTX=00080100). Require the idle signature on 3
                         * consecutive defers (~9 s) — a snapshot between
                         * retries can transiently look idle.
                         */
                        BOOL wedge_sig = FALSE;
                        {
                            struct USB2OTGChannel *whc = &otg_Unit->hu_Channel[chan];
                            ULONG nptxsts = rd32le(USB2OTG_NONPERIFIFOSTATUS);
                            ULONG fifo_dwords =
                                (rd32le(USB2OTG_NONPERIFIFOSIZE) >> 16) & 0xFFFF;
                            ULONG q_depth =
                                2 << USB2OTG_HW2_NPTXQDEPTH(rd32le(USB2OTG_HARDWARE2));
                            BOOL np_idle = ((nptxsts & 0xFFFF) >= fifo_dwords) &&
                                           (((nptxsts >> 16) & 0xFF) >= q_depth);

                            if (np_idle)
                                whc->hc_QuietIdleStreak++;
                            else
                                whc->hc_QuietIdleStreak = 0;

                            if (whc->hc_QuietIdleStreak >= 3)
                            {
                                bug("[USB2OTG] Watchdog: chan=%d WAIT-QUIET wedge signature (NPTX=%08x x%u) — recovering\n",
                                    chan, nptxsts,
                                    (unsigned)whc->hc_QuietIdleStreak);
                                whc->hc_QuietIdleStreak = 0;
                                whc->hc_DeferCount = 0;
                                wedge_sig = TRUE;
                                /* fall through to active-too-long halt+fail */
                            }
                        }

                        if (!wedge_sig &&
                            otg_Unit->hu_Channel[chan].hc_DeferCount < USB2OTG_FLASH_BUSY_DEFER_CAP)
                        {
                            otg_Unit->hu_Channel[chan].hc_DeferCount++;
                            otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;

                            D(
                                {
                                    struct USB2OTGChannel *hc = &otg_Unit->hu_Channel[chan];
                                    UWORD nd = hc->hc_DeferCount;
                                    int dump_now;

                                    /* Log cadence: dense at first, sparser later. */
                                    dump_now = (nd == 1 || nd == 2 || nd == 5 ||
                                                nd == 10 || nd == 20 || nd == 40 ||
                                                nd == 80 || nd == 160 ||
                                                (nd > 160 && (nd % 80) == 0));

                                    if (dump_now)
                                    {
                                        ULONG d_char  = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                                        ULONG d_intr  = rd32le(USB2OTG_CHANNEL_REG(chan, INTR));
                                        ULONG d_tsize = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));
                                        ULONG d_dmaa  = rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR));
                                        ULONG d_nptx  = rd32le(USB2OTG_NONPERIFIFOSTATUS);
                                        ULONG d_hfnum = rd32le(USB2OTG_HOSTFRAMENO);
                                        LONG hfd = (LONG)((d_hfnum & 0x3fff) -
                                                          (hc->hc_LastSampleHfnum & 0x3fff));
                                        if (hfd < 0)
                                            hfd += 0x4000;
                                        {
                                            UWORD  intr_chg = (UWORD)d_intr ^ hc->hc_LastSampleIntr;
                                            ULONG  tsize_chg = d_tsize ^ hc->hc_LastSampleTsize;
                                            LONG   actual_chg = (LONG)req->iouh_Actual -
                                                                (LONG)hc->hc_LastSampleActual;
                                            bug("[USB2OTG] WAIT-QUIET chan=%d defer=%u dev=%d ep=%d"
                                                " CHAR=%08lx INTR=%04lx TSIZE=%08lx DMAA=%08lx NPTX=%08lx"
                                                " HFNUM=%08lx hfd=%ld act=%lu/%lu"
                                                " d[intr^=%04x tsize^=%08lx act+=%ld]\n",
                                                chan, (unsigned)nd,
                                                (int)req->iouh_DevAddr,
                                                (int)req->iouh_Endpoint,
                                                (unsigned long)d_char,
                                                (unsigned long)d_intr,
                                                (unsigned long)d_tsize,
                                                (unsigned long)d_dmaa,
                                                (unsigned long)d_nptx,
                                                (unsigned long)d_hfnum,
                                                (long)hfd,
                                                (unsigned long)req->iouh_Actual,
                                                (unsigned long)req->iouh_Length,
                                                (unsigned)intr_chg,
                                                (unsigned long)tsize_chg,
                                                (long)actual_chg);
                                        }

                                        hc->hc_LastSampleHfnum  = d_hfnum;
                                        hc->hc_LastSampleIntr   = (UWORD)d_intr;
                                        hc->hc_LastSampleTsize  = d_tsize;
                                        hc->hc_LastSampleActual = req->iouh_Actual;
                                    }
                                }
                            )
                            continue;
                        }

                        if (!wedge_sig)
                        {
                            /*
                             * Cap hit (~10 min): no real device is flash-
                             * busy this long — treat as wedged so halt/
                             * escalate recovery runs. Class driver handles
                             * BOT reset.
                             */
                            bug("[USB2OTG] Watchdog: chan=%d WAIT-QUIET cap hit dev=%d ep=%d — surfacing UHIOERR_TIMEOUT\n",
                                chan, req->iouh_DevAddr, req->iouh_Endpoint);
                            otg_Unit->hu_Channel[chan].hc_DeferCount = 0;
                        }
                        /* fall through to active-too-long halt+fail */
                    }

                    {
                        otg_Unit->hu_LogWdCount++;
                        if (usb2otg_log_rate(otg_Unit->hu_LogWdCount))
                        {
                            /* Extended wedge dump for post-mortem. */
                            ULONG w_dmaa  = rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR));
                            ULONG w_nptx  = rd32le(USB2OTG_NONPERIFIFOSTATUS);
                            ULONG w_ptx   = rd32le(USB2OTG_HOSTFIFOSTATUS);
                            ULONG w_corei = rd32le(USB2OTG_INTR);
                            ULONG w_haint = rd32le(USB2OTG_HOSTINTR);
                            ULONG w_hfnum = rd32le(USB2OTG_HOSTFRAMENO);
                            bug("[USB2OTG] Watchdog: chan=%d active too long dev=%d ep=%d (#%lu)\n"
                                "  CHAR=%08x INTR=%04x SPLIT=%08x TSIZE=%08x\n"
                                "  DMAA=%08x NPTX=%08x HPTX=%08x coreINTR=%08x HAINT=%08x HFNUM=%08x\n"
                                "  splitstate=%d ssuf=%04x delayed=%d req=%p cmd=%lu\n",
                                chan, req->iouh_DevAddr, req->iouh_Endpoint,
                                (unsigned long)otg_Unit->hu_LogWdCount,
                                w_char, w_intr, w_split, w_tsize,
                                w_dmaa, w_nptx, w_ptx, w_corei, w_haint, w_hfnum,
                                (int)otg_Unit->hu_Channel[chan].hc_SplitState,
                                (unsigned)otg_Unit->hu_Channel[chan].hc_SplitSSUframe,
                                (int)otg_Unit->hu_DelayedChannel[chan],
                                req,
                                (unsigned long)req->iouh_Req.io_Command);
                        }
                    }

                    /*
                     * Claim before halting so the halt-triggered IRQ on
                     * another CPU sees hc_Request=NULL and skips. Halt
                     * stays in the lock to prevent reassignment racing.
                     */
                    Disable();
#if defined(__AROSEXEC_SMP__)
                    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
                    if (otg_Unit->hu_Channel[chan].hc_Request != req)
                    {
#if defined(__AROSEXEC_SMP__)
                        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                        Enable();
                        otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;
                        continue;
                    }

                    /* Claim now — IRQ path will read hc_Request == NULL and skip */
                    otg_Unit->hu_Channel[chan].hc_Request = NULL;
                    otg_Unit->hu_Channel[chan].hc_SplitCSplitPending = 0;
                    otg_Unit->hu_Channel[chan].hc_WatchdogCount = 0;

                    /* Fresh TSIZE for advance_pktcnt; w_tsize above may be stale. */
                    ULONG wd_tsize = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE));

                    usb2otg_halt_channel_preserve_char(chan);
                    {
                        /* Belt-and-braces second halt wait (~200 µs). */
                        int timeout = 200000;
                        while ((rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE) && --timeout > 0)
                            asm volatile("yield\n");
                    }

                    /* If CHENA still stuck after full flush → escalate to HCLKSOFT. */
                    if (rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE))
                        & USB2OTG_HOSTCHAR_ENABLE)
                    {
                        usb2otg_escalate_hw_reset(otg_Unit, chan);
                    }

                    wr32le(USB2OTG_CHANNEL_REG(chan, INTR), USB2OTG_INTR_CLEAR_ALL);

                    /* Update Actual so class driver sees accurate progress. */
                    usb2otg_bulk_out_advance_pktcnt(otg_Unit, chan, req, wd_tsize);

                    /*
                     * Wedged INT: requeue with fresh timing instead of
                     * failing. UHIOERR_TIMEOUT on an interrupt pipe makes
                     * the class drivers (hid/hub) treat the device as
                     * unplugged — a lost poll is harmless. Only give up
                     * (and poke the port-change heuristic below) after
                     * several consecutive wedges. Counter in DP2, reset
                     * on completion/NAK liveness.
                     */
                    if (req->iouh_Req.io_Command == UHCMD_INTXFER)
                    {
                        ULONG wedges = (ULONG)(IPTR)req->iouh_DriverPrivate2 + 1;

                        /*
                         * An INT channel silent for the whole watchdog
                         * period with entries still in the periodic
                         * request queue = orphaned entry (CSPLIT for a
                         * halted channel) jamming the queue head and
                         * starving every INT channel. Clear it before
                         * requeueing, or the retry wedges the same way.
                         */
                        if (usb2otg_periodic_queue_jammed())
                            usb2otg_recover_periodic_queue(otg_Unit);

                        if (wedges <= USB2OTG_INT_WEDGE_RETRY_LIMIT)
                        {
                            ULONG frnm_now = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;
                            ULONG interval = usb2otg_clamp_interval(req->iouh_Interval);

                            if ((req->iouh_Flags & UHFF_SPLITTRANS) && interval < 2)
                                interval = 2;

                            bug("[USB2OTG] Watchdog: chan=%d wedged INT dev=%d ep=%d — requeueing (wedge %lu/%d)\n",
                                chan,
                                (int)req->iouh_DevAddr,
                                (int)req->iouh_Endpoint,
                                (unsigned long)wedges,
                                USB2OTG_INT_WEDGE_RETRY_LIMIT);

                            req->iouh_DriverPrivate2 = (APTR)(IPTR)wedges;
                            req->iouh_DriverPrivate1 =
                                (APTR)(IPTR)((frnm_now << 16) |
                                       ((frnm_now + interval) & 0x7ff));
                            ADDTAIL(&otg_Unit->hu_IntXFerQueue, (struct Node *)req);
#if defined(__AROSEXEC_SMP__)
                            KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                            Enable();
                            continue;
                        }
                        /* Retry budget exhausted — fail for real below. */
                    }

                    /*
                     * Repeatedly wedged split INT-IN with INTR=0 ≈ device
                     * behind TT unplugged. Poke root-hub port-change so
                     * Poseidon re-walks the tree.
                     */
                    if ((req->iouh_Req.io_Command == UHCMD_INTXFER) &&
                        (req->iouh_Dir == UHDIR_IN) &&
                        (req->iouh_Flags & UHFF_SPLITTRANS) &&
                        (w_intr == 0))
                    {
                        otg_Unit->hu_HubPortChanged = TRUE;
                    }

                    req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                    usb2otg_bulk_finish(otg_Unit, chan, req);
                    /* Abandoned split → unjam the TT (see helper). */
                    usb2otg_queue_clear_tt_buffer(otg_Unit, req);
                    ADDTAIL(&otg_Unit->hu_FinishedXfers, (struct Node *)req);
#if defined(__AROSEXEC_SMP__)
                    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
                    Enable();
                    watchdog_finished = TRUE;
                }
                } /* wd_thresh scope */
            }
        }
    }

    if (watchdog_finished)
        FNAME_DEV(Cause)(otg_Unit->hu_USB2OTGBase, &otg_Unit->hu_PendingInt);
#endif

    FNAME_ROOTHUB(PendingIO)(otg_Unit);

    /* Safety net: promote overdue INT transfers SOF may have missed. */
    {
        struct IOUsbHWReq *req = NULL, *next = NULL;
        ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;
        BOOL promoted = FALSE;
        int q_count = 0, s_count = 0, f_count = 0;

        Disable();
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
        ForeachNodeSafe(&otg_Unit->hu_IntXFerQueue, req, next)
        {
            ULONG last_handled = (ULONG)(IPTR)req->iouh_DriverPrivate1 >> 16;
            ULONG next_to_handle = (ULONG)(IPTR)req->iouh_DriverPrivate1 & 0x7ff;

            q_count++;

            /* See the SOF walk: a cross-linked node makes this cycle. */
            if (q_count > 128)
            {
                bug("[USB2OTG] WD: IntXFerQueue walk runaway — list cycle, aborting walk\n");
                break;
            }

            /* Wrap-safe due test — see the SOF promotion for rationale. */
            if (((frnm - last_handled) & 0x7ff) >=
                ((next_to_handle - last_handled) & 0x7ff))
            {
                REMOVE(req);
                ADDTAIL(&otg_Unit->hu_IntXFerScheduled, req);
                promoted = TRUE;
                q_count--;
            }
        }

        ForeachNode(&otg_Unit->hu_IntXFerScheduled, req)
            s_count++;
        ForeachNode(&otg_Unit->hu_FinishedXfers, req)
            f_count++;

#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
        Enable();

        if (promoted)
            FNAME_DEV(ScheduleIntTDs)(otg_Unit);
    }

    /*
     * Hotplug diagnostic (~5 s cadence): per-device INT pipe activity
     * (cumulative poll/comp/nak/chh) + queue/channel occupancy + ctrl
     * lifecycle counters, so a stalled pipe is visible at a glance.
     */
#if DEBUG
    if (++otg_Unit->hu_IntStatsTicks >= 34)
    {
        struct IOUsbHWReq *dreq;
        int d;

        otg_Unit->hu_IntStatsTicks = 0;

        bug("[USB2OTG:INTSTAT2] p/c/n/h:");
        for (d = 0; d < 8; d++)
        {
            if (otg_Unit->hu_IntPollCount[d] || otg_Unit->hu_IntCompCount[d] ||
                otg_Unit->hu_IntNakCount[d] || otg_Unit->hu_IntChhCount[d])
                bug(" d%d=%lu/%lu/%lu/%lu[seen=%04lx arm=%04lx/%08lx hlt=%04lx]", d,
                    (unsigned long)otg_Unit->hu_IntPollCount[d],
                    (unsigned long)otg_Unit->hu_IntCompCount[d],
                    (unsigned long)otg_Unit->hu_IntNakCount[d],
                    (unsigned long)otg_Unit->hu_IntChhCount[d],
                    (unsigned long)otg_Unit->hu_IntChhSeenIntr[d],
                    (unsigned long)otg_Unit->hu_IntArmHfnum[d],
                    (unsigned long)otg_Unit->hu_IntArmChar[d],
                    (unsigned long)otg_Unit->hu_IntHltHfnum[d]);
        }

        Disable();
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
        bug(" q:");
        ForeachNode(&otg_Unit->hu_IntXFerQueue, dreq)
            bug(" %d.%d/i%d", (int)dreq->iouh_DevAddr, (int)dreq->iouh_Endpoint,
                (int)dreq->iouh_Interval);
        bug(" s:");
        ForeachNode(&otg_Unit->hu_IntXFerScheduled, dreq)
            bug(" %d.%d", (int)dreq->iouh_DevAddr, (int)dreq->iouh_Endpoint);
        bug(" c:");
        for (d = 0; d < 8; d++)
        {
            dreq = otg_Unit->hu_Channel[d].hc_Request;
            if (dreq != NULL)
                bug(" %d=%d.%d", d,
                    (int)dreq->iouh_DevAddr, (int)dreq->iouh_Endpoint);
        }
        bug(" ctlq:");
        ForeachNode(&otg_Unit->hu_CtrlXFerQueue, dreq)
            bug(" %d/%02x", (int)dreq->iouh_DevAddr,
                (unsigned)dreq->iouh_SetupData.bRequest);
        bug(" blkq:");
        ForeachNode(&otg_Unit->hu_BulkXFerQueue, dreq)
            bug(" %d.%d", (int)dreq->iouh_DevAddr, (int)dreq->iouh_Endpoint);
        bug(" HPRT=%08x HFN=%04x", rd32le(USB2OTG_HOSTPORT),
            (unsigned)(rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff));
        bug(" dead=%02x ctl=%lu/%lu/%lu(%02x) ctlf=%lu/%lu/%lu tick=%lu",
            (unsigned)otg_Unit->hu_DeadChannels,
            (unsigned long)otg_Unit->hu_CtrlArmCount,
            (unsigned long)otg_Unit->hu_CtrlFinCount,
            (unsigned long)otg_Unit->hu_CtrlErrCount,
            (unsigned)otg_Unit->hu_CtrlLastErr,
            (unsigned long)otg_Unit->hu_CtrlNakRequeues,
            (unsigned long)otg_Unit->hu_CtrlChhRequeues,
            (unsigned long)otg_Unit->hu_CtrlXactRetries,
            (unsigned long)otg_Unit->hu_WdTicks);
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
        Enable();
        bug("\n");
    }
#endif /* DEBUG */

    /*
     * Post-core-reset port bring-up SM. Mirrors the Init/OpenUnit
     * sequence (wait connect -> PRTRST >=60 ms -> deassert -> settle)
     * with 150 ms tick granularity; reports the connect to Poseidon
     * only when the port is actually up. Runs off the timer, so a
     * stopped frame counter (port disabled => no SOF) cannot stall it.
     */
#if USB2OTG_DEBUG_FORCE_RECOVER_TICK > 0
    {
        static BOOL force_recover_done = FALSE;

        if (!force_recover_done &&
            otg_Unit->hu_WdTicks >= USB2OTG_DEBUG_FORCE_RECOVER_TICK &&
            otg_Unit->hu_PortRecoverState == 0)
        {
            force_recover_done = TRUE;
            bug("[USB2OTG:RECOVER] DEBUG: forcing power-cycle recovery (tick=%lu)\n",
                (unsigned long)otg_Unit->hu_WdTicks);
            otg_Unit->hu_PortRecoverState = 10;
            otg_Unit->hu_PortRecoverTick = 0;
        }
    }
#endif

    switch (otg_Unit->hu_PortRecoverState)
    {
        case 10: /* power-cycle the USB block — cold-boot equivalence */
        {
            bug("[USB2OTG:RECOVER] power-cycling USB block via mailbox\n");
            /* IRQ blackout for the OFF window + reinit (~30-40 ms):
             * the IRQ handler must not touch a powered-down block. */
            Disable();
            if (!usb2otg_set_usb_power(FALSE))
                bug("[USB2OTG:RECOVER] power OFF failed\n");
            if (!usb2otg_set_usb_power(TRUE))
                bug("[USB2OTG:RECOVER] power ON failed\n");
            usb2otg_core_reset_recover(otg_Unit, 0);
            Enable();
            otg_Unit->hu_PortRecoverState = 1;
            otg_Unit->hu_PortRecoverTick = 0;
            break;
        }
        case 1: /* wait for the hub chip to reconnect */
        {
            ULONG hprt = rd32le(USB2OTG_HOSTPORT);

            if (hprt & USB2OTG_HOSTPORT_PRTCONNSTS)
            {
                /*
                 * Connect debounce (TATTDB, USB 2.0 §7.1.7.3, 100 ms)
                 * before driving reset — the boot path gets this from
                 * its 100 ms DoIO wait. Resetting inside the debounce
                 * interval (HPRT still showing the pre-HS-detect line
                 * state, PrtSpd=FS) can leave the hub unresponsive.
                 */
                bug("[USB2OTG:RECOVER] port connected (HPRT=%08x), debouncing\n", hprt);
                otg_Unit->hu_PortRecoverState = 4;
            }
            else if (++otg_Unit->hu_PortRecoverTick > 67) /* ~10 s */
            {
                bug("[USB2OTG:RECOVER] port never reconnected (HPRT=%08x) — giving up\n", hprt);
                otg_Unit->hu_PortRecoverState = 0;
            }
            break;
        }
        case 4: /* debounce elapsed (>=150 ms) — now drive reset */
            bug("[USB2OTG:RECOVER] debounce done (HPRT=%08x), asserting reset\n",
                rd32le(USB2OTG_HOSTPORT));
            usb2otg_hostport_rmw(USB2OTG_HOSTPORT_PRTRST, 0);
            otg_Unit->hu_PortRecoverState = 2;
            break;
        case 2: /* PRTRST held >=1 tick (150 ms > 60 ms minimum) */
            usb2otg_hostport_rmw(0, USB2OTG_HOSTPORT_PRTRST);
            otg_Unit->hu_PortRecoverState = 3;
            break;
        case 3: /* settled 1 tick — report the change */
        {
            bug("[USB2OTG:RECOVER] port bring-up done (HPRT=%08x)\n",
                rd32le(USB2OTG_HOSTPORT));
            usb2otg_dump_core_state("recover");
            Disable();
#if defined(__AROSEXEC_SMP__)
            KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
            /*
             * Present the recovery as unplug-then-replug of the root
             * port — the flow Poseidon fully supports. Phase 1:
             * synthetic DISCONNECT (the hub class only tears down and
             * later re-enumerates on !connected→connected; a lone
             * connect-change with the old device tree still present is
             * discarded). Phase 2 (reconnect) is flipped on by the
             * CLEAR_FEATURE(C_PORT_CONNECTION) handler.
             */
            otg_Unit->hu_PortForceDisconnect = TRUE;
            otg_Unit->hu_PortForceConnChange = FALSE;
            otg_Unit->hu_HubPortChanged = TRUE;
#if defined(__AROSEXEC_SMP__)
            KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
            Enable();
            FNAME_DEV(Cause)(otg_Unit->hu_USB2OTGBase, &otg_Unit->hu_PendingInt);
            otg_Unit->hu_PortRecoverState = 0;
            break;
        }
        default:
            break;
    }

    /* Backed-off ctrl retries: ensure <=150 ms scheduling latency. */
    if (!IsListEmpty(&otg_Unit->hu_CtrlXFerQueue))
        FNAME_DEV(ScheduleCtrlTDs)(otg_Unit);

    otg_Unit->hu_NakTimeoutReq.tr_time.tv_secs = 0;
    otg_Unit->hu_NakTimeoutReq.tr_time.tv_micro = 150 * 1000;
    SendIO((APTR) &otg_Unit->hu_NakTimeoutReq);

    return FALSE;
}

AROS_INTH1(FNAME_DEV(NakTimeoutInt), struct USB2OTGUnit *, otg_Unit)
{
    AROS_INTFUNC_INIT

    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    BOOL ret;

    if (!usb2otg_require_cpu0(USB2OTGBase, __PRETTY_FUNCTION__))
        return FALSE;

    /* See PendingInt: serialize the watchdog against GlobalIRQHandler. */
    Disable();
    ret = usb2otg_process_naktimeout(otg_Unit);
    Enable();

    return ret;

    AROS_INTFUNC_EXIT
}
