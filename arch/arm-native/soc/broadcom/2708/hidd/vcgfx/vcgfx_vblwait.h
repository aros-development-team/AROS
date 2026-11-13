#ifndef _VIDEOCOREGFX_VBLWAIT_H
#define _VIDEOCOREGFX_VBLWAIT_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Tasks sleeping on the vblank interrupt. Several clients (GL,
          video layers) wait for their own latch at once; one with no
          free slot spins instead.
*/

#include <exec/types.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#define VCGFX_VBL_WAITERS   4

struct vcgfx_vblwait
{
    struct Task * volatile  vw_Task[VCGFX_VBL_WAITERS];
    ULONG                   vw_Sig[VCGFX_VBL_WAITERS];
};

/* Slot index, -1 = none free */
static inline LONG vcgfx_vbl_add(struct vcgfx_vblwait *vw, ULONG sigmask)
{
    LONG i;

    Disable();
    for (i = 0; i < VCGFX_VBL_WAITERS; i++)
    {
        if (!vw->vw_Task[i])
        {
            vw->vw_Sig[i]  = sigmask;
            vw->vw_Task[i] = FindTask(NULL);
            break;
        }
    }
    Enable();
    return (i < VCGFX_VBL_WAITERS) ? i : -1;
}

static inline void vcgfx_vbl_rem(struct vcgfx_vblwait *vw, LONG slot)
{
    Disable();
    vw->vw_Task[slot] = NULL;
    Enable();
}

/* Interrupt context, or Disable()d */
static inline void vcgfx_vbl_signal(struct vcgfx_vblwait *vw)
{
    struct Task *t;
    LONG i;

    for (i = 0; i < VCGFX_VBL_WAITERS; i++)
    {
        if ((t = vw->vw_Task[i]))
            Signal(t, vw->vw_Sig[i]);
    }
}

#endif /* _VIDEOCOREGFX_VBLWAIT_H */
