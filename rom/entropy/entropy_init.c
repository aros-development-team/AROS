/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: entropy.resource initialisation.
*/

#include <exec/types.h>
#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "entropy_intern.h"

#include LC_LIBDEFS_FILE

static int Entropy_Init(struct EntropyBase *EntropyBase)
{
    UBYTE seed[128];
    ULONG n;

    InitSemaphore(&EntropyBase->eb_Lock);

    EntropyBase->eb_Counter  = 0;
    EntropyBase->eb_Sequence = 0;
    EntropyBase->eb_Bits     = 0;
    EntropyBase->eb_HWGather = NULL;
    EntropyBase->eb_HWData   = NULL;
    EntropyBase->eb_Flags    = EIF_SOFTWARE;

    /*
     * Give the CSPRNG a starting personality from the software collector.  The
     * first few bytes also seed the nonce so that two boots (whose sampled
     * pointers/counters differ) start from different (key, nonce) points even
     * before any hardware entropy is mixed in.
     */
    n = Entropy_GatherSoftware(EntropyBase, seed, sizeof(seed));
    if (n >= ENTROPY_NONCESZ)
        CopyMem(seed, EntropyBase->eb_Nonce, ENTROPY_NONCESZ);
    Entropy_CRNG_Reseed(EntropyBase, seed, n);

    /*
     * Let the architecture back-end detect and install a hardware source
     * (e.g. x86 RDRAND/RDSEED).  If present, fold a generous hardware sample
     * into the state right away so the very first GetEntropy() is already
     * strongly seeded.
     */
    Entropy_HW_Init(EntropyBase);

    if (EntropyBase->eb_HWGather)
    {
        UBYTE hw[64];
        ULONG hn = EntropyBase->eb_HWGather(EntropyBase, hw, sizeof(hw));

        if (hn)
        {
            Entropy_CRNG_Reseed(EntropyBase, hw, hn);
            EntropyBase->eb_Bits = ENTROPY_BITS_MAX;    /* fully seeded */
        }
        Entropy_Wipe(hw, sizeof(hw));
    }

    Entropy_Wipe(seed, sizeof(seed));

    return TRUE;
}

ADD2INITLIB(Entropy_Init, 0)
