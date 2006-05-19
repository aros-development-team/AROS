/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CachePostDMA() - Do what is necessary for DMA.
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

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
	DMA must follow a call to CachePreDMA() and must be followed
	by a call to CachePostDMA().

    EXAMPLE

    BUGS

    SEE ALSO
	CachePreDMA()

    INTERNALS
	This function should be replaced by a function in the $(KERNEL)
	directory.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning "TODO: Write exec/CachePostDMA()"
#if defined(DEBUG) && (DEBUG > 0)
    aros_print_not_implemented("CachePostDMA");
#endif

    AROS_LIBFUNC_EXIT
} /* CachePostDMA */

