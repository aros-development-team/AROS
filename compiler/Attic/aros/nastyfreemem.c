/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/15 14:39:42  digulla
    Delete contents of memory before freeing it

    Revision 1.1  1996/08/15 13:24:20  digulla
    New function: kprintf() allows to print a text which is always shown to the
    user no matter what.

    Revision 1.1  1996/08/01 18:46:31  digulla
    Simple string compare function

    Desc:
    Lang:
*/
#include <aros/system.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>

extern struct ExecBase * SysBase;
extern void PurgeChunk (ULONG *, ULONG);

/*****************************************************************************

    NAME */
	#include <clib/aros_protos.h>

	void NastyFreeMem (

/*  SYNOPSIS */
	void * mem,
	ULONG size)

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
	FreeMem()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    PurgeChunk ((ULONG *)mem, size);
    FreeMem (mem, size);
} /* NastyFreeMem */

