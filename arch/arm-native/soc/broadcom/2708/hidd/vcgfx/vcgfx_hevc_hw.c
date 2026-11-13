/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    HEVC block: the engine. A task of its own runs the jobs one at a
    time, phase 1 (entropy decode into the PU and coefficient buffers)
    and then phase 2 (reconstruction into the target). Phase 1 restarts
    with bigger buffers when a CTB row did not fit.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <devices/timer.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/mbox.h>

#include <string.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"
#include "vcgfx_hevc_hw.h"

extern APTR KernelBase;

#ifdef MBoxBase
#undef MBoxBase
#endif
#define MBoxBase        xsd->vcsd_MBoxBase

#define SIGF_JOB        SIGBREAKF_CTRL_E
#define SIGF_QUIT       SIGBREAKF_CTRL_C
#define PHASE_TIMEOUT   1000    /* ms */
#define SETTLE_TIMEOUT  500     /* ms for a timed out phase to go idle */
#define BUF_MAX         (64 << 20)
#define NOW_US()        AROS_LE2LONG(*(volatile ULONG *)(ARM_PERIIOBASE + 0x3004))   /* system timer */

static struct vcgfx_hevc *hevc_dev;
static BOOL hevc_tried;

static void reg_w(struct vcgfx_hevc *h, ULONG off, ULONG v)
{
    *(volatile ULONG *)(h->regs + off) = AROS_LONG2LE(v);
}

static ULONG reg_r(struct vcgfx_hevc *h, ULONG off)
{
    return AROS_LE2LONG(*(volatile ULONG *)(h->regs + off));
}

static void intc_w(struct vcgfx_hevc *h, ULONG v)
{
    *(volatile ULONG *)(h->intc + HEVC_ICTRL) = AROS_LONG2LE(v);
}

static ULONG intc_r(struct vcgfx_hevc *h)
{
    return AROS_LE2LONG(*(volatile ULONG *)(h->intc + HEVC_ICTRL));
}

/* Pi 4: the block sees ARM physical addresses */
UQUAD vcgfx_hevc_bus(APTR p)
{
    return (UQUAD)(IPTR)KrnVirtualToPhysical(p);
}

/* Firmware clock 11 to its maximum: the block runs on it */
static BOOL hevc_clock_on(struct VideoCoreGfx_staticdata *xsd)
{
    unsigned int *m = xsd->vcsd_MBoxMessage;
    ULONG rate = 0;

    VC4_MBOX_LOCK(xsd);
    m[0] = AROS_LONG2LE(8 * 4);
    m[1] = AROS_LONG2LE(VCTAG_REQ);
    m[2] = AROS_LONG2LE(VCTAG_GETMAXCLKRATE);
    m[3] = AROS_LONG2LE(8);
    m[4] = 0;
    m[5] = AROS_LONG2LE(HEVC_FW_CLOCK_ID);
    m[6] = 0;
    m[7] = 0;
    if ((MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, m) != (volatile unsigned int *)-1)
        && (m[1] == AROS_LONG2LE(VCTAG_RESP)))
        rate = AROS_LE2LONG(m[6]);

    if (rate)
    {
        m[0] = AROS_LONG2LE(9 * 4);
        m[1] = AROS_LONG2LE(VCTAG_REQ);
        m[2] = AROS_LONG2LE(VCTAG_SETCLKRATE);
        m[3] = AROS_LONG2LE(12);
        m[4] = 0;
        m[5] = AROS_LONG2LE(HEVC_FW_CLOCK_ID);
        m[6] = AROS_LONG2LE(rate);
        m[7] = 0;   /* no turbo */
        m[8] = 0;
        if ((MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, m) == (volatile unsigned int *)-1)
            || (m[1] != AROS_LONG2LE(VCTAG_RESP)))
            rate = 0;
        else
            rate = AROS_LE2LONG(m[6]);
    }
    VC4_MBOX_UNLOCK(xsd);

    return rate != 0;
}

