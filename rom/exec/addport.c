/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a port to the public list of ports.
    Lang: english
*/

#include <exec/ports.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_debug.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, AddPort,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 59, Exec)

/*  FUNCTION
	Add a port to the public port list. The ln_Name and ln_Pri fields
	must be initialized prior to calling this function, while
	the port itself is reinitialized before adding. Therefore it's
	not allowed to add an active port.

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
    ASSERT_VALID_PTR(port);

    /* Arbitrate for the list of messageports. */
    Forbid();

    /* Yes, this is a messageport */
    port->mp_Node.ln_Type=NT_MSGPORT;

    /* Clear the list of messages */
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_INIT(&port->mp_SpinLock);
#endif
    NEWLIST(&port->mp_MsgList);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->PortListSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    /* And add the actual port */
    Enqueue(&SysBase->PortList,&port->mp_Node);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->PortListSpinLock);
#endif

    /* All done */
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddPort */
