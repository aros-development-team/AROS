/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/macros.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/dma.h>

#include <hardware/bcm2708_dma.h>

#include "dma_private.h"

/*
 * Channels the resource may hand out. The VideoCore firmware claims
 * several channels for itself, we assume the following is available:
 *
 * Full engines (2, 4, 5) support 2D/TDMODE and wide bursts, lite engines
 * do 32-bit transfers only. Lite channels 13-14 have no dedicated ARM
 * IRQ line (only DMA0-12 map to GPU IRQ 16+N), so they are excluded —
 * users may rely on per-channel completion interrupts. (On the BCM2711
 * 13-14 are DMA4 engines with a line each, see below.)
 */
#define DMA_POOL_FULL   ((1 << 2) | (1 << 4) | (1 << 5))
#define DMA_POOL_LITE   ((1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | \
                         (1 << 12))

/*
 * BCM2711 keeps more for itself: brcm,dma-channel-mask 0x7f5 against 0x7f35,
 * so 11, 12 and the full engine 5 are the firmware's, not ours.
 */
#define DMA_POOL_FULL_BCM2711  ((1 << 2) | (1 << 4))
#define DMA_POOL_LITE_BCM2711  ((1 << 8) | (1 << 9) | (1 << 10))

/*
 * The BCM2711 DMA4 engines: 40-bit addressing and a 128-bit AXI path, so no
 * bus alias and no 1GB ceiling on either end of a transfer. All four are the
 * OS's - the firmware keeps channel 15, which is not ours to hand out anyway.
 *
 * Handed out only on request (DMACHF_DMA4): the control block layout differs,
 * so a caller that got one unasked would program it as garbage.
 */
#define DMA_POOL_DMA4_BCM2711  ((1 << 11) | (1 << 12) | (1 << 13) | (1 << 14))

/*
 * BCM2712, from brcm,dma-channel-mask: 0-5 legacy (all treated as full,
 * unverified), 6-10 DMA4, 11 is the firmware's.
 */
#define DMA_POOL_FULL_BCM2712  0x3f
#define DMA_POOL_LITE_BCM2712  0
#define DMA_POOL_DMA4_BCM2712  ((1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | \
                                (1 << 10))

APTR KernelBase __attribute__((used)) = NULL;

static int dma_init(struct DMABase *DMABase)
{
    D(bug("[DMA] %s()\n", __PRETTY_FUNCTION__));

    KernelBase = OpenResource("kernel.resource");

    if ((DMABase->dma_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase)) == 0)
        return FALSE;

    InitSemaphore(&DMABase->dma_Sem);
    DMABase->dma_InUse = 0;

    D(bug("[DMA] %s: channel allocator initialised\n", __PRETTY_FUNCTION__));

    return TRUE;
}

/* 1 MHz system timer for wait deadlines. */
static inline ULONG dma_now_us(struct DMABase *DMABase)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

static void dma_zero(void *p, ULONG len)
{
    UBYTE *b = p;
    while (len--)
        *b++ = 0;
}

/*
 * Per-channel completion IRQ: W1C the INT flag (END is left for the
 * waiter to consume) and signal the registered waiter.
 */
static void dma_irq_handler(void *data1, void *data2)
{
    struct DMABase *DMABase = (struct DMABase *)data1;
    int channel = (int)(IPTR)data2;
    volatile ULONG *cs = (volatile ULONG *)DMA_CS(channel);

    if (AROS_LE2LONG(*cs) & DMA_CS_INT)
    {
        struct Task *t;

        /*
         * Fine for DMAWaitChannel - its transfer is done by now - but this
         * write also clears ACTIVE and the AXI priorities, which share the
         * register. A caller chaining control blocks wants BCM2708_DMA_CS_ACK
         * instead (see bcm2708_dma.h), which is why the AHI drivers run their
         * own handlers.
         */
        *cs = AROS_LONG2LE(DMA_CS_INT);
        t = DMABase->dma_Wait[channel].waiter;
        if (t)
            Signal(t, 1UL << DMABase->dma_Wait[channel].sig);
    }
}

