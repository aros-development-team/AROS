/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BSD function div
*/

/*****************************************************************************

    NAME */
#include <stdlib.h>

	div_t div (

/*  SYNOPSIS */
	int numer,
	int denom)

/*  FUNCTION
 	Compute quotient en remainder of two int variables

    INPUTS
        numer = the numerator
        denom = the denominator

    RESULT
        a struct with two ints quot and rem with
        quot = numer / denom and rem = numer % denom.

        typedef struct div_t {
            int quot;
            int rem;
        } div_t;
 
   NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ldiv()

    INTERNALS

******************************************************************************/
{
    div_t ret;

    ret.quot = numer / denom;
    ret.rem  = numer % denom;
  
    return ret;
}
