/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Erase contents of memory before freeing it.
    Lang: english
*/
#include <aros/system.h>
#include <exec/execbase.h>
#undef FreeMem /* Don't use any kind of macro here :) We want the real thing */
#include <proto/exec.h>

extern struct ExecBase * SysBase;
extern void PurgeChunk (ULONG *, ULONG);

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	void NastyFreeMem (

/*  SYNOPSIS */
	APTR  mem,
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


/* Don't use #if on this one since it may be used by some other routine, too.
    It's not static by design. */
void PurgeChunk (ULONG * ptr, ULONG size)
{
    while (size >= sizeof (ULONG))
    {
#if SIZEOFULONG > 4
	*ptr ++ = 0xDEAFBEEFDEADBEEFL;
#else
	*ptr ++ = 0xDEAFBEEFL;
#endif
	size -= sizeof (ULONG);
    }
}

