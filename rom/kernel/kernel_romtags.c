/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    ROMTag scanner.
*/

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/memheaderext.h>
#include <proto/alib.h>
#include <proto/exec.h>

#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_romtags.h>

#define D(x)

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
 * Allocate memory space for boot-time usage. Returns address and size of the usable area.
 * It's strongly adviced to return enough space to store resident list of sane length.
 */
static APTR krnGetSysMem(struct MemHeader *mh, IPTR *size)
{
    if (mh->mh_Attributes & MEMF_MANAGED)
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

        if (mhe->mhe_Alloc)
        {
            *size = 1024*1024;
            return mhe->mhe_Alloc(mhe, *size, NULL);
        }
    }
    else
    {
        /* Just dequeue the first MemChunk. It's assumed that it has the required space for sure. */
        struct MemChunk *mc = mh->mh_First;

        mh->mh_First = mc->mc_Next;
        mh->mh_Free -= mc->mc_Bytes;

        D(bug("[RomTagScanner] Using chunk 0x%p of %lu bytes\n", mc, mc->mc_Bytes));

        *size = mc->mc_Bytes;
        return mc;
    }
}

/* Release unused boot-time memory */
static void krnReleaseSysMem(struct MemHeader *mh, APTR addr, IPTR chunkSize, IPTR allocSize)
{
    if (mh->mh_Attributes & MEMF_MANAGED)
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

        if (mhe->mhe_Realloc)
            mhe->mhe_Realloc(mhe, addr, allocSize);
    }
    else
    {
        struct MemChunk *mc;

        allocSize = AROS_ROUNDUP2(allocSize, MEMCHUNK_TOTAL);
        chunkSize -= allocSize;

        D(bug("[RomTagScanner] Chunk 0x%p, %lu of %lu bytes used\n", addr, allocSize, chunkSize));

        if (chunkSize < MEMCHUNK_TOTAL)
            return;

        mc = addr + allocSize;

        mc->mc_Next  = mh->mh_First;
        mc->mc_Bytes = chunkSize - allocSize;

        mh->mh_First = mc;
        mh->mh_Free += mc->mc_Bytes;
    }
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
     * We don't know resident list size until it's created. Because of this, we use two-step memory allocation
     * for this purpose.
     * First we dequeue some space from the MemHeader, and remember its starting address and size. Then we
     * construct resident list in this area. After it's done, we return part of the used space to the system.
     */
    IPTR chunkSize;
    struct Resident **RomTag = krnGetSysMem(mh, &chunkSize);
    IPTR  limit = chunkSize / sizeof(APTR);
    ULONG num = 0;

    if (!RomTag)
    	return NULL;

    /* Look in whole kickstart for resident modules */
    while (*ranges != (UWORD *)~0)
    {
	ptr = *ranges++;
	end = *ranges++;

        /* Make sure that addresses are UWORD-aligned. In some circumstances they can be not. */
        ptr = (UWORD *)(((IPTR)ptr + 1) & ~1);
        end = (UWORD *)((IPTR)end & ~1);

	D(bug("RomTagScanner: Start = %p, End = %p\n", ptr, end));
	do
	{
	    res = (struct Resident *)ptr;

	    /* Do we have RTC_MATCHWORD and rt_MatchTag*/
	    if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res)
	    {
		/* Yes, it is Resident module. Check if there is module with such name already */
		i = findname(RomTag, num, res->rt_Name);
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
		    /*
		     * 'limit' holds a length of our MemChunk in pointers.
		     * Actually it's a number or pointers we can safely store in it (including NULL terminator).
		     * If it's exceeded, return NULL.
		     * TODO: If ever needed, this routine can be made smarter. There can be
		     * the following approaches:
		     * a) Move the data to a next MemChunk which is bigger than the current one
		     *    and continue.
		     * b) In the beginning of this routine, find the largest available MemChunk and use it.
		     * Note that we exit with destroyed MemChunk here. Anyway, failure here means the system
		     * is completely unable to boot up.
		     */
		    if (num == limit)
		    	return NULL;
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
    krnReleaseSysMem(mh, RomTag, chunkSize, (num + 1) * sizeof(struct Resident *));

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

    return RomTag;
}

struct Resident *krnFindResident(struct Resident **resList, const char *name)
{
    ULONG i;

    for (i = 0; resList[i]; i++)
    {
    	if (!strcmp(resList[i]->rt_Name, name))
    	    return resList[i];
    }
    return NULL;
}
