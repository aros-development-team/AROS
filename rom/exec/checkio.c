/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Check if an I/O request is completed.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1I(struct IORequest *, CheckIO,

/*  SYNOPSIS */
	AROS_LHA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 78, Exec)

/*  FUNCTION
	Check if an I/O request is completed.

    INPUTS
	iORequest - Pointer to iorequest structure.

    RESULT
	Pointer to the iorequest structure or NULL if the device is still
	at work.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice(), CloseDevice(), DoIO(), SendIO(), AbortIO(), WaitIO()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
	The I/O request is still in use if it wasn't done quick
	and isn't yet replied (ln_Type==NT_MESSAGE).
    */
    if(!(iORequest->io_Flags&IOF_QUICK)&&
       iORequest->io_Message.mn_Node.ln_Type==NT_MESSAGE)

	/* Still in use */
	return NULL;
    else
	/* done */
	return iORequest;

    AROS_LIBFUNC_EXIT
} /* CheckIO */

