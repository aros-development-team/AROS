/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearU - Simple way of clearing the caches.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include <defines/exec_LVO.h>

extern void AROS_SLIB_ENTRY(CacheClearU_00,Exec,LVOCacheClearU)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_20,Exec,LVOCacheClearU)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_40,Exec,LVOCacheClearU)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_60,Exec,LVOCacheClearU)(void);

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
     *
     * NOTE: We must NOT call 'func' directly. The assembler cache routines
     * expect the library base in A6, but a plain C call through a function
     * pointer lets the compiler (observed with GCC 16) allocate that pointer
     * into A6 itself, passing a bogus base. The routine would then execute
     * 'jmp Supervisor(A6)' through a garbage vector and the machine hangs
     * (68040 cpusha) or crashes (68020 returns to an odd address).
     *
     * So install the routine first and then invoke it through the freshly
     * patched library vector, which always loads A6 = SysBase.
     */

    Disable();
    if (SysBase->AttnFlags & AFF_68060) {
        /* 68060 support */
        func = AROS_SLIB_ENTRY(CacheClearU_60, Exec, LVOCacheClearU);
    } else if (SysBase->AttnFlags & AFF_68040) {
        /* 68040 support */
        func = AROS_SLIB_ENTRY(CacheClearU_40, Exec, LVOCacheClearU);
    } else if (SysBase->AttnFlags & AFF_68020) {
        /* 68020 support */
        func = AROS_SLIB_ENTRY(CacheClearU_20, Exec, LVOCacheClearU);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CacheClearU_00, Exec, LVOCacheClearU);
    }
    SetFunction((struct Library *)SysBase, -LVOCacheClearU * LIB_VECTSIZE, func);
    Enable();

    /* Now perform the actual cache clear through the patched library vector
     * (A6 = SysBase). SetFunction() above already invoked it once internally. */
    CacheClearU();

    AROS_LIBFUNC_EXIT
} /* CacheClearU */
