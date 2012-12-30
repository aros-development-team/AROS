/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Erase contents of memory before freeing it.
    Lang: english
*/

#include <proto/exec.h>
#include <aros/system.h>
#include <aros/debug.h>
#include <exec/execbase.h>

#undef FreeMem /* Don't use any kind of macro here :) We want the real thing */
void FreeMem(void *, IPTR);


extern struct ExecBase * SysBase;

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	void NastyFreeMem (

/*  SYNOPSIS */
	APTR mem,
	IPTR size)

/*  FUNCTION
	Overwrites the memory with 0xDEADBEEF before actually freeing it.

    INPUTS
	mem - Pointer which was returned by AllocMem()
	size - Size which was given to AllocMem()

    RESULT
	The function may print some infos using kprintf().

    NOTES
	This function depends on SysBase.

    EXAMPLE

    BUGS

    SEE ALSO
	exec.library/FreeMem()

    INTERNALS

******************************************************************************/
{
    MUNGE_BLOCK (mem, size, MEMFILL_FREE);
    FreeMem (mem, size);
} /* NastyFreeMem */
