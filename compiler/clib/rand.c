/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C functions rand() and srand()
    Lang: english
*/

static unsigned int a = 1;

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int rand (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	A random number generator.

    INPUTS
	None.

    RESULT
	A pseudo-random integer between 0 and RAND_MAX.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return (a = a * 1103515245 + 12345) % RAND_MAX;
} /* rand */


/*****************************************************************************

    NAME */
	#include <stdlib.h>

	void srand (

/*  SYNOPSIS */
	unsigned int seed)

/*  FUNCTION
	Set the starting value for the random number generator rand()

    INPUTS
	seed - New start value

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
    a = seed;
} /* srand */


