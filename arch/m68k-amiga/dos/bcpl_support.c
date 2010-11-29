/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL support
    Lang: english
*/
#define DEBUG 1
#include <aros/debug.h>

#include <dos/dosextens.h>
#include <proto/exec.h>

#include "bcpl.h"

/* Externs */
extern void BCPL_dummy(void);
#define BCPL(id, name)	extern void BCPL_##name(void);
#include "bcpl.inc"
#undef BCPL

/* Default Global Vector */
#define BCPL(id, name) \
	[(BCPL_GlobVec_NegSize + id)>>2] = (ULONG)BCPL_##name,

const ULONG BCPL_GlobVec[BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize] = {
#include "bcpl.inc"
};
#undef BCPL

/*
 * Patch up a segList that looks like BCPL
 *
 * We define 'looks like BCPL' as having a segment where:
 *   - *(ULONG)BADDR(segList+1) < *(ULONG)BADDR(segList-1)
 */
BOOL BCPL_Setup(struct Process *me, BPTR segList, APTR DOSBase)
{
    ULONG *segment;
    ULONG *table;
    ULONG slots = BCPL_GlobVec_PosSize;
    ULONG *GlobVec;
    ULONG oldsize;
    BOOL is_bcpl = FALSE;

    for (segment = BADDR(segList); segment != NULL; segment = BADDR(segment[0])) {
    	if ((segment[-1] > segment[1]))
    	    is_bcpl = TRUE;
    }

    if (!is_bcpl) {
	D(bug("BCPL_Fixup: segList @%p does not look like BCPL.\n", segment));
	me->pr_GlobVec = NULL;
	return 0;
    }

    D(bug("BCPL_Fixup: segList @%p looks like BCPL.\n", segment));

    /* Find the maximum # of slots we need to allocate */
    for (segment = BADDR(segList); segment != NULL; segment = BADDR(segment[0])) {
    	if ((segment[-1] <= segment[1]))
    	    continue;

    	table = &segment[segment[1]];

    	D(bug("\t%p Slots: %d\n", segment, table[0]));
    	if (slots < table[0]) 
    	    slots = table[0];
    }

    D(bug("\tMax Slots: %d\n", slots));

    GlobVec = AllocVec(BCPL_GlobVec_NegSize+slots, 0);
    if (GlobVec == NULL)
    	return -1;

    /* Use the current Global Vector if we have one */
    if (me->pr_GlobVec) {
    	oldsize = (*(ULONG *)(me->pr_GlobVec));
    	CopyMem(me->pr_GlobVec - BCPL_GlobVec_NegSize, GlobVec, BCPL_GlobVec_NegSize + oldsize*4);
    } else {
    	oldsize =  BCPL_GlobVec_PosSize;
    	CopyMem(BCPL_GlobVec, GlobVec, BCPL_GlobVec_NegSize + oldsize);
    }
    D(bug("\tGlobal Vector @%p (real base is %p)\n", ((APTR)GlobVec) + BCPL_GlobVec_NegSize, GlobVec));
    GlobVec = ((APTR)GlobVec) + BCPL_GlobVec_NegSize;

    /* Zero-fill any 'new' slots */
    while (oldsize < slots)
    	GlobVec[oldsize++] = 0;

    me->pr_GlobVec = GlobVec;

    /* A few manual fixups.. */
    GlobVec[0x170 >> 2] = (IPTR)OpenLibrary("intuition.library", 0);
    GlobVec[BCPL_DOSBase >> 2] = (IPTR)DOSBase;

    for (segment = BADDR(segList); segment != NULL; segment = BADDR(segment[0])) {
    	if ((segment[-1] <= segment[1]))
    	    continue;

    	table = &segment[segment[1]];

    	D(bug("\tFill in for %p:\n", segment));

    	for (; table[-1] != 0; table = &table[-2]) {
    	    D(bug("\t GlobVec[%d] = %p\n", table[-2], (APTR)&segment[1] + table[-1]));
    	    GlobVec[table[-2]] = (ULONG)((APTR)&segment[1] + table[-1]);
    	}
    }

    D(bug("\tBCPL Entry point: %p\n", GlobVec[1]));

    return 1;
}

void BCPL_Cleanup(struct Process *me)
{
    ULONG *GlobVec = me->pr_GlobVec;
   
    if (GlobVec == NULL)
    	return;

    CloseLibrary((APTR)(GlobVec[0x170 >> 2]));

    GlobVec = ((APTR)GlobVec) - BCPL_GlobVec_NegSize;
    FreeVec(GlobVec);
    me->pr_GlobVec = NULL;
}