/* Latch what the phases signalled; the task looks at it */
static void hevc_irq(struct vcgfx_hevc *h, void *unused)
{
    ULONG v = intc_r(h);

    if (!(v & HEVC_ICTRL_PENDING))
        return;
    intc_w(h, v & ~HEVC_ICTRL_RESERVED);
    __atomic_fetch_or(&h->irqbits, v & (HEVC_ICTRL_ACTIVE1_INT | HEVC_ICTRL_ACTIVE2_INT),
                      __ATOMIC_SEQ_CST);
    Signal(h->task, 1UL << h->irqsig);
}

/* Until the phase's interrupt, or the timeout. On the engine task. */
static BOOL hevc_wait_irq(struct vcgfx_hevc *h, ULONG bit, struct timerequest *tr)
{
    ULONG irqmask = 1UL << h->irqsig;
    ULONG tmask = 1UL << tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit;
    BOOL got = FALSE;

    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = PHASE_TIMEOUT / 1000;
    tr->tr_time.tv_micro = (PHASE_TIMEOUT % 1000) * 1000;
    SetSignal(0, tmask);
    SendIO((struct IORequest *)tr);

    for (;;)
    {
        if (__atomic_fetch_and(&h->irqbits, ~bit, __ATOMIC_SEQ_CST) & bit)
        {
            got = TRUE;
            break;
        }
        if (CheckIO((struct IORequest *)tr))
            break;
        Wait(irqmask | tmask);
    }

    if (!CheckIO((struct IORequest *)tr))
        AbortIO((struct IORequest *)tr);
    WaitIO((struct IORequest *)tr);
    return got;
}

static void hevc_delay(struct timerequest *tr, ULONG ms)
{
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = ms / 1000;
    tr->tr_time.tv_micro = (ms % 1000) * 1000;
    DoIO((struct IORequest *)tr);
}

/*
 * A phase that timed out may still be running and writing its buffers:
 * wait for its active line to drop, then forget what it latched. One that
 * never stops leaves the engine dead, its buffers in place.
 */
static BOOL hevc_settle(struct vcgfx_hevc *h, ULONG active, struct timerequest *tr)
{
    ULONG ms;

    for (ms = 0; ms < SETTLE_TIMEOUT; ms++)
    {
        if (!(intc_r(h) & active))
        {
            hevc_delay(tr, 1);      /* its late interrupt */
            __atomic_store_n(&h->irqbits, 0, __ATOMIC_SEQ_CST);
            return TRUE;
        }
        hevc_delay(tr, 1);
    }
    bug("[VideoCoreGfx] HEVC block stays busy: no more decoding\n");
    h->dead = TRUE;
    return FALSE;
}

/* The size step of the spec: 3 x 2^floor(log2 x), at least 768 */
static ULONG hevc_bufsize(ULONG x)
{
    ULONG p = 1;

    if (x < 256)
        return 768;
    while (p <= x / 2)
        p <<= 1;
    return 3 * p;
}

static BOOL hevc_grow(struct vcgfx_hevc *h, struct vcgfx_scanbuf *b, ULONG *size, ULONG want)
{
    if (b->sb_Raw && (*size >= want))
        return TRUE;
    if (want > BUF_MAX)
        return FALSE;
    vcgfx_scanout_free(h->xsd, b);
    *size = 0;
    if (!vcgfx_scanout_alloc(h->xsd, b, want))
        return FALSE;
    *size = want;
    return TRUE;
}

