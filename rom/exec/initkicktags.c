/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: initkicktags.c

    Desc: Handle CoolCapture and KickTags (reset proof residents)
    Lang: english
*/

#if AROS_SERIAL_DEBUG
#define PRINT_LIST
#define DEBUG 1
#endif

#include <aros/debug.h>
#include <exec/rawfmt.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include "exec_debug.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"


/* I don't think KickTag merge can be implemented without ugly hacks.. */

static IPTR *CopyResidents(IPTR *list, IPTR *dst, IPTR *oldlist)
{
    struct Resident *RomTag;

    while(*list)
    {
	IPTR *oldresident;

	if (*list & RESLIST_NEXT)
	{
	    list = (IPTR *)(*list & ~RESLIST_NEXT);
	    continue;
	}

	RomTag = (struct Resident*)*list;

#ifdef PRINT_LIST
	bug("* %p: %4d %02x %3d \"%s\"\n",
            RomTag,
            RomTag->rt_Pri,
            RomTag->rt_Flags,
            RomTag->rt_Version,
            RomTag->rt_Name);
#endif

	/* Try to find a resident with this name in original list */
	oldresident = InternalFindResident(RomTag->rt_Name, oldlist);
	if (oldresident)
	{
	    /*
	     * If found, check version.
	     * The resident is replaced if:
	     * a) Version of new resident is greater than version of old one.
	     * b) Versions are equal, but priority of new resident is greater than priority of old one.
	     */
	    struct Resident *OldRomTag = (struct Resident *)*oldresident;

	    if ((OldRomTag->rt_Version >= RomTag->rt_Version) ||
		(OldRomTag->rt_Version == RomTag->rt_Version && OldRomTag->rt_Pri >= RomTag->rt_Pri))
	    {
	    	oldresident = NULL;
	    }
	}
	
	if (oldresident)
	    *oldresident = *list;
	else
	    *dst++ = *list;

	list++;
    }
    
    /* Terminate the list */
    *dst = 0;
    return dst;
}

/* Count a number of residents in the list */
static int CountResidents(IPTR *list)
{
    int cnt = 0;

    while(*list)
    {
	if (*list & RESLIST_NEXT)
	{
	    list = (IPTR *)(*list & ~RESLIST_NEXT);
	    continue;
	}

	cnt++;
	list++;
    }
    return cnt;
}

static void SortResidents(IPTR *list)
{
    BOOL sorted;
    int i, num;
    struct Resident **RomTag = (struct Resident**)list;

    num = CountResidents(list);
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
}

/*
 * Residents in our list must always be sorted by priority.
 * In order to maintain this, we can't just append new residents to the end of our list.
 * We have to build a complete new list.
 */
static void AddToResidentList(IPTR *list)
{
    IPTR *newlist, *tmplist;
    int oldcnt = CountResidents(SysBase->ResModules);
    int addcnt = CountResidents(list);

    /* Allocate space for the new list */
    newlist = AllocMem((oldcnt + addcnt + 1) * sizeof(struct Resident*), MEMF_PUBLIC);
    if (!newlist)
    	return;

    /* Merge two lists and sort. */
    tmplist = CopyResidents(SysBase->ResModules, newlist, NULL);
    CopyResidents(list, tmplist, SysBase->ResModules);
    SortResidents(newlist);

    /*
     * Replace the list.
     * We just drop the old list without deallocation because noone really knows if it's safe
     * to deallocate it. With future page-based memory allocator it will certainly be not.
     */
    SysBase->ResModules = newlist;

#ifndef NO_RUNTIME_DEBUG
    if (SysBase->ex_DebugFlags & EXECDEBUGF_INITCODE)
    {
    	int i;
    
    	DINITCODE("Resident modules after KickTags merge:");

	for (i = 0; i < addcnt + oldcnt; i++)
	{
    	    struct Resident *RomTag = (struct Resident*)newlist[i];

            NewRawDoFmt("+ %p: %4ld %02x %3ld \"%s\"\n", (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL,
		        RomTag, RomTag->rt_Pri, RomTag->rt_Flags, RomTag->rt_Version, RomTag->rt_Name);
	}
    }
#endif
}

void InitKickTags(struct ExecBase *SysBase)
{
    ULONG chk = (ULONG)(IPTR)SysBase->KickCheckSum;
    ULONG chkold = SumKickData();
    struct MemList *ml = (struct MemList*)SysBase->KickMemPtr;

    DINITCODE("kickmemptr=0x%p kicktagptr=0x%p kickchecksum=0x%08lx", SysBase->KickMemPtr, SysBase->KickTagPtr, chk);

    if (chkold != chk)
    {
    	DINITCODE("Kicktag checksum mismatch %08lx!=%08lx", chkold, chk);

    	SysBase->KickMemPtr = NULL;
    	SysBase->KickTagPtr = NULL;
    	SysBase->KickCheckSum = 0;

    	return;
    }

    /*
     * Before we do anything else, we need to lock down the entries in KickMemPtr
     * If we get a single failure, don't run any of the KickTags.
     */
    while (ml) /* single linked! */
    {
    	UWORD i;

	DINITCODE("KickMemList 0x%p, NumEntries: %u", ml->ml_NumEntries);
    	for (i = 0; i < ml->ml_NumEntries; i++)
    	{
    	    DINITCODE(" + Addr 0x%p, Len %u", ml->ml_ME[i].me_Addr, ml->ml_ME[i].me_Length);

    	    /* 
    	     * Use the non-Munwalling AllocAbs, since regions may be consecutive.
    	     * Mungwall headers can trash them in this case.
    	     */
    	    if (!InternalAllocAbs(ml->ml_ME[i].me_Addr, ml->ml_ME[i].me_Length, SysBase))
    	    {
		DINITCODE("KickMem allocation failed");
		/* Should we free already allocated KickMem lists? */
 	    	return;
 	    }
    	}
    	ml = (struct MemList*)ml->ml_Node.ln_Succ;
    }

    if (SysBase->KickTagPtr)
    {
    	AddToResidentList(SysBase->KickTagPtr);
    }
}
