/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Add a port to the public list of ports.
    Lang: english
*/
#include <exec/ports.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Yes, this is a messageport */
    port->mp_Node.ln_Type=NT_MSGPORT;

    /* Clear the list of messages */
    port->mp_MsgList.lh_Head=(struct Node *)&port->mp_MsgList.lh_Tail;
    port->mp_MsgList.lh_Tail=NULL;
    port->mp_MsgList.lh_TailPred=(struct Node *)&port->mp_MsgList.lh_Head;

    /* Arbitrate for the list of messageports. */
    Forbid();

    /* And add the actual port */
    Enqueue(&SysBase->PortList,&port->mp_Node);

    /* All done */
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddPort */

