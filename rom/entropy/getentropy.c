/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GetEntropy() function.
*/

#include <exec/types.h>
#include <exec/semaphores.h>
#include <proto/exec.h>

#include "entropy_intern.h"

/*****************************************************************************

    NAME */
#include <proto/entropy.h>

        AROS_LH2(LONG, GetEntropy,

/*  SYNOPSIS */
        AROS_LHA(APTR,  buffer, A0),
        AROS_LHA(ULONG, length, D0),

/*  LOCATION */
        struct EntropyBase *, EntropyBase, 1, Entropy)

/*  FUNCTION
        Fill a buffer with cryptographically-suitable random bytes.

        The bytes are produced by the resource's ChaCha20 CSPRNG, which is
        reseeded on every call from the generic software collector and, on
        platforms that provide one, a CPU/board hardware entropy source (for
        example the x86 RDRAND/RDSEED instructions).  Hardware output is only
        ever mixed into the CSPRNG, never returned verbatim, so a weak or
        compromised hardware source cannot weaken the result.

    INPUTS
        buffer - where to store the random data.
        length - the number of bytes wanted.

    RESULT
        The number of bytes written to buffer (always equal to length on
        success), or -1 if buffer is NULL.

    NOTES
        This function is safe to call from multiple tasks; access to the pool
        is serialised internally.  It must not be called from interrupts (it
        obtains a semaphore).

    EXAMPLE
        UBYTE key[32];
        struct Library *EntropyBase = OpenResource("entropy.resource");
        if (EntropyBase)
            GetEntropy(key, sizeof(key));

    BUGS

    SEE ALSO
        AddEntropy(), GetEntropyInfo()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE seed[128];
    ULONG n;

    if (buffer == NULL)
        return -1;
    if (length == 0)
        return 0;

    ObtainSemaphore(&EntropyBase->eb_Lock);

    /* Freshen the pool from the always-available software collector... */
    n = Entropy_GatherSoftware(EntropyBase, seed, sizeof(seed));
    Entropy_CRNG_Reseed(EntropyBase, seed, n);

    /* ...and from the hardware source, when one is installed. */
    if (EntropyBase->eb_HWGather)
    {
        UBYTE hw[64];
        ULONG hn = EntropyBase->eb_HWGather(EntropyBase, hw, sizeof(hw));

        if (hn)
        {
            Entropy_CRNG_Reseed(EntropyBase, hw, hn);
            EntropyBase->eb_Bits = ENTROPY_BITS_MAX;
        }
        Entropy_Wipe(hw, sizeof(hw));
    }

    Entropy_CRNG_Generate(EntropyBase, (UBYTE *)buffer, length);

    Entropy_Wipe(seed, sizeof(seed));

    ReleaseSemaphore(&EntropyBase->eb_Lock);

    return (LONG)length;

    AROS_LIBFUNC_EXIT
} /* GetEntropy */
