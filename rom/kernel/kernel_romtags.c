/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    ROMTag scanner.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_romtags.h>

/*
 * Private exec.library include, needed for MEMCHUNK_TOTAL.
 * TODO: may be bring it out to public includes ?
 */
#include "memory.h"

#define PRINT_LIST

static LONG findname(struct Resident **list, ULONG len, CONST_STRPTR name)
{
    ULONG i;
    
    for (i = 0; i < len; i++)
    {
    	if (!strcmp(name, list[i]->rt_Name))
    	    return i;
    }

    return -1;
}

static inline void krnAllocBootMem(struct MemHeader *mh, ULONG size)
{
    size = (size + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);

    mh->mh_First          = (struct MemChunk *)((APTR)mh->mh_First + size);
    mh->mh_First->mc_Next = NULL;
    mh->mh_Free           = mh->mh_First->mc_Bytes = mh->mh_Free - size;
}

/*
 * RomTag scanner.
 *
 * This function scans kernel for existing Resident modules. If two modules
 * with the same name are found, the one with higher version or priority
 * wins.
 *
 * After building list of kernel modules, the KickTagPtr and KickMemPtr are
 * checksummed. If checksum is proper and all memory pointed in KickMemPtr
 * may be allocated, then all modules from KickTagPtr are added to RT list
 *
 * Afterwards the proper RomTagList is created (see InitCode() for details)
 * and memory after list and nodes is freed.
 *
 * The array ranges gives a [ start, end ] pair to scan, with an entry of
 * -1 used to break the loop.
 */

APTR krnRomTagScanner(struct MemHeader *mh, UWORD *ranges[])
{
    UWORD	    *end;
    UWORD	    *ptr;		/* Start looking here */
    struct Resident *res;               /* module found */
    ULONG	    i;
    BOOL	    sorted;
    /* 
     * We take the beginning of free memory from our boot MemHeader
     * and construct resident list there.
     * When we are done we know list length, so we can seal the used
     * memory by allocating it from the MemHeader.
     * This is 100% safe because we are here long before multitasking
     * is started up.
     */
    struct Resident **RomTag = (struct Resident **)mh->mh_First;
    ULONG	    num = 0;

    /* Look in whole kickstart for resident modules */
    while (*ranges != (UWORD *)~0)
    {
	ptr = *ranges++;
	end = *ranges++;

	bug("RomTagScanner: Start = %p, End = %p\n", ptr, end);
	do
	{
	    res = (struct Resident *)ptr;

	    /* Do we have RTC_MATCHWORD and rt_MatchTag*/
	    if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res)
	    {
		/* Yes, it is Resident module. Check if there is module with such name already */
		i = findname((struct Resident **)mh->mh_First, num, res->rt_Name);
		if (i != -1)
		{
		    struct Resident *old = RomTag[i];
		    /*
			Rules for replacing modules:
			1. Higher version always wins.
			2. If the versions are equal, then lower priority
			    looses.
		    */
		    if ((old->rt_Version < res->rt_Version) ||
			(old->rt_Version == res->rt_Version && old->rt_Pri <= res->rt_Pri))
		    {
		    	RomTag[i] = res;
		    }
		}
		else
		{
		    /* New module */
		    RomTag[num++] = res;
		}

		/* Get address of EndOfResident from RomTag but only when
		 * it's higher then present one - this avoids strange locks
		 * when not all modules have Resident structure in .text
		 * section */
		ptr = ((IPTR)res->rt_EndSkip > (IPTR)ptr)
			?   (UWORD *)res->rt_EndSkip - 2
			:   ptr;

		if ((IPTR)ptr & 0x01)
		   ptr = (UWORD *)((IPTR)ptr+1);
	    }

	    /* Get next address... */
	    ptr++;
	} while (ptr < (UWORD*)end);
    }

    /* Terminate the list */
    RomTag[num] = NULL;

    /* Seal our used memory as allocated */
    krnAllocBootMem(mh, (num + 1) * sizeof(struct Resident *));

    /*
     * By now we have valid list of kickstart resident modules.
     *
     * Now, we will have to analyze used-defined RomTags (via KickTagPtr and
     * KickMemPtr)
     */
    /* TODO: Implement external modules! */

    /*
     * Building list is complete, sort RomTags according to their priority.
     * I use BubbleSort algorithm.
     */
    do
    {
    	sorted = TRUE;

    	for (i = 0; i < num - 1; i++)
    	{
    	    if (RomTag[i]->rt_Pri < RomTag[i+1]->rt_Pri)
    	    {
    	    	struct Resident *tmp;

    	    	tmp = RomTag[i+1];
    	    	RomTag[i+1] = RomTag[i];
    	    	RomTag[i] = tmp;

    	    	sorted = FALSE;
    	    }
    	}
    } while (!sorted);

#ifdef PRINT_LIST
    bug("Resident modules (addr: pri version name):\n");
    for (i = 0; i < num; i++)
    {
        bug("+ %p: %4d %3d \"%s\"\n",
            RomTag[i],
            RomTag[i]->rt_Pri,
            RomTag[i]->rt_Version,
            RomTag[i]->rt_Name);
    }
#endif

    return RomTag;
}
