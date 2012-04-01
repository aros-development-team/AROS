/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CachePreDMA() - Do what is necessary for DMA.
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/kernel.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH3(APTR, CachePreDMA,

/*  SYNOPSIS */
	AROS_LHA(APTR,    address, A0),
	AROS_LHA(ULONG *, length,  A1),
	AROS_LHA(ULONG,   flags,  D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 127, Exec)

/*  FUNCTION
	Do everything necessary to make CPU caches aware that a DMA
	will happen. Virtual memory systems will make it possible
	that your memory is not at one block and not at the address
	you thought. This function gives you all the information
	you need to split the DMA request up and to convert virtual
	to physical addresses.

    INPUTS
	address - Virtual address of memory affected by the DMA
	*length - Number of bytes affected
	flags   - DMA_Continue    - This is a call to continue a
				    request that was broken up.
		  DMA_ReadFromRAM - Indicate that the DMA goes from
				    RAM to the device. Set this bit
				    in both calls.

    RESULT
	The physical address in memory.
	*length contains the number of contiguous bytes in physical
	memory.

    NOTES
	DMA must follow a call to CachePreDMA() and must be followed
	by a call to CachePostDMA().

    EXAMPLE

    BUGS

    SEE ALSO
	CachePostDMA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* At PreDMA stage only data caches need to be flushed */
    //if (flags & DMA_ReadFromRAM)

    void *addr = KrnVirtualToPhysical(address);

    D(bug("[exec] CachePreDMA(%08x, %d, %c) = %08x\n", address, *length, flags & DMA_ReadFromRAM ? 'R':'W', addr));

    /* At PreDMA stage only data caches need to be flushed */
    CacheClearE(address, *length, CACRF_ClearD);

    return addr;

    AROS_LIBFUNC_EXIT
} /* CachePreDMA() */