static BOOL hevc_phase1(struct vcgfx_hevc *h, struct vcgfx_hevc_job *job, struct timerequest *tr)
{
    ULONG rows = job->p2.numrows;
    ULONG w = job->p2.framesize & 0xffff, ht = job->p2.framesize >> 16;
    ULONG area = ((job->p2.config2 & (1 << 8)) ? HEVC_SAND10_BYTES(w) : HEVC_SAND_WIDTH(w))
                 * HEVC_SAND_HEIGHT(ht);
    ULONG tries;

    if (!hevc_grow(h, &h->coeff, &h->coeffsize, hevc_bufsize(area))
        || !hevc_grow(h, &h->pu, &h->pusize, hevc_bufsize(area / 4)))
        return FALSE;

    for (tries = 0; tries < 8; tries++)
    {
        ULONG status;

        job->pustride = (h->pusize / rows) & ~63;
        job->costride = (h->coeffsize / rows) & ~63;
        reg_w(h, HEVC_PUWBASE, HEVC_ADDR(vcgfx_hevc_bus(h->pu.sb_Pixels)));
        reg_w(h, HEVC_PUWSTRIDE, HEVC_LEN(job->pustride));
        reg_w(h, HEVC_COEFFWBASE, HEVC_ADDR(vcgfx_hevc_bus(h->coeff.sb_Pixels)));
        reg_w(h, HEVC_COEFFWSTRIDE, HEVC_LEN(job->costride));
        reg_w(h, HEVC_CFNUM, job->cl.n);
        __atomic_store_n(&h->irqbits, 0, __ATOMIC_SEQ_CST);
        __asm__ __volatile__("dsb sy" ::: "memory");
        reg_w(h, HEVC_CFBASE, HEVC_ADDR(vcgfx_hevc_bus(job->cl.cmds)));

        if (!hevc_wait_irq(h, HEVC_ICTRL_ACTIVE1_INT, tr))
        {
            bug("[VideoCoreGfx] HEVC phase 1 timed out (%u of %u commands)\n",
                (unsigned)reg_r(h, HEVC_CFSTATUS), (unsigned)job->cl.n);
            hevc_settle(h, HEVC_ICTRL_ACTIVE1, tr);
            return FALSE;
        }
        if (reg_r(h, HEVC_CFSTATUS) == job->cl.n)
            return TRUE;

        status = reg_r(h, HEVC_STATUS);
        if (!(status & (HEVC_STATUS_COEFF_FULL | HEVC_STATUS_PU_FULL)))
        {
            D(bug("[VideoCoreGfx] HEVC phase 1 error: %u of %u commands, status 0x%08x\n",
                  (unsigned)reg_r(h, HEVC_CFSTATUS), (unsigned)job->cl.n, (unsigned)status));
            return FALSE;
        }

        /* A CTB row did not fit: bigger buffers, the same picture again */
        D(bug("[VideoCoreGfx] HEVC buffers full (status 0x%08x), growing\n", (unsigned)status));
        if ((status & HEVC_STATUS_COEFF_FULL)
            && !hevc_grow(h, &h->coeff, &h->coeffsize, hevc_bufsize(h->coeffsize + 1)))
            return FALSE;
        if ((status & HEVC_STATUS_PU_FULL)
            && !hevc_grow(h, &h->pu, &h->pusize, hevc_bufsize(h->pusize + 1)))
            return FALSE;
    }
    return FALSE;
}