/* Where a channel reports its errors - DMA4 moved the register. */
static inline IPTR dma_debug_reg(struct DMABase *DMABase, int channel)
{
    if (BCM2708_DMA_IS_DMA4(DMABase->dma_periiobase, channel))
        return DMA4_DEBUG(channel);

    return DMA_DEBUG(channel);
}

/* Stop the channel and leave it idle. FALSE if it would not come to rest. */
static BOOL dma_channel_quiesce(struct DMABase *DMABase, int channel)
{
    volatile ULONG *cs = (volatile ULONG *)DMA_CS(channel);
    int try = 10000;

    if (BCM2708_DMA_IS_DMA4(DMABase->dma_periiobase, channel))
    {
        ULONG v = AROS_LE2LONG(*cs);
        ULONG dbg;

        /* No reset bit in CS: halt a running engine, drain, reset via DEBUG.
         * HALT is sticky and reports DMA_BUSY, so never halt an idle one. */
        if (v & DMA4_CS_ACTIVE)
        {
            *cs = AROS_LONG2LE(DMA4_CS_HALT);
            while (try-- > 0)
            {
                v = AROS_LE2LONG(*cs);
                if (!(v & (DMA4_CS_ACTIVE | DMA4_CS_OUTSTANDING_TRANSACTIONS)))
                    break;
            }
            *cs = AROS_LONG2LE(DMA4_CS_INT | DMA4_CS_END);
            v = AROS_LE2LONG(*cs);
        }

        /* Error latches are read-to-clear */
        dbg = AROS_LE2LONG(*(volatile ULONG *)DMA4_DEBUG(channel));

        /* Reset is only unsafe with AXI traffic in flight; BUSY alone is a
         * paused DREQ write the reset clears. */
        if (v & (DMA4_CS_ACTIVE | DMA4_CS_OUTSTANDING_TRANSACTIONS))
        {
            bug("[DMA] channel %d would not halt (cs=0x%08x debug=0x%08x) - "
                "reset skipped\n", channel, v, dbg);
            return FALSE;
        }

        *(volatile ULONG *)DMA4_DEBUG(channel) =
            AROS_LONG2LE(DMA4_DEBUG_RESET);
        *cs = AROS_LONG2LE(DMA4_CS_INT | DMA4_CS_END);

        v = AROS_LE2LONG(*cs);
        if (v & (DMA4_CS_ACTIVE | DMA4_CS_DMA_BUSY))
        {
            bug("[DMA] channel %d still busy after reset (cs=0x%08x)\n",
                channel, v);
            return FALSE;
        }
        return TRUE;
    }

    *cs = AROS_LONG2LE(DMA_CS_RESET);
    while (try-- > 0)
    {
        if (!(AROS_LE2LONG(*cs) & DMA_CS_RESET))
            break;
    }
    if (AROS_LE2LONG(*cs) & DMA_CS_RESET)
    {
        bug("[DMA] channel %d would not reset (cs=0x%08x)\n",
            channel, AROS_LE2LONG(*cs));
        return FALSE;
    }
    *cs = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
    return TRUE;
}

/* Enable the channel and bring the engine to a clean, idle state. */
static BOOL dma_channel_reset(struct DMABase *DMABase, int channel)
{
    if (BCM2708_DMA_HAS_GLOBAL_REGS(DMABase->dma_periiobase))
    {
        volatile ULONG *enable = (volatile ULONG *)DMA_ENABLE_REG;

        *enable = AROS_LONG2LE(AROS_LE2LONG(*enable) | (1 << channel));
    }

    return dma_channel_quiesce(DMABase, channel);
}

/*****************************************************************************

    NAME */
        AROS_LH1(int, DMAAllocChannel,

/*  SYNOPSIS */
                        AROS_LHA(unsigned int, flags, D0),

/*  LOCATION */
                        struct DMABase *, DMABase, 1, Dma)
