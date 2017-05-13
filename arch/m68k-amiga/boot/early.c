/*
 * Copyright (C) 2011-2017, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#define DEBUG 0

#include <proto/exec.h>
#include <exec/memory.h>

#include "amiga_hwreg.h"

#include "early.h"
#include "debug.h"

void Early_ScreenCode(ULONG code)
{
	reg_w(BPLCON0, 0x0200);
	reg_w(BPL1DAT, 0x0000);
	reg_w(COLOR00, code & RGB_MASK);
}

void Early_Alert(ULONG alert)
{
    const int bright = ((alert >> 4) & 1) ? 0xf : 0x7;
    const int color = 
    		RGB(((alert >> 2) & 1) * bright,
    		    ((alert >> 1) & 1) * bright,
    		    ((alert >> 0) & 1) * bright);

    DEBUGPUTHEX(("Early_Alert", alert));

    for (;;) {
    	volatile int i;
    	Early_ScreenCode(color);
    	for (i = 0; i < 100000; i++);
    	Early_ScreenCode(0x000);
    	for (i = 0; i < 100000; i++);

    	if (!(alert & AT_DeadEnd))
    	    break;
    }
}

/* Fatal trap for early problems */
extern void Exec_MagicResetCode(void);
void __attribute__((interrupt)) Early_TrapHandler(void)
{
    volatile int i;
    Early_ScreenCode(CODE_TRAP_FAIL);

    /* If we have a valid SysBase, then
     * we can run the debugger.
     */
    if (SysBase != NULL)
	Debug(0);
    else
    	Early_Alert(AT_DeadEnd | 1);

    /* Sleep for a while */
    for (i = 0; i < 100000; i++);

    /* Reset everything but the CPU, then restart
     * at the ROM exception vector
     */
    Exec_MagicResetCode();
}

APTR Early_AllocAbs(struct MemHeader *mh, APTR location, IPTR byteSize)
{
    APTR ret = NULL;
    APTR endlocation = location + byteSize;
    struct MemChunk *p1, *p2, *p3, *p4;

    if (mh->mh_Lower > location || mh->mh_Upper < endlocation)
    	return (APTR)1;
        
    /* Align size to the requirements */
    byteSize += (IPTR)location&(MEMCHUNK_TOTAL - 1);
    byteSize  = (byteSize + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);
    
    /* Align the location as well */
    location=(APTR)((IPTR)location & ~(MEMCHUNK_TOTAL-1));
    
    /* Start and end(+1) of the block */
    p3=(struct MemChunk *)location;
    p4=(struct MemChunk *)((UBYTE *)p3+byteSize);
    
    /*
	The free memory list is only single linked, i.e. to remove
	elements from the list I need the node's predessor. For the
	first element I can use freeList->mh_First instead of a real
	predecessor.
    */
    p1 = (struct MemChunk *)&mh->mh_First;
    p2 = p1->mc_Next;

    /* Follow the list to find a chunk with our memory. */
    while (p2 != NULL)
    {
	/* Found a chunk that fits? */
	if((UBYTE *)p2+p2->mc_Bytes>=(UBYTE *)p4&&p2<=p3)
	{
	    /* Check if there's memory left at the end. */
	    if((UBYTE *)p2+p2->mc_Bytes!=(UBYTE *)p4)
	    {
		/* Yes. Add it to the list */
		p4->mc_Next  = p2->mc_Next;
		p4->mc_Bytes = (UBYTE *)p2+p2->mc_Bytes-(UBYTE *)p4;
		p2->mc_Next  = p4;
	    }

	    /* Check if there's memory left at the start. */
	    if(p2!=p3)
		/* Yes. Adjust the size */
		p2->mc_Bytes=(UBYTE *)p3-(UBYTE *)p2;
	    else
		/* No. Skip the old chunk */
		p1->mc_Next=p2->mc_Next;

	    /* Adjust free memory count */
	    mh->mh_Free-=byteSize;

	    /* Return the memory */
	    ret = p3;
	    break;
	}
	/* goto next chunk */
    
	p1=p2;
	p2=p2->mc_Next;
    }

    return ret;
}
