/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Kernel symbol resolver backed by the module list.
*/

#include <aros/atomic.h>
#include <aros/kernel.h>
#include <libraries/debug.h>
#include <proto/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "debug_intern.h"

/*
 * Admit a supervisor/trap reader without waiting.
 *
 * On UP, the generation is sufficient: a trap and its interrupted writer
 * cannot execute concurrently on the same CPU. On SMP, the reader count
 * extends the lifetime of borrowed module data across CPUs.
 */
BOOL Debug_ResolverAcquire(struct DebugBase *debugBase)
{
#if defined(__i386__) || defined(__x86_64__)
    ULONG generation;

    if (debugBase->db_SymResolverABI < KRN_SYMRESOLVER_ABI_LEASE)
        return FALSE;

    generation = debugBase->db_ResolverGeneration;
    if (generation & 1)
        return FALSE;

#if defined(__AROSEXEC_SMP__)
    AROS_ATOMIC_INC(debugBase->db_ResolverReaders);

    if ((debugBase->db_ResolverGeneration != generation) ||
        (debugBase->db_ResolverGeneration & 1))
    {
        AROS_ATOMIC_DEC(debugBase->db_ResolverReaders);
        return FALSE;
    }
#endif

    return TRUE;
#else
    (void)debugBase;
    return FALSE;
#endif
}

/* Release one previously admitted supervisor/trap reader. */
void Debug_ResolverRelease(struct DebugBase *debugBase)
{
#if (defined(__i386__) || defined(__x86_64__)) && defined(__AROSEXEC_SMP__)
    AROS_ATOMIC_DEC(debugBase->db_ResolverReaders);
#else
    (void)debugBase;
#endif
}

/*
 * Start a module-database write. The caller owns db_ModSem exclusively.
 * Interrupt masking closes the same-CPU window while the generation becomes
 * odd. The atomic generation update also orders that publication before an
 * SMP writer checks for already admitted readers.
 */
void Debug_ResolverWriteBegin(struct DebugBase *debugBase)
{
#if defined(__i386__) || defined(__x86_64__)
    if (debugBase->db_SymResolverABI < KRN_SYMRESOLVER_ABI_LEASE)
        return;

    Disable();
    AROS_ATOMIC_INC(debugBase->db_ResolverGeneration);
    Enable();

#if defined(__AROSEXEC_SMP__)
    while (debugBase->db_ResolverReaders != 0)
        __asm__ __volatile__("pause");
#endif
#else
    (void)debugBase;
#endif
}

/* Publish completion of a module-database write owned under db_ModSem. */
void Debug_ResolverWriteEnd(struct DebugBase *debugBase)
{
#if defined(__i386__) || defined(__x86_64__)
    if (debugBase->db_SymResolverABI < KRN_SYMRESOLVER_ABI_LEASE)
        return;

    Disable();
    AROS_ATOMIC_INC(debugBase->db_ResolverGeneration);
    Enable();
#else
    (void)debugBase;
#endif
}

static LONG ResolveLocation(struct Library *DebugBase, APTR addr,
                            struct KrnSymInfo *out, BOOL internal)
{
    struct TagItem tags[] =
    {
        { DL_ModuleName,     (IPTR)&out->mod_name  },
        { DL_SegmentName,    (IPTR)&out->seg_name  },
        { DL_SegmentPointer, (IPTR)&out->seg_bptr  },
        { DL_SegmentNumber,  (IPTR)&out->seg_num   },
        { DL_SegmentStart,   (IPTR)&out->seg_start },
        { DL_SegmentEnd,     (IPTR)&out->seg_end   },
        { DL_SymbolName,     (IPTR)&out->sym_name  },
        { DL_SymbolStart,    (IPTR)&out->sym_start },
        { DL_SymbolEnd,      (IPTR)&out->sym_end   },
        { TAG_DONE,          0                     }
    };

    if (internal)
        return Debug_DecodeLocationAInternal(addr, tags, DebugBase) ? 1 : 0;

    return DecodeLocationA(addr, tags) ? 1 : 0;
}

/* Task-context leases keep the existing database semaphore held to consumption. */
static void Debug_SymReleaseTask(APTR cookie)
{
    struct DebugBase *debugBase = cookie;

    ReleaseSemaphore(&debugBase->db_ModSem);
}

/* Supervisor/trap leases never block. */
static void Debug_SymReleaseTrap(APTR cookie)
{
    Debug_ResolverRelease(cookie);
}

/* Old kernels receive only the prefix-compatible legacy result fields. */
LONG Debug_SymResolverLegacy(APTR priv, APTR addr, struct KrnSymInfo *out)
{
    return ResolveLocation(priv, addr, out, FALSE);
}

/*
 * Resolver ABI 2. Task callers lease db_ModSem; supervisor/trap callers use
 * generation admission and, on SMP, an atomic reader lifetime. In both cases
 * the kernel owns the admission until it invokes out->release.
 */
LONG Debug_SymResolver(APTR priv, APTR addr, struct KrnSymInfo *out)
{
    struct Library *DebugBase = priv;
    struct DebugBase *debugBase = DBGBASE(DebugBase);

    if (KrnIsSuper())
    {
        if (!Debug_ResolverAcquire(debugBase))
            return 0;

        out->release = Debug_SymReleaseTrap;
    }
    else
    {
        ObtainSemaphoreShared(&debugBase->db_ModSem);
        out->release = Debug_SymReleaseTask;
    }

    out->release_cookie = debugBase;

    return ResolveLocation(DebugBase, addr, out, TRUE);
}
