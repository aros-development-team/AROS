/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:07  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:40  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:46  aros
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
#include <dos/dos.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, CloseDevice,

/*  SYNOPSIS */
	AROS_LHA(struct IORequest *, iORequest, A1),

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
    AROS_LIBFUNC_INIT

    /* Single-thread the close routine. */
    Forbid();

    /* Something to do? */
    if(iORequest->io_Device!=NULL)
    {
	(void)AROS_LVO_CALL1(BPTR,
	    AROS_LCA(struct IORequest *,iORequest, A1),
	    struct Device *,iORequest->io_Device,2,
	);
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

    AROS_LIBFUNC_EXIT
} /* CloseDevice */

