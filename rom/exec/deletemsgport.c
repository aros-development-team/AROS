/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:43  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:47  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:00  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:09  digulla
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
