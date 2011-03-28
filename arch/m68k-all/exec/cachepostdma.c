/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CachePostDMA() - Do what is necessary for DMA.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

extern void AROS_SLIB_ENTRY(CachePostDMA_00,Exec)(void);
extern void AROS_SLIB_ENTRY(CachePostDMA_30,Exec)(void);
extern void AROS_SLIB_ENTRY(CachePostDMA_40,Exec)(void);
/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH3(void, CachePostDMA,

/*  SYNOPSIS */
	AROS_LHA(APTR,    address, A0),
	AROS_LHA(ULONG *, length,  A1),
	AROS_LHA(ULONG,   flags,  D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 128, Exec)

/*  FUNCTION
	Do everything necessary to make CPU caches aware that a DMA has
	happened.

    INPUTS
	address - Virtual address of memory affected by the DMA
	*length - Number of bytes affected
	flags   - DMA_NoModify    - Indicate that the memory did not change.
		  DMA_ReadFromRAM - Indicate that the DMA goes from RAM
				    to the device. Set this bit in
				    both calls.

    RESULT

    NOTES
	DMA must follow a call to CachePostDMA() and must be followed
	by a call to CachePostDMA().

    EXAMPLE

    BUGS

    SEE ALSO
	CachePostDMA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    /* When called the first time, this patches up the
     * Exec syscall table to directly point to the right routine.
     */
    Disable();
    if (SysBase->AttnFlags & AFF_68040) {
        /* 68040 support */
        func = AROS_SLIB_ENTRY(CachePostDMA_40, Exec);
    } else if (SysBase->AttnFlags & AFF_68030) {
        /* 68030 support */
        func = AROS_SLIB_ENTRY(CachePostDMA_30, Exec);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CachePostDMA_00, Exec);
    }

    SetFunction((struct Library *)SysBase, -LIB_VECTSIZE * 128, func);
    Enable();

    /* Call 'myself', which is now pointing to the correct routine */
    return CachePostDMA(address, length, flags);

    AROS_LIBFUNC_EXIT
} /* CachePostDMA */

