/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Start an IO request and wait until it completes.
    Lang: English
*/
#include <exec/execbase.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	BYTE DosDoIO(

/*  SYNOPSIS */
	struct IORequest * iORequest,

/*  LOCATION */
	struct ExecBase * SysBase)

/*  FUNCTION
	It's like the exec's DoIO, but this one handles CTRL-C.

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
#if 1
    return DoIO(iORequest);
#else    
    /*
	Prepare the message. Tell the device that it is OK to wait in the
	BeginIO() call by setting the quick bit.
    */
    iORequest->io_Flags=IOF_QUICK;
    iORequest->io_Message.mn_Node.ln_Type=0;

    /* Call BeginIO() vector */
    AROS_LVO_CALL1NR(
	AROS_LCA(struct IORequest *,iORequest,A1),
	struct Device *,iORequest->io_Device,5,
    );

    /* If the quick flag is cleared it wasn't done quick. Wait for completion. */
    if(!(iORequest->io_Flags&IOF_QUICK))
    {
	ULONG iosigf = 1<<iORequest->io_Message.mn_ReplyPort->mp_SigBit;
	ULONG sigs = 0;


loop:
	while
	(
	    !(sigs & SIGBREAKF_CTRL_C)  &&

	    !(iORequest->io_Flags&IOF_QUICK) &&
	    iORequest->io_Message.mn_Node.ln_Type==NT_MESSAGE
	)
	{
	    sigs = Wait(iosigf | SIGBREAKF_CTRL_C);
	}

	if(iORequest->io_Message.mn_Node.ln_Type==NT_REPLYMSG)
        {
	    /* Arbitrate for the message queue. */
	    Disable();

	    /* Remove the message */
	    Remove(&iORequest->io_Message.mn_Node);
  	    Enable();
	}
	else
	if (sigs & SIGBREAKF_CTRL_C)
	{
	    AbortIO(iORequest);
	    sigs = 0;

	    goto loop;
	}
    }

    /* All done. Get returncode. */
    return iORequest->io_Error;

#endif
} /* DosDoIO */

