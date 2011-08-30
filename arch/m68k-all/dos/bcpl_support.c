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

#define BCPL_SlotCount  (BCPL_GlobVec_PosSize>>2)

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

    /* Set default BCPL GlobVec */
    me->pr_GlobVec = ((struct DosLibrary *)DOSBase)->dl_GV;

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
    	globvec[GV_DOSBase >> 2] = (IPTR)OpenLibrary("dos.library",0); 

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

extern void BCPL_thunk(void);

/* Under AOS, BCPL handlers expect the OS to build
 * their GlobalVector, and to receive a pointer to their
 * startup packet in D1.
 *
 * This wrapper is here to support that.
 */
void BCPL_RunHandler(void)
{
    struct DosPacket *dp;
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DeviceNode *dn;
    APTR entry, globvec;
    int i;
    ULONG *seglist = BADDR(me->pr_SegList);

    WaitPort(&me->pr_MsgPort);
    dp = (struct DosPacket *)(GetMsg(&me->pr_MsgPort)->mn_Node.ln_Name);
    D(bug("[RunHandlerBCPL] Startup packet = %p\n", dp));

    dn = BADDR(dp->dp_Arg3);
    entry = BADDR(dn->dn_SegList) + sizeof(IPTR);

    globvec = AllocMem(sizeof(BCPL_GlobVec), MEMF_ANY | MEMF_CLEAR);
    if (globvec == NULL) {
        internal_ReplyPkt(dp, &me->pr_MsgPort, DOSFALSE, ERROR_NO_FREE_STORE);
        return;
    }

    globvec += BCPL_GlobVec_NegSize;
    ((ULONG *)globvec)[0] = BCPL_GlobVec_PosSize >> 2;

    /* Install the segments into the Global Vector */
    for (i = 0; i < seglist[0]; i++) {
        BCPL_InstallSeg(seglist[i+1], globvec);
    }

    me->pr_GlobVec = globvec;

    /* AOS File Handlers are BCPL programs *without*
     * the standard CLI startup routine, so they have
     * to be called as if they were BCPL subroutines.
     *
     * Instead of cluttering up CallEntry() with yet
     * another special case, just directly jump here.
     *
     * We can do this, especially because we know
     * that they won't call Dos/Exit() on startup.
     *
     * On m68k, AROS_UFC doesn't work well when the
     * frame pointer is an argument, so we're going to call
     * a thunk for the A5/A6 BCPL jsr/rts
     */
    AROS_UFC9(ULONG, BCPL_thunk,
            AROS_UFCA(ULONG, 16, D0),
            AROS_UFCA(BPTR, MKBADDR(dp), D1),
            AROS_UFCA(ULONG, 0, D2),
            AROS_UFCA(ULONG, 0, D3),
            AROS_UFCA(ULONG, 0, D4),
            AROS_UFCA(APTR, NULL, A0),
            AROS_UFCA(APTR, me->pr_Task.tc_SPLower + 16, A1),
            AROS_UFCA(APTR, me->pr_GlobVec, A2),
            AROS_UFCA(APTR, entry + sizeof(ULONG), A4));

    globvec -= BCPL_GlobVec_NegSize;
    FreeMem(globvec, sizeof(BCPL_GlobVec));
    me->pr_GlobVec = NULL;

    return;
}

