/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Wait for a message on a port.
    Lang: english
*/
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
	port	- Pointer to messageport.

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
	No Disable() necessary here since emptiness can be checked
	without and nobody is allowed to change the signal bit as soon
	as the current task entered WaitPort() (and probably did not yet
	have a chance to Disable()).
    */

    /* Is messageport empty? */
    while (IsListEmpty (&port->mp_MsgList))
    {
	/*
	    Yes. Wait for the signal to arrive. Remember that signals may
	    arrive without a message so check again.
	*/
	Wait(1<<port->mp_SigBit);
    }

    /* Return the first node in the list. */
    return (struct Message *)port->mp_MsgList.lh_Head;

    AROS_LIBFUNC_EXIT
} /* WaitPort() */

