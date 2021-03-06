/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Wait for a message on a port.
*/

#define DEBUG 0
#include <aros/debug.h>

#include "exec_intern.h"
#include <exec/ports.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH1(struct Message *, WaitPort,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, port, A0),

/*  LOCATION */
        struct ExecBase *, SysBase, 64, Exec)

/*  FUNCTION
        Wait until a message arrives at the given port. If there is already
        a message in it this function returns immediately.

    INPUTS
        port    - Pointer to messageport.

    RESULT
        Pointer to the first message that arrived at the port. The message
        is _not_ removed from the port. GetMsg() does this for you.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        WaitPort(), GetMsg()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(port);
    /*
        On uniprocessors systems, Disable() is not necessary  since emptiness
            can be checked without it - and nobody is allowed to change the signal bit as soon
        as the current task entered WaitPort() (and probably did not yet
        have a chance to Disable()).
    */
    D(bug("[Exec] WaitPort(0x%p)\n", port);)

    /* Is messageport empty? */
#if defined(__AROSEXEC_SMP__)
    Disable();
    EXEC_SPINLOCK_LOCK(&port->mp_SpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    while (IsListEmpty(&port->mp_MsgList))
    {
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&port->mp_SpinLock);
        Enable();
#endif
        D(bug("[Exec] WaitPort: Msg list empty, waiting for activity...\n");)

        /*
            Yes. Wait for the signal to arrive. Remember that signals may
            arrive without a message so check again.
        */
        Wait(1<<port->mp_SigBit);

        D(bug("[Exec] WaitPort: Msgport signal received ...\n");)
#if defined(__AROSEXEC_SMP__)
        Disable();
        EXEC_SPINLOCK_LOCK(&port->mp_SpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    }
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&port->mp_SpinLock);
    Enable();
#endif

    D(bug("[Exec] WaitPort: Returning...\n");)

    /* Return the first node in the list. */
    return (struct Message *)port->mp_MsgList.lh_Head;

    AROS_LIBFUNC_EXIT
} /* WaitPort() */

