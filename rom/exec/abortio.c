/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:03  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:34  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:40  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:55  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:01  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1I(LONG, AbortIO,

/*  SYNOPSIS */
	AROS_LHA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 80, Exec)

/*  FUNCTION
	Calls the AbortIO vector of the appropriate device to stop an
	asynchronously started io request before completion. This may
	or may not be done. You still have to do a WaitIO() on the
	iorequest structure.

    INPUTS
	iORequest - Pointer to iorequest structure.

    RESULT
	Errorcode if the abort request failed, 0 if the abort request went
	well. io_Error will then be set to IOERR_ABORTED.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice(), CloseDevice(), DoIO(), SendIO(), WaitIO()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AROS_LVO_CALL1(ULONG,
	AROS_LCA(struct IORequest *,iORequest,A1),
	struct Device *,iORequest->io_Device,6,
    );

    AROS_LIBFUNC_EXIT
} /* AbortIO */