/*  FUNCTION
        Reserve a DMA channel and reset its engine before returning it.

    INPUTS
        flags - Combination of flags from <hardware/bcm2708_dma.h>:
                DMACHF_TDMODE requests a full legacy engine.
                DMACHF_DMA4 requests a DMA4 engine and takes precedence
                over DMACHF_TDMODE. There is no legacy-engine fallback.
                DMACHF_IRQ requests the resource's completion interrupt
                handler for DMAWaitChannel(). Leave it clear when installing
                a driver-owned handler. Unknown flag bits are ignored.

    RESULT
        An allocated channel number from 0 through 14, or -1 if the
        selected pool has no free channel that can be reset. A nonnegative
        result does not guarantee that the optional IRQ handler was installed.

    NOTES
        Call from task context where waiting is permitted. Allocation and
        reset run under the resource semaphore. Reset polling has a finite
        iteration count, not a specified wall-clock deadline.

        Without DMACHF_DMA4 or DMACHF_TDMODE, free lite channels are preferred.
        The full pool is selected only when no lite channel is free. If
        all free lite channels fail reset, this call does not then try the
        full pool. Within the selected pool, the lowest usable number wins.

        The configured pools are:
            BCM283x: full 2, 4, 5; lite 8 through 12; no DMA4.
            BCM2711: full 2, 4; lite 8 through 10; DMA4 11 through 14.
            BCM2712: full 0 through 5; no lite; DMA4 6 through 10.
        These are fixed resource policy, not a live firmware reservation query.

        The caller owns the channel until DMAFreeChannel(). Allocation does
        not create control blocks, map buffers, maintain caches or start a
        transfer. Use the programming model and bus addresses of the returned
        engine. Keep transfer storage valid until the engine is stopped.

        The resource IRQ handler clears ACTIVE and priority fields when
        acknowledging an interrupt. It is intended for DMAWaitChannel(),
        not a running chain with per-control-block interrupts. Do not share
        its interrupt line with a competing acknowledgement handler.

    BUGS
        IRQ-handler installation failure is not returned to the caller;
        DMAWaitChannel() then uses polling. Failed reset attempts can leave
        channels enabled. The allocator does not quarantine those channels.
        The BCM2712 legacy pool is explicitly unverified in this implementation.

    SEE ALSO
        DMAFreeChannel(), DMAStopChannel(), DMAWaitChannel()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG avail;
    int channel = -1;
    int ch;

    D(bug("[DMA] %s(0x%x)\n", __PRETTY_FUNCTION__, flags));

    ObtainSemaphore(&DMABase->dma_Sem);

    {
        int is2711 = (DMABase->dma_periiobase == BCM2711_PERIIOBASE);
        int is2712 = (DMABase->dma_periiobase == BCM2712_PERIIOBASE);
        ULONG full = is2712 ? DMA_POOL_FULL_BCM2712 : (is2711 ? DMA_POOL_FULL_BCM2711 : DMA_POOL_FULL);
        ULONG lite = is2712 ? DMA_POOL_LITE_BCM2712 : (is2711 ? DMA_POOL_LITE_BCM2711 : DMA_POOL_LITE);

        /* DMA4 is a distinct programming model, so it is exactly what was
         * asked for or nothing - never a substitute from another pool. */
        if (flags & DMACHF_DMA4)
        {
            ULONG dma4 = is2712 ? DMA_POOL_DMA4_BCM2712
                                : (is2711 ? DMA_POOL_DMA4_BCM2711 : 0);

            avail = dma4 & ~DMABase->dma_InUse;
        }
        /* Prefer lite channels so the scarce full engines stay available
         * for users that need TDMODE. */
        else if (flags & DMACHF_TDMODE)
            avail = full & ~DMABase->dma_InUse;
        else
        {
            avail = lite & ~DMABase->dma_InUse;
            if (avail == 0)
                avail = full & ~DMABase->dma_InUse;
        }
    }

    /* A channel that will not come to rest never completes; skip it. */
    for (ch = 0; ch < 15; ch++)
    {
        if (!(avail & (1 << ch)))
            continue;

        if (dma_channel_reset(DMABase, ch))
        {
            channel = ch;
            break;
        }

        bug("[DMA] channel %d is not usable, trying the next one\n", ch);
    }

    if (channel >= 0)
    {
        DMABase->dma_InUse |= (1 << channel);

        /* Completion IRQ for DMAWaitChannel — opt-in: drivers that run
         * their own handler on the channel's line (the AHI drivers, with
         * per-CB interrupts) must own it exclusively, or the two handlers
         * race to W1C the INT flag and loses events. */
        DMABase->dma_Wait[channel].waiter = NULL;
        DMABase->dma_Wait[channel].irq_handle = NULL;
        if (flags & DMACHF_IRQ)
            DMABase->dma_Wait[channel].irq_handle =
                KrnAddIRQHandler(BCM2708_DMA_IRQ(DMABase->dma_periiobase, channel),
                                 dma_irq_handler,
                                 DMABase, (void *)(IPTR)channel);
    }

    ReleaseSemaphore(&DMABase->dma_Sem);

    D(bug("[DMA] %s: allocated channel %d\n", __PRETTY_FUNCTION__, channel));

    return channel;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, DMAFreeChannel,

