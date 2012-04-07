/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function abs().
*/

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int abs (

/*  SYNOPSIS */
	int j)

/*  FUNCTION
	Compute the absolute value of j.

    INPUTS
	j - A signed integer

    RESULT
	The absolute value of j.

    NOTES

    EXAMPLE
	// returns 1
	abs (1);

	// returns 1
	abs (-1);

    BUGS

    SEE ALSO
	labs(), fabs()

    INTERNALS

******************************************************************************/
{
    return (j < 0) ? -j : j;
} /* abs */
