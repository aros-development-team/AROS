/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:56  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:59  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:10  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:22  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include <exec/ports.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <clib/exec_protos.h>

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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

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
    AROS_LIBFUNC_EXIT
}

