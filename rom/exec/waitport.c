/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:22  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/ports.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(struct Message *, WaitPort,

/*  SYNOPSIS */
	__AROS_LA(struct MsgPort *, port, A0),

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

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /*
	No Disable() necessary here since emptiness can be checked
	without and nobody is allowed to change the signal bit as soon
	as the current task entered WaitPort() (and probably did not yet
	have a chance to Disable()).
    */

    /* Is messageport empty? */
    while(port->mp_MsgList.lh_Head->ln_Succ==NULL)
	/*
	    Yes. Wait for the signal to arrive. Remember that signals may
	    arrive without a message so check again.
	*/
	Wait(1<<port->mp_SigBit);

    /* Return the first node in the list. */
    return (struct Message *)port->mp_MsgList.lh_Head;
    __AROS_FUNC_EXIT
}

