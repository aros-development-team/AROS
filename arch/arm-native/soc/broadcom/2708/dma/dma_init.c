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
 * users may rely on per-channel completion interrupts.
 */
#define DMA_POOL_FULL   ((1 << 2) | (1 << 4) | (1 << 5))
#define DMA_POOL_LITE   ((1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | \
                         (1 << 12))

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

        *cs = AROS_LONG2LE(DMA_CS_INT);
        t = DMABase->dma_Wait[channel].waiter;
        if (t)
            Signal(t, 1UL << DMABase->dma_Wait[channel].sig);
    }
}

/* Enable the channel and bring the engine to a clean, idle state. */
static void dma_channel_reset(struct DMABase *DMABase, int channel)
{
    volatile ULONG *enable = (volatile ULONG *)DMA_ENABLE_REG;
    volatile ULONG *cs = (volatile ULONG *)DMA_CS(channel);
    int try = 10000;

    *enable = AROS_LONG2LE(AROS_LE2LONG(*enable) | (1 << channel));

    *cs = AROS_LONG2LE(DMA_CS_RESET);
    while (try-- > 0)
    {
        if (!(AROS_LE2LONG(*cs) & DMA_CS_RESET))
            break;
    }
    *cs = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
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

    /* Prefer lite channels so the scarce full engines stay available
     * for users that need TDMODE. */
    if (flags & DMACHF_TDMODE)
        avail = DMA_POOL_FULL & ~DMABase->dma_InUse;
    else
    {
        avail = DMA_POOL_LITE & ~DMABase->dma_InUse;
        if (avail == 0)
            avail = DMA_POOL_FULL & ~DMABase->dma_InUse;
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
                KrnAddIRQHandler(IRQ_DMA0 + channel, dma_irq_handler,
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
        volatile ULONG *cs = (volatile ULONG *)DMA_CS(channel);
        int try = 10000;

        *cs = AROS_LONG2LE(DMA_CS_RESET);
        while (try-- > 0)
        {
            if (!(AROS_LE2LONG(*cs) & DMA_CS_RESET))
                break;
        }

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
            ret = 0;
            break;
        }
        if (!(v & DMA_CS_ACTIVE))
        {
            *cs = AROS_LONG2LE(DMA_CS_RESET);
            break;
        }
        if ((dma_now_us(DMABase) - start) > timeout_us)
        {
            *cs = AROS_LONG2LE(DMA_CS_RESET);
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
