/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:22  digulla
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

	__AROS_LH1(BYTE, WaitIO,

/*  SYNOPSIS */
	__AROS_LA(struct IORequest *, iORequest, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 79, Exec)

/*  FUNCTION
	Waits until the I/O request is completed and removes it from the
	reply port. If the message is already done when calling this function
	it doesn't wait but just remove the message.

    INPUTS
	iORequest - Pointer to iorequest structure.

    RESULT
	Error state of I/O request.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenDevice(), CloseDevice(), DoIO(), SendIO(), AbortIO(), CheckIO()

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /*
	The I/O request is still in use if it wasn't done quick
	and isn't yet replied (ln_Type==NT_MESSAGE).
	If it is still in use wait until it is complete.
	Note the the port may be used for other things as well - so
	don't just wait but repeat the check.
    */
    while(!(iORequest->io_Flags&IOF_QUICK)&&
	  iORequest->io_Message.mn_Node.ln_Type==NT_MESSAGE)
	/*
	    Wait at the reply port. Don't use WaitPort() - there may
	    already be other messages waiting at it.
	*/
	Wait(1<<iORequest->io_Message.mn_ReplyPort->mp_SigBit);

    /*
	If ln_Type is NT_REPLYMSG the I/O request must be removed from
	the replyport's waiting queue.
    */
    if(iORequest->io_Message.mn_Node.ln_Type==NT_REPLYMSG)
    {
	/* Arbitrate for the message queue. */
	Disable();

	/* Remove the message */
	Remove(&iORequest->io_Message.mn_Node);
	Enable();
    }

    /* All done. Get returncode. */
    return iORequest->io_Error;

    __AROS_FUNC_EXIT
} /* WaitIO */

