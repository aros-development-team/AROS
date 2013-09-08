/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    ISO C function div().
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

    /*
	This comment is from FreeBSD's src/lib/libc/stdlib/div.c, but
	the code was written before the comment was added (see CVS log).

	Thus the code isn't really under the BSD license. The comment is...
     */

    /*
     * The ANSI standard says that |r.quot| <= |n/d|, where
     * n/d is to be computed in infinite precision.  In other
     * words, we should always truncate the quotient towards
     * 0, never -infinity.
     *
     * Machine division and remainer may work either way when
     * one or both of n or d is negative.  If only one is
     * negative and r.quot has been truncated towards -inf,
     * r.rem will have the same sign as denom and the opposite
     * sign of num; if both are negative and r.quot has been
     * truncated towards -inf, r.rem will be positive (will
     * have the opposite sign of num).  These are considered
     * `wrong'.
     *
     * If both are num and denom are positive, r will always
     * be positive.
     *
     * This all boils down to:
     *      if num >= 0, but r.rem < 0, we got the wrong answer.
     * In that case, to get the right answer, add 1 to r.quot and
     * subtract denom from r.rem.
     */
    if (numer >= 0 && ret.rem < 0)
    {
	ret.quot++;
	ret.rem -= denom;
    }
    return ret;
}