static BOOL hevc_phase2(struct vcgfx_hevc *h, struct vcgfx_hevc_job *job, struct timerequest *tr)
{
    const struct hevc_phase2 *p = &job->p2;
    ULONG i;

    reg_w(h, HEVC_PURBASE, HEVC_ADDR(vcgfx_hevc_bus(h->pu.sb_Pixels)));
    reg_w(h, HEVC_PURSTRIDE, HEVC_LEN(job->pustride));
    reg_w(h, HEVC_COEFFRBASE, HEVC_ADDR(vcgfx_hevc_bus(h->coeff.sb_Pixels)));
    reg_w(h, HEVC_COEFFRSTRIDE, HEVC_LEN(job->costride));

    reg_w(h, HEVC_OUTYBASE, HEVC_ADDR(p->outy));
    reg_w(h, HEVC_OUTCBASE, HEVC_ADDR(p->outc));
    reg_w(h, HEVC_OUTYSTRIDE, HEVC_LEN(p->outstride));
    reg_w(h, HEVC_OUTCSTRIDE, HEVC_LEN(p->outstride));

    for (i = 0; i < HEVC_REF_SLOTS; i++)
    {
        reg_w(h, HEVC_REFS + 16 * i, HEVC_ADDR(p->refy[i]));
        reg_w(h, HEVC_REFS + 16 * i + 4, HEVC_LEN(p->outstride));
        reg_w(h, HEVC_REFS + 16 * i + 8, HEVC_ADDR(p->refc[i]));
        reg_w(h, HEVC_REFS + 16 * i + 12, HEVC_LEN(p->outstride));
    }

    reg_w(h, HEVC_CONFIG2, p->config2);
    reg_w(h, HEVC_FRAMESIZE, p->framesize);
    reg_w(h, HEVC_CURRPOC, p->currpoc);

    reg_w(h, HEVC_COLSTRIDE, HEVC_LEN(p->mvstride));
    reg_w(h, HEVC_MVSTRIDE, HEVC_LEN(p->mvstride));
    reg_w(h, HEVC_MVBASE, HEVC_ADDR(p->mvbase));
    reg_w(h, HEVC_COLBASE, HEVC_ADDR(p->colbase));

    __atomic_store_n(&h->irqbits, 0, __ATOMIC_SEQ_CST);
    __asm__ __volatile__("dsb sy" ::: "memory");
    reg_w(h, HEVC_NUMROWS, p->numrows);

    if (!hevc_wait_irq(h, HEVC_ICTRL_ACTIVE2_INT, tr))
    {
        bug("[VideoCoreGfx] HEVC phase 2 timed out\n");
        hevc_settle(h, HEVC_ICTRL_ACTIVE2, tr);
        return FALSE;
    }
    return TRUE;
}

/* A job the dead block may still read stays allocated */
static void hevc_finish(struct vcgfx_hevc *h, struct vcgfx_hevc_job *job, ULONG status, BOOL release)
{
    struct vcgfx_hevc_waiter *w;

    ObtainSemaphore(&h->lock);
    if (job->status)
        *job->status = status;
    h->completed = job->seq;
    ForeachNode(&h->waiters, w)
        Signal(w->task, w->sigmask);
    ReleaseSemaphore(&h->lock);

    if (release)
        vcgfx_hevc_job_free(h, job);
}

