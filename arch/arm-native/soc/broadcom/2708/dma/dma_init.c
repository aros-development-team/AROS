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
 * BCM2712 DMA channel allocation.
 */
#define DMA_POOL_FULL_BCM2712  ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3))
#define DMA_POOL_LITE_BCM2712  ((1 << 4) | (1 << 5) | (1 << 6) | (1 << 7))

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

/* Stop the channel and leave the engine idle with its flags clear. */
static void dma_channel_quiesce(struct DMABase *DMABase, int channel)
{
    volatile ULONG *cs = (volatile ULONG *)DMA_CS(channel);
    int try = 10000;

    if (BCM2708_DMA_IS_DMA4(DMABase->dma_periiobase, channel))
    {
        /*
         * DMA4 has no reset bit in CS - bit 31 is HALT there - and resetting
         * while the AXI bus still has transactions in flight is documented as
         * probably fatal. So halt, let the engine drain, then reset via DEBUG.
         */
        *cs = AROS_LONG2LE(DMA4_CS_HALT);
        while (try-- > 0)
        {
            if (!(AROS_LE2LONG(*cs) & (DMA4_CS_HALT | DMA4_CS_DMA_BUSY)))
                break;
        }

        /* The DMA4 error latches are read-to-clear, not W1C - and they
         * must be cleared, or CS.ERROR stays up and the engine
         * misbehaves on later transfers. */
        (void)*(volatile ULONG *)DMA4_DEBUG(channel);

        if (AROS_LE2LONG(*cs) & (DMA4_CS_HALT | DMA4_CS_DMA_BUSY))
            /* Still draining: a DEBUG reset now is documented as
             * "probably fatal", so leave the engine as it stands. */
            bug("[DMA] channel %d would not halt (cs=0x%08x) - "
                "reset skipped\n", channel, AROS_LE2LONG(*cs));
        else
            *(volatile ULONG *)DMA4_DEBUG(channel) =
                AROS_LONG2LE(DMA4_DEBUG_RESET);
        *cs = AROS_LONG2LE(DMA4_CS_INT | DMA4_CS_END);
        return;
    }

    *cs = AROS_LONG2LE(DMA_CS_RESET);
    while (try-- > 0)
    {
        if (!(AROS_LE2LONG(*cs) & DMA_CS_RESET))
            break;
    }
    *cs = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
}

/* Enable the channel and bring the engine to a clean, idle state. */
static void dma_channel_reset(struct DMABase *DMABase, int channel)
{
    volatile ULONG *enable = (volatile ULONG *)DMA_ENABLE_REG;

    *enable = AROS_LONG2LE(AROS_LE2LONG(*enable) | (1 << channel));

    dma_channel_quiesce(DMABase, channel);
}

AROS_LH1(int, DMAAllocChannel,
                AROS_LHA(unsigned int, flags, D0),
                struct DMABase *, DMABase, 1, Dma)
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
            avail = is2711 ? (DMA_POOL_DMA4_BCM2711 & ~DMABase->dma_InUse) : 0;
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

    if (avail != 0)
    {
        for (ch = 0; ch < 15; ch++)
        {
            if (avail & (1 << ch))
            {
                channel = ch;
                break;
            }
        }

        DMABase->dma_InUse |= (1 << channel);
        dma_channel_reset(DMABase, channel);

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

AROS_LH1(void, DMAFreeChannel,
                AROS_LHA(int, channel, D0),
                struct DMABase *, DMABase, 2, Dma)
{
    AROS_LIBFUNC_INIT

    D(bug("[DMA] %s(%d)\n", __PRETTY_FUNCTION__, channel));

    if ((channel < 0) || (channel > 14))
        return;

    ObtainSemaphore(&DMABase->dma_Sem);

    if (DMABase->dma_InUse & (1 << channel))
    {
        volatile ULONG *enable = (volatile ULONG *)DMA_ENABLE_REG;

        dma_channel_quiesce(DMABase, channel);

        *enable = AROS_LONG2LE(AROS_LE2LONG(*enable) & ~(1 << channel));
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

ADD2INITLIB(dma_init, 0)
