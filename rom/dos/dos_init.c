/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
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

static int DosInit(struct DosLibrary *LIBBASE)
{
    D(bug("DosInit\n"));
    
    __AROS_SETVECADDR(LIBBASE, 15, __AROS_GETVECADDR(LIBBASE, 6));
    __AROS_SETVECADDR(LIBBASE, 62, __AROS_GETVECADDR(LIBBASE, 16));
    __AROS_SETVECADDR(LIBBASE, 65, __AROS_GETVECADDR(LIBBASE, 17));
    __AROS_SETVECADDR(LIBBASE, 68, __AROS_GETVECADDR(LIBBASE, 67));
    
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

	    LIBBASE->dl_lib.lib_Node.ln_Name = "dos.library";
	    LIBBASE->dl_lib.lib_Node.ln_Type = NT_LIBRARY;
	    LIBBASE->dl_lib.lib_Version = VERSION_NUMBER;

	    AddLibrary((struct Library *)LIBBASE);

	    KernelBase = OpenResource("kernel.resource");

	    /* This is where we start the RTC_AFTERDOS residents */
bug("[DOS] DosInit: InitCode(RTF_AFTERDOS)\n");
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
