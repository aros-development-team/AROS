/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AddEntropy() function.
*/

#include <exec/types.h>
#include <exec/semaphores.h>
#include <proto/exec.h>

#include "entropy_intern.h"

/*****************************************************************************

    NAME */
#include <proto/entropy.h>

        AROS_LH3(void, AddEntropy,

/*  SYNOPSIS */
        AROS_LHA(CONST_APTR, data,   A0),
        AROS_LHA(ULONG,      length, D0),
        AROS_LHA(ULONG,      bits,   D1),

/*  LOCATION */
        struct EntropyBase *, EntropyBase, 2, Entropy)

/*  FUNCTION
        Mix externally-gathered entropy into the resource's pool.

        Drivers and other subsystems that observe unpredictable events
        (interrupt timing jitter, mouse/keyboard timing, network packet
        arrival, etc.) can feed those samples here to improve the quality of
        the CSPRNG.  The data is folded into the pool state; it can only ever
        add to, never reduce, the unpredictability of future GetEntropy()
        output.

    INPUTS
        data   - pointer to the entropy samples to mix in.
        length - number of bytes at data.
        bits   - the caller's (conservative) estimate of how many bits of
                 true entropy the samples contain.  Used only to update the
                 informational counter reported by GetEntropyInfo(); it does
                 not affect how the data is mixed.

    RESULT
        None.

    NOTES
        Safe to call from multiple tasks.  Because it obtains a semaphore it
        must not be called from interrupt context; an interrupt handler should
        buffer its samples and hand them over from a task.

    EXAMPLE

    BUGS

    SEE ALSO
        GetEntropy(), GetEntropyInfo()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (data == NULL || length == 0)
        return;

    ObtainSemaphore(&EntropyBase->eb_Lock);

    Entropy_CRNG_Reseed(EntropyBase, (const UBYTE *)data, length);

    /* Saturating add of the (informational) entropy estimate. */
    if (bits)
    {
        if (EntropyBase->eb_Bits >= ENTROPY_BITS_MAX - bits)
            EntropyBase->eb_Bits = ENTROPY_BITS_MAX;
        else
            EntropyBase->eb_Bits += bits;
    }

    ReleaseSemaphore(&EntropyBase->eb_Lock);

    AROS_LIBFUNC_EXIT
} /* AddEntropy */
