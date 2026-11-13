/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BSD extension arc4random().
*/

#include <stdint.h>

#include "__stdc_intbase.h"
#include "__arc4random.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        uint32_t arc4random (

/*  SYNOPSIS */
        void)

/*  FUNCTION
        Return a single 32-bit cryptographically-strong random value.

        The value is obtained from entropy.resource (a ChaCha20 CSPRNG seeded
        from software and, where available, CPU hardware entropy).  If the
        resource is not present a self-contained fallback is used so that the
        function always returns a value.

    INPUTS
        None.

    RESULT
        A uniformly-distributed random value in the range [0, 2^32).

    NOTES
        Unlike rand()/random(), this generator is not seeded by the caller and
        has no reproducible sequence; it is suitable for keys, nonces, tokens
        and similar security-sensitive uses.

    EXAMPLE
        uint32_t token = arc4random();

    BUGS

    SEE ALSO
        arc4random_buf(), arc4random_uniform(), rand()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    uint32_t v;

    __arc4random_buf(StdCBase, &v, sizeof(v));

    return v;
} /* arc4random */
