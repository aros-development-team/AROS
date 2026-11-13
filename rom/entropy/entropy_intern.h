/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal data structures for entropy.resource.
*/

#ifndef ENTROPY_INTERN_H
#define ENTROPY_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>

#include <resources/entropy.h>

/* Size of the ChaCha20 CSPRNG state that backs the resource. */
#define ENTROPY_KEYSZ   32      /* 256-bit key                              */
#define ENTROPY_NONCESZ 12      /* 96-bit nonce                             */

/* Upper bound we track for the (purely informational) entropy estimate. */
#define ENTROPY_BITS_MAX 4096

struct EntropyBase;

/*
 * A hardware entropy back-end.  Architecture-specific code installs one of
 * these through Entropy_HW_Init(); it must place up to length bytes of raw
 * hardware entropy into buffer and return the number of bytes actually
 * produced (0 if the hardware failed to deliver this time).  The generic
 * resource treats whatever it returns as an additional seed - it is always
 * run through the CSPRNG, never handed to the caller directly - so a weak or
 * even hostile source can only ever help, never weaken, the output.
 */
typedef ULONG (*entropy_hwfn)(struct EntropyBase *EntropyBase,
                              UBYTE *buffer, ULONG length);

struct EntropyBase
{
    struct Library          eb_LibNode;
    struct SignalSemaphore  eb_Lock;        /* serialises pool access       */

    /* ChaCha20 "fast key erasure" CSPRNG state. */
    UBYTE                   eb_Key[ENTROPY_KEYSZ];
    UBYTE                   eb_Nonce[ENTROPY_NONCESZ];
    ULONG                   eb_Counter;     /* ChaCha block counter         */

    ULONG                   eb_Sequence;    /* monotonic per-request tick   */
    ULONG                   eb_Bits;        /* informational entropy tally  */

    /* Hardware back-end, installed by Entropy_HW_Init() (NULL = software). */
    entropy_hwfn            eb_HWGather;
    APTR                    eb_HWData;      /* private to the back-end      */
    ULONG                   eb_Flags;       /* EIF_* source flags           */
};

/* --- CSPRNG core (entropy_chacha.c) ------------------------------------- */

/* Fold len bytes of seed material into the CSPRNG key (never reduces the
   existing entropy) and run one diffusion/rekey step. */
void Entropy_CRNG_Reseed(struct EntropyBase *EntropyBase,
                         const UBYTE *seed, ULONG len);

/* Produce len bytes of CSPRNG output into out, advancing (and erasing) the
   key as it goes so past output cannot be recomputed from the new state. */
void Entropy_CRNG_Generate(struct EntropyBase *EntropyBase,
                           UBYTE *out, ULONG len);

/* --- Generic software entropy source (entropy_software.c) --------------- */

/* Sample cheap, always-available system state (scheduler counters, pointers,
   free-memory figures, timing jitter) into buffer, returning the number of
   bytes written (<= length). */
ULONG Entropy_GatherSoftware(struct EntropyBase *EntropyBase,
                             UBYTE *buffer, ULONG length);

/* --- Hardware back-end hook (entropy_hw.c, overridable per arch) --------- */

/* Called once at resource init.  The generic implementation installs no
   hardware source; an architecture override detects CPU/board entropy
   hardware (e.g. x86 RDRAND/RDSEED) and, if present, sets eb_HWGather plus
   the matching EIF_* bits in eb_Flags. */
void Entropy_HW_Init(struct EntropyBase *EntropyBase);

/* --- small shared helper ------------------------------------------------ */

/* Zero memory such that the compiler may not optimise the wipe away (used to
   erase transient key/seed material). */
static inline void Entropy_Wipe(void *p, ULONG len)
{
    volatile UBYTE *b = (volatile UBYTE *)p;
    while (len--)
        *b++ = 0;
}

#endif /* ENTROPY_INTERN_H */
