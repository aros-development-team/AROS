/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:44  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:47  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:01  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:09  digulla
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

    HISTORY

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
    AROS_LVO_CALL1(void,
	AROS_LCA(struct IORequest *,iORequest,A1),
	struct Device *,iORequest->io_Device,5,
    );

    /* It the quick flag is cleared it wasn't done quick. Wait for completion. */
    if(!(iORequest->io_Flags&IOF_QUICK))
	WaitIO(iORequest);

    /* All done. Get returncode. */
    return iORequest->io_Error;
    AROS_LIBFUNC_EXIT
} /* DoIO */

