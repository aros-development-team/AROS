/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:55:56  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:02  digulla
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

	__AROS_LH1(void, AddPort,

/*  SYNOPSIS */
	__AROS_LHA(struct MsgPort *, port, A1),

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
    __AROS_FUNC_INIT

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
    __AROS_FUNC_EXIT
} /* AddPort */