/*  SYNOPSIS */
                        AROS_LHA(int, channel, D0),

/*  LOCATION */
                        struct DMABase *, DMABase, 2, Dma)
/*  FUNCTION
        Release a channel reservation and remove its resource-owned IRQ handler.

    INPUTS
        channel - Channel returned by DMAAllocChannel() and still owned
                  by the caller. Values outside 0 through 14 are ignored.

    RESULT
        None. An unallocated channel has no effect. Hardware-stop failure
        is not returned to the caller.

    NOTES
        Call from task context where waiting is permitted. Under the
        resource semaphore, this call attempts to stop/reset the engine,
        clears its global enable bit where that register exists, releases
        the reservation and removes the optional resource IRQ handler.
        It does not free caller-owned buffers or control blocks.

        Finish or cancel any DMAWaitChannel() call and coordinate all users
        before release. No waiter is explicitly notified. Ownership is not
        tracked by task; a valid channel number is not proof of ownership.

        Use DMAStopChannel() and check its result before releasing transfer
        storage. If stopping fails, retain the reservation and storage until
        the hardware is known to be quiescent; this function cannot certify
        that condition.

    BUGS
        The result of the internal stop/reset operation is ignored. The
        reservation and handler are released even if the engine did not
        stop. On controllers without a global enable register, this path
        does not provide a second hardware-disable operation.

    SEE ALSO
        DMAAllocChannel(), DMAStopChannel(), DMAWaitChannel()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[DMA] %s(%d)\n", __PRETTY_FUNCTION__, channel));

    if ((channel < 0) || (channel > 14))
        return;

    ObtainSemaphore(&DMABase->dma_Sem);

    if (DMABase->dma_InUse & (1 << channel))
    {
        dma_channel_quiesce(DMABase, channel);

        if (BCM2708_DMA_HAS_GLOBAL_REGS(DMABase->dma_periiobase))
        {
            volatile ULONG *enable = (volatile ULONG *)DMA_ENABLE_REG;

            *enable = AROS_LONG2LE(AROS_LE2LONG(*enable) & ~(1 << channel));
        }
        DMABase->dma_InUse &= ~(1 << channel);

        if (DMABase->dma_Wait[channel].irq_handle)
        {
            KrnRemIRQHandler(DMABase->dma_Wait[channel].irq_handle);
            DMABase->dma_Wait[channel].irq_handle = NULL;
        }
        DMABase->dma_Wait[channel].waiter = NULL;
    }

    ReleaseSemaphore(&DMABase->dma_Sem);

    AROS_LIBFUNC_EXIT
}

/*
 * Wait for the channel's current transfer to complete. The control
 * block must have DMA_TI_INTEN set for the IRQ fast path; without it
 * (or before timer.device is up) the wait degrades to a bounded poll.
 * Returns 0 when the transfer ended normally (INT|END cleared), -1 on
 * early stop or timeout (channel reset).
 */
