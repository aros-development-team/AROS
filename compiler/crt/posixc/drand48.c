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

extern unsigned short _rand48_seed[3];

/****************************************************************************
    NAME
        double drand48(

    SYNOPSIS
        void)

    FUNCTION
        Advance the shared 48-bit pseudo-random state and return
        a fraction derived from the new state.

    INPUTS
        None.

    RESULT
        The new state divided by 2^48, in the range [0.0, 1.0).
        See BUGS for an arithmetic defect that invalidates this
        result contract.

    NOTES
        Include <stdlib.h>. The default recurrence is
        X = (0x5deece66d * X + 11) modulo 2^48. lcong48()
        changes the multiplier and increment used by this function;
        srand48() and seed48() restore their defaults.

        This generator is not suitable for cryptographic use.

        The initial state is 0x1234abcd330e. srand48(), seed48()
        and lcong48() replace this state; lrand48() and mrand48()
        advance it. Calls using this shared state or changing
        the generator parameters require external synchronization.

    BUGS
        The shared generator can overflow signed int arithmetic
        on targets with 32-bit int, including on the first call
        from the default state. Its sequence and return value
        are not reliable under standard C overflow semantics.

    SEE ALSO
        erand48(), srand48(), seed48(), lcong48(), lrand48(),
        mrand48()

*****************************************************************************/
double drand48(void)
{
	return erand48(_rand48_seed);
}
