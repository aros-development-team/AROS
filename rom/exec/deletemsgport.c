/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free a message port.
    Lang: english
*/
#include <exec/ports.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, DeleteMsgPort,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 112, Exec)

/*  FUNCTION
	Delete a messageport allocated with CreateMsgPort(). The signal bit
	is freed and the memory is given back to the memory pool. Remaining
	messages are not replied. It is safe to call this function with a
	NULL pointer.

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

    /* Only if there is something to free */
    if(port!=NULL)
    {
	/* Free signal bit */
	FreeSignal(port->mp_SigBit);

	/* And memory */
	FreeMem(port,sizeof(struct MsgPort));
    }
    AROS_LIBFUNC_EXIT
} /* DeleteMsgPort */
