/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheClearU - Simple way of clearing the caches.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

extern void AROS_SLIB_ENTRY(CacheClearU_00,Exec,106)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_20,Exec,106)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_40,Exec,106)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_60,Exec,106)(void);

#include <proto/exec.h>

/* See rom/exec/cacheclearu.c for documentation */

AROS_LH0(void, CacheClearU,
    struct ExecBase *, SysBase, 106, Exec)
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    if (SysBase->LibNode.lib_OpenCnt == 0)
    	/* We were called from PrepareExecBase. AttnFlags isn't set yet.
    	 * Do nothing or we would always install 68000 routine.
    	 * No harm done, caches are disabled at this point.
    	 */
    	 return;

    /* When called the first time, this patches up the
     * Exec syscall table to directly point to the right routine.
     * NOTE: We may have been originally called from SetFunction()
     * We must clear caches before calling SetFunction()
     */

    Disable();
    if (SysBase->AttnFlags & AFF_68060) {
        /* 68060 support */
        func = AROS_SLIB_ENTRY(CacheClearU_60, Exec, 106);
    } else if (SysBase->AttnFlags & AFF_68040) {
        /* 68040 support */
        func = AROS_SLIB_ENTRY(CacheClearU_40, Exec, 106);
    } else if (SysBase->AttnFlags & AFF_68020) {
        /* 68020 support */
        func = AROS_SLIB_ENTRY(CacheClearU_20, Exec, 106);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CacheClearU_00, Exec, 106);
    }
    func();
    SetFunction((struct Library *)SysBase, -LIB_VECTSIZE * 106, func);
    Enable();

    AROS_LIBFUNC_EXIT
} /* CacheClearU */
