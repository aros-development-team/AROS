/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: Removes a port from the list of public ports.
*/
#include <exec/ports.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, RemPort,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, port, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 60, Exec)

/*  FUNCTION
        Remove a public port from the public port list to make it private
        again. Any further attempts to find this port in the public port
        list will fail.

    INPUTS
        port - Pointer to messageport structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* AddPort and FindPort use PortListSpinLock, so removal must take
     * that same lock. */
    Forbid();
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->PortListSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif

    /* Remove the current port. */
    Remove(&port->mp_Node);

    /* All done. */
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->PortListSpinLock);
#endif
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemPort */


