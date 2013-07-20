/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Flush Caches
    Lang: english
*/

#include <errno.h>
#if 0
	#include <asm/cachectl.h>
	/* GNU libc 2 has this included in <sys/syscall.h>, but libc5 doesn't :-( */
	#include <asm/unistd.h>
	static inline _syscall4(int,cacheflush,unsigned long,addr,int,scope,int,cache,unsigned long,len)
#endif

#include <proto/exec.h>
#include <exec/execbase.h>

/* See rom/exec/cacheclear*.c for documentation */

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR,  address, A0),
    AROS_LHA(ULONG, length,  D0),
    AROS_LHA(ULONG, caches,  D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT
#if 0
    ULONG scope, cpucache = 0;

    if (caches & CACRF_ClearD)
	cpucache |= FLUSH_CACHE_DATA;

    if (caches & CACRF_ClearI)
	cpucache |= FLUSH_CACHE_INSN;

    if (length == (ULONG)-1)
	scope = FLUSH_SCOPE_ALL;
    else
	scope = FLUSH_SCOPE_LINE;

    (void) cacheflush((unsigned long)address, scope, cpucache, length);
#endif
    AROS_LIBFUNC_EXIT
} /* CacheClearE */


AROS_LH0(void, CacheClearU,
    struct ExecBase *, SysBase, 106, Exec)
{
    AROS_LIBFUNC_INIT
#if 0 
	(void) cacheflush(0, FLUSH_SCOPE_ALL, FLUSH_CACHE_BOTH, 0);
#endif
    AROS_LIBFUNC_EXIT
} /* CacheClearU */

