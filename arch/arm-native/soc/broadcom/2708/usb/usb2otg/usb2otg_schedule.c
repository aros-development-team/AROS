/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "usb2otg_intern.h"

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

/* Forward decls — helpers defined further down in this file */
static int usb2otg_endpoint_in_flight(struct USB2OTGUnit *otg_Unit,
    struct IOUsbHWReq *candidate);
static int usb2otg_tt_in_flight(struct USB2OTGUnit *otg_Unit,
    struct IOUsbHWReq *candidate);

/* TRUE if dev_addr has un-gated bulk work queued (for channel reclaim). */
static BOOL usb2otg_dev_has_ready_bulk(struct USB2OTGUnit *otg_Unit,
    UBYTE dev_addr)
{
    struct IOUsbHWReq *req;

    if (dev_addr == 0)
        return FALSE;
    if (usb2otg_nak_gated(otg_Unit, dev_addr))
        return FALSE;

    ForeachNode(&otg_Unit->hu_BulkXFerQueue, req)
    {
        if (req->iouh_Req.io_Command == UHCMD_BULKXFER &&
            req->iouh_DevAddr == dev_addr)
            return TRUE;
    }
    return FALSE;
}

/* TRUE if any device != skip_dev has ready bulk work (fairness). */
static BOOL usb2otg_other_dev_has_ready_bulk(struct USB2OTGUnit *otg_Unit,
    UBYTE skip_dev)
{
    struct IOUsbHWReq *req;

    ForeachNode(&otg_Unit->hu_BulkXFerQueue, req)
    {
        if (req->iouh_Req.io_Command == UHCMD_BULKXFER &&
            req->iouh_DevAddr != skip_dev &&
            !usb2otg_nak_gated(otg_Unit, req->iouh_DevAddr))
            return TRUE;
    }
    return FALSE;
}

/*
 * Find/claim a bulk channel for req. Prefer one already owned by
 * req's device; otherwise steal from an idle owner with no ready work.
 * Returns -1 if nothing usable. Caller holds hu_Lock.
 */
static int usb2otg_claim_bulk_channel(struct USB2OTGUnit *otg_Unit,
    struct IOUsbHWReq *req)
{
    static const UBYTE bulk_chans[2] = { CHAN_BULK, CHAN_BULK2 };
    BOOL other_waiting;
    int max_chans_per_dev;
    int own_busy = 0;
    int own_idle_chan = -1;
    int i;

    /* Fairness cap: contention → 1 channel/dev; alone → both allowed. */
    other_waiting = usb2otg_other_dev_has_ready_bulk(otg_Unit,
                                                     req->iouh_DevAddr);
    max_chans_per_dev = other_waiting ? 1 : 2;

    /* Inventory: how many channels does req's device currently hold? */
    for (i = 0; i < 2; i++)
    {
        int chan = bulk_chans[i];
        if (otg_Unit->hu_BulkOwnerDev[i] != req->iouh_DevAddr)
            continue;
        if (otg_Unit->hu_Channel[chan].hc_Request != NULL)
            own_busy++;
        else if (own_idle_chan < 0)
            own_idle_chan = chan;
    }

    /* Sticky locality: reuse own idle channel if within cap. */
    if (own_idle_chan >= 0 && own_busy + 1 <= max_chans_per_dev)
        return own_idle_chan;

    /* Already at cap on busy channels alone — no room for more. */
    if (own_busy >= max_chans_per_dev)
        return -1;

    /* Pass 2: claim unowned, or steal from owner who already has another busy. */
    for (i = 0; i < 2; i++)
    {
        int chan = bulk_chans[i];
        UBYTE owner = otg_Unit->hu_BulkOwnerDev[i];

        if (otg_Unit->hu_Channel[chan].hc_Request != NULL)
            continue;
        if (owner == req->iouh_DevAddr)
            continue;  /* handled in inventory pass */

        if (owner != 0 && usb2otg_dev_has_ready_bulk(otg_Unit, owner))
        {
            /* Owner has queued work — only steal if they already hold another busy chan. */
            int j;
            int owner_busy = 0;
            for (j = 0; j < 2; j++)
            {
                if (otg_Unit->hu_BulkOwnerDev[j] == owner &&
                    otg_Unit->hu_Channel[bulk_chans[j]].hc_Request != NULL)
                    owner_busy++;
            }
            if (owner_busy < 1)
                continue;
        }

        otg_Unit->hu_BulkOwnerDev[i] = req->iouh_DevAddr;
        return chan;
    }

    return -1;
}

/*
 * Walk bulk queue, return first request passing NAK-gate / TT / EP /
 * retry filters that can claim a channel. Caller holds hu_Lock.
 */
static struct IOUsbHWReq *usb2otg_find_schedulable_bulk_req(
    struct USB2OTGUnit *otg_Unit, int *out_chan)
{
    struct IOUsbHWReq *req, *next;

    ForeachNodeSafe(&otg_Unit->hu_BulkXFerQueue, req, next)
    {
        int chan;
        ULONG delay;
        BOOL already_armed = FALSE;
        int scan;

        if (req->iouh_Req.io_Command != UHCMD_BULKXFER)
            continue;

        /* Defensive: skip if req is already armed on a channel. */
        for (scan = 0; scan < 8; scan++)
        {
            if (otg_Unit->hu_Channel[scan].hc_Request == req)
            {
                already_armed = TRUE;
                break;
            }
        }
        if (already_armed)
            continue;

        if (usb2otg_nak_gated(otg_Unit, req->iouh_DevAddr))
            continue;
        if (usb2otg_tt_in_flight(otg_Unit, req))
            continue;
        if (usb2otg_endpoint_in_flight(otg_Unit, req))
            continue;

        /* Legacy per-request retry-delay counter (call-count, not wall time). */
        delay = (ULONG)req->iouh_DriverPrivate1;
        if (delay > 0)
        {
            req->iouh_DriverPrivate1 = (APTR)(delay - 1);
            continue;
        }

        chan = usb2otg_claim_bulk_channel(otg_Unit, req);
        if (chan < 0)
            continue;

        *out_chan = chan;
        return req;
    }

    *out_chan = -1;
    return NULL;
}

/*
 * 1 if another channel already holds same (Dev,EP,Dir) — arming a
 * second would NYET forever on shared TT endpoint state.
 */
static int usb2otg_endpoint_in_flight(struct USB2OTGUnit *otg_Unit,
    struct IOUsbHWReq *candidate)
{
    int scan;
    for (scan = 0; scan < 8; scan++)
    {
        struct IOUsbHWReq *active = otg_Unit->hu_Channel[scan].hc_Request;
        if (active != NULL && active != candidate &&
            active->iouh_DevAddr == candidate->iouh_DevAddr &&
            active->iouh_Endpoint == candidate->iouh_Endpoint &&
            active->iouh_Dir == candidate->iouh_Dir)
        {
            return 1;
        }
    }
    return 0;
}

/*
 * 1 if another async split is already active on the same TT
 * (SplitHubAddr+Port). Serializes async SSPLITs so two LS/FS devices
 * don't over-subscribe TT budget (NYET storm). Periodic candidates
 * bypass (USB 2.0 §11.18 separate periodic budget).
 */
