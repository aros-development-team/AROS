/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() - Clear the caches with extended control, ARM Linux-hosted implementation
*/

#include <exec/execbase.h>
#include <aros/atomic.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <aros/debug.h>

AROS_LH3(void, CacheClearE,
         AROS_LHA(APTR, address, A0),
         AROS_LHA(ULONG, length, D0),
         AROS_LHA(ULONG, caches, D1),
         struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    D(bug("CacheClearE(%08x, %08x, %x)\n", address, length, caches));

    /* Linux supports only instruction cache flush */
    if (caches & CACRF_ClearI)
    {
        int ret;

        /* Forbid(). We inline it because we could use real executable jumptable,
           in this case this function can be called for validating ExecBase
           itself. */
        AROS_ATOMIC_INC(SysBase->TDNestCnt);

        register APTR  _r0 __asm__("r0") = address;
        register APTR  _r1 __asm__("r1") = (STRPTR)address + length;
        register ULONG _r2 __asm__("r2") = 0;        // flags
        register ULONG _r7 __asm__("r7") = 0xF0002;  // cacheflush syscall

        __asm__ volatile(
                         "svc 0\n"
                         : "+r"(_r0)
                         : "r"(_r1), "r"(_r2), "r"(_r7)
                         : "r3", "r4", "r5", "r6", "r8", "r9", "r10", "r12", "memory", "cc"
                         );
        ret = (int)_r0;

        /* It's okay to use library base now */
        Permit();

        if (ret != 0) {
            bug("CPU cache could not be cleared!\n");
        }
    }

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
