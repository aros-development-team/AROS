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
#include "exec_intern.h"
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <exec/io.h>
#include <clib/exec_protos.h>

	AROS_LH1(void, DeleteIORequest,

/*  SYNOPSIS */
	AROS_LHA(struct IORequest *, iorequest, A0),

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
    AROS_LIBFUNC_INIT

    if(iorequest!=NULL)
	/* Just free the memory */
	FreeMem(iorequest,iorequest->io_Message.mn_Length);
    AROS_LIBFUNC_EXIT
}

