/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ISO C function lldiv
*/

#if defined(__GNUC__) || \
    defined(__ICC) || \
    defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/*****************************************************************************

    NAME */
#include <stdlib.h>

	lldiv_t lldiv (

/*  SYNOPSIS */
	long long int numer,
	long long int denom)

/*  FUNCTION
	Compute quotient en remainder of two long long variables

    INPUTS
        numer = the numerator
        denom = the denominator

    RESULT
        a struct with two long ints quot and rem with
        quot = numer / denom and rem = numer % denom.

        typedef struct lldiv_t {
            long long int quot;
            long long int rem;
        } lldiv_t;

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	div(), ldiv()

    INTERNALS

******************************************************************************/
#if 0
    /* Currently disabled because of linking issues */
{
    lldiv_t ret;

    ret.quot = numer / denom;
    ret.rem  = numer % denom;

    /* See div.c for why this is done */
    if (numer >= 0 && ret.rem < 0)
    {
	ret.quot++;
	ret.rem -= denom;
    }

    return ret;
}
#else
{
    abort();
}
#endif /* 0 */
#endif /* compiler supports long long */