AROS_LH2(int, DMAWaitChannel,
                AROS_LHA(int, channel, D0),
                AROS_LHA(unsigned int, timeout_us, D1),
                struct DMABase *, DMABase, 3, Dma)
{
    AROS_LIBFUNC_INIT

    volatile ULONG *cs;
    ULONG start;
    BYTE dsig = -1;
    BYTE tsig = -1;
    int ret = -1;
    BOOL do_reset = FALSE;
    const char *reason = NULL;

    if ((channel < 0) || (channel > 14) ||
        !(DMABase->dma_InUse & (1 << channel)))
        return -1;

    cs = (volatile ULONG *)DMA_CS(channel);

    /* Lazily open timer.device for the safety pulse (it doesn't exist
     * yet when the resource initialises). */
    if (!DMABase->dma_TimerTried)
    {
        ObtainSemaphore(&DMABase->dma_Sem);
        if (!DMABase->dma_TimerTried)
        {
            struct timerequest *tt = &DMABase->dma_TimerTemplate;

            tt->tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
            tt->tr_node.io_Message.mn_Length = sizeof(*tt);
            if (OpenDevice("timer.device", UNIT_MICROHZ,
                           (struct IORequest *)tt, 0) == 0)
                DMABase->dma_TimerOk = TRUE;
            DMABase->dma_TimerTried = TRUE;
        }
        ReleaseSemaphore(&DMABase->dma_Sem);
    }

    /* Sample the deadline only after the (possibly slow) one-shot timer
     * open, so its latency isn't charged against the caller's timeout. */
    start = dma_now_us(DMABase);

    if (DMABase->dma_Wait[channel].irq_handle &&
        (dsig = AllocSignal(-1)) >= 0)
    {
        DMABase->dma_Wait[channel].sig = dsig;
        __asm__ __volatile__("dmb sy" ::: "memory");
        DMABase->dma_Wait[channel].waiter = FindTask(NULL);

        /* One reusable timer signal for the whole wait — the safety
         * pulse re-arms each iteration but the bit is allocated once. */
        if (DMABase->dma_TimerOk)
            tsig = AllocSignal(-1);
    }

    for (;;)
    {
        ULONG v = AROS_LE2LONG(*cs);

        if (v & DMA_CS_END)
        {
            *cs = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
            /* DMA4: the error latches are read-to-clear and CS.ERROR
             * mirrors them; clear on every completion so latched state
             * cannot poison the channel's next transfer. Report the
             * transfer as failed if any were set. */
            if (BCM2708_DMA_IS_DMA4(DMABase->dma_periiobase, channel))
            {
                ULONG dbg = AROS_LE2LONG(
                    *(volatile ULONG *)DMA4_DEBUG(channel));

                if (dbg & DMA4_DEBUG_ERRORS)
                {
                    bug("[DMA] channel %d completed with errors: "
                        "debug=0x%08x\n", channel, dbg);
                    ret = -1;
                    break;
                }
            }
            ret = 0;
            break;
        }
        if (!(v & DMA_CS_ACTIVE))
        {
            reason = "stopped";
            do_reset = TRUE;
            break;
        }
        if ((dma_now_us(DMABase) - start) > timeout_us)
        {
            reason = "timeout";
            do_reset = TRUE;
            break;
        }

        if (dsig >= 0 && tsig >= 0)
        {
            /* Sleep on the completion IRQ with a 4 ms timer as the
             * safety pulse — a wedged channel (no END, no IRQ) still
             * reaches the timeout instead of parking the task. */
            struct MsgPort port;
            struct timerequest tr;

            dma_zero(&port, sizeof(port));
            port.mp_Node.ln_Type = NT_MSGPORT;
            port.mp_Flags        = PA_SIGNAL;
            port.mp_SigBit       = tsig;
            port.mp_SigTask      = FindTask(NULL);
            NEWLIST(&port.mp_MsgList);

            tr = DMABase->dma_TimerTemplate;
            tr.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
            tr.tr_node.io_Message.mn_ReplyPort = &port;
            tr.tr_node.io_Message.mn_Length = sizeof(tr);
            tr.tr_node.io_Command = TR_ADDREQUEST;
            tr.tr_time.tv_secs  = 0;
            tr.tr_time.tv_micro = 4000;

            SendIO((struct IORequest *)&tr);
            Wait((1UL << dsig) | (1UL << tsig));
            AbortIO((struct IORequest *)&tr);
            WaitIO((struct IORequest *)&tr);
        }
        /* else: bounded poll until END/timeout */
    }

