/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Abort an I/O request.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, AbortIO,

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
    ASSERT_VALID_PTR(iORequest);
    ASSERT_VALID_PTR(iORequest->io_Device);

    return AROS_LVO_CALL1(ULONG,
	AROS_LCA(struct IORequest *,iORequest,A1),
	struct Device *,iORequest->io_Device,6,
    );

    AROS_LIBFUNC_EXIT
} /* AbortIO */

