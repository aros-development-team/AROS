/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:10  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:46  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:51  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:03  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:12  digulla
    Added standard header for all files

    Desc:
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

    /* Protect the message list. */
    Disable();

    /* Get first node. */
    msg=(struct Message *)RemHead(&port->mp_MsgList);

    /* All done. */
    Enable();
    return msg;
    AROS_LIBFUNC_EXIT
}

