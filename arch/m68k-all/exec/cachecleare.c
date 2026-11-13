/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() - Clear the caches with extended control.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include <defines/exec_LVO.h>

extern void AROS_SLIB_ENTRY(CacheClearE_00,Exec,LVOCacheClearE)(void);
extern void AROS_SLIB_ENTRY(CacheClearE_20,Exec,LVOCacheClearE)(void);
extern void AROS_SLIB_ENTRY(CacheClearE_4060,Exec,LVOCacheClearE)(void);

#include <proto/exec.h>

/* See rom/exec/cachecleare.c for documentation */

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR, address, A0),
    AROS_LHA(ULONG, length, D0),
    AROS_LHA(ULONG, caches, D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    void (*func)();

    if (SysBase->LibNode.lib_OpenCnt == 0)
        return;

    Disable();
    if (SysBase->AttnFlags & AFF_68060) {
        /* 68060 support */
        func = AROS_SLIB_ENTRY(CacheClearE_4060, Exec, LVOCacheClearE);
    } else if (SysBase->AttnFlags & AFF_68040) {
        /* 68040 support */
        func = AROS_SLIB_ENTRY(CacheClearE_4060, Exec, LVOCacheClearE);
    } else if (SysBase->AttnFlags & AFF_68020) {
        /* 68020 support */
        func = AROS_SLIB_ENTRY(CacheClearE_20, Exec, LVOCacheClearE);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CacheClearE_00, Exec, LVOCacheClearE);
    }
    SetFunction((struct Library *)SysBase, -LVOCacheClearE * LIB_VECTSIZE, func);
    Enable();

    /* Invoke through the patched library vector so the library base is passed
     * in A6. A direct call (bare func() or AROS_UFC) lets the compiler
     * (GCC 16) allocate the function pointer into A6, clobbering SysBase; the
     * assembler routine then runs 'jmp Supervisor(A6)' through a garbage
     * vector and the machine traps. */
    CacheClearE(address, length, caches);

    AROS_LIBFUNC_EXIT
} /* CacheClearE */

