/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2001 function rand_r().
*/

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int rand_r(

/*  SYNOPSIS */
        unsigned int *seed)

/*  FUNCTION
        Returns a pseudo-random integer in the range 0 to RAND_MAX,
        using a caller-supplied seed pointer for reentrant behavior.

        The algorithm is a linear congruential generator (LCG) and
        matches the behavior of traditional ANSI C rand() but avoids
        using global state. This allows rand_r() to be safely used
        in multithreaded environments.

    INPUTS
        seed - Pointer to an unsigned int used as the state of the
               random number generator. The pointed-to value will be
               updated on each call.

    RESULT
        Returns a pseudo-random integer value between 0 and RAND_MAX
        (typically 32767). The value is deterministically derived from
        the current seed.

    NOTES
        - rand_r() is a POSIX extension and not part of ISO C.
        - It is reentrant and thread-safe, unlike rand().
        - Sequences produced by rand_r() are deterministic and repeatable
          for a given initial seed value.

    EXAMPLE
        unsigned int seed = 12345;
        int r = rand_r(&seed);  // safe to use in multithreaded code

    BUGS
        The quality of the random sequence is limited by the LCG algorithm.
        It is not suitable for cryptographic use.

    SEE ALSO
        rand(), srand(), drand48(), random(), arc4random()

    INTERNALS
        The implementation uses the formula:
            seed = seed * 1103515245 + 12345;
            return (seed >> 16) & 0x7FFF;

******************************************************************************/
{
    unsigned int next = *seed;

    // Linear congruential generator (same constants as POSIX libc)
    next = next * 1103515245 + 12345;

    *seed = next;

    // Return a 15-bit result (like standard rand())
    return (int)((next >> 16) & 0x7FFF);
}
