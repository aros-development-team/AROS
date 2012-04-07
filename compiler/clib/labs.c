/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function labs().
*/

/*****************************************************************************

    NAME */
#include <stdlib.h>

	long labs (

/*  SYNOPSIS */
	long j)

/*  FUNCTION
	Compute the absolute value of j.

    INPUTS
	j - A signed long

    RESULT
	The absolute value of j.

    NOTES

    EXAMPLE
	// returns 1
	labs (1L);

	// returns 1
	labs (-1L);

    BUGS

    SEE ALSO
	abs(), fabs()

    INTERNALS

******************************************************************************/
{
    return (j < 0) ? -j : j;
} /* labs */