static int usb2otg_tt_in_flight(struct USB2OTGUnit *otg_Unit,
    struct IOUsbHWReq *candidate)
{
    int scan;

    if (!(candidate->iouh_Flags & UHFF_SPLITTRANS))
        return 0;

    /* Periodic candidates have their own TT budget — don't block them. */
    if (candidate->iouh_Req.io_Command == UHCMD_INTXFER ||
        candidate->iouh_Req.io_Command == UHCMD_ISOXFER)
        return 0;

    for (scan = 0; scan < 8; scan++)
    {
        struct IOUsbHWReq *active = otg_Unit->hu_Channel[scan].hc_Request;
        if (active != NULL && active != candidate &&
            (active->iouh_Flags & UHFF_SPLITTRANS) &&
            active->iouh_SplitHubAddr == candidate->iouh_SplitHubAddr &&
            active->iouh_SplitHubPort == candidate->iouh_SplitHubPort &&
            (active->iouh_Req.io_Command == UHCMD_CONTROLXFER ||
             active->iouh_Req.io_Command == UHCMD_BULKXFER))
        {
            return 1;
        }
    }
    return 0;
}


/* DMA buffers live in USB2OTGUnit (heap), not static — see intern.h. */

#if DEBUG
static void DumpChannelRegs(int channel)
{
    D(bug("CHARBASE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, CHARBASE))));
    D(bug("SPLITCTRL=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, SPLITCTRL))));
    D(bug("INTR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTR))));
    D(bug("INTRMASK=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTRMASK))));
    D(bug("TRANSSIZE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, TRANSSIZE))));
    D(bug("DMAADR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, DMAADDR))));
}
#endif

static void usb2otg_finish_setup_error(struct USB2OTGUnit *otg_Unit, int chan,
    struct IOUsbHWReq *req, BYTE error)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    req->iouh_Req.io_Error = error;
    otg_Unit->hu_Channel[chan].hc_Request = NULL;
    otg_Unit->hu_Channel[chan].hc_OrigBuffer = NULL;
    otg_Unit->hu_Channel[chan].hc_SplitCSplitPending = 0;
    ADDTAIL(&otg_Unit->hu_FinishedXfers, (struct Node *)req);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&otg_Unit->hu_Lock);
#endif
    Enable();

    FNAME_DEV(Cause)(otg_Unit->hu_USB2OTGBase, &otg_Unit->hu_PendingInt);
}

/* Prepare Channel for transfer */
BOOL FNAME_DEV(SetupChannel)(struct USB2OTGUnit *otg_Unit, int chan)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    struct IOUsbHWReq *req = NULL;
    uint8_t direction = 0;
    ULONG xfer_size = 0;
    ULONG pkt_count = 0;
    APTR buffer = NULL;
    ULONG reg = 0;
    int pid = 0;

    if (chan < 0 || chan > 7)
        return FALSE;

    /* Caller just assigned hc_Request — no lock needed to read it */
    req = otg_Unit->hu_Channel[chan].hc_Request;

    D(bug("[USB2OTG] %s(%p, %d)\n", __PRETTY_FUNCTION__, otg_Unit, chan));

    if (req == NULL)
        return FALSE;

    /* Fresh request: clear watchdog defer counter. */
    otg_Unit->hu_Channel[chan].hc_DeferCount = 0;

    /*
     * Inherit PING state from per-EP bitmap (channel-local flag does
     * not survive requeue; USB 2.0 §8.5.1).
     */
    otg_Unit->hu_Channel[chan].hc_PingPending =
        (otg_Unit->hu_PingBits[req->iouh_DevAddr] >> req->iouh_Endpoint) & 1;
    otg_Unit->hu_Channel[chan].hc_PingState =
        otg_Unit->hu_Channel[chan].hc_PingPending ? 1 : 0;

    /* Control xfer: set up SETUP packet buffers, flush caches, direction. */
    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        req->iouh_Actual = 0;

        /* Direction is out, setup packet goes always to the device */
        direction = 0;

        /* Buffer points to the setup data, 8 bytes */
        buffer = &req->iouh_SetupData;
        xfer_size = 8;

        /* Flush caches for setup data. The control data stage is armed
         * separately by AdvanceChannel(), which does its own cache
         * maintenance against the bounce or direct DMA target -- doing
         * it here on the caller's iouh_Data would only force redundant
         * work on a buffer we don't yet know the alignment-routing of. */
        CacheClearE(buffer, xfer_size, CACRF_ClearD);
        if (req->iouh_Data != NULL && req->iouh_Length != 0
            && !(req->iouh_SetupData.bmRequestType & URTF_IN))
        {
            CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_ClearD);
        }

        /* Set toggle bit to SETUP */
        otg_Unit->hu_PIDBits[req->iouh_DevAddr] &= ~(3 << (2 * req->iouh_Endpoint));
        otg_Unit->hu_PIDBits[req->iouh_DevAddr] |= (USB2OTG_PID_SETUP << (2 * req->iouh_Endpoint));

        /* Get pid */
        pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;
    }
    else
    {
        /* Data direction as requested */
        if (req->iouh_Dir == UHDIR_IN) direction = 1; else direction = 0;

        if (req->iouh_Actual > req->iouh_Length)
        {
            bug("[USB2OTG] Refusing to resume chan=%d dev=%d ep=%d cmd=%lu with actual=%lu > len=%lu\n",
                chan,
                (int)req->iouh_DevAddr,
                (int)req->iouh_Endpoint,
                (unsigned long)req->iouh_Req.io_Command,
                (unsigned long)req->iouh_Actual,
                (unsigned long)req->iouh_Length);
            usb2otg_finish_setup_error(otg_Unit, chan, req, UHIOERR_HOSTERROR);
            return FALSE;
        }

        /* Resume non-control transfers from the current request offset. */
        buffer = req->iouh_Data ? (APTR)((IPTR)req->iouh_Data + req->iouh_Actual) : NULL;
        xfer_size = req->iouh_Length - req->iouh_Actual;

        /* Cache flush is deferred until after the bounce-vs-direct
         * decision below, mirroring AdvanceChannel(). For IN, flushing
         * the caller buffer before that decision can corrupt adjacent
         * heap on partial cache lines when the buffer is misaligned. */

        /* Alignment diagnostic (64 B cache line on A7). */
        D(
            if (buffer != NULL && xfer_size != 0 && req->iouh_Req.io_Command == UHCMD_BULKXFER)
            {
                static ULONG align_count = 0;
                align_count++;
                if (align_count <= 5 || (align_count & 0xff) == 0)
                    bug("[USB2OTG:DMA] setup chan=%d dev=%d ep=%d dir=%s buf=%p (mod64=%lu) size=%lu\n",
                        chan, (int)req->iouh_DevAddr, (int)req->iouh_Endpoint,
                        direction ? "IN" : "OUT", buffer,
                        (unsigned long)((IPTR)buffer & 0x3F),
                        (unsigned long)xfer_size);
            }
        )

        /* Get pid */
        pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;
    }

    /* Flush non-periodic TX FIFO before programming the channel */
    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        wr32le(USB2OTG_RESET, USB2OTG_RESET_TXFIFOFLUSH | (0 << 6));
        /* Wait for hardware to clear the flush bit */
        {
            int timeout = 100000;
            while ((rd32le(USB2OTG_RESET) & USB2OTG_RESET_TXFIFOFLUSH) && --timeout > 0)
                asm volatile("mov r0, r0\n");
        }
    }

    if (req->iouh_MaxPktSize == 0)
    {
        bug("[USB2OTG] Refusing to start chan=%d dev=%d ep=%d cmd=%lu with MaxPktSize=0 len=%lu flags=%04x\n",
            chan,
            (int)req->iouh_DevAddr,
            (int)req->iouh_Endpoint,
            (unsigned long)req->iouh_Req.io_Command,
            (unsigned long)req->iouh_Length,
            (unsigned int)req->iouh_Flags);
        usb2otg_finish_setup_error(otg_Unit, chan, req, UHIOERR_HOSTERROR);
        return FALSE;
    }

    /* Prepare HOSTCHAR register */
    reg = USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
          USB2OTG_HOSTCHAR_EPDIR(direction) |
          USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
          USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize);

    if (req->iouh_Flags & UHFF_LOWSPEED)
        reg |= USB2OTG_HOSTCHAR_LOWSPEED;

    switch(req->iouh_Req.io_Command)
    {
        case UHCMD_CONTROLXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL);
            break;
        case UHCMD_INTXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_INT);
            break;
        case UHCMD_BULKXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_BULK);
            break;
        case UHCMD_ISOXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_ISO);
            break;
        default:
            bug("[USB2OTG] Refusing to start chan=%d dev=%d ep=%d with unsupported cmd=%lu\n",
                chan, (int)req->iouh_DevAddr, (int)req->iouh_Endpoint,
                (unsigned long)req->iouh_Req.io_Command);
            usb2otg_finish_setup_error(otg_Unit, chan, req, UHIOERR_HOSTERROR);
            return FALSE;
    }

    /* If split transaction requested limit transfer size to max packet size or 188 bytes, whichever is less */
    if (req->iouh_Flags & UHFF_SPLITTRANS)
    {
        if (req->iouh_Req.io_Command == UHCMD_INTXFER ||
            req->iouh_Req.io_Command == UHCMD_ISOXFER)
            reg |= USB2OTG_HOSTCHAR_EC(3);
        else
            reg |= USB2OTG_HOSTCHAR_EC(1);

        {
            ULONG splitpos;

            /*
             * splitpos per USB 2.0 §11.18: 0=CSPLIT (after SSPLIT ACK),
             * 3=ALL (periodic), 2=BEGIN (async — must advance via IRQ).
             */
            if (otg_Unit->hu_Channel[chan].hc_SplitCSplitPending)
                splitpos = 0; /* CSPLIT */
            else if (req->iouh_Req.io_Command == UHCMD_INTXFER ||
                     req->iouh_Req.io_Command == UHCMD_ISOXFER)
                splitpos = 3; /* ALL */
            else
                splitpos = 2; /* BEGIN */

            wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL),
                    (1 << 31) |
                    (splitpos << 14) |
                    ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                    ((req->iouh_SplitHubPort & 0x0f)));
        }

        if (xfer_size > req->iouh_MaxPktSize)
            xfer_size = req->iouh_MaxPktSize;

        /* 188 bytes is maximal payload in single split transaction */
        if (xfer_size > 188)
            xfer_size = 188;
    }
    else
    {
        reg |= USB2OTG_HOSTCHAR_EC(1);

        wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), 0);

        /*
         * Bulk OUT: full multi-packet burst. hu_PingBits tracks PING
         * state across requeues; WAIT-PING/WAIT-QUIET defer is
         * unbounded, so DWC2 auto-PING during a long burst is safe.
         */
    }

    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), reg);

    /* Large unaligned buffers must be chunked through the bounce buffer.
     * Alignment criterion is 64 bytes (cache line) so per-buffer cache
     * flush doesn't drag in adjacent unrelated data on line boundaries. */
    if (buffer != NULL && ((ULONG)buffer & 0x3F) && xfer_size > DMA_BOUNCE_SIZE)
        xfer_size = DMA_BOUNCE_SIZE;

    /* Get packet count and adjust this and the transfer size */
    if (xfer_size > (1 << (otg_Unit->hu_XferSizeWidth - 1)))
        xfer_size = 1 << (otg_Unit->hu_XferSizeWidth - 1);

    pkt_count = (xfer_size + req->iouh_MaxPktSize - 1) / req->iouh_MaxPktSize;
    if (pkt_count > ((1 << (otg_Unit->hu_PktSizeWidth)) - 1))
    {
        pkt_count = (1 << (otg_Unit->hu_PktSizeWidth)) - 1;
        xfer_size = pkt_count * req->iouh_MaxPktSize;
    }

    /*
     * Setup the size, PID, packet count and length. OR in DoPing bit
     * if the NYET handler set hc_PingPending — per USB 2.0 §8.5.1 we
     * must PING after NYET until the device ACKs. HW auto-DoPing gets
     * cleared when we re-program HCTSIZ, so we have to re-apply it
     * explicitly on behalf of the software state machine.
     */
    {
        ULONG tsize = USB2OTG_HOSTTSIZE_PID(pid) |
                      USB2OTG_HOSTTSIZE_PKTCNT(pkt_count) |
                      USB2OTG_HOSTTSIZE_SIZE(xfer_size);
        if (otg_Unit->hu_Channel[chan].hc_PingPending)
        {
            /* HW-driven PING+DATA combined arm. HW issues PING first;
             * on ACK it follows with the OUT data tokens; on NAK/NYET
             * it halts and we re-arm via the requeue path. Letting HW
             * drive the PING flow is what works on BCM2835 — software-
             * driven PING-only arms (XferSize=0) result in bare CHHLTD
             * with no token on the bus (verified empirically on Pi 3B+).
             */
            tsize |= USB2OTG_HOSTTSIZE_PING;
            D(
                {
                    static ULONG ping_arm_count = 0;
                    ping_arm_count++;
                    if (ping_arm_count <= 3 || (ping_arm_count & 0x3f) == 0)
                        bug("[USB2OTG:PING] ARM #%lu chan=%d dev=%d ep=%d TSIZE=%08x\n",
                            (unsigned long)ping_arm_count, chan,
                            (int)req->iouh_DevAddr, (int)req->iouh_Endpoint, tsize);
                }
            )
        }
        wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE), tsize);
    }

    otg_Unit->hu_Channel[chan].hc_XferSize = xfer_size;

    /*
     * DMA alignment: route any non-cache-line-aligned (64 B) buffer
     * through the aligned bounce buffer. DWC2 itself only requires
     * DWORD alignment, but the per-buffer cache flush operates on
     * full cache lines — a misaligned buffer would drag adjacent
     * unrelated bytes across the flush, which is harmless for OUT
     * but a real cache-pollution risk for IN.
     */
    otg_Unit->hu_Channel[chan].hc_OrigBuffer = NULL;
    {
        APTR dma_buf = buffer;
        if (buffer != NULL && ((ULONG)buffer & 0x3F) && xfer_size <= DMA_BOUNCE_SIZE)
        {
            otg_Unit->hu_Channel[chan].hc_OrigBuffer = buffer;
            otg_Unit->hu_Channel[chan].hc_BounceLen = xfer_size;
            otg_Unit->hu_Channel[chan].hc_BounceDir = direction;

            if (direction)
                CacheClearE(otg_Unit->hu_BounceBuf[chan], xfer_size, CACRF_InvalidateD);
            else
            {
                CopyMem(buffer, otg_Unit->hu_BounceBuf[chan], xfer_size);
                CacheClearE(otg_Unit->hu_BounceBuf[chan], xfer_size, CACRF_ClearD);
            }

            dma_buf = (APTR)otg_Unit->hu_BounceBuf[chan];
            D(bug("[USB2OTG] Setup: bounce align %p -> %p (%d bytes)\n", buffer, dma_buf, xfer_size));
        }
        else if (buffer != NULL && xfer_size != 0
                 && req->iouh_Req.io_Command != UHCMD_CONTROLXFER)
        {
            /* Aligned direct DMA -- arm-time cache flush. Control data
             * stage is flushed by AdvanceChannel() when it arms. */
            if (direction)
                CacheClearE(buffer, xfer_size, CACRF_InvalidateD);
            else
                CacheClearE(buffer, xfer_size, CACRF_ClearD);
        }
        asm volatile("dsb sy" ::: "memory");
        wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), 0xc0000000 | (ULONG)dma_buf);
    }

    /* Dump SETUP data and DMA info for control transfers to help debug STALL/XactErr */
    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        UBYTE *p = (UBYTE *)buffer;
        D(bug("[USB2OTG] Setup: dev=%d buf=%p dma=%08x data=[%02x %02x %02x %02x %02x %02x %02x %02x]\n",
            req->iouh_DevAddr, buffer,
            0xc0000000 | (ULONG)buffer,
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
        D(bug("[USB2OTG] Setup: CHAR=%08x TSIZE=%08x pid=%d mps=%d len=%d data=%p\n",
            rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
            rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)),
            pid, req->iouh_MaxPktSize, req->iouh_Length, req->iouh_Data));
    }

    return TRUE;
}

