/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:08  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <exec/ports.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, DeleteMsgPort,

/*  SYNOPSIS */
	__AROS_LA(struct MsgPort *, port, A0),

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
    __AROS_FUNC_INIT

    /* Only if there is something to free */
    if(port!=NULL)
    {
	/* Free signal bit */
	FreeSignal(port->mp_SigBit);

	/* And memory */
	FreeMem(port,sizeof(struct MsgPort));
    }
    __AROS_FUNC_EXIT
} /* DeleteMsgPort */
