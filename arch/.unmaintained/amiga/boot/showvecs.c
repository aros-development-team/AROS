/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: prints Kick Vector pointers
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>

extern struct ExecBase *SysBase;

const UBYTE ver[] = "$VER: showvecs 41.2 (14.3.1997)\n\r";

int main(void)
{
    Printf("KickTagPtr = 0x%08lx\nKickMemPtr = 0x%08lx\nKickChkSum = 0x%08lx\n",
		(ULONG)SysBase->KickTagPtr, (ULONG)SysBase->KickMemPtr, (ULONG)SysBase->KickCheckSum);

    if(SumKickData() == (ULONG)SysBase->KickCheckSum)
    {
	if(SysBase->KickTagPtr)
	{
	    ULONG *list;

	    Printf("Modules in use:\n");

	    list = SysBase->KickTagPtr;

	    while(*list)
	    {
		if(*list & 0x80000000) list = (ULONG *)(*list & 0x7fffffff);
		Printf("\t0x%08lx\t%s\n", *list, (ULONG)((struct Resident *)*list)->rt_IdString);
		list++;
	    }
        }

        if(SysBase->KickMemPtr)
        {
	    struct MemList *memlist;

	    Printf("MemLists in use:\n");

	    memlist = (struct MemList *)SysBase->KickMemPtr;

	    while(memlist)
	    {
		struct MemEntry *me = &memlist->ml_ME[0];
		UWORD i;

		Printf("ml_NumEntries = %ld\n", memlist->ml_NumEntries);

		for(i = 0; i < memlist->ml_NumEntries; i++)
		{
		    Printf("ml_ME[%2ld] = 0x%08lx size 0x%08lx (%8ld)\n",
		     i, (ULONG)me->me_Addr, me->me_Length, me->me_Length);
		    me = (struct MemEntry *)((ULONG)me + (ULONG)sizeof(struct MemEntry));
		}

		memlist = (struct MemList *)memlist->ml_Node.ln_Succ;
	    }
	}
    }
    else
    {
	/* Don't print message if all vectors are NULL */
	if(SysBase->KickMemPtr || SysBase->KickTagPtr || SysBase->KickCheckSum)
	    Printf("Vectors have incorrect checksum.\n");
    }

    return(0);
}
