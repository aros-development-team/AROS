/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

	struct IOStdReq * CreateStdIO (

/*  SYNOPSIS */
	struct MsgPort * port)

/*  FUNCTION
	Create a standard IORequest structure. The structure
	can be freed with DeleteStdIO().

    INPUTS
	port - The port to be signaled on events.

    RESULT
	A pointer to the new IORequest structure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreateExtIO(), DeleteStdIO()

    INTERNALS

    HISTORY

******************************************************************************/
{
    return (struct IOStdReq *)CreateExtIO (port, sizeof (struct IOStdReq));
} /* CreateStdIO */