/*
 * Advance channel state after an XFERCOMPL: update iouh_Actual, toggle
 * PID, finalise direction. Returns 1 (done) when the IOReq is fully
 * complete and the caller should put it on FinishedXfers; returns 0
 * (more) when this was an intermediate phase and the caller must
 * StartChannel again.
 */
int FNAME_DEV(AdvanceChannel)(struct USB2OTGUnit *otg_Unit, int chan)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    uint8_t direction = 0;
    ULONG xfer_size = 0;
    ULONG pkt_count = 0;
    APTR buffer = NULL;
    ULONG reg = 0;
    int done = 1;

    struct IOUsbHWReq *req;
    ULONG phase_actual;
    APTR completed_buf = NULL;
    UBYTE completed_dir = 0;
    int last_pid;

    /* Caller holds lock or is in IRQ context — no lock needed */
    req = otg_Unit->hu_Channel[chan].hc_Request;

    D(bug("[USB2OTG] %s(%p, %d)\n", __PRETTY_FUNCTION__, otg_Unit, chan));

    /* TRANSSIZE.SIZE decrements only on IN; OUT keeps the requested size. */
    phase_actual = req->iouh_Actual;
    int remaining = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)) & 0x7FFFF;
    int txsize;

    /*
     * Direction of just-completed phase: SETUP is always OUT, data/
     * status follow bmRequestType, bulk/int/iso use iouh_Dir.
     * Bulk-IN ZLPs decrement PktCnt without moving SIZE — must not
     * use the old "SIZE decremented => IN" heuristic.
     */
    {
        int is_in;
        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
        {
            int cur_pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >>
                           (2 * req->iouh_Endpoint)) & 3;
            if (cur_pid == USB2OTG_PID_SETUP)
                is_in = 0;
            else
                is_in = (req->iouh_SetupData.bmRequestType & URTF_IN) ? 1 : 0;
        }
        else
        {
            is_in = (req->iouh_Dir == UHDIR_IN) ? 1 : 0;
        }

        if (is_in)
        {
            if (remaining > otg_Unit->hu_Channel[chan].hc_XferSize)
                txsize = 0; /* Defensive — should never happen */
            else
                txsize = otg_Unit->hu_Channel[chan].hc_XferSize - remaining;
        }
        else
        {
            /* OUT: hardware does not decrement SIZE */
            txsize = otg_Unit->hu_Channel[chan].hc_XferSize;
        }
    }
    last_pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;

    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        if (last_pid == USB2OTG_PID_SETUP)
        {
            completed_buf = &req->iouh_SetupData;
            completed_dir = 0;
        }
        else
        {
            completed_buf = req->iouh_Data ? (APTR)((IPTR)req->iouh_Data + phase_actual) : NULL;
            completed_dir = (req->iouh_SetupData.bmRequestType & URTF_IN) ? 1 : 0;
        }
    }
    else
    {
        completed_buf = req->iouh_Data ? (APTR)((IPTR)req->iouh_Data + phase_actual) : NULL;
        completed_dir = (req->iouh_Dir == UHDIR_IN) ? 1 : 0;
    }

    /* DMA bounce copy-back for IN if previous phase used bounce. */
    if (otg_Unit->hu_Channel[chan].hc_OrigBuffer != NULL)
    {
        if (otg_Unit->hu_Channel[chan].hc_BounceDir == 1 && txsize > 0)
        {
            ULONG copylen = txsize;
            if (copylen > otg_Unit->hu_Channel[chan].hc_BounceLen)
                copylen = otg_Unit->hu_Channel[chan].hc_BounceLen;
            CacheClearE(otg_Unit->hu_BounceBuf[chan], copylen, CACRF_InvalidateD);
            CopyMem(otg_Unit->hu_BounceBuf[chan], otg_Unit->hu_Channel[chan].hc_OrigBuffer, copylen);
            D(bug("[USB2OTG] Advance: bounce copy-back %p <- %p (%d bytes)\n",
                otg_Unit->hu_Channel[chan].hc_OrigBuffer, otg_Unit->hu_BounceBuf[chan], copylen));
        }
        otg_Unit->hu_Channel[chan].hc_OrigBuffer = NULL;
    }
    else if (completed_dir && completed_buf != NULL)
    {
        /*
         * Post-DMA invalidate for direct transfers; ARM speculative
         * prefetch can refill stale lines during the split-CSPLIT gap.
         * Cover ≥1 MaxPktSize so a following IN doesn't read stale.
         */
        ULONG inval_len = txsize > 0 ? txsize : req->iouh_MaxPktSize;
        CacheClearE(completed_buf, inval_len, CACRF_InvalidateD);
    }

    int pid = 0;

    /* Packets from PktCnt delta; txsize/MPS misses ZLPs (no PID toggle). */
    int initial_pktcnt =
        (otg_Unit->hu_Channel[chan].hc_XferSize + req->iouh_MaxPktSize - 1)
            / req->iouh_MaxPktSize;
    if (initial_pktcnt == 0)
        initial_pktcnt = 1;
    int remaining_pktcnt =
        (rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)) >> 19) & 0x3FF;
    int pktcnt = initial_pktcnt - remaining_pktcnt;
    if (pktcnt < 0)
        pktcnt = 0;

    D(bug("[USB2OTG] Advance: chan=%d dev=%d requested=%d actual=%d remaining=%d last_pid=%d len=%d actual_done=%d\n",
        chan, req->iouh_DevAddr, otg_Unit->hu_Channel[chan].hc_XferSize, txsize, remaining,
        last_pid, req->iouh_Length, req->iouh_Actual));

    if (pktcnt & 1)
    {
        /* Toggle PID */
        otg_Unit->hu_PIDBits[req->iouh_DevAddr] ^= (USB2OTG_PID_DATA1 << (2 * req->iouh_Endpoint));
    }
    /* Update the transferred size unless it was control channel in setup phase */
    req->iouh_Actual += txsize;
    req->iouh_Req.io_Error = 0;
    usb2otg_diag_bulk_progress(otg_Unit, chan, req, USB2OTG_INTRCHAN_TRANSFERCOMPLETE);

    /* OUT ACKed → PING phase done; clear channel + per-EP bits. */
    otg_Unit->hu_Channel[chan].hc_PingPending = 0;
    otg_Unit->hu_PingBits[req->iouh_DevAddr] &=
        ~(1U << req->iouh_Endpoint);

    /* If it was CTRL channel in setup phase reset PID and iouh_Actual */
    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        if (req->iouh_SetupData.bmRequestType & URTF_IN) direction = 1; else direction = 0;

        if (last_pid == USB2OTG_PID_SETUP)
        {
            D(bug("[USB2OTG] AdvanceChannel: SETUP phase done, transitioning to DATA phase (dir=%d)\n", direction));
            req->iouh_Actual = 0;
            otg_Unit->hu_PIDBits[req->iouh_DevAddr] &= ~(3 << (2 * req->iouh_Endpoint));
            otg_Unit->hu_PIDBits[req->iouh_DevAddr] |= (USB2OTG_PID_DATA1 << (2 * req->iouh_Endpoint));
        }
    }
    else
    {
        /* Data direction as requested */
        if (req->iouh_Dir == UHDIR_IN) direction = 1; else direction = 0;
    }

    /* Get new PID for transfer */
    pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;

    /* Get buffer und xfer length */
    buffer = (APTR)((IPTR)req->iouh_Data + req->iouh_Actual);
    xfer_size = req->iouh_Length - req->iouh_Actual;

    /*
     * Short-packet (incl. ZLP) on IN terminates the transfer per USB
     * spec. Gate on iouh_Dir, not txsize>0 (OUT never decrements SIZE).
     */
    if (xfer_size > 0 &&
        req->iouh_Dir == UHDIR_IN &&
        req->iouh_Req.io_Command != UHCMD_CONTROLXFER &&
        txsize < otg_Unit->hu_Channel[chan].hc_XferSize)
    {
        xfer_size = 0;
    }

    /* CTRL STATUS/ACK done: txsize==0 past SETUP → transfer complete. */
    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER &&
        txsize == 0 && last_pid != USB2OTG_PID_SETUP)
    {
        return 1;
    }

    D(bug("[USB2OTG] AdvanceChannel: next phase: pid=%d dir=%d buf=%p xfer_size=%d\n",
        pid, direction, buffer, xfer_size));

    /* If there is still anything to transmit, do it now */
    if (xfer_size != 0 && buffer != NULL)
    {
        done = 0;

        D(bug("[USB2OTG] AdvanceChannel: DATA phase buf=%p size=%d\n", buffer, xfer_size));
    }
    else
    {
        /* Nothing to transmit — issue CTRL ACK phase unless last was empty. */
        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER && txsize != 0)
        {
            done = 0;
            buffer = (APTR)otg_Unit->hu_StatusDmaBuf;
            xfer_size = 0;
            pkt_count = 1;

            pid = USB2OTG_PID_DATA1;

            /* Turn direction */
            if (req->iouh_Length == 0 || direction == 0)
                direction = 1;
            else
                direction = 0;

            D(bug("[USB2OTG] AdvanceChannel: ACK phase dir=%d\n", direction));
        }
    }

    if (!done)
    {
        D(bug("[USB2OTG] Req %p on channel %d continuing with transfer: buf=%p len=%d, act=%d, pid=%d\n",
                                        req, chan, buffer, xfer_size, req->iouh_Actual, pid));

        /*
         * Halt before reprogramming (DWC2 spec). For non-split bulk
         * OUT also force a state-machine reset by zeroing HCCHAR —
         * without it, channel scheduler state drifts after N packets
         * and the device goes silent mid-transfer.
         */
        {
            ULONG hcchar = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
            BOOL force_state_reset = (req->iouh_Req.io_Command == UHCMD_BULKXFER &&
                                      req->iouh_Dir == UHDIR_OUT &&
                                      !(req->iouh_Flags & UHFF_SPLITTRANS));
            if (hcchar & USB2OTG_HOSTCHAR_ENABLE)
            {
                hcchar |= USB2OTG_HOSTCHAR_DISABLE;
                wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), hcchar);
                /* ~200 µs budget — matches halt_channel_preserve_char. */
                int timeout = 200000;
                while ((rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE) && --timeout > 0)
                    asm volatile("mov r0, r0\n");
                if (timeout <= 0)
                    bug("[USB2OTG] AdvanceChannel halt timeout chan=%d CHAR=%08x INTR=%08x\n",
                        chan,
                        rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
                        rd32le(USB2OTG_CHANNEL_REG(chan, INTR)));
            }
            else if (force_state_reset)
            {
                /* Force state-machine reset by zeroing HCCHAR. */
                wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0);
            }
            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);
        }

        if (req->iouh_MaxPktSize == 0)
        {
            bug("[USB2OTG] Refusing to continue chan=%d dev=%d ep=%d cmd=%lu with MaxPktSize=0 act=%lu len=%lu flags=%04x\n",
                chan,
                (int)req->iouh_DevAddr,
                (int)req->iouh_Endpoint,
                (unsigned long)req->iouh_Req.io_Command,
                (unsigned long)req->iouh_Actual,
                (unsigned long)req->iouh_Length,
                (unsigned int)req->iouh_Flags);
            req->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            return 1;
        }

        /* Prepare HOSTCHAR register */
        reg = USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
            USB2OTG_HOSTCHAR_EPDIR(direction) |
            USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
            USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize);

        if (req->iouh_Flags & UHFF_LOWSPEED)
            reg |= USB2OTG_HOSTCHAR_LOWSPEED;

        switch(req->iouh_Req.io_Command)
        {
            case UHCMD_CONTROLXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL);
                break;
            case UHCMD_INTXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_INT);
                break;
            case UHCMD_BULKXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_BULK);
                break;
            case UHCMD_ISOXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_ISO);
                break;
            default:
                req->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                return 1;
        }

        /* If split transaction requested limit transfer size to max packet size or 188 bytes, whichever is less */
        if (req->iouh_Flags & UHFF_SPLITTRANS)
        {
            if (req->iouh_Req.io_Command == UHCMD_INTXFER ||
                req->iouh_Req.io_Command == UHCMD_ISOXFER)
                reg |= USB2OTG_HOSTCHAR_EC(3);
            else
                reg |= USB2OTG_HOSTCHAR_EC(1);

            {
                ULONG splitpos;

                if (otg_Unit->hu_Channel[chan].hc_SplitCSplitPending)
                    splitpos = 0; /* CSPLIT */
                else if (req->iouh_Req.io_Command == UHCMD_INTXFER ||
                         req->iouh_Req.io_Command == UHCMD_ISOXFER)
                    splitpos = 3; /* ALL */
                else
                    splitpos = 2; /* BEGIN */

                wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL),
                        (1 << 31) |
                        (splitpos << 14) |
                        ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                        ((req->iouh_SplitHubPort & 0x0f)));
            }

            if (xfer_size > req->iouh_MaxPktSize)
                xfer_size = req->iouh_MaxPktSize;

            /* 188 bytes is maximal payload in single split transaction */
            if (xfer_size > 188)
                xfer_size = 188;
        }
        else
        {
            reg |= USB2OTG_HOSTCHAR_EC(1);

            wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), 0);

            /* Bulk-OUT multi-packet burst — see SetupChannel for
             * the rationale behind removing the per-packet clamp. */
        }

        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), reg);

        /* Get packet count and adjust this and the transfer size.
         * 64-byte cache-line alignment criterion — see SetupChannel. */
        if (buffer != NULL && ((ULONG)buffer & 0x3F) && xfer_size > DMA_BOUNCE_SIZE)
            xfer_size = DMA_BOUNCE_SIZE;

        if (xfer_size > (1 << (otg_Unit->hu_XferSizeWidth - 1)))
            xfer_size = 1 << (otg_Unit->hu_XferSizeWidth - 1);

        pkt_count = (xfer_size + req->iouh_MaxPktSize - 1) / req->iouh_MaxPktSize;

        if (pkt_count == 0)
            pkt_count = 1;

        if (pkt_count > ((1 << (otg_Unit->hu_PktSizeWidth)) - 1))
        {
            pkt_count = (1 << (otg_Unit->hu_PktSizeWidth)) - 1;
            xfer_size = pkt_count * req->iouh_MaxPktSize;
        }

        /* Size/PID/PktCnt; OR in DoPing if NYET handler set PingPending. */
        {
            ULONG tsize = USB2OTG_HOSTTSIZE_PID(pid) |
                          USB2OTG_HOSTTSIZE_PKTCNT(pkt_count) |
                          USB2OTG_HOSTTSIZE_SIZE(xfer_size);
            if (otg_Unit->hu_Channel[chan].hc_PingPending)
            {
                tsize |= USB2OTG_HOSTTSIZE_PING;
                D(
                    {
                        static ULONG ping_adv_count = 0;
                        ping_adv_count++;
                        if (ping_adv_count <= 3 || (ping_adv_count & 0x3f) == 0)
                            bug("[USB2OTG:PING] ADV #%lu chan=%d dev=%d ep=%d TSIZE=%08x\n",
                                (unsigned long)ping_adv_count, chan,
                                (int)req->iouh_DevAddr, (int)req->iouh_Endpoint, tsize);
                    }
                )
            }
            wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE), tsize);
        }

        otg_Unit->hu_Channel[chan].hc_XferSize = xfer_size;

        /*
         * Misaligned buffer → bounce. Aligned direct DMA needs per-arm
         * cache flush — speculative prefetch refills stale lines
         * between continuation arms, controller sends garbage.
         */
        otg_Unit->hu_Channel[chan].hc_OrigBuffer = NULL;
        {
            APTR dma_buf = buffer;
            if (buffer != NULL && ((ULONG)buffer & 0x3F) && xfer_size <= DMA_BOUNCE_SIZE)
            {
                otg_Unit->hu_Channel[chan].hc_OrigBuffer = buffer;
                otg_Unit->hu_Channel[chan].hc_BounceLen = xfer_size;
                otg_Unit->hu_Channel[chan].hc_BounceDir = direction;
                dma_buf = (APTR)otg_Unit->hu_BounceBuf[chan];

                if (direction == 0)
                {
                    /* OUT: copy data to aligned bounce buffer before DMA */
                    CopyMem(buffer, otg_Unit->hu_BounceBuf[chan], xfer_size);
                    CacheClearE(otg_Unit->hu_BounceBuf[chan], xfer_size, CACRF_ClearD);
                }
                else
                {
                    /* IN: invalidate bounce buffer, copy-back happens after transfer */
                    CacheClearE(otg_Unit->hu_BounceBuf[chan], xfer_size, CACRF_InvalidateD);
                }
                D(bug("[USB2OTG] Advance: bounce align %p -> %p (%d bytes, dir=%d)\n",
                    buffer, dma_buf, xfer_size, direction));
            }
            else if (buffer != NULL && xfer_size > 0)
            {
                /* Aligned direct DMA — flush cache per arm. */
                if (direction == 0)
                    CacheClearE(buffer, xfer_size, CACRF_ClearD);
                else
                    CacheClearE(buffer, xfer_size, CACRF_InvalidateD);
            }
            /* DSB so cache maint is visible before DMA. */
            asm volatile("dsb sy" ::: "memory");

            /* Advance-arm alignment diagnostic. */
            D(
                if (buffer != NULL && xfer_size > 0 && req->iouh_Req.io_Command == UHCMD_BULKXFER)
                {
                    static ULONG adv_align_count = 0;
                    adv_align_count++;
                    if (adv_align_count <= 5 || (adv_align_count & 0xff) == 0)
                        bug("[USB2OTG:DMA] advance chan=%d dev=%d ep=%d dir=%s buf=%p (mod64=%lu) size=%lu actual=%lu\n",
                            chan, (int)req->iouh_DevAddr, (int)req->iouh_Endpoint,
                            direction ? "IN" : "OUT", buffer,
                            (unsigned long)((IPTR)buffer & 0x3F),
                            (unsigned long)xfer_size,
                            (unsigned long)req->iouh_Actual);
                }
            )
            wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), 0xc0000000 | (ULONG)dma_buf);
        }

        /* Log phase transitions for control transfers */
        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
        {
            D(bug("[USB2OTG] Advance: dev=%d dir=%d pid=%d xfer=%d buf=%p dma=%08x\n",
                req->iouh_DevAddr, direction, pid, xfer_size, buffer,
                0xc0000000 | (ULONG)buffer));
            D(bug("[USB2OTG] Advance: CHAR=%08x TSIZE=%08x\n",
                rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)),
                rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE))));
        }
    }

    return done;
}

