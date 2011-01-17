/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheClearE() - Clear the caches with extended control, ARM Linux-hosted implementation
    Lang: english
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
	/* Forbid(). We inline it because we could use real executable jumptable,
	   in this case this function can be called for validating ExecBase
	   itself. */
	AROS_ATOMIC_INC(SysBase->TDNestCnt);

	const int syscall = 0xf0002;
	__asm __volatile (
		"mov	 %%r0, %0\n"			
		"mov	 %%r1, %1\n"
		"mov	 %%r7, %2\n"
		"mov     %%r2, #0x0\n"
		"svc     0x00000000\n"
		:
		:	"r" (address), "r" ((IPTR)address + length), "r" (syscall)
		:	"r0", "r1", "r2", "r7", "memory"
		);

	/* It's okay to use library base now */
	Permit();
    }

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
