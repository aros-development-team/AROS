/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get a message from a message port.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/ports.h>
#include <aros/libcall.h>
#include <proto/exec.h>

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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Message *msg;

    ASSERT_VALID_PTR(port);

    /* Protect the message list. */
    Disable();

    /* Get first node. */
    msg=(struct Message *)RemHead(&port->mp_MsgList);

    /* All done. */
    Enable();

    ASSERT_VALID_PTR_OR_NULL(msg);
    return msg;
    AROS_LIBFUNC_EXIT
}

