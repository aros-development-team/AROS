/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function llabs().
*/

/*****************************************************************************

    NAME */
#include <stdlib.h>

	long long llabs (

/*  SYNOPSIS */
	long long j)

/*  FUNCTION
	Compute the absolute value of j.

    INPUTS
	j - A signed long long

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
	abs(), labs()

    INTERNALS

******************************************************************************/
{
    return (j < 0) ? -j : j;
} /* labs */

