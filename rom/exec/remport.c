/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Removes a port from the list of public ports.
    Lang: english
*/
#include <exec/ports.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the list of message ports.*/
    Forbid();

    /* Remove the current port. */
    Remove(&port->mp_Node);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemPort */



