/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:01  digulla
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

	__AROS_LH1I(LONG, AbortIO,

/*  SYNOPSIS */
	__AROS_LA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 80, Exec)

/*  FUNCTION
	Calls the AbortIO vector of the appropriate device to stop an
	asyncronously started io request before completion. This may
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
    __AROS_FUNC_INIT

    return __AROS_LVO_CALL1(ULONG,6,iORequest->io_Device,iORequest,A1);

    __AROS_FUNC_EXIT
} /* AbortIO */

