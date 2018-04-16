/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$

    C99 function imaxabs().
*/

/*****************************************************************************

    NAME */
#include <inttypes.h>

	intmax_t imaxabs (

/*  SYNOPSIS */
	intmax_t j)

/*  FUNCTION
	Compute the absolute value of an integer "j".

    INPUTS

    RESULT
	Return the absolute value.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    return j>0 ? j : -j;
}
