/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:24  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(BYTE, DoIO,

/*  SYNOPSIS */
	__AROS_LA(struct IORequest *, iORequest, A1),

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

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /*
	Prepare the message. Tell the device that it is OK to wait in the
	BeginIO() call by setting the quick bit.
    */
    iORequest->io_Flags=IOF_QUICK;
    iORequest->io_Message.mn_Node.ln_Type=0;

    /* Call BeginIO() vector */
    __AROS_LVO_CALL1(void,5,iORequest->io_Device,iORequest,A1);

    /* It the quick flag is cleared it wasn't done quick. Wait for completion. */
    if(!(iORequest->io_Flags&IOF_QUICK))
	WaitIO(iORequest);

    /* All done. Get returncode. */
    return iORequest->io_Error;
    __AROS_FUNC_EXIT
} /* DoIO */

