/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:03  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:12  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/ports.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(struct Message *, GetMsg,

/*  SYNOPSIS */
	__AROS_LHA(struct MsgPort *, port, A0),

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
    __AROS_FUNC_INIT

    struct Message *msg;

    /* Protect the message list. */
    Disable();

    /* Get first node. */
    msg=(struct Message *)RemHead(&port->mp_MsgList);

    /* All done. */
    Enable();
    return msg;
    __AROS_FUNC_EXIT
}

