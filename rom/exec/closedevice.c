/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:55:59  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:07  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, CloseDevice,

/*  SYNOPSIS */
	__AROS_LHA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 75, Exec)

/*  FUNCTION
	Closes a previously opened device. Any outstanding I/O requests must
	be finished. It is safe to call CloseDevice with a cleared iorequest
	structure or one that failed to open.

    INPUTS
	iORequest - Pointer to iorequest structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice().

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* Single-thread the close routine. */
    Forbid();

    /* Something to do? */
    if(iORequest->io_Device!=NULL)
    {
	(void)__AROS_LVO_CALL1(BPTR,2,iORequest->io_Device,iORequest,A1);
	/*
	    Normally you'd expect the device to be expunged if this returns
	    non-zero, but this is only exec which doesn't know anything about
	    seglists - therefore dos.library has to SetFunction() into this
	    vector for the additional functionality.
	*/

	/* Trash device field */
	iORequest->io_Device=(struct Device *)-1;
    }

    /* All done. */
    Permit();

    __AROS_FUNC_EXIT
} /* CloseDevice */

