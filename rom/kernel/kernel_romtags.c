/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    ROMTag scanner.
*/

#include <aros/debug.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/alib.h>
#include <proto/exec.h>

#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_romtags.h>

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
 * Our own, very simplified down memory allocator.
 * It is okay to use this routine because this is one of the first allocations (actually
 * should be the first one) and it is never going to be freed.
 * Additionally, in order to be able to discover exec.library (and its early init function)
 * dynamically, we want krnRomTagScanner() to work before ExecBase is created.
 */
static inline void krnAllocBootMem(struct MemHeader *mh, struct MemChunk *next, IPTR chunkSize, IPTR allocSize)
{
    allocSize = AROS_ROUNDUP2(allocSize, MEMCHUNK_TOTAL);

    mh->mh_First          = (struct MemChunk *)((APTR)mh->mh_First + allocSize);
    mh->mh_First->mc_Next = next;
    mh->mh_Free          -= allocSize;
    mh->mh_First->mc_Bytes = chunkSize - allocSize;
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
     * is started up. However in some situations we can already have several
     * MemChunks in our MemHeader (this happens on softkicked m68k Amiga).
     * So, we preserve origiinal chunk size and next chunk pointer.
     * Note that we expect the first chunk in the MemHeader to have enough space for resident list.
     */
    struct MemChunk *nextChunk = mh->mh_First->mc_Next;
    IPTR chunkSize = mh->mh_First->mc_Bytes;
    IPTR limit = chunkSize / sizeof(APTR);
    struct Resident **RomTag = (struct Resident **)mh->mh_First;
    ULONG	    num = 0;

    /* Look in whole kickstart for resident modules */
    while (*ranges != (UWORD *)~0)
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
    krnAllocBootMem(mh, nextChunk, chunkSize, (num + 1) * sizeof(struct Resident *));

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

struct ExecBase *krnPrepareExecBase(UWORD *ranges[], struct MemHeader *mh, struct TagItem *bootMsg)
{
    struct Resident *exec; 
    struct ExecBase *sysBase;
    struct Resident **resList = krnRomTagScanner(mh, ranges);

    /* krnRomTagScanner() never fails */

    exec = krnFindResident(resList, "exec.library");
    if (!exec)
    {
	krnPanic("Failed to create ExecBase\n"
		 "exec.library is not found");
    	return NULL;
    }

    /* Magic. Described in rom/exec/exec_init.c. */
    sysBase = AROS_UFC3(struct ExecBase *, exec->rt_Init,
                	AROS_UFCA(struct MemHeader *, mh, D0),
                	AROS_UFCA(struct TagItem *, bootMsg, A0),
 	                AROS_UFCA(struct ExecBase *, NULL, A6));

    if (!sysBase)
    {
	krnPanic("Failed to create ExecBase\n"
		 "MemHeader 0x%p, First chunk 0x%p, %u bytes free",
		 mh, mh->mh_First, mh->mh_Free);

	return NULL;
    }

    sysBase->ResModules = resList;

#ifndef NO_RUNTIME_DEBUG
    /* Print out modules list if requested by the user */
    if (SysBase->ex_DebugFlags & EXECDEBUGF_INITCODE)
    {
	ULONG i;

	bug("Resident modules (addr: pri flags version name):\n");

	for (i = 0; resList[i]; i++)
	{
	    bug("+ %p: %4d %02x %3d \"%s\"\n", resList[i], resList[i]->rt_Pri,
		resList[i]->rt_Flags, resList[i]->rt_Version, resList[i]->rt_Name);
	}
    }
#endif
    return sysBase;
}
