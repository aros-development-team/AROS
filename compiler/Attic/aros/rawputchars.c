/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Formats a message and makes sure the user will see it.
    Lang: english
*/
#include <proto/exec.h>
#include "exec_private.h"

/*****************************************************************************

    NAME */
#include <proto/aros.h>

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

