/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a standard IORequest structure
    Lang: english
*/

#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/io.h>
#include <proto/alib.h>

	struct IOStdReq * CreateStdIO(

/*  SYNOPSIS */
	struct MsgPort * port)

/*  FUNCTION
	Create a standard IORequest structure. The structure must be freed
	with DeleteStdIO().

    INPUTS
	port - The port to be signaled on events. May be NULL, in which case
	    no IORequest is allocated.

    RESULT
	A pointer to the new IORequest structure, or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreateExtIO(), DeleteExtIO(), DeleteStdIO()

    INTERNALS

******************************************************************************/
{
    return (struct IOStdReq *)CreateExtIO(port, sizeof(struct IOStdReq));
}
