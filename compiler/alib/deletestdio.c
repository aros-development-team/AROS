/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: amiga.lib function DeleteStdIo()
    Lang: english
*/
#include <exec/memory.h>

/*****************************************************************************

    NAME */
#include <exec/io.h>
#include <proto/alib.h>
#include <proto/exec.h>

	void DeleteStdIO (

/*  SYNOPSIS */
	struct IOStdReq * io)

/*  FUNCTION
	Delete a structure which was created by CreateStdIO().

    INPUTS
	io - The value returned by CreateStdIO(). Must be
	    non-NULL.

    RESULT
	None.

    NOTES
	Invalidates values of certain fields, to assure the developer
	doesn't	depend on them remaining valid after the structure is freed.

    EXAMPLE

    BUGS

    SEE ALSO
	CreateStdIO()

    INTERNALS

    HISTORY

******************************************************************************/
{
#   define ioreq    ((struct IORequest *)io)
    /* Write illegal values to some fields to enforce crashes */
    ioreq->io_Message.mn_Node.ln_Type = -1L;

    ioreq->io_Device = (struct Device *)-1L;
    ioreq->io_Unit   = (struct Unit *)-1L;

    FreeMem (ioreq, ioreq->io_Message.mn_Length);
} /* DeleteStdIO */

