/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Kernel symbol resolver backed by the module list.

    Registered with kernel.resource so KrnPrintBacktrace() and the trap
    handlers can name addresses. The lookup is DecodeLocationA(), which
    takes db_ModSem only when it is not called in supervisor mode, so
    the resolver never waits on the semaphore in trap context.
*/

#include <aros/kernel.h>
#include <libraries/debug.h>
#include <proto/debug.h>
#include <proto/kernel.h>

#include "debug_intern.h"

static LONG ResolveLocation(struct Library *DebugBase, APTR addr,
                            struct KrnSymInfo *out, BOOL resolverLease)
{
    struct TagItem tags[] =
    {
        { DL_ModuleName,              (IPTR)&out->mod_name  },
        { DL_SegmentName,             (IPTR)&out->seg_name  },
        { DL_SegmentPointer,          (IPTR)&out->seg_bptr  },
        { DL_SegmentNumber,           (IPTR)&out->seg_num   },
        { DL_SegmentStart,            (IPTR)&out->seg_start },
        { DL_SegmentEnd,              (IPTR)&out->seg_end   },
        { DL_SymbolName,              (IPTR)&out->sym_name  },
        { DL_SymbolStart,             (IPTR)&out->sym_start },
        { DL_SymbolEnd,               (IPTR)&out->sym_end   },
        { TAG_DONE,                   0                     }
    };

    if (resolverLease)
        return Debug_DecodeLocationAInternal(addr, tags, DebugBase) ? 1 : 0;
    return DecodeLocationA(addr, tags) ? 1 : 0;
}

static void Debug_SymRelease(APTR cookie)
{
    struct DebugBase *DebugBase = cookie;

    KrnSpinUnLock(&DebugBase->db_ResolverSpin);
}

LONG Debug_SymResolverLegacy(APTR priv, APTR addr, struct KrnSymInfo *out)
{
    struct Library *DebugBase = priv;

    return ResolveLocation(DebugBase, addr, out, FALSE);
}

LONG Debug_SymResolver(APTR priv, APTR addr, struct KrnSymInfo *out)
{
    struct Library *DebugBase = priv;
    struct DebugBase *debugBase = DBGBASE(DebugBase);

    if (!KrnSpinTryLock(&debugBase->db_ResolverSpin, SPINLOCK_MODE_READ))
        return 0;

    if (!ResolveLocation(DebugBase, addr, out, TRUE))
    {
        KrnSpinUnLock(&debugBase->db_ResolverSpin);
        return 0;
    }

    out->release = Debug_SymRelease;
    out->release_cookie = debugBase;

    return 1;
}