static void hevc_task(struct vcgfx_hevc *h)
{
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *tr = port ? CreateIORequest(port, sizeof(*tr)) : NULL;
    BOOL ok = FALSE;

    h->irqsig = AllocSignal(-1);
    if (tr && (h->irqsig != -1) && !OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
        ok = TRUE;
    h->taskok = ok;
    Signal(h->starter, SIGF_SINGLE);

    while (ok)
    {
        struct vcgfx_hevc_job *job;
        ULONG sigs = Wait(SIGF_JOB | SIGF_QUIT);

        for (;;)
        {
            ObtainSemaphore(&h->lock);
            job = (struct vcgfx_hevc_job *)RemHead((struct List *)&h->queue);
            ReleaseSemaphore(&h->lock);
            if (!job)
                break;

            if (h->dead)
                hevc_finish(h, job, HEVC_JOB_ERROR, TRUE);
            else if (hevc_phase1(h, job, tr) && hevc_phase2(h, job, tr))
                hevc_finish(h, job, HEVC_JOB_DONE, TRUE);
            else
                hevc_finish(h, job, HEVC_JOB_ERROR, !h->dead);
        }

        if (sigs & SIGF_QUIT)
            break;
    }

    if (ok)
        CloseDevice((struct IORequest *)tr);
    if (tr)
        DeleteIORequest((struct IORequest *)tr);
    if (port)
        DeleteMsgPort(port);
}

/* Bring the block up on first use: Pi 4 and Pi 5 */
struct vcgfx_hevc *vcgfx_hevc_open(struct VideoCoreGfx_staticdata *xsd)
{
    struct vcgfx_hevc *h;
    ULONG v;

    ObtainSemaphore(&xsd->vcsd_ScanoutLock);
    if (hevc_tried || !vcgfx_has_hevc(xsd))
    {
        ReleaseSemaphore(&xsd->vcsd_ScanoutLock);
        return hevc_dev;
    }
    hevc_tried = TRUE;

    h = AllocVec(sizeof(*h), MEMF_ANY | MEMF_CLEAR);
    if (!h)
        goto out;
    h->xsd = xsd;
    h->regs = (UBYTE *)(IPTR)(xsd->vcsd_IsBCM2711 ? HEVC_BCM2711_REGS : HEVC_BCM2712_REGS);
    h->intc = (UBYTE *)(IPTR)(xsd->vcsd_IsBCM2711 ? HEVC_BCM2711_INTC : HEVC_BCM2712_INTC);
    InitSemaphore(&h->lock);
    NEWLIST((struct List *)&h->queue);
    NEWLIST((struct List *)&h->waiters);

    /* Both phase interrupts on, falling edges; then clear what is latched */
    intc_w(h, HEVC_ICTRL_ACTIVE1_EN | HEVC_ICTRL_ACTIVE2_EN);
    v = intc_r(h);
    intc_w(h, v & ~HEVC_ICTRL_RESERVED);

    if (!hevc_clock_on(xsd))
    {
        bug("[VideoCoreGfx] HEVC: the firmware gives the block no clock\n");
        FreeVec(h);
        goto out;
    }

    h->starter = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    h->task = NewCreateTask(TASKTAG_PC,        hevc_task,
                            TASKTAG_NAME,      "VideoCoreGfx HEVC",
                            TASKTAG_STACKSIZE, 16384,
                            TASKTAG_PRI,       5,
                            TASKTAG_ARG1,      h,
                            TAG_DONE);
    if (h->task)
        Wait(SIGF_SINGLE);
    if (!h->task || !h->taskok)
    {
        /* A task that failed its setup has ended by now */
        FreeVec(h);
        goto out;
    }

    h->irq = KrnAddIRQHandler(HEVC_IRQ, hevc_irq, h, NULL);
    if (!h->irq)
    {
        /* The task stays, idle: nothing ever queues to it */
        bug("[VideoCoreGfx] HEVC: no interrupt\n");
        goto out;
    }

    bug("[VideoCoreGfx] HEVC decoder ready, block version 0x%08x\n", (unsigned)reg_r(h, HEVC_VERSION));
    hevc_dev = h;
out:
    ReleaseSemaphore(&xsd->vcsd_ScanoutLock);
    return hevc_dev;
}

ULONG vcgfx_hevc_submit(struct vcgfx_hevc *h, struct vcgfx_hevc_job *job)
{
    ULONG seq;

    ObtainSemaphore(&h->lock);
    seq = job->seq = ++h->submitted;
    if (job->status)
        *job->status = HEVC_JOB_PENDING;
    AddTail((struct List *)&h->queue, (struct Node *)&job->node);
    ReleaseSemaphore(&h->lock);
    Signal(h->task, SIGF_JOB);

    return seq;
}

static BOOL hevc_done(struct vcgfx_hevc *h, volatile ULONG *status, ULONG seq)
{
    return status ? (*status != HEVC_JOB_PENDING) : ((LONG)(h->completed - seq) >= 0);
}

/*
 * A short sleep without a signal of our own: a timer request replied to
 * SIGB_SINGLE, as dos.library's Delay() does (not callable from a ROM hidd).
 */
static void hevc_nap(ULONG us)
{
    struct MsgPort mp;
    struct timerequest tr;

    memset(&mp, 0, sizeof(mp));
    mp.mp_Node.ln_Type = NT_MSGPORT;
    mp.mp_Flags        = PA_SIGNAL;
    mp.mp_SigBit       = SIGB_SINGLE;
    mp.mp_SigTask      = FindTask(NULL);
    NEWLIST(&mp.mp_MsgList);

    memset(&tr, 0, sizeof(tr));
    tr.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    tr.tr_node.io_Message.mn_ReplyPort    = &mp;
    tr.tr_node.io_Message.mn_Length       = sizeof(tr);
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)&tr, 0))
        return;
    tr.tr_node.io_Command = TR_ADDREQUEST;
    tr.tr_time.tv_micro   = us;
    SetSignal(0, SIGF_SINGLE);
    DoIO((struct IORequest *)&tr);
    CloseDevice((struct IORequest *)&tr);
}

