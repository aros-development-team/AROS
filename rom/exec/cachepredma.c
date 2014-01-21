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

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_NTLH3(APTR, CachePreDMA,

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
	This function should be replaced by a function in $(KERNEL).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * Due to the strong cache coherency of x86 systems this function
     * is actually not needed. CPU snoops the address lines and 
     * invalidate all cache which is out-of-date. It is valid for both
     * D and I caches). Even a BM-DMA transfer are perfectly safe here.
     */
    return address;

    AROS_LIBFUNC_EXIT
} /* CachePreDMA() */