/* Start Channel */
void FNAME_DEV(StartChannel)(struct USB2OTGUnit *otg_Unit, int chan, int quick)
{
    ULONG tmp = 0;

    if (!usb2otg_require_cpu0(otg_Unit->hu_USB2OTGBase, __PRETTY_FUNCTION__))
        return;

    if (quick == 0)
    {
        if (chan < 0 || chan > 7)
            return;

        /*
         * Enable HALT/XFERCOMPL/NYET/XactErr/DTERR. Intentionally NOT
         * ACK/NAK — they fire before CHHLTD on splits and break the
         * CSPLIT state machine.
         */
        tmp = rd32le(USB2OTG_CHANNEL_REG(chan, INTRMASK));
        tmp |= USB2OTG_INTRCHAN_HALT
             | USB2OTG_INTRCHAN_TRANSFERCOMPLETE
             | USB2OTG_INTRCHAN_NOTREADY              /* NYET */
             | USB2OTG_INTRCHAN_TRANSACTIONERROR      /* XactErr */
             | USB2OTG_INTRCHAN_DATATOGGLEERROR;      /* DTERR */
        wr32le(USB2OTG_CHANNEL_REG(chan, INTRMASK), tmp);
        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

        /* Enable channel IRQ; HAINT is RO, cleared via per-channel INTR. */
        tmp = rd32le(USB2OTG_HOSTINTRMASK);
        tmp |= 1 << chan;
        wr32le(USB2OTG_HOSTINTRMASK, tmp);

        tmp = rd32le(USB2OTG_INTRMASK);
        tmp |= USB2OTG_INTRCORE_HOSTCHANNEL;
        wr32le(USB2OTG_INTRMASK, tmp);
    }

    /* Finally enable the channel and thus start transaction */
    tmp = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));

    /*
     * Split-periodic channels: set OddFrm to match the frame parity
     * SSPLIT will fire in. Predict via HFNUM.FRREM (PHY clocks left,
     * HFIR=60000/ms) — if <20 % remains, target the next frame.
     * Non-split must not have OddFrm set.
     */
    {
        ULONG eptype = (tmp >> 18) & 3;
        ULONG splitctrl = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));

        if ((eptype == USB2OTG_TYPE_INT || eptype == USB2OTG_TYPE_ISO) &&
            (splitctrl & (1 << 31)))
        {
            ULONG hfnum = rd32le(USB2OTG_HOSTFRAMENO);
            ULONG frame = (hfnum >> 3) & 0x7ff;         /* bits [13:3] */
            ULONG frrem = (hfnum >> 16) & 0xffff;       /* PHY clocks left */
            ULONG frint = rd32le(USB2OTG_HOSTFRAMEINTERV) & 0xffff;

            /* <20 % frame left → land in the next frame. */
            if (frint > 0 && frrem < frint / 5)
                frame++;

            if (frame & 1)
                tmp |= (1 << 29);   /* HCCHAR.OddFrm */
            else
                tmp &= ~(1 << 29);
        }
    }

    /* Force EC=1 for non-split bulk/CTRL: EC=0 halts BCM2835 instantly. */
    {
        ULONG eptype = (tmp >> 18) & 3;
        ULONG splitctrl = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
        if (!(splitctrl & (1U << 31)) &&
            (eptype == USB2OTG_TYPE_BULK || eptype == USB2OTG_TYPE_CTRL))
        {
            tmp = (tmp & ~USB2OTG_HOSTCHAR_EC(3)) | USB2OTG_HOSTCHAR_EC(1);
        }
    }

    /*
     * Clear stale CHDIS before arming. BCM2835 DWC2 doesn't auto-clear
     * CHDIS after halt; re-arming with CHENA=1+CHDIS=1 cancels the
     * transaction immediately. Quick re-arm paths need this clear.
     */
    tmp &= ~USB2OTG_HOSTCHAR_DISABLE;
    tmp |= USB2OTG_HOSTCHAR_ENABLE;

    if (tmp & 0x40000000)
        D(bug("writing Disable bit!!!\n"));

    /* Snapshot HFNUM for bare-CHHLTD diag (bulk only). */
    {
        struct IOUsbHWReq *diag_req = otg_Unit->hu_Channel[chan].hc_Request;
        if (diag_req != NULL && diag_req->iouh_Req.io_Command == UHCMD_BULKXFER)
        {
            otg_Unit->hu_Channel[chan].hc_StartHfnum = rd32le(USB2OTG_HOSTFRAMENO);
        }
    }

    /*
     * NPTxFIFO drain gate for non-split bulk OUT. Mirrors FreeBSD
     * dwc_otg.c:765-767 NPTxFEmpty gate. Without it, sustained
     * bulk-OUT eventually wedges with WAIT-QUIET.
     */
    {
        struct IOUsbHWReq *arm_req = otg_Unit->hu_Channel[chan].hc_Request;
        BOOL is_bulk_out = (arm_req != NULL &&
                            arm_req->iouh_Req.io_Command == UHCMD_BULKXFER &&
                            arm_req->iouh_Dir == UHDIR_OUT &&
                            !(arm_req->iouh_Flags & UHFF_SPLITTRANS));

        if (is_bulk_out)
        {
            /* NPTxFIFO size in dwords from HNPTXFSIZ[31:16]. */
            ULONG nptxfsiz = rd32le(USB2OTG_NONPERIFIFOSIZE);
            ULONG fifo_dwords = (nptxfsiz >> 16) & 0xFFFF;
            int timeout = 200000; /* ~200 µs */
            ULONG nptxsts;

            do {
                nptxsts = rd32le(USB2OTG_NONPERIFIFOSTATUS);
                /* NPTxFSpcAvail==fifo_dwords → FIFO empty; QSpcAvail==8 → q empty. */
                if (((nptxsts & 0xFFFF) >= fifo_dwords) &&
                    (((nptxsts >> 16) & 0xFF) >= 8))
                    break;
            } while (--timeout > 0);

            if (timeout <= 0)
            {
                bug("[USB2OTG] NPTxFIFO drain timeout chan=%d NPTXSTS=%08x (need fifo>=%04x qspc>=08)\n",
                    chan, nptxsts, (unsigned)fifo_dwords);
                /* Proceed anyway — better to try than to stall arming. */
            }

#if USB2OTG_BULK_OUT_THROTTLE_COUNT > 0
            {
                int t = USB2OTG_BULK_OUT_THROTTLE_COUNT;
                while (--t > 0)
                    asm volatile("mov r0, r0\n");
            }
#endif
        }
    }

    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), tmp);
}

