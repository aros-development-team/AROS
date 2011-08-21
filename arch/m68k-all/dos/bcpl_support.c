/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL support
    Lang: english
*/
#define DEBUG 0
#include <aros/debug.h>
#include <aros/asmcall.h>

#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include "bcpl.h"

/* Externs */
extern void BCPL_dummy(void);
#define BCPL(id, name)	extern void BCPL_##name(void);
#include "bcpl.inc"
#undef BCPL

#define BCPL_SlotCount  (BCPL_GlobVec_PosSize<<2)

/* Default Global Vector */
#define BCPL(id, name) \
	[(BCPL_GlobVec_NegSize + id)>>2] = (ULONG)BCPL_##name,

const ULONG BCPL_GlobVec[BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize] = {
#include "bcpl.inc"
};
#undef BCPL

/*
 * Set up the process's initial global vector
 */
#define SEGLIST_SIZE (6)
#define FAKESEG_SIZE (4)

/* Set DOSBase to non-NULL for a BCPL setup
 * with a valid GlobVec, and NULL for a setup
 * without a GlobVec.
 */
APTR BCPL_Setup(struct Process *me, BPTR segList, APTR entry, APTR DOSBase)
{
    ULONG *segment;

    segment = AllocVec(SEGLIST_SIZE * sizeof(ULONG), MEMF_ANY | MEMF_CLEAR);
    if (segment == NULL)
    	return NULL;

    /* create fake seglist if only entrypoint was given.
     * This fake SegList will be automatically unloaded
     * when the progam exits.
     */
    if (entry && !segList) {
    	segList = CreateSegList(entry);
    	D(bug("fakeseglist @%p\n", BADDR(segList)));
    	entry = NULL;

    	/* Make sure to free the seglist when we're done */
    	me->pr_Flags |= PRB_FREESEGLIST;
    }
    if (!entry)
    	entry = (APTR)((BPTR*)BADDR(segList) + 1);

    segment[0] = 4;
    segment[1] = (ULONG)-1;	/* 'system' segment */
    segment[2] = (ULONG)-2;	/* 'dosbase' segment */
    segment[3] = segList;
    segment[4] = 0;
    segment[5] = segList;

    me->pr_SegList = MKBADDR(segment);

    segment = BADDR(segList);
    if (segment[2] == 0x0000abcd) {
   	/* overlayed executable, fun..
   	 * 2 = id
   	 * 3 = filehandle (BPTR)
   	 * 4 = overlay table (APTR)
   	 * 5 = hunk table (BPTR)
   	 * 6 = global vector (APTR)
   	 */
   	 segment[6] = (ULONG)me->pr_GlobVec;
    }
    D(bug("BCPL_Setup '%s' @%p (%s)\n", me->pr_Task.tc_Node.ln_Name));
    return entry;
}

void BCPL_Cleanup(struct Process *me)
{
    FreeVec(BADDR(me->pr_SegList));
    me->pr_SegList = BNULL;
    me->pr_GlobVec = NULL;
}

ULONG BCPL_InstallSeg(BPTR seg, ULONG *globvec)
{
    ULONG *segment;
    ULONG *table;

    if (seg == BNULL) {
    	D(bug("BCPL_InstallSeg: Empty segment\n"));
    	return DOSTRUE;
    }

    if (seg == (ULONG)-1) {
    	ULONG slots = globvec[0];
    	int i;
    	if (slots > (BCPL_GlobVec_PosSize>>2))
    	    slots = (BCPL_GlobVec_PosSize>>2);
    	D(bug("BCPL_InstallSeg: Inserting %d Faux system entries.\n", slots));

	/* Copy over the negative entries from the default global vector */
	CopyMem(&BCPL_GlobVec[0], &globvec[-(BCPL_GlobVec_NegSize>>2)], BCPL_GlobVec_NegSize);

    	for (i = 2; i < slots; i++) {
    	    ULONG gv = BCPL_GlobVec[(BCPL_GlobVec_NegSize>>2) + i];
    	    if (gv == 0)
    	    	continue;

    	    globvec[i] = gv;
    	}

    	D(bug("BCPL_InstallSeg: Inserting DOSBase global\n"));
    	globvec[BCPL_DOSBase >> 2] = (IPTR)OpenLibrary("dos.library",0); 

    	return DOSTRUE;
    }

    if (seg == (ULONG)-2) {
    	return DOSTRUE;
    }

    while (seg != BNULL) {
	segment = BADDR(seg);
	D(bug("BCPL_InstallSeg: SegList @%p\n", segment));

	if ((segment[-1] < segment[1])) {
	    D(bug("BCPL_InstallSeg: segList @%p does not look like BCPL.\n", segment));
	    return DOSTRUE;
	}

	table = &segment[segment[1]];

	D(bug("\tFill in for %p:\n", segment));

	for (; table[-1] != 0; table = &table[-2]) {
	    D(bug("\t globvec[%d] = %p\n", table[-2], (APTR)&segment[1] + table[-1]));
	    globvec[table[-2]] = (ULONG)((APTR)&segment[1] + table[-1]);
	}

	seg = segment[0];
    }

    return DOSTRUE;

}
