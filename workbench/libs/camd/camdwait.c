/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>

#include "camd_intern.h"

#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE


struct SignalSemaphore camdwaitsemaphore={{0}};
struct SignalSemaphore camdwaitsemaphore2={{0}};

struct Task *camdwaittask;
ULONG camdwaitsig;
ULONG camdwaitsig2;

ULONG camdwaitprocstatus=0;	//0=not alive or not initialized, 1=alive and fine, 2=failed, 3=dead.


SAVEDS void CamdTimerProc(void){
	struct timerequest *TimerIO;
	struct MsgPort  *TimerMP;
	ULONG sig;
	int error;

	D(bug("camdtimerproc1\n"));
	camdwaitsig=AllocSignal(-1);
	if(camdwaitsig==-1){
		camdwaitprocstatus=2;
		return;
	}
	D(bug("camdtimerproc2\n"));
	camdwaitsig2=AllocSignal(-1);
	if(camdwaitsig2==-1){
		FreeSignal(1L<<camdwaitsig);
		camdwaitprocstatus=2;
		return;
	}

	D(bug("camdtimerproc3\n"));
	TimerMP=CreateMsgPort();
	if(TimerMP==NULL){
		FreeSignal(1L<<camdwaitsig2);
		FreeSignal(1L<<camdwaitsig);
		camdwaitprocstatus=2;
		return;
	}
	D(bug("camdtimerproc4\n"));

	TimerIO=(struct timerequest *)AllocMem(sizeof(struct timerequest),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	D(bug("camdtimerproc5\n"));

	if(TimerIO==NULL){
		FreeSignal(1L<<camdwaitsig2);
		FreeSignal(1L<<camdwaitsig);
		DeleteMsgPort(TimerMP);
		camdwaitprocstatus=2;
		return;
	}
	D(bug("camdtimerproc6\n"));

	TimerIO->tr_node.io_Message.mn_Node.ln_Type=NT_MESSAGE;
	TimerIO->tr_node.io_Message.mn_ReplyPort=TimerMP;
	TimerIO->tr_node.io_Message.mn_Length=sizeof(struct timerequest);

	/* No support for eclock in AROS. */
#ifndef __AROS__
	if((error=OpenDevice(
			TIMERNAME,UNIT_ECLOCK,(struct IORequest *)TimerIO,0L
	))!=0){
#else
	D(bug("camdtimerproc7\n"));
	if((error=OpenDevice(
			TIMERNAME,UNIT_VBLANK,(struct IORequest *)TimerIO,0L
	))!=0){
#endif
	  D(bug("camdtimerproc7.1\n"));
		FreeSignal(1L<<camdwaitsig2);
		FreeSignal(1L<<camdwaitsig);
		TimerIO->tr_node.io_Message.mn_Node.ln_Type=(UBYTE)-1;
		TimerIO->tr_node.io_Device=(struct Device *)-1L;
		TimerIO->tr_node.io_Unit=(struct Unit *)-1L;
		FreeMem(TimerIO,sizeof(struct timerequest));
		DeleteMsgPort(TimerMP);
		D(bug("failed camdtimerproc11, error: %ld\n",error));
		camdwaitprocstatus=2;
		return;
	}

	D(bug("camdtimerproc8\n"));
	ObtainSemaphore(&camdwaitsemaphore);

	D(bug("camdtimerproc9\n"));
	camdwaittask=FindTask(0L);

	camdwaitprocstatus=1;

	for(;;){
		sig=Wait(
			1L<<camdwaitsig |
			SIGBREAKF_CTRL_C
		);
		if(sig&SIGBREAKF_CTRL_C) break;

		if(sig & 1L<<camdwaitsig){				// Someone wants to obtain the semaphore?
			TimerIO->tr_node.io_Command=TR_ADDREQUEST;
			TimerIO->tr_time.tv_secs=0;
			TimerIO->tr_time.tv_micro=10;
			DoIO((struct IORequest *)TimerIO);	// ..Better let them wait.

			ReleaseSemaphore(&camdwaitsemaphore);		// Well, okey then.

			Wait(1L<<camdwaitsig2);					// Finished soon?

			ObtainSemaphore(&camdwaitsemaphore);		// But I want it back!
		}

	}

	ReleaseSemaphore(&camdwaitsemaphore);

	FreeSignal(1L<<camdwaitsig2);
	FreeSignal(1L<<camdwaitsig);
	CloseDevice((struct IORequest *)TimerIO);
	TimerIO->tr_node.io_Message.mn_Node.ln_Type=(UBYTE)-1;
	TimerIO->tr_node.io_Device=(struct Device *)-1L;
	TimerIO->tr_node.io_Unit=(struct Unit *)-1L;
	FreeMem(TimerIO,sizeof(struct timerequest));

	DeleteMsgPort(TimerMP);

	camdwaitprocstatus=3;

	return;
}

BOOL InitCamdTimer(void){
	struct Process *process;

	InitSemaphore(&camdwaitsemaphore);
	InitSemaphore(&camdwaitsemaphore2);

	process=CreateNewProcTags(
		NP_Entry, (IPTR) CamdTimerProc,
		NP_Name,  (IPTR) "Camd Wait Proc",
		NP_Priority,5,
		TAG_END
	);
	if(process==NULL) return FALSE;

	D(bug("4.7\n"));
	while(camdwaitprocstatus==0) Delay(1);
	D(bug("4.8\n"));

	if(camdwaitprocstatus==2) return FALSE;

	return TRUE;
}

void UninitCamdTimer(void){

  if(camdwaitprocstatus==2) return;

  Signal(camdwaittask,SIGBREAKF_CTRL_C);
  while(camdwaitprocstatus!=3) Delay(1);
}



/*
 * When a hardware-sender buffer is full, or we are sending a sysex-message
 * to hardware, we need to wait for small amounts of time. That is what
 * this function tries to do.
 *
 */

void CamdWait(void){

	static BOOL iswaiting=FALSE;
	BOOL imjustgoingtoreturnsoon=iswaiting;

	ObtainSemaphore(&camdwaitsemaphore2);

		if(imjustgoingtoreturnsoon==TRUE){			// No big point having more than one visitor to wait.
			ReleaseSemaphore(&camdwaitsemaphore2);
			return;
		}

		iswaiting=TRUE;

		Signal(camdwaittask,1L<<camdwaitsig);		// Give me the semaphore!
		ObtainSemaphore(&camdwaitsemaphore);			// This should take some time.
			Signal(camdwaittask,1L<<camdwaitsig2);	// Okey, I've got it.
		ReleaseSemaphore(&camdwaitsemaphore);		// You're welcome.

		iswaiting=FALSE;

	ReleaseSemaphore(&camdwaitsemaphore2);

	return;
}

