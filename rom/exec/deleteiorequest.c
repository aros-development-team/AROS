/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:00  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:09  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/io.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(void, DeleteIORequest,

/*  SYNOPSIS */
	__AROS_LHA(struct IORequest *, iorequest, A0),

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

