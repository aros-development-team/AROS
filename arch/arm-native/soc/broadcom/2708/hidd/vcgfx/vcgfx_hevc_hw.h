/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    HEVC block: the engine task and its jobs (vcgfx_hevc_hw.c).
*/

#ifndef VCGFX_HEVC_HW_H
#define VCGFX_HEVC_HW_H

#include <exec/lists.h>
#include <exec/semaphores.h>

#include "vcgfx_hevc.h"

/* A job's result, as the decode interface reports it */
#define HEVC_JOB_DONE       0   /* vHidd_DecodeStatus_Ready */
#define HEVC_JOB_PENDING    1   /* vHidd_DecodeStatus_Busy  */
#define HEVC_JOB_ERROR      2   /* vHidd_DecodeStatus_Error */

/* One picture: its command list and bitstream live with it */
struct vcgfx_hevc_job
{
    struct MinNode          node;
    ULONG                   seq;
    volatile ULONG         *status;     /* the target buffer's, set when done */
    struct hevc_cl          cl;
    struct hevc_phase2      p2;
    ULONG                   pustride, costride;
    struct vcgfx_scanbuf    clmem;
    struct vcgfx_scanbuf    bsmem;
};

struct vcgfx_hevc_waiter
{
    struct MinNode          node;
    struct Task            *task;
    ULONG                   sigmask;
};

struct vcgfx_hevc
{
    struct VideoCoreGfx_staticdata *xsd;
    UBYTE                  *regs;
    UBYTE                  *intc;
    APTR                    irq;
    struct Task            *task;
    struct Task            *starter;
    BOOL                    taskok;
    BOOL                    dead;       /* a phase never finished: no more jobs run */
    BYTE                    irqsig;
    ULONG                   irqbits;    /* latched ICTRL bits, atomic */

    struct SignalSemaphore  lock;       /* queue, waiters, sequence numbers */
    struct MinList          queue;
    struct MinList          waiters;
    ULONG                   submitted;
    ULONG                   completed;

    /* One PU/coefficient pair: the engine runs one picture at a time */
    struct vcgfx_scanbuf    pu, coeff;
    ULONG                   pusize, coeffsize;
};

UQUAD vcgfx_hevc_bus(APTR p);

/* The block, brought up on first use; NULL when there is none */
struct vcgfx_hevc *vcgfx_hevc_open(struct VideoCoreGfx_staticdata *xsd);

/* Queue a job; it is freed once done. Returns its sequence number. */
ULONG vcgfx_hevc_submit(struct vcgfx_hevc *h, struct vcgfx_hevc_job *job);

/* Wait for a status to leave PENDING, or for every job up to seq */
ULONG vcgfx_hevc_wait(struct vcgfx_hevc *h, volatile ULONG *status, ULONG timeoutms);
void  vcgfx_hevc_drain(struct vcgfx_hevc *h, ULONG seq);

void  vcgfx_hevc_job_free(struct vcgfx_hevc *h, struct vcgfx_hevc_job *job);

#endif /* VCGFX_HEVC_HW_H */
