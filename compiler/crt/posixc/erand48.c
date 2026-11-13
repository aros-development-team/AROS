/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

#include "rand48.h"

/****************************************************************************
    NAME
        double erand48(

    SYNOPSIS
        unsigned short xseed[3])

    FUNCTION
        Advance a caller-supplied 48-bit pseudo-random state and
        return a fraction derived from the new state.

    INPUTS
        xseed - Writable array of three initialized unsigned short
                values, from least to most significant 16-bit word.
                The array must remain valid for the call; NULL is
                not accepted. All three words are updated in place.

    RESULT
        The updated state divided by 2^48, in the range [0.0, 1.0).
        See BUGS for an arithmetic defect that invalidates this
        result contract.

    NOTES
        Include <stdlib.h>. The default recurrence is
        X = (0x5deece66d * X + 11) modulo 2^48. lcong48()
        changes the multiplier and increment used by this function;
        srand48() and seed48() restore their defaults.

        This generator is not suitable for cryptographic use.

        The function retains no pointer to xseed. Reuse the updated
        array to continue a sequence. A separate array keeps its
        state independent of drand48(), but does not isolate it
        from changes to the shared multiplier and increment.
        Synchronize access to a shared array and exclude concurrent
        changes to those parameters.

    BUGS
        The shared generator can overflow signed int arithmetic
        on targets with 32-bit int, including on the first call
        from the default state. Its sequence and return value
        are not reliable under standard C overflow semantics.

    SEE ALSO
        drand48(), nrand48(), jrand48(), srand48(), seed48(),
        lcong48()

*****************************************************************************/
double erand48(unsigned short xseed[3])
{
	_dorand48(xseed);
	return ldexp((double) xseed[0], -48) +
	       ldexp((double) xseed[1], -32) +
	       ldexp((double) xseed[2], -16);
}
