/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Architecture-independent software entropy collector for
          entropy.resource.

    This is the always-available fallback source.  On its own it does not
    pretend to be a strong physical entropy source - it samples cheap,
    constantly-changing system state (scheduler dispatch/idle counters, live
    task and list pointers, free-memory figures and a little scheduling
    jitter) and lets the ChaCha20 CSPRNG accumulate and whiten it.  On
    platforms that expose real hardware entropy the architecture back-end in
    entropy_hw.c supplements this; where none exists this keeps the resource
    usable.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "entropy_intern.h"

/* Append one machine-word sample to buf/pos, stopping at max. */
static inline void put_iptr(UBYTE *buf, ULONG *pos, ULONG max, IPTR value)
{
    ULONG p = *pos;
    ULONG i;

    for (i = 0; i < sizeof(IPTR) && p < max; i++)
        buf[p++] = (UBYTE)(value >> (i * 8));

    *pos = p;
}

ULONG Entropy_GatherSoftware(struct EntropyBase *EntropyBase,
                             UBYTE *buffer, ULONG length)
{
    struct ExecBase *sysBase = (struct ExecBase *)SysBase;
    volatile ULONG   spin;
    ULONG            pos = 0;
    ULONG            disp0, idle0, jitter;
    UBYTE            here;      /* address of a stack object */

    if (length == 0)
        return 0;

    /* Address-layout samples (stack, caller buffer, resource base, current
       task). */
    put_iptr(buffer, &pos, length, (IPTR)&here);
    put_iptr(buffer, &pos, length, (IPTR)buffer);
    put_iptr(buffer, &pos, length, (IPTR)EntropyBase);
    put_iptr(buffer, &pos, length, (IPTR)FindTask(NULL));

    /* Live scheduler / list state. */
    put_iptr(buffer, &pos, length, (IPTR)sysBase->TaskReady.lh_Head);
    put_iptr(buffer, &pos, length, (IPTR)sysBase->TaskWait.lh_Head);

    disp0 = sysBase->DispCount;
    idle0 = sysBase->IdleCount;
    put_iptr(buffer, &pos, length, (IPTR)disp0);
    put_iptr(buffer, &pos, length, (IPTR)idle0);
    put_iptr(buffer, &pos, length,
             (IPTR)(((ULONG)sysBase->Quantum << 16) | (ULONG)sysBase->Elapsed));
    put_iptr(buffer, &pos, length,
             (IPTR)(((ULONG)(UBYTE)sysBase->IDNestCnt << 24) |
                    ((ULONG)(UBYTE)sysBase->TDNestCnt << 16) |
                    ((ULONG)sysBase->SysFlags << 0) |
                    ((ULONG)sysBase->AttnResched << 8)));

    /* Free-memory figures shift with every allocation in the system. */
    put_iptr(buffer, &pos, length, (IPTR)AvailMem(MEMF_ANY));
    put_iptr(buffer, &pos, length, (IPTR)AvailMem(MEMF_ANY | MEMF_LARGEST));

    /* A little scheduling jitter: spin for a data-dependent number of
       iterations and fold in how the dispatch/idle counters moved while we
       ran, plus the low bits of the spin counter itself. */
    jitter = (disp0 ^ idle0 ^ EntropyBase->eb_Sequence) & 0x3ff;
    for (spin = 0; spin <= jitter; spin++)
        ;   /* volatile: not optimised away */
    put_iptr(buffer, &pos, length,
             (IPTR)((sysBase->DispCount - disp0) ^
                    ((sysBase->IdleCount - idle0) << 8) ^ (ULONG)spin));

    /* Monotonic per-request counter guarantees successive calls differ even
       if every other sampled value happened to repeat. */
    put_iptr(buffer, &pos, length, (IPTR)(++EntropyBase->eb_Sequence));

    return pos;
}
