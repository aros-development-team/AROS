/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1997/03/27 01:10:58  ldp
    libaros.a -> libarossupport.a

    Revision 1.6  1997/01/27 00:17:41  ldp
    Include proto instead of clib

    Revision 1.5  1996/12/10 13:59:44  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/09/21 15:47:33  digulla
    Use Amiga types

    Revision 1.3  1996/09/11 16:50:52  digulla
    Moved PurgeChunk() to here to avoid problems during link

    Revision 1.2  1996/08/16 14:03:26  digulla
    NastyFreeMem() should itself call FreeMem, no matter what :)

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
#undef FreeMem /* Don't use any kind of macro here :) We want the real thing */
#include <proto/exec.h>

extern struct ExecBase * SysBase;
extern void PurgeChunk (ULONG *, ULONG);

/*****************************************************************************

    NAME */
#include <proto/aros.h>

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

