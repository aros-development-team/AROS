/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:39  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:45  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:59  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

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