/*
 * Control transfer phases (SETUP, optional DATA, ACK) — DWC2 cannot
 * join them, so the IRQ handler advances phase-by-phase.
 */
void FNAME_DEV(ScheduleCtrlTDs)(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    struct IOUsbHWReq *req = NULL;

    if (!usb2otg_require_cpu0(USB2OTGBase, __PRETTY_FUNCTION__))
        return;

    D(bug("[USB2OTG] %s(%p)\n", __PRETTY_FUNCTION__, otg_Unit));

    D(
        if (!IsListEmpty(&otg_Unit->hu_CtrlXFerQueue))
            bug("[USB2OTG:SCHED] CtrlTDs: queue non-empty, chan_req=%p\n",
                otg_Unit->hu_Channel[CHAN_CTRL].hc_Request);
    )

    /* First try to get head of the transfer queue */
    Disable();
    KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
    req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_CtrlXFerQueue);

    /* If there was any request in the queue, try to put it into InProgress slot */
    if (req)
    {
        /* Check retry delay — DriverPrivate1 counts down microframes between retries */
            ULONG delay = (ULONG)req->iouh_DriverPrivate1;
            if (delay > 0)
            {
                req->iouh_DriverPrivate1 = (APTR)(delay - 1);
                ADDTAIL(&otg_Unit->hu_CtrlXFerQueue, req);
                KrnSpinUnLock(&otg_Unit->hu_Lock);
                Enable();
                return;
            }

        /* Channel slot empty? If yes store the request there. Otherwise put it back to the queue */
        if (otg_Unit->hu_Channel[CHAN_CTRL].hc_Request == NULL) {
            /* Defer if another channel is mid-split to the same TT. */
            if (usb2otg_tt_in_flight(otg_Unit, req))
            {
                ADDHEAD(&otg_Unit->hu_CtrlXFerQueue, req);
                KrnSpinUnLock(&otg_Unit->hu_Lock);
                Enable();
                return;
            }
            otg_Unit->hu_Channel[CHAN_CTRL].hc_Request = req;
        } else {
            ADDHEAD(&otg_Unit->hu_CtrlXFerQueue, req);
            KrnSpinUnLock(&otg_Unit->hu_Lock);
            Enable();
            return;
        }
        KrnSpinUnLock(&otg_Unit->hu_Lock);
        Enable();

        D(bug("[USB2OTG:SCHED] CTRL-START: dev=%ld req=%02lx len=%ld\n",
            (LONG)req->iouh_DevAddr, (ULONG)req->iouh_SetupData.bRequest,
            (LONG)req->iouh_Length));

        D(bug("[USB2OTG:DBG] CTRL-ARM dev=%d ep=%d bReq=%02x wVal=%04x wIdx=%04x len=%lu\n",
            (int)req->iouh_DevAddr,
            (int)req->iouh_Endpoint,
            (unsigned)req->iouh_SetupData.bRequest,
            (unsigned)AROS_LE2WORD(req->iouh_SetupData.wValue),
            (unsigned)AROS_LE2WORD(req->iouh_SetupData.wIndex),
            (unsigned long)req->iouh_Length));
        if (FNAME_DEV(SetupChannel)(otg_Unit, CHAN_CTRL))
            FNAME_DEV(StartChannel)(otg_Unit, CHAN_CTRL, 0);
    }
    else
    {
        KrnSpinUnLock(&otg_Unit->hu_Lock);
        Enable();
    }
}

