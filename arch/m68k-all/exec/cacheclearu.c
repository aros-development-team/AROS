/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheClearU - Simple way of clearing the caches.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

extern void AROS_SLIB_ENTRY(CacheClearU_00,Exec)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_20,Exec)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_40,Exec)(void);
extern void AROS_SLIB_ENTRY(CacheClearU_60,Exec)(void);
/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, CacheClearU,

/*  LOCATION */
	struct ExecBase *, SysBase, 106, Exec)

/*  FUNCTION
	Flush the entire contents of the CPU instruction and data caches.
	If some of the cache contains dirty data, push it to memory first.

	For most systems DMA will not effect processor caches. If *any*
	external (non-processor) event changes system memory, you MUST
	clear the cache. For example:

	    DMA
	    Code relocation to run at a different address
	    Building jump tables
	    Loading code from disk

    INPUTS

    RESULT
	The caches will be flushed.

    NOTES
	It is possible that on some systems the entire cache will be
	even if this was not the specific request.

    EXAMPLE

    BUGS

    SEE ALSO
	CacheClearE(), CacheControl(), CachePreDMA(), CachePostDMA()

    INTERNALS
	Although it is not necessary, but it could be more efficient if
	this function was replaced by a function in $(KERNEL).

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    /* When called the first time, this patches up the
     * Exec syscall table to directly point to the right routine.
     * NOTE: We may have been originally called from SetFunction()
     * We must clear caches before calling SetFunction()
     */
    Disable();
    if (SysBase->AttnFlags & AFF_68060) {
        /* 68060 support */
        func = AROS_SLIB_ENTRY(CacheClearU_60, Exec);
    } else if (SysBase->AttnFlags & AFF_68040) {
        /* 68040 support */
        func = AROS_SLIB_ENTRY(CacheClearU_40, Exec);
    } else if (SysBase->AttnFlags & AFF_68020) {
        /* 68020 support */
        func = AROS_SLIB_ENTRY(CacheClearU_20, Exec);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CacheClearU_00, Exec);
    }
    func();
    SetFunction(SysBase, -LIB_VECTSIZE * 106, func);
    Enable();

    AROS_LIBFUNC_EXIT
} /* CacheClearU */
