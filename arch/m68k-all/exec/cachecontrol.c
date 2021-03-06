/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: CacheControl() - Global control of the system caches.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include <defines/exec_LVO.h>

/* See rom/exec/cachecontrol.c for documentation */

extern void AROS_SLIB_ENTRY(CacheControl_00,Exec,LVOCacheControl)(void);
extern void AROS_SLIB_ENTRY(CacheControl_20,Exec,LVOCacheControl)(void);
extern void AROS_SLIB_ENTRY(CacheControl_40,Exec,LVOCacheControl)(void);

#include <proto/exec.h>

AROS_LH2(ULONG, CacheControl,
    AROS_LHA(ULONG, cacheBits, D0),
    AROS_LHA(ULONG, cacheMask, D1),
    struct ExecBase *, SysBase, 108, Exec)
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    Disable();
    if (SysBase->AttnFlags & AFF_68040) {
        /* 68040/68060 support */
        func = AROS_SLIB_ENTRY(CacheControl_40, Exec, LVOCacheControl);
    } else if (SysBase->AttnFlags & AFF_68020) {
        /* 68020/68030 support */
        func = AROS_SLIB_ENTRY(CacheControl_20, Exec, LVOCacheControl);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CacheControl_00, Exec, LVOCacheControl);
    }
    SetFunction((struct Library *)SysBase, -LVOCacheControl * LIB_VECTSIZE, func);
    Enable();

    return CacheControl(cacheBits, cacheMask);

    AROS_LIBFUNC_EXIT
} /* CacheControl */