/*
 * Schedule bulk. CHAN_BULK/CHAN_BULK2 are partitioned per-device
 * (usb2otg_claim_bulk_channel) so MSC + ethernet cannot block each
 * other.
 */
void FNAME_DEV(ScheduleBulkTDs)(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    int slot;

    if (!usb2otg_require_cpu0(USB2OTGBase, __PRETTY_FUNCTION__))
        return;

    D(bug("[USB2OTG] %s(%p)\n", __PRETTY_FUNCTION__, otg_Unit));

    /* At most two bulk channels → iterate at most twice. */
    for (slot = 0; slot < 2; slot++)
    {
        struct IOUsbHWReq *req;
        int chan = -1;

        Disable();
        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);

        req = usb2otg_find_schedulable_bulk_req(otg_Unit, &chan);

        if (req == NULL || chan < 0)
        {
            KrnSpinUnLock(&otg_Unit->hu_Lock);
            Enable();
            return;  /* Nothing schedulable this pass */
        }

        REMOVE(req);
        otg_Unit->hu_Channel[chan].hc_Request = req;
        usb2otg_diag_bulk_assign(otg_Unit, chan, req);

        KrnSpinUnLock(&otg_Unit->hu_Lock);
        Enable();

        if (FNAME_DEV(SetupChannel)(otg_Unit, chan))
            FNAME_DEV(StartChannel)(otg_Unit, chan, 0);
    }
}

