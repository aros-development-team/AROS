/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:07  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1I(struct IORequest *, CheckIO,

/*  SYNOPSIS */
	__AROS_LA(struct IORequest *, iORequest, A1),

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
    __AROS_FUNC_INIT

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

    __AROS_FUNC_EXIT
} /* CheckIO */