/* Sleep until hevc_done(), or timeoutms (~0: for ever) */
static void hevc_sleep(struct vcgfx_hevc *h, volatile ULONG *status, ULONG seq, ULONG timeoutms)
{
    struct vcgfx_hevc_waiter w;
    struct MsgPort *port = NULL;
    struct timerequest *tr = NULL;
    BYTE sig;

    if (hevc_done(h, status, seq) || !timeoutms)
        return;

    sig = AllocSignal(-1);
    if (sig == -1)
    {
        /* No signal to wait on: poll, napping in between */
        ULONG start = NOW_US();

        while (!hevc_done(h, status, seq)
               && ((timeoutms == (ULONG)~0) || ((NOW_US() - start) / 1000 < timeoutms)))
            hevc_nap(1000);
        return;
    }
    w.task = FindTask(NULL);
    w.sigmask = 1UL << sig;

    if (timeoutms != (ULONG)~0)
    {
        port = CreateMsgPort();
        tr = port ? CreateIORequest(port, sizeof(*tr)) : NULL;
        if (tr && !OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
        {
            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_secs = timeoutms / 1000;
            tr->tr_time.tv_micro = (timeoutms % 1000) * 1000;
            SendIO((struct IORequest *)tr);
        }
        else if (tr)
        {
            DeleteIORequest((struct IORequest *)tr);
            tr = NULL;
        }
    }

    ObtainSemaphore(&h->lock);
    AddTail((struct List *)&h->waiters, (struct Node *)&w.node);
    ReleaseSemaphore(&h->lock);

    while (!hevc_done(h, status, seq))
    {
        if (tr && CheckIO((struct IORequest *)tr))
            break;
        Wait(w.sigmask | (tr ? 1UL << port->mp_SigBit : 0));
    }

    ObtainSemaphore(&h->lock);
    Remove((struct Node *)&w.node);
    ReleaseSemaphore(&h->lock);

    if (tr)
    {
        if (!CheckIO((struct IORequest *)tr))
            AbortIO((struct IORequest *)tr);
        WaitIO((struct IORequest *)tr);
        CloseDevice((struct IORequest *)tr);
        DeleteIORequest((struct IORequest *)tr);
    }
    if (port)
        DeleteMsgPort(port);
    SetSignal(0, w.sigmask);
    FreeSignal(sig);
}

ULONG vcgfx_hevc_wait(struct vcgfx_hevc *h, volatile ULONG *status, ULONG timeoutms)
{
    hevc_sleep(h, status, 0, timeoutms);
    return *status;
}

void vcgfx_hevc_drain(struct vcgfx_hevc *h, ULONG seq)
{
    hevc_sleep(h, NULL, seq, ~0);
}

void vcgfx_hevc_job_free(struct vcgfx_hevc *h, struct vcgfx_hevc_job *job)
{
    vcgfx_scanout_free(h->xsd, &job->clmem);
    vcgfx_scanout_free(h->xsd, &job->bsmem);
    FreeVec(job);
}
