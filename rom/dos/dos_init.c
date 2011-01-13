/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
#include <aros/debug.h>
#include LC_LIBDEFS_FILE
#include "dos_intern.h"

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
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

    /* Exit -> BCPL_Exit */
    void BCPL_Exit(void);
    __AROS_SETVECADDR(dosbase, 24, BCPL_Exit);

    CacheClearU();
}
#endif

static int DosInit(struct DosLibrary *LIBBASE)
{
    D(bug("DosInit\n"));
    
#ifndef AROS_DOS_PACKETS
    __AROS_SETVECADDR(LIBBASE, 15, __AROS_GETVECADDR(LIBBASE, 6));
    __AROS_SETVECADDR(LIBBASE, 62, __AROS_GETVECADDR(LIBBASE, 16));
    __AROS_SETVECADDR(LIBBASE, 65, __AROS_GETVECADDR(LIBBASE, 17));
    __AROS_SETVECADDR(LIBBASE, 68, __AROS_GETVECADDR(LIBBASE, 67));
#endif

    IPTR * taskarray;
    struct DosInfo *dosinfo;

    LIBBASE->dl_Root = (struct RootNode *)AllocMem(sizeof(struct RootNode),
                                                   MEMF_PUBLIC|MEMF_CLEAR);
    dosinfo = AllocMem(sizeof(struct DosInfo), MEMF_PUBLIC|MEMF_CLEAR);

    /* Init the RootNode structure */
    taskarray = AllocMem(sizeof(IPTR) + sizeof(APTR), MEMF_CLEAR);
    taskarray[0] = 1;
    LIBBASE->dl_Root->rn_TaskArray = MKBADDR(taskarray);
    LIBBASE->dl_Root->rn_Info= MKBADDR(dosinfo);

    NEWLIST((struct List *)&LIBBASE->dl_Root->rn_CliList);
    InitSemaphore(&LIBBASE->dl_Root->rn_RootLock);

    InitSemaphore(&dosinfo->di_DevLock);
    InitSemaphore(&dosinfo->di_EntryLock);
    InitSemaphore(&dosinfo->di_DeleteLock);

    /* Initialize for the fools that illegally used this field */
    LIBBASE->dl_UtilityBase = OpenLibrary("utility.library", 0);
    LIBBASE->dl_SysBase = SysBase;
    LIBBASE->dl_IntuitionBase = NULL;

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
    PatchDOS(LIBBASE);
#endif
    {
	/*  iaint:
	    I know this is bad, but I also know that the timer.device
	    will never go away during the life of dos.library. I also
	    don't intend to make any I/O calls using this.

	    I also know that timer.device does exist in the device list
	    at this point in time.

	    I can't allocate a timerequest/MsgPort pair here anyway,
	    because I need a separate one for each caller to Delay()
	*/

        struct MsgPort timermp;
	
	timermp.mp_Node.ln_Succ = NULL;
	timermp.mp_Node.ln_Pred = NULL;
	timermp.mp_Node.ln_Type = NT_MSGPORT;
	timermp.mp_Node.ln_Pri  = 0;
	timermp.mp_Node.ln_Name = NULL;
	timermp.mp_Flags 	= PA_SIGNAL;
	timermp.mp_SigBit 	= SIGB_SINGLE;
	timermp.mp_SigTask	= FindTask(NULL);
	NEWLIST(&timermp.mp_MsgList);
	
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Succ = NULL;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Pred = NULL;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Pri  = 0;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Name = NULL;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_ReplyPort    = &timermp;	
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Length       = sizeof(struct timerequest);


	SetSignal(0, SIGF_SINGLE);
	
	if(OpenDevice("timer.device", UNIT_VBLANK, 
		      &LIBBASE->dl_TimerIO.tr_node, 0) == 0)
	{
	    LIBBASE->dl_TimerBase = LIBBASE->dl_TimerIO.tr_node.io_Device;
	    LIBBASE->dl_TimeReq = &LIBBASE->dl_TimerIO;

	    LIBBASE->dl_lib.lib_Node.ln_Name = "dos.library";
	    LIBBASE->dl_lib.lib_Node.ln_Type = NT_LIBRARY;
	    LIBBASE->dl_lib.lib_Version = VERSION_NUMBER;

	    AddLibrary((struct Library *)LIBBASE);

	    KernelBase = OpenResource("kernel.resource");

	    /* This is where we start the RTC_AFTERDOS residents */
	    D(bug("[DOS] DosInit: InitCode(RTF_AFTERDOS)\n"));
	    InitCode(RTF_AFTERDOS, 0);

	    /* We now restart the multitasking	- this is done
	       automatically by RemTask() when it switches.
	    */
	    RemTask(NULL);
	}
	Alert(AT_DeadEnd | AG_OpenDev | AN_DOSLib | AO_TimerDev);
    }

    return FALSE;
}

ADD2INITLIB(DosInit, 0);
