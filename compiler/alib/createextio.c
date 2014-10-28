/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/io.h>
#include <proto/alib.h>

	struct IORequest * CreateExtIO(

/*  SYNOPSIS */
	struct MsgPort * port,
	ULONG		 iosize)

/*  FUNCTION
	Create an extended IORequest structure. This structure must be freed
	with DeleteExtIO().

    INPUTS
	port - MsgPort to be signaled on events. May be NULL, in which case
	    no IORequest is allocated.
	iosize - Size of the structure

    RESULT
	A pointer to the new IORequest structure, or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreateStdIO(), DeleteExtIO(), DeleteStdIO()

    INTERNALS

******************************************************************************/
{
    struct IORequest *ioreq=NULL;

    if (port && (ioreq = AllocMem(iosize, MEMF_CLEAR|MEMF_PUBLIC)))
    {
	/* Initialize the structure */
	ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
	ioreq->io_Message.mn_ReplyPort	  = port;
	ioreq->io_Message.mn_Length	  = iosize;
    }

    return ioreq;
}
