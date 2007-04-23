/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Formats a message and makes sure the user will see it.
    Lang: english
*/

#include <aros/config.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	void RawPutChars (

/*  SYNOPSIS */
	const UBYTE * str,
	int	      len)

/*  FUNCTION
	Emits len bytes of fmt via RawPutChar()

    INPUTS
	str - string to print
	len - how many bytes of str to print

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    while (*str && len --)
	RawPutChar (*str ++);
} /* RawPutChars */

