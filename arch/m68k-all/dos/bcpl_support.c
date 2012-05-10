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
#include <dos/dostags.h>

#include <proto/dos.h>
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

const ULONG BCPL_GlobVec[(BCPL_GlobVec_NegSize + BCPL_GlobVec_PosSize) >> 2] = {
#include "bcpl.inc"
};
#undef BCPL

#define BCPL_ENTRY(proc)        (((APTR *)(proc)->pr_GlobVec)[1])
/*
 * Set up the process's initial global vector
 */
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

    while (seg) {
        segment = BADDR(seg);
        seg = (BPTR)segment[0]; /* next segment */

        if ((segment[-1] < segment[1])) {
            D(bug("BCPL_InstallSeg: segList @%p does not look like BCPL.\n", segment));
            continue;
        }

        D(bug("BCPL_InstallSeg: SegList @%p\n", segment));
        table = &segment[segment[1]];

        D(bug("\tFill in for %p:\n", segment));

        for (; table[-1] != 0; table = &table[-2]) {
            D(bug("\t globvec[%d] = %p\n", table[-2], (APTR)&segment[1] + table[-1]));
            globvec[table[-2]] = (ULONG)((APTR)&segment[1] + table[-1]);
        }
    }

    return DOSTRUE;

}

/* Create the global vector for a process
 */
BOOL BCPL_AllocGlobVec(struct Process *me)
{
    APTR globvec;
    int i;
    ULONG *seglist = BADDR(me->pr_SegList);

    globvec = AllocMem(sizeof(BCPL_GlobVec), MEMF_ANY | MEMF_CLEAR);
    if (globvec == NULL)
        return FALSE;

    globvec += BCPL_GlobVec_NegSize;
    ((ULONG *)globvec)[0] = BCPL_GlobVec_PosSize >> 2;

    /* Install the segments into the Global Vector */
    for (i = 0; i < seglist[0]; i++) {
        BCPL_InstallSeg(seglist[i+1], globvec);
    }

    me->pr_GlobVec = globvec;

    return TRUE;
}

void BCPL_FreeGlobVec(struct Process *me)
{
    APTR globvec = me->pr_GlobVec;
    struct DosLibrary *DOSBase;

    DOSBase = *(APTR *)(globvec + GV_DOSBase);
    D(bug("[BCPL_FreeGlobVec] Freed globvec %p\n", globvec));

    globvec -= BCPL_GlobVec_NegSize;
    FreeMem(globvec, sizeof(BCPL_GlobVec));

    me->pr_GlobVec = DOSBase->dl_GV;
}

void BCPL_Fixup(struct Process *me)
{
    BPTR *segment = BADDR(me->pr_SegList);

    if (segment[2] == (BPTR)0x0000abcd) {
        D(bug("[BCPL_Fixup] Fixing up overlay\n"));

   	/* overlayed executable, fun..
   	 * 2 = id
   	 * 3 = filehandle (BPTR)
   	 * 4 = overlay table (APTR)
   	 * 5 = hunk table (BPTR)
   	 * 6 = global vector (APTR)
   	 */
   	segment[6] = (ULONG)me->pr_GlobVec;
    }
}

extern void BCPL_thunk(void);

/* Under AOS, BCPL handlers expect the OS to build
 * their GlobalVector, and to receive a pointer to their
 * startup packet in D1.
 *
 * Both filesystem handlers and CLI shells use this routine.
 *
 * The 'Shell' shell is C based, and does not go here.
 *
 * This wrapper is here to support that.
 */
ULONG BCPL_RunHandler(void)
{
    struct DosPacket *dp;
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldReturnAddr;
    ULONG ret;

    WaitPort(&me->pr_MsgPort);
    dp = (struct DosPacket *)(GetMsg(&me->pr_MsgPort)->mn_Node.ln_Name);
    D(bug("[RunHandlerBCPL] Startup packet = %p\n", dp));

    if (!BCPL_AllocGlobVec(me)) {
        if (dp != NULL)
                internal_ReplyPkt(dp, &me->pr_MsgPort, DOSFALSE, ERROR_NO_FREE_STORE);
        return ERROR_NO_FREE_STORE;
    }

    D(bug("[RunHandlerBCPL] BCPL_ENTRY = %p\n", BCPL_ENTRY(me)));

    oldReturnAddr = me->pr_ReturnAddr;
    ret = AROS_UFC8(ULONG, BCPL_thunk,
            AROS_UFCA(ULONG,  MKBADDR(dp), D1),
            AROS_UFCA(ULONG,  0, D2),
            AROS_UFCA(ULONG,  0, D3),
            AROS_UFCA(ULONG,  0, D4),
            AROS_UFCA(APTR, me->pr_Task.tc_SPLower, A1),
            AROS_UFCA(APTR, me->pr_GlobVec, A2),
            AROS_UFCA(APTR, &me->pr_ReturnAddr, A3),
            AROS_UFCA(LONG_FUNC, BCPL_ENTRY(me), A4));
    me->pr_ReturnAddr = oldReturnAddr;

    BCPL_FreeGlobVec(me);

    return ret;
}

/* Create the necessary process wrappings for a BCPL 
 * segment. Only needed by Workbench's C:Run, C:NewCLI,
 * C:NewShell, and a few other applications.
 */
struct MsgPort *BCPL_CreateProcBCPL(struct DosLibrary *DOSBase, CONST_STRPTR name, BPTR *segarray, ULONG stacksize, LONG pri)
{
    struct Process *proc, *me = (struct Process *)FindTask(NULL);

    D(bug("[BCPL_CreateProcBCPL] Window=%p name=\"%s\", segArray=%p, stacksize=%u, pri=%d\n", me->pr_WindowPtr, name, segarray, stacksize, pri));

    proc = CreateNewProcTags(
            NP_Name, name,
            NP_Entry, BCPL_RunHandler,
            NP_Input, BNULL,
            NP_Output, BNULL,
            NP_Error, BNULL,
            NP_CloseInput, FALSE,
            NP_CloseOutput, FALSE,
            NP_CloseError, FALSE,
            NP_StackSize, stacksize,
            NP_WindowPtr, me->pr_WindowPtr,
            NP_CurrentDir, 0,
            NP_Cli, TRUE,
            NP_HomeDir, 0,
            TAG_END);

    /* Fix up the segarray before the first packet gets
     * to it.
     */
    if (proc) {
        BPTR *oldsegarray;

        oldsegarray = BADDR(proc->pr_SegList);
        if (oldsegarray[0] < segarray[0]) {
            FreeVec(oldsegarray);
            oldsegarray = AllocVec(sizeof(BPTR)*(segarray[0]+1), MEMF_PUBLIC | MEMF_CLEAR);
            oldsegarray[0] = segarray[0];
            oldsegarray[1] = (BPTR)-1;
            oldsegarray[2] = (BPTR)-2;
        }
        CopyMem(&segarray[3], &oldsegarray[3], (oldsegarray[0]-2)*sizeof(BPTR));

    }

    return proc ? &proc->pr_MsgPort : NULL;
}

void bcpl_command_name(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli = BADDR(me->pr_CLI);

    if (cli == NULL || cli->cli_Module == BNULL)
        bug("%s: ", me->pr_Task.tc_Node.ln_Name);
    else
        bug("%b: ", cli->cli_CommandName);
}

