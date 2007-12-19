/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start an asynchronous I/O request.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, SendIO,

/*  SYNOPSIS */
	AROS_LHA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 77, Exec)

/*  FUNCTION
	Start an asynchronous I/O request by calling the device's BeginIO()
	vector. After sending the messages asynchronously you can wait for
	the message to be replied at the I/O reply port.

    INPUTS
	iORequest - Pointer to iorequest structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice(), CloseDevice(), DoIO(), CheckIO(), AbortIO(), WaitIO()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Prepare the message. Don't set quick bit. */
    iORequest->io_Flags=0;
    iORequest->io_Message.mn_Node.ln_Type=0;

    /* Call BeginIO() vector */
    AROS_LVO_CALL1NR(void,
	AROS_LCA(struct IORequest *,iORequest,A1),
	struct Device *,iORequest->io_Device,5,
    );

    AROS_LIBFUNC_EXIT
} /* SendIO */

