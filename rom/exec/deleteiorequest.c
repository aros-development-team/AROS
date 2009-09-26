/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free an I/O request.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <exec/io.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, DeleteIORequest,

/*  SYNOPSIS */
	AROS_LHA(APTR, iorequest, A0),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(iorequest != NULL)
	/* Just free the memory */
	FreeMem(iorequest, ((struct Message *)iorequest)->mn_Length);
    AROS_LIBFUNC_EXIT
} /* DeleteIORequest() */
