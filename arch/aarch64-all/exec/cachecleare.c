/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() - Clear the caches with extended control, AArch64.
*/

#include <exec/execbase.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*
 * AArch64 needs explicit data/instruction cache maintenance for newly written
 * executable code. On hosted targets the user-mode maintenance instructions
 * may be unavailable (the host can trap them); the hosted kernel instead
 * performs the instruction-cache invalidation in KrnSetProtection() when code
 * pages are made executable, so this function is a no-op there.
 */
AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR, address, A0),
    AROS_LHA(IPTR, length, D0),
    AROS_LHA(ULONG, caches, D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    (void)address;
    (void)length;
    (void)caches;

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
