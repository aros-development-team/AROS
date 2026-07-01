/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: x86 (i386/x86_64) hardware entropy back-end for entropy.resource.

    Uses the CPU's on-die entropy instructions when available:

      - RDSEED (preferred) draws directly from the hardware entropy source and
        is the better seed material.
      - RDRAND draws from the CPU's internal CSPRNG (reseeded from the same
        hardware source) and is used when RDSEED is not present.

    Both are detected through CPUID and confirmed with a short self-test (some
    hypervisors/microcode advertise the feature yet return "no data").  If no
    working instruction is found the resource falls back to the generic
    software collector, exactly as on a platform with no override at all.

    The bytes produced here are never handed to callers directly - the generic
    resource only ever folds them into its ChaCha20 CSPRNG - so even a biased
    or hostile instruction can only add to, never weaken, the output.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>

#include "entropy_intern.h"

#if defined(__x86_64__) || defined(__i386__)

#define CPUID_1_ECX_RDRAND  (1u << 30)  /* CPUID.01H:ECX.RDRAND[bit 30]      */
#define CPUID_7_EBX_RDSEED  (1u << 18)  /* CPUID.07H:EBX.RDSEED[bit 18]      */

#if defined(__x86_64__)
typedef unsigned long long xword;       /* RDRAND/RDSEED fill a 64-bit reg  */
#define XWORD_BYTES 8
#else
typedef unsigned int xword;             /* 32-bit on i386                    */
#define XWORD_BYTES 4
#endif

/* Which instructions this CPU offers.  These are an architecture-private
   detail stashed in EntropyBase->eb_HWData; the generic resource only ever
   sees the neutral EIF_HARDWARE flag. */
#define X86ENT_RDRAND   (1 << 0)
#define X86ENT_RDSEED   (1 << 1)

/* PIC-safe CPUID (preserves EBX/RBX, which the compiler may use for PIC). */
static inline void hw_cpuid(unsigned int leaf, unsigned int subleaf,
                            unsigned int *pa, unsigned int *pb,
                            unsigned int *pc, unsigned int *pd)
{
#if defined(__x86_64__)
    unsigned int a = leaf, b, c = subleaf, d;
    __asm__ volatile(
        "mov %%rbx, %%rdi\n\t"
        "cpuid\n\t"
        "xchg %%rbx, %%rdi\n\t"
        : "+a"(a), "=D"(b), "+c"(c), "=d"(d)
        :: "memory");
#else
    unsigned int a, b, c, d;
    __asm__ volatile(
        "xchg %%ebx, %%edi\n\t"
        "cpuid\n\t"
        "xchg %%ebx, %%edi\n\t"
        : "=a"(a), "=D"(b), "=c"(c), "=d"(d)
        : "a"(leaf), "c"(subleaf)
        : "memory");
#endif
    if (pa) *pa = a;
    if (pb) *pb = b;
    if (pc) *pc = c;
    if (pd) *pd = d;
}

static inline void hw_pause(void)
{
    __asm__ volatile("pause");
}

/* Each returns 1 and stores a fresh value on success, 0 if the CPU declined
   to provide data this time (carry flag clear). */
static inline int hw_rdrand(xword *v)
{
    unsigned char ok;
    __asm__ volatile("rdrand %0\n\tsetc %1" : "=r"(*v), "=qm"(ok) :: "cc");
    return ok;
}

static inline int hw_rdseed(xword *v)
{
    unsigned char ok;
    __asm__ volatile("rdseed %0\n\tsetc %1" : "=r"(*v), "=qm"(ok) :: "cc");
    return ok;
}

/* Retry wrapper: RDRAND/RDSEED can transiently fail, so spin a bounded number
   of times before giving up on this word. */
static int hw_word(int use_seed, xword *v)
{
    int tries;

    for (tries = 0; tries < 64; tries++)
    {
        if (use_seed ? hw_rdseed(v) : hw_rdrand(v))
            return 1;
        hw_pause();
    }
    return 0;
}

/* Installed hardware gather: fill up to length bytes, preferring RDSEED. */
static ULONG x86_hw_gather(struct EntropyBase *EntropyBase,
                           UBYTE *buffer, ULONG length)
{
    IPTR  caps     = (IPTR)EntropyBase->eb_HWData;
    int   use_seed = (caps & X86ENT_RDSEED) != 0;
    ULONG got = 0;

    while (got < length)
    {
        xword v;
        ULONG take, i;

        if (!hw_word(use_seed, &v))
        {
            /* RDSEED can be scarcer than RDRAND; if it stalls, top up the
               remainder from RDRAND rather than returning short. */
            if (use_seed && (caps & X86ENT_RDRAND) && hw_word(0, &v))
            {
                /* fall through with v from RDRAND */
            }
            else
                break;
        }

        take = (length - got) < XWORD_BYTES ? (length - got) : XWORD_BYTES;
        for (i = 0; i < take; i++)
            buffer[got + i] = (UBYTE)(v >> (i * 8));
        got += take;

        v = 0;              /* do not leave raw hardware output on the stack */
        (void)v;
    }

    return got;
}

void Entropy_HW_Init(struct EntropyBase *EntropyBase)
{
    unsigned int maxleaf = 0, a = 0, b = 0, c = 0, d = 0;
    int have_rdrand, have_rdseed, use_seed;
    IPTR caps = 0;
    xword test;

    EntropyBase->eb_Flags   |= EIF_SOFTWARE;
    EntropyBase->eb_HWGather  = NULL;
    EntropyBase->eb_HWData    = NULL;

    hw_cpuid(1, 0, &a, &b, &c, &d);
    have_rdrand = (c & CPUID_1_ECX_RDRAND) != 0;

    hw_cpuid(0, 0, &maxleaf, &b, &c, &d);
    have_rdseed = 0;
    if (maxleaf >= 7)
    {
        hw_cpuid(7, 0, &a, &b, &c, &d);
        have_rdseed = (b & CPUID_7_EBX_RDSEED) != 0;
    }

    if (have_rdrand)
        caps |= X86ENT_RDRAND;
    if (have_rdseed)
        caps |= X86ENT_RDSEED;

    if (!caps)
    {
        D(bug("[entropy] no RDRAND/RDSEED, using software source\n"));
        return;
    }

    /* Confirm the advertised instruction actually delivers data. */
    use_seed = have_rdseed;
    if (!hw_word(use_seed, &test))
    {
        /* Advertised but non-functional - try the other one before giving up. */
        if (have_rdrand && have_rdseed && hw_word(0, &test))
        {
            caps &= ~X86ENT_RDSEED;                 /* RDSEED unusable */
        }
        else
        {
            D(bug("[entropy] RDRAND/RDSEED present but non-functional\n"));
            return;
        }
    }
    test = 0;
    (void)test;

    /* Keep the concrete instruction set private to this back-end; the generic
       resource is told only that some hardware source is active. */
    EntropyBase->eb_HWData   = (APTR)caps;
    EntropyBase->eb_HWGather = x86_hw_gather;
    EntropyBase->eb_Flags   |= EIF_HARDWARE;

    D(bug("[entropy] hardware source active (flags 0x%08lx)\n",
          (unsigned long)EntropyBase->eb_Flags));
}

#else /* not x86 - should not happen for arch=pc, but keep it safe */

void Entropy_HW_Init(struct EntropyBase *EntropyBase)
{
    EntropyBase->eb_Flags   |= EIF_SOFTWARE;
    EntropyBase->eb_HWGather  = NULL;
    EntropyBase->eb_HWData    = NULL;
}

#endif
