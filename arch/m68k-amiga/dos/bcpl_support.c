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
 * Set up the process's initial global vector
 */
BOOL BCPL_Setup(struct Process *me, BPTR segList, APTR DOSBase)
{
    ULONG *segment;
    ULONG *GlobVec;
    int segs = 0;

    for (segment = BADDR(segList); segment != NULL; segment = BADDR(segment[0])) {
    	segs++;
    }

    /* BCPL programs have only two segments */
    if (segs != 2)
    	return TRUE;

    GlobVec = AllocVec(sizeof(BCPL_GlobVec), MEMF_ANY);
    if (GlobVec == NULL)
    	return FALSE;

    CopyMem(BCPL_GlobVec, GlobVec, sizeof(BCPL_GlobVec));

    GlobVec[0] = 4;
    GlobVec[1] = (ULONG)-1;	/* 'system' segment */
    GlobVec[2] = (ULONG)-2;	/* 'dosbase' segment */
    GlobVec[3] = 0;
    GlobVec[4] = 0;
    GlobVec[5] = segList;

    GlobVec = ((APTR)GlobVec) + BCPL_GlobVec_NegSize;
    GlobVec[0] = BCPL_GlobVec_PosSize >> 2;
    me->pr_GlobVec = GlobVec;
    return TRUE;
}

void BCPL_Cleanup(struct Process *me)
{
    ULONG *GlobVec = me->pr_GlobVec;
   
    if (GlobVec == NULL)
    	return;

    GlobVec = ((APTR)GlobVec) - BCPL_GlobVec_NegSize;
    FreeVec(GlobVec);
    me->pr_GlobVec = NULL;
}

BOOL BCPL_InstallSeg(BPTR seg, ULONG *GlobVec)
{
    ULONG *segment;
    ULONG *table;
    ULONG *pr_GlobVec = ((struct Process *)FindTask(NULL))->pr_GlobVec;

    if (seg == BNULL) {
    	D(bug("BCPL_InstallSeg: Empty segment\n"));
    	return TRUE;
    }

    if (seg == (ULONG)-1) {
    	ULONG slots = GlobVec[0];
    	int i;
    	if (slots > (BCPL_GlobVec_PosSize>>2))
    	    slots = (BCPL_GlobVec_PosSize>>2);
    	D(bug("BCPL_InstallSeg: Inserting %d Faux system entries.\n", slots));

	/* Copy over the negative entries from the process's global vector */
	CopyMem(&pr_GlobVec[-(BCPL_GlobVec_NegSize>>2)], &GlobVec[-(BCPL_GlobVec_NegSize>>2)], BCPL_GlobVec_NegSize);

    	for (i = 2; i < slots; i++) {
    	    ULONG gv = pr_GlobVec[i];
    	    if (gv == 0)
    	    	continue;

    	    GlobVec[i] = gv;
    	}

    	return TRUE;
    }

    if (seg == (ULONG)-2) {
    	D(bug("BCPL_InstallSeg: Inserting DOSBase global\n"));
    	GlobVec[BCPL_DOSBase >> 2] = (IPTR)OpenLibrary("dos.library",0);
    	return TRUE;
    }

    segment = BADDR(seg);
    D(bug("BCPL_InstallSeg: SegList @%p\n", segment));

    if ((segment[-1] < segment[1])) {
    	D(bug("BCPL_InstallSeg: segList @%p does not look like BCPL.\n", segment));
    	return FALSE;
    }

    table = &segment[segment[1]];

    D(bug("\tFill in for %p:\n", segment));

    for (; table[-1] != 0; table = &table[-2]) {
    	D(bug("\t GlobVec[%d] = %p\n", table[-2], (APTR)&segment[1] + table[-1]));
    	GlobVec[table[-2]] = (ULONG)((APTR)&segment[1] + table[-1]);
    }

    return TRUE;
}
