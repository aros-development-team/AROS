/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start an IO request and wait until it completes.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(BYTE, DoIO,

/*  SYNOPSIS */
	AROS_LHA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 76, Exec)

/*  FUNCTION
	Start an I/O request by calling the devices's BeginIO() vector.
	It waits until the request is complete.

    INPUTS
	iORequest - Pointer to iorequest structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice(), CloseDevice(), DoIO(), SendIO(), AbortIO(), WaitIO()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
	Prepare the message. Tell the device that it is OK to wait in the
	BeginIO() call by setting the quick bit.
    */
    iORequest->io_Flags=IOF_QUICK;
    iORequest->io_Message.mn_Node.ln_Type=0;

    /* Call BeginIO() vector */
    AROS_LVO_CALL1NR(void,
	AROS_LCA(struct IORequest *,iORequest,A1),
	struct Device *,iORequest->io_Device,5,
    );

    /* If the quick flag is cleared it wasn't done quickly. Wait for completion. */
    if(!(iORequest->io_Flags&IOF_QUICK))
	WaitIO(iORequest);

    /* All done. Get returncode. */
    return iORequest->io_Error;
    AROS_LIBFUNC_EXIT
} /* DoIO */

