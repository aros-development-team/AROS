/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BSD extension arc4random_uniform().
*/

#include <stdint.h>

#include "__stdc_intbase.h"
#include "__arc4random.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        uint32_t arc4random_uniform (

/*  SYNOPSIS */
        uint32_t upper_bound)

/*  FUNCTION
        Return a cryptographically-strong random value, uniformly distributed
        and free of modulo bias, in the range [0, upper_bound).

    INPUTS
        upper_bound - the exclusive upper bound.

    RESULT
        A uniformly-distributed random value in [0, upper_bound).  If
        upper_bound is 0 or 1 the result is 0.

    NOTES
        Rejection sampling is used to avoid the bias a plain
        arc4random() % upper_bound would introduce when upper_bound does not
        evenly divide 2^32.

    EXAMPLE
        // an unbiased dice roll in 1..6
        int roll = 1 + arc4random_uniform(6);

    BUGS

    SEE ALSO
        arc4random(), arc4random_buf()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    uint32_t r, min;

    if (upper_bound < 2)
        return 0;

    /* min = 2^32 mod upper_bound; reject values below it so the remaining
       range is an exact multiple of upper_bound. */
    min = (uint32_t)(-upper_bound) % upper_bound;

    do {
        __arc4random_buf(StdCBase, &r, sizeof(r));
    } while (r < min);

    return r % upper_bound;
} /* arc4random_uniform */