    /* Drain the reset: while RESET is asserted the channel ignores CONBLK_AD
     * writes, so the next transfer would silently run the old control block. */
    if (do_reset)
    {
        /* Before the reset wipes them. Legacy DEBUG bit 0 is
         * READ_LAST_NOT_SET, 1 FIFO_ERROR, 2 READ_ERROR; CS bit 0
         * ACTIVE, 1 END, 8 ERROR. DMA4: bit 0 is WRITE_ERROR, bit 3
         * READ_CB_ERROR, CS.ERROR at bit 10, and this DEBUG read
         * CLEARS the DMA4 latches (they are RC). Note the time printed
         * is the actual elapsed time, not the caller's budget. */
        bug("[DMA] channel %d %s after %uus (budget %uus): "
            "cs=0x%08x debug=0x%08x\n",
            channel, reason, (unsigned)(dma_now_us(DMABase) - start),
            timeout_us, AROS_LE2LONG(*cs),
            AROS_LE2LONG(*(volatile ULONG *)dma_debug_reg(DMABase, channel)));

        dma_channel_quiesce(DMABase, channel);
    }

    if (dsig >= 0)
    {
        /* Stop the IRQ handler from signalling a bit we're about to free */
        Disable();
        DMABase->dma_Wait[channel].waiter = NULL;
        __asm__ __volatile__("dmb sy" ::: "memory");
        if (tsig >= 0)
            FreeSignal(tsig);
        FreeSignal(dsig);
        Enable();
    }

    return ret;

    AROS_LIBFUNC_EXIT
}

/* Stop the channel without freeing it. 0, or -1 if it would not stop. */
/*****************************************************************************

    NAME */
        AROS_LH1(int, DMAStopChannel,

/*  SYNOPSIS */
                        AROS_LHA(int, channel, D0),

/*  LOCATION */
                        struct DMABase *, DMABase, 4, Dma)
/*  FUNCTION
        Stop and reset an allocated DMA engine without releasing its reservation.

    INPUTS
        channel - Channel returned by DMAAllocChannel(), still allocated
                  and exclusively controlled by the caller.

    RESULT
        0 if the engine passes the implementation's idle/reset checks;
        -1 for an out-of-range or unallocated channel, or if those checks
        fail. A -1 result does not establish that bus activity has ceased.

    NOTES
        The legacy path requests RESET, polls for its completion and
        clears the interrupt/end flags. The DMA4 path halts an active engine,
        checks for outstanding transactions, resets through DEBUG and checks
        for remaining activity. Reading DMA4 DEBUG clears its error latches.
        The operation discards transfer state; 0 does not mean that the
        requested transfer completed successfully.

        The reservation and resource IRQ handler remain installed. No buffer
        or control block is freed, and no cache maintenance is performed.
        The reset loop polls at most 10000 iterations; this is not a duration
        in microseconds and does not bound a stalled register access.

        This function does not acquire the allocation semaphore or notify
        a waiting task. Serialize it with channel programming, waiting,
        release and driver interrupt handling. Do not resume or free transfer
        storage on a -1 result without establishing hardware quiescence.

    SEE ALSO
        DMAAllocChannel(), DMAFreeChannel(), DMAWaitChannel()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[DMA] %s(%d)\n", __PRETTY_FUNCTION__, channel));

    if ((channel < 0) || (channel > 14) ||
        !(DMABase->dma_InUse & (1 << channel)))
        return -1;

    return dma_channel_quiesce(DMABase, channel) ? 0 : -1;

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(dma_init, 0)
