/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <kernel_base.h>
#include <kernel_spinlock.h>

#if defined(KRN_SPINLOCK_ATOMICS)
#include <exec/tasks.h>
#include <proto/exec.h>

/* Called directly: it takes no base, and the lock is used before the base exists */
extern int AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)(void);
#define krnSpinIsSuper()        AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)()

/* A port can leave a trail for its own diagnostics while a CPU waits */
#if !defined(krnSpinWaitDecl)
#define krnSpinWaitDecl
#define krnSpinWaitBegin(lock)
#define krnSpinWaitEnd()
#endif
#endif

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH3(spinlock_t *, KrnSpinLock,

/*  SYNOPSIS */
        AROS_LHA(spinlock_t *, lock, A1),
        AROS_LHA(struct Hook *, failhook, A0),
        AROS_LHA(ULONG, mode, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 52, Kernel)

/*  FUNCTION
        Acquire a spinlock in read or write mode, waiting while it is held.

        A spinlock gives exclusion between CPUs and nothing else. It does
        not stop task switching and it does not mask interrupts. A caller
        that needs either takes the lock inside Forbid() or Disable().

    INPUTS
        lock     - Spinlock to acquire.
        failhook - Optional hook, called each time an attempt fails.
        mode     - SPINLOCK_MODE_READ or SPINLOCK_MODE_WRITE.

    RESULT
        The supplied spinlock pointer.

    NOTES
        The behaviour is the same on every port with real spinlocks.

        A task may be switched out while it holds a lock, so the holder of
        a lock is not always running. A write lock taken by a task records
        the task as the owner of the lock. A task of higher priority that
        waits for the lock lowers its own priority to the owner's until it
        has the lock, so that the owner can run and release it.

        A caller in supervisor mode (interrupt or kernel context) records
        no owner and never changes a priority. A lock that is taken in
        supervisor mode must be taken with interrupts disabled wherever a
        task takes it.

        Ports without real spinlocks return lock without acquiring it.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnSpinInit(), KrnSpinTryLock(), KrnSpinUnLock()

    INTERNALS
        The port supplies only the atomic steps, in kernel_spinlock.h.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(KRN_SPINLOCK_ATOMICS)
    krnSpinWaitDecl

    if (mode == SPINLOCK_MODE_WRITE)
    {
        struct Task *me = NULL;
        BYTE old_pri = 0;
        BOOL priority_changed = FALSE;

        /* Only a task is an owner, and only a task changes its priority */
        if (!krnSpinIsSuper())
            me = krnSpinThisTask();

        while (!krnSpinTryWrite(lock))
        {
            struct Task *t = lock->s_Owner;

            krnSpinWaitBegin(lock);
            krnSpinPause();

            if (failhook)
                CALLHOOKPKT(failhook, (APTR)lock, 0);

            /*
             * The owner may not be running. If its priority is below ours
             * it never will while we spin, so come down to its priority
             * until we have the lock.
             */
            if (me && t && (me->tc_Node.ln_Pri > t->tc_Node.ln_Pri))
            {
                BYTE pri = SetTaskPri(me, t->tc_Node.ln_Pri);

                /* The owner can change while we wait; keep our first priority */
                if (!priority_changed)
                {
                    old_pri = pri;
                    priority_changed = TRUE;
                }
            }
        }

        lock->s_Owner = me;
        krnSpinWaitEnd();

        if (priority_changed)
            SetTaskPri(me, old_pri);
    }
    else
    {
        while (!krnSpinTryRead(lock))
        {
            krnSpinWaitBegin(lock);
            krnSpinPause();

            if (failhook)
                CALLHOOKPKT(failhook, (APTR)lock, 0);
        }
        krnSpinWaitEnd();
    }
#endif

    return lock;

    AROS_LIBFUNC_EXIT
}
