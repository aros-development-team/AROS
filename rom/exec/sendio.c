/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:19  digulla
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

	__AROS_LH1(void, SendIO,

/*  SYNOPSIS */
	__AROS_LA(struct IORequest *, iORequest, A1),

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

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* Prepare the message. Don't set quick bit. */
    iORequest->io_Flags=0;
    iORequest->io_Message.mn_Node.ln_Type=0;

    /* Call BeginIO() vector */
    __AROS_LVO_CALL1(void,5,iORequest->io_Device,iORequest,A1);

    __AROS_FUNC_EXIT
} /* SendIO */

