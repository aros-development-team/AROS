/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Build checksum for Kickstart.
    Lang: english
*/

#include <aros/debug.h>
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH0(ULONG, SumKickData,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct ExecBase *, SysBase, 102, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG chksum = 0;
    BOOL isdata = FALSE;

    D(bug("[SumKickData] KickTagPtr 0x%p KickMemPtr 0x%p\n", SysBase->KickTagPtr, SysBase->KickMemPtr));

    if (SysBase->KickTagPtr)
    {
    	IPTR *list = SysBase->KickTagPtr;

 	while(*list)
	{
   	    chksum += (ULONG)*list;
   	    D(bug("KickTag: %p: %08x\n", list, *list));

            /*
	     * on amiga, if bit 31 is set then this points to another list of
             * modules rather than pointing to a single module. bit 31 is
             * inconvenient on architectures where code may be loaded above
             * 2GB. on these platforms we assume aligned pointers and use bit
             * 0 instead
	     */
	    if (*list & RESLIST_NEXT)
	    {
		list = (IPTR *)(*list & ~RESLIST_NEXT);
		continue;
	    }
	    list++;
   	    isdata = TRUE;
   	}
    }

    if (SysBase->KickMemPtr)
    {
	struct MemList *ml = (struct MemList*)SysBase->KickMemPtr;

	while (ml)
	{
	    UBYTE i;
	    ULONG *p = (ULONG*)ml;

	    for (i = 0; i < sizeof(struct MemList) / sizeof(ULONG); i++)
	    	chksum += p[i];
	    D(bug("MemList: %p: %08x %08x %d %08x %08x %08x%08x%08x%08x\n", ml, ml->ml_Node.ln_Succ, ml->ml_Node.ln_Pred, ml->ml_NumEntries,
	    	ml->ml_ME[0].me_Un.meu_Addr, ml->ml_ME[0].me_Length,
	    	p[sizeof(struct MemList) / sizeof(ULONG)],
	    	p[sizeof(struct MemList) / sizeof(ULONG) + 1],
	    	p[sizeof(struct MemList) / sizeof(ULONG) + 2],
	    	p[sizeof(struct MemList) / sizeof(ULONG) + 3])); 
	    ml = (struct MemList*)ml->ml_Node.ln_Succ;
	    isdata = TRUE;
	}
    }
    if (isdata && !chksum)
    	chksum--;
    return chksum;

    AROS_LIBFUNC_EXIT
} /* SumKickData */
