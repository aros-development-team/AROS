/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get a message from a message port.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/execbase.h>
#include <exec/ports.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(struct Message *, GetMsg,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 62, Exec)

/*  FUNCTION
	Get a message from a given messageport. This function doesn't wait
	and returns NULL if the messageport is empty. Therefore it's
	generally a good idea to WaitPort() or Wait() on the given port first.

    INPUTS
	port - Pointer to messageport

    RESULT
	Pointer to message removed from the port.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	WaitPort(), PutMsg()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Message *msg;

    D(bug("[Exec] GetMsg(0x%p)\n", port);)

    ASSERT_VALID_PTR(port);

    /*
     * Protect the message list, and get the first node.
     */
    Disable();
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&port->mp_SpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    msg=(struct Message *)RemHead(&port->mp_MsgList);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&port->mp_SpinLock);
#endif
    Enable();

    /* All done. */
    ASSERT_VALID_PTR_OR_NULL(msg);

    return msg;
    AROS_LIBFUNC_EXIT
} /* GetMsg() */