/* Schedule INT transfers from Scheduled list (SOF maintains IntXferQueue). */
void FNAME_DEV(ScheduleIntTDs)(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice *USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    int chan = 0;

    if (!usb2otg_require_cpu0(USB2OTGBase, __PRETTY_FUNCTION__))
        return;

    ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;

    /* Check if any of INT channels is free */
    for (chan = CHAN_INT1; chan <= CHAN_INT_LAST; chan++)
    {
        struct IOUsbHWReq *req = NULL;

        /* If channel is in use unlock Enable() and continue checking, otherwise stay in Disable() state for a while */
        Disable();
        KrnSpinLock(&otg_Unit->hu_Lock, NULL, SPINLOCK_MODE_WRITE);
        if (otg_Unit->hu_Channel[chan].hc_Request != NULL)
        {
            KrnSpinUnLock(&otg_Unit->hu_Lock);
            Enable();
            continue;
        }
        /* Pick first INT req not already in flight (shared TT state). */
        {
            struct IOUsbHWReq *candidate, *cnext;
            req = NULL;
            ForeachNodeSafe(&otg_Unit->hu_IntXFerScheduled, candidate, cnext)
            {
                if (!usb2otg_endpoint_in_flight(otg_Unit, candidate) &&
                    !usb2otg_tt_in_flight(otg_Unit, candidate))
                {
                    REMOVE(candidate);
                    req = candidate;
                    break;
                }
            }
        }

        if (req)
        {
            /* Channel was free and there is request available. Assign the request to given channel now */
            otg_Unit->hu_Channel[chan].hc_Request = req;
            KrnSpinUnLock(&otg_Unit->hu_Lock);
            Enable();

            D(bug("[USB2OTG] ScheduleIntTD: chan=%d dev=%ld ep=%ld len=%ld\n",
                chan, (LONG)req->iouh_DevAddr, (LONG)req->iouh_Endpoint, (LONG)req->iouh_Length));

            ULONG interval = req->iouh_Interval;
            /* 2-frame clamp only for non-periodic split; INT keeps interval=1. */
            if ((req->iouh_Flags & UHFF_SPLITTRANS) &&
                req->iouh_Req.io_Command != UHCMD_INTXFER &&
                interval < 2)
                interval = 2;

            ULONG last_handled = frnm;
            ULONG next_to_handle = (last_handled + interval) & 0x7ff;
            req->iouh_DriverPrivate1 = (APTR)((last_handled << 16) | next_to_handle);

            if (!FNAME_DEV(SetupChannel)(otg_Unit, chan))
                continue;

            FNAME_DEV(StartChannel)(otg_Unit, chan, 0);
        }
        else
        {
            /* No more requests in the queue, return */
            KrnSpinUnLock(&otg_Unit->hu_Lock);
            Enable();
            return;
        }
    }
}
