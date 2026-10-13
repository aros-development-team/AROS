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

#include "debug_intern.h"

LONG Debug_SymResolver(APTR priv, APTR addr, struct KrnSymInfo *out)
{
    struct Library *DebugBase = priv;
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

    return DecodeLocationA(addr, tags) ? 1 : 0;
}
