/*
    (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: CacheClearU - Simple way of clearing the caches.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH0(void, CacheClearU,

    LOCATION
	struct ExecBase *, SysBase, 106, Exec)

    FUNCTION
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

    HISTORY

******************************************************************************/

#include "machine.i"

	    .text
	    .globl	AROS_SLIB_ENTRY(CacheClearU,Exec)
	    .type	AROS_SLIB_ENTRY(CacheClearU,Exec),@function
AROS_SLIB_ENTRY(CacheClearU,Exec):
	    wbinvd
	    ret
