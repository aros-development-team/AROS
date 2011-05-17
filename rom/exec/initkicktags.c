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
#include <exec/resident.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "memory.h"

#ifdef __mc68000__
#define NEXTRESIDENT(list) \
	if(*list & 0x80000000) { list = (IPTR *)(*list & 0x7fffffff); continue; }
#else
#define NEXTRESIDENT(list) \
        if(*list & 0x1) { list = (IPTR *)(*list & ~(IPTR)0x1); continue; }
#endif

/* I don't think KickTag merge can be implemented without ugly hacks.. */


static IPTR *FindOldResident(IPTR *oldlist, struct Resident *RomTag)
{
    IPTR *oldlisttmp = oldlist;
    if (!oldlist)
    	return NULL;
    while (*oldlisttmp) {
	struct Resident *OldRomTag;
	NEXTRESIDENT(oldlisttmp);
	OldRomTag = (struct Resident*)*oldlisttmp;
	if (!strcmp(OldRomTag->rt_Name, RomTag->rt_Name)) {
	    if ((OldRomTag->rt_Version < RomTag->rt_Version) ||
		(OldRomTag->rt_Version == RomTag->rt_Version && OldRomTag->rt_Pri <= RomTag->rt_Pri))
		return oldlisttmp;
	}
	oldlisttmp++;
    }
    return NULL;
}

static IPTR *CopyResidents(IPTR *list, IPTR *dst, IPTR *oldlist)
{
    struct Resident *RomTag;
    while(*list)
    {
	IPTR *oldresident;
	NEXTRESIDENT(list);
	RomTag = (struct Resident*)*list;
#ifdef PRINT_LIST
	bug("* %p: %4d %02x %3d \"%s\"\n",
            RomTag,
            RomTag->rt_Pri,
            RomTag->rt_Flags,
            RomTag->rt_Version,
            RomTag->rt_Name);
#endif
	oldresident = FindOldResident(oldlist, RomTag);
	if (oldresident)
	    *oldresident = *list;
	else
	    *dst++ = *list;
	list++;
    }
    *dst = 0;
    return dst;
}

static int CountResidents(IPTR *list)
{
    int cnt = 0;
    while(*list)
    {
	NEXTRESIDENT(list);
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

static void AddToResidentList(IPTR *list)
{
    IPTR *newlist, *tmplist;
    int oldcnt = CountResidents(SysBase->ResModules);
    int addcnt = CountResidents(list);
#ifdef PRINT_LIST
    int i;
#endif
    
    newlist = AllocMem((oldcnt + addcnt + 1) * sizeof(struct Resident*), MEMF_PUBLIC);
    if (!newlist)
    	return;
    tmplist = CopyResidents(SysBase->ResModules, newlist, NULL);
#ifdef PRINT_LIST
    bug("KickTag residents:\n");
#endif
    CopyResidents(list, tmplist, SysBase->ResModules);
    SortResidents(newlist);
    /* Redirect InitCode() loop to new list */
    /* We assume we got here between SINGLETASK and COLDSTART */
    tmplist = SysBase->ResModules;
    while (*tmplist) {
	NEXTRESIDENT(tmplist);
#ifdef __mc68000__
	*tmplist++ = (IPTR)(0x80000000 | (IPTR)newlist);
#else
	*tmplist++ = (IPTR)(0x00000001 | (IPTR)newlist);
#endif
    }
    SysBase->ResModules = newlist;
#ifdef PRINT_LIST
    bug("Resident modules after KickTags merge:\n");
    for (i = 0; i < addcnt + oldcnt; i++) {
    	struct Resident *RomTag = (struct Resident*)newlist[i];
        bug("+ %p: %4d %02x %3d \"%s\"\n",
            RomTag,
            RomTag->rt_Pri,
            RomTag->rt_Flags,
            RomTag->rt_Version,
            RomTag->rt_Name);
    }
#endif
}

void InitKickTags(void)
{
    ULONG chk = (ULONG)(IPTR)SysBase->KickCheckSum;
    ULONG chkold = SumKickData();
    struct MemList *ml = (struct MemList*)SysBase->KickMemPtr;

    D(bug("coolcapture=%p kickmemptr=%p kicktagptr=%p kickchecksum=%08x\n",
	SysBase->CoolCapture, SysBase->KickMemPtr, SysBase->KickTagPtr, chk));

    if (SysBase->CoolCapture) {
	AROS_UFC1(void, SysBase->CoolCapture,
            AROS_UFCA(struct Library *, (struct Library *)SysBase, A6));
    }
	
    if (chkold != chk) {
    	D(bug("Kicktag checksum mismatch %08x!=%08x\n", chkold, chk));
    	SysBase->KickMemPtr = NULL;
    	SysBase->KickTagPtr = NULL;
    	SysBase->KickCheckSum = 0;
    	return;
    }
    	
    while (ml) { /* single linked! */
    	UWORD i;
    	for (i = 0; i < ml->ml_NumEntries; i++) {
    	    D(bug("KickMem at %x len %d\n", ml->ml_ME[i].me_Un.meu_Addr, ml->ml_ME[i].me_Length));
    	    /* Use the non-Munwalling AllocAbs, since the regions
    	     * may be consecutive.
    	     */
    	    if (!InternalAllocAbs(ml->ml_ME[i].me_Addr, ml->ml_ME[i].me_Length, SysBase)) {
		D(bug("KickMem allocation failed\n"));
		/* Should we free already allocated KickMem lists? */
 	    	return;
 	    }
    	}
    	ml = (struct MemList*)ml->ml_Node.ln_Succ;
    }

    if (SysBase->KickTagPtr)
    	AddToResidentList(SysBase->KickTagPtr);
    
}
    
