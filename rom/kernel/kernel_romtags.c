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

#if AROS_SERIAL_DEBUG
#define PRINT_LIST
#endif

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
#define MAX_ROMTAGS	256

APTR krnRomTagScanner(struct MemHeader *mh, UWORD *ranges[])
{
    UWORD	    *end;
    UWORD	    *ptr;		/* Start looking here */
    struct Resident *res;               /* module found */
    ULONG	    i;
    BOOL	    sorted;
    struct Resident **RomTag;
    APTR	tmp;
    ULONG	num = 0;

    /* Look for a chunk that can hold at least MAX_ROMTAGS entries.
     */
    RomTag = stdAlloc(mh, MAX_ROMTAGS * sizeof(struct Resident *), MEMF_ANY, NULL);
    if (RomTag == NULL)
    	return NULL;

    /* Look in whole kickstart for resident modules */
    while ((*ranges != (UWORD *)~0) && (num < MAX_ROMTAGS))
    {
	ptr = *ranges++;
	end = *ranges++;

	D(bug("RomTagScanner: Start = %p, End = %p\n", ptr, end));
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

    /* Try to reduce our memory usage to exactly what we need. */
    tmp = stdAlloc(mh, (num + 1) * sizeof(struct Resident *), MEMF_ANY, NULL);
    if (tmp == NULL) {
    	/* We're going to hope that we'll get some more memory soon,
    	 * once Exec processes some of these RomTags...
    	 */
    } else {
	memcpy(tmp, RomTag, (num + 1) * sizeof(struct Resident *));
	stdDealloc(mh, RomTag, MAX_ROMTAGS * sizeof(struct Resident *), NULL);
	RomTag = tmp;
    }

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
    bug("Resident modules (addr: pri flags version name):\n");
    for (i = 0; i < num; i++)
    {
        bug("+ %p: %4d %02x %3d \"%s\"\n",
            RomTag[i],
            RomTag[i]->rt_Pri,
            RomTag[i]->rt_Flags,
            RomTag[i]->rt_Version,
            RomTag[i]->rt_Name);
    }
#endif

    return RomTag;
}
