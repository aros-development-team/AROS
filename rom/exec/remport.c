/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:56  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:07  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:17  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/ports.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

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



