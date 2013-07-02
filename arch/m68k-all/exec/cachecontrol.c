/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheControl() - Global control of the system caches.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

extern void AROS_SLIB_ENTRY(CacheControl_00,Exec,108)(void);
extern void AROS_SLIB_ENTRY(CacheControl_20,Exec,108)(void);
extern void AROS_SLIB_ENTRY(CacheControl_40,Exec,108)(void);

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH2(ULONG, CacheControl,

/*  SYNOPSIS */
	AROS_LHA(ULONG, cacheBits, D0),
	AROS_LHA(ULONG, cacheMask, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 108, Exec)

/*  FUNCTION
	This function will provide global control of all the processor
	instruction and data caches. It is not possible to have per
	task control.

	The actions undertaken by this function are very CPU dependant,
	however the actions performed will match the specified options
	as close as is possible.

	The commands currently defined in the include file exec/execbase.h
	are closely related to the cache control register of the Motorola
	MC68030 CPU.

    INPUTS
	cacheBits   -   The new state of the bits
	cacheMask   -   A mask of the bits you wish to change.

    RESULT
	oldBits     -   The complete value of the cache control bits
			prior to the call of this function.

	Your requested actions will have been performed. As a side effect
	this function will also cause the caches to be cleared.

    NOTES
	On CPU's without a separate instruction and data cache, these will
	be considered as equal.

    EXAMPLE

    BUGS

    SEE ALSO
	CacheClearE(), CacheClearU()

    INTERNALS
	This function requires replacing in $(KERNEL), or possibly
	even $(ARCH) in some cases.

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    Disable();
    if (SysBase->AttnFlags & AFF_68040) {
        /* 68040/68060 support */
        func = AROS_SLIB_ENTRY(CacheControl_40, Exec, 108);
    } else if (SysBase->AttnFlags & AFF_68020) {
        /* 68020/68030 support */
        func = AROS_SLIB_ENTRY(CacheControl_20, Exec, 108);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CacheControl_00, Exec, 108);
    }
    SetFunction((struct Library *)SysBase, -LIB_VECTSIZE * 108, func);
    Enable();

    return CacheControl(cacheBits, cacheMask);

    AROS_LIBFUNC_EXIT
} /* CacheControl */
