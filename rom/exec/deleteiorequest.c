/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:24  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/io.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, DeleteIORequest,

/*  SYNOPSIS */
	__AROS_LA(struct IORequest *, iorequest, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 110, Exec)

/*  FUNCTION
	Delete an I/O request created with CreateIORequest().

    INPUTS
	iorequest - Pointer to I/O request structure or NULL.

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

    if(iorequest!=NULL)
	/* Just free the memory */
	FreeMem(iorequest,iorequest->io_Message.mn_Length);
    __AROS_FUNC_EXIT
}

