/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a standard IORequest structure
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
#include <exec/io.h>
#include <clib/alib_protos.h>

	struct IOStdReq * CreateStdIO (

/*  SYNOPSIS */
	struct MsgPort * port)

/*  FUNCTION
	Create a standard IORequest structure. The structire
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

