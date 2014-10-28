/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: amiga.lib function DeleteStdIO()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <exec/io.h>
#include <proto/alib.h>

	void DeleteStdIO(

/*  SYNOPSIS */
	struct IOStdReq * io)

/*  FUNCTION
	Delete a structure which was created by CreateStdIO().

    INPUTS
	io - The value returned by CreateStdIO(). May be NULL.

    RESULT
	None.

    NOTES
	Invalidates values of certain fields, to assure the developer
	doesn't	depend on them remaining valid after the structure is freed.

    EXAMPLE

    BUGS

    SEE ALSO
	CreateStdIO(), CreateExtIO(), DeleteExtIO()

    INTERNALS

******************************************************************************/
{
    DeleteExtIO((struct IORequest *)io);
}
