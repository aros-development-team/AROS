/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Header for dos.library
    Lang: english
*/


#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <resources/filesysres.h>
#include <aros/debug.h>
#include LC_LIBDEFS_FILE
#include "dos_intern.h"

#ifdef __mc68000

/* LoadSeg() needs D1-D3 parameters for overlay hunk support */
AROS_UFP4(BPTR, LoadSeg_Overlay,
    AROS_UFPA(UBYTE*, name, D1),
    AROS_UFPA(BPTR, hunktable, D2),
    AROS_UFPA(BPTR, fh, D3),
    AROS_UFPA(struct DosLibrary *, DosBase, A6));

static void PatchDOS(struct DosLibrary *dosbase)
{
    UWORD highfunc = 37, lowfunc = 5, skipfuncs = 2;
    UWORD i;
    UWORD *asmcall;
    IPTR func;

    /* patch all 1.x dos functions to return value in both D1 and D0
     * For example most overlayed programs require this */
    asmcall = AllocMem(5 * (highfunc - lowfunc + 1 - skipfuncs) * sizeof(UWORD) + 12 * sizeof(UWORD), MEMF_PUBLIC);
    for (i = lowfunc; i <= highfunc; i++) {
    	if (i == 24 || i == 25)
    	    continue;
    	func = (IPTR)__AROS_GETJUMPVEC(dosbase, i)->vec;
    	asmcall[0] = 0x4eb9; // JSR
	asmcall[1] = (UWORD)(func >> 16);
	asmcall[2] = (UWORD)(func >>  0);
	asmcall[3] = 0x2200; // MOVE.L D0,D1
	asmcall[4] = 0x4e75; // RTS
 	__AROS_SETVECADDR(dosbase, i, asmcall);
 	asmcall += 5;
    }

    /* redirect LoadSeg() to LoadSeg_Overlay() if D1 == NULL */
    func = (IPTR)__AROS_GETJUMPVEC(dosbase, 25)->vec;
    asmcall[0] = 0x4a81; // TST.L D1
    asmcall[1] = 0x660a; // BNE.B 7 (D1 not NULL = normal LoadSeg)
    asmcall[2] = 0x4eb9; // JSR LoadSeg_Overlay
    asmcall[3] = (UWORD)((ULONG)LoadSeg_Overlay >> 16);
    asmcall[4] = (UWORD)((ULONG)LoadSeg_Overlay >>  0);
    asmcall[5] = 0x2200; // MOVE.L D0,D1
    asmcall[6] = 0x4e75; // RTS
    asmcall[7] = 0x4eb9; // JSR LoadSeg_Original
    asmcall[8] = (UWORD)(func >> 16);
    asmcall[9] = (UWORD)(func >>  0);
    asmcall[10] = 0x2200; // MOVE.L D0,D1
    asmcall[11] = 0x4e75; // RTS
    __AROS_SETVECADDR(dosbase, 25, asmcall);

    CacheClearU();
}

#else

#define PatchDOS(base)

#endif

static int DosInit(struct DosLibrary *LIBBASE)
{
    D(bug("DosInit\n"));
    
    IPTR * taskarray;
    struct DosInfo *dosinfo;
#ifdef AROS_DOS_PACKETS
    struct FileSysResource *fsr;
#endif

    LIBBASE->dl_Root = (struct RootNode *)AllocMem(sizeof(struct RootNode),
                                                   MEMF_PUBLIC|MEMF_CLEAR);
    dosinfo = AllocMem(sizeof(struct DosInfo), MEMF_PUBLIC|MEMF_CLEAR);

    /* Init the RootNode structure */
    taskarray = AllocMem(sizeof(IPTR) + sizeof(APTR), MEMF_CLEAR);
    taskarray[0] = 1;
    LIBBASE->dl_Root->rn_TaskArray = MKBADDR(taskarray);
    LIBBASE->dl_Root->rn_Info      = MKBADDR(dosinfo);

    NEWLIST((struct List *)&LIBBASE->dl_Root->rn_CliList);
    InitSemaphore(&LIBBASE->dl_Root->rn_RootLock);

    InitSemaphore(&dosinfo->di_DevLock);
    InitSemaphore(&dosinfo->di_EntryLock);
    InitSemaphore(&dosinfo->di_DeleteLock);

#ifdef AROS_DOS_PACKETS
    /* Set dl_Root->rn_FileHandlerSegment to the AFS handler,
     * if it's been loaded. Otherwise, use the first handler
     * on the FileSystemResource list that has fse_PatchFlags
     * set to mark it with a valid SegList */
    if ((fsr = OpenResource("FileSystem.resource"))) {
    	struct FileSysEntry *fse;
    	BPTR defseg = BNULL;
    	const ULONG DosMagic = 0x444f5301; /* DOS\001 */

    	ForeachNode(&fsr->fsr_FileSysEntries, fse) {
    	    if (fse->fse_DosType == DosMagic &&
    	    	(fse->fse_PatchFlags & FSEF_SEGLIST)) {
    	    	defseg = fse->fse_SegList;
    	    	break;
    	    }
    	}

    	if (defseg == BNULL) {
    	    ForeachNode(&fsr->fsr_FileSysEntries, fse) {
    	    	if ((fse->fse_PatchFlags & FSEF_SEGLIST) &&
    	    	    (fse->fse_SegList != BNULL)) {
    	    	    defseg = fse->fse_SegList;
    	    	}
    	    }
    	}

    	LIBBASE->dl_Root->rn_FileHandlerSegment = defseg;
    }
#endif

    /* Initialize for the fools that illegally used this field */
    LIBBASE->dl_UtilityBase   = OpenLibrary("utility.library", 0);
    LIBBASE->dl_IntuitionBase = NULL;

    PatchDOS(LIBBASE);

    /*
     * iaint:
     * I know this is bad, but I also know that the timer.device
     * will never go away during the life of dos.library. I also
     * don't intend to make any I/O calls using this.
     *
     * I also know that timer.device does exist in the device list
     * at this point in time.
     *
     * I can't allocate a timerequest/MsgPort pair here anyway,
     * because I need a separate one for each caller to Delay()
     */
    LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Succ = NULL;
    LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Pred = NULL;
    LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Pri  = 0;
    LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Name = NULL;
    LIBBASE->dl_TimerIO.tr_node.io_Message.mn_ReplyPort    = NULL;
    LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Length       = sizeof(struct timerequest);

    if (OpenDevice("timer.device", UNIT_VBLANK, &LIBBASE->dl_TimerIO.tr_node, 0) == 0)
    {
	LIBBASE->dl_TimeReq = &LIBBASE->dl_TimerIO;

	LIBBASE->dl_lib.lib_Node.ln_Name = "dos.library";
	LIBBASE->dl_lib.lib_Node.ln_Type = NT_LIBRARY;
	LIBBASE->dl_lib.lib_Version = VERSION_NUMBER;

	AddLibrary((struct Library *)LIBBASE);

	KernelBase = OpenResource("kernel.resource");

	/* This is where we start the RTC_AFTERDOS residents */
	D(bug("[DOS] DosInit: InitCode(RTF_AFTERDOS)\n"));
	InitCode(RTF_AFTERDOS, 0);

	/*
	 * We now restart the multitasking - this is done
	 * automatically by RemTask() when it switches.
	 */
	RemTask(NULL);
    }

    Alert(AT_DeadEnd | AG_OpenDev | AN_DOSLib | AO_TimerDev);
    return FALSE;
}

ADD2INITLIB(DosInit, 0);
