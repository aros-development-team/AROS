/*
    Copyright (C) 1995-2004, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>
#include <dos/notify.h>

#include "camd_intern.h"

#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE


struct SignalSemaphore camdwaitsemaphore2={{0}};

struct Task *camdwaittask;
struct Task *camdwaitcaller;	// under camdwaitsemaphore2
ULONG camdwaitcallersig;
BYTE camdwaitsig;

ULONG camdwaitprocstatus=0;	//0=not alive or not initialized, 1=alive and fine, 2=failed, 3=dead.


SAVEDS void CamdTimerProc(void){
	struct timerequest *TimerIO;
	struct MsgPort  *TimerMP;
	ULONG sig;
	int error;
#ifndef __amigaos4__
	struct NotifyRequest *drivernotify=NULL;
	BYTE notifysig;
	BOOL notifying=FALSE;
#endif

	camdwaitsig=AllocSignal(-1);
	if(camdwaitsig==-1){
		camdwaitprocstatus=2;
		return;
	}
	TimerMP=CreateMsgPort();
	if(TimerMP==NULL){
		FreeSignal(camdwaitsig);
		camdwaitprocstatus=2;
		return;
	}

	TimerIO=(struct timerequest *)AllocMem(sizeof(struct timerequest),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);

	if(TimerIO==NULL){
		FreeSignal(camdwaitsig);
		DeleteMsgPort(TimerMP);
		camdwaitprocstatus=2;
		return;
	}

	TimerIO->tr_node.io_Message.mn_Node.ln_Type=NT_MESSAGE;
	TimerIO->tr_node.io_Message.mn_ReplyPort=TimerMP;
	TimerIO->tr_node.io_Message.mn_Length=sizeof(struct timerequest);

	/* No support for eclock in AROS. */
#ifndef __AROS__
	if((error=OpenDevice(
			TIMERNAME,UNIT_ECLOCK,(struct IORequest *)TimerIO,0L
	))!=0){
#else
	if((error=OpenDevice(
			TIMERNAME,UNIT_VBLANK,(struct IORequest *)TimerIO,0L
	))!=0){
#endif

		FreeSignal(camdwaitsig);
		TimerIO->tr_node.io_Message.mn_Node.ln_Type=(UBYTE)-1;
		TimerIO->tr_node.io_Device=(struct Device *)-1L;
		TimerIO->tr_node.io_Unit=(struct Unit *)-1L;
		FreeMem(TimerIO,sizeof(struct timerequest));
		DeleteMsgPort(TimerMP);
		camdwaitprocstatus=2;
		return;
	}

	camdwaittask=FindTask(0L);

#ifndef __amigaos4__
	/* Tell CAMD when drivers are added to DEVS:Midi. Where the filesystem
	   cannot notify, drivers are found when CAMD starts, as before. */
	notifysig=AllocSignal(-1);
	if(notifysig!=-1){
		drivernotify=AllocMem(sizeof(struct NotifyRequest),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	}
	if(drivernotify!=NULL){
		drivernotify->nr_Name=(STRPTR)"DEVS:Midi";
		drivernotify->nr_Flags=NRF_SEND_SIGNAL;
		drivernotify->nr_stuff.nr_Signal.nr_Task=camdwaittask;
		drivernotify->nr_stuff.nr_Signal.nr_SignalNum=notifysig;
		notifying=StartNotify(drivernotify);
	}
#endif

	camdwaitprocstatus=1;

	for(;;){
		sig=Wait(
			1L<<camdwaitsig |
#ifndef __amigaos4__
			(notifying ? 1L<<notifysig : 0) |
#endif
			SIGBREAKF_CTRL_C
		);
		if(sig&SIGBREAKF_CTRL_C) break;

#ifndef __amigaos4__
		if(notifying && (sig & 1L<<notifysig)){
			DriversChanged();
		}
#endif

		if(sig & 1L<<camdwaitsig){				// Someone wants to wait a little.
			TimerIO->tr_node.io_Command=TR_ADDREQUEST;
			TimerIO->tr_time.tv_secs=0;
			TimerIO->tr_time.tv_micro=10;
			DoIO((struct IORequest *)TimerIO);

			Signal(camdwaitcaller,camdwaitcallersig);
		}

	}

#ifndef __amigaos4__
	if(notifying) EndNotify(drivernotify);
	if(drivernotify!=NULL) FreeMem(drivernotify,sizeof(struct NotifyRequest));
	if(notifysig!=-1) FreeSignal(notifysig);
#endif

	FreeSignal(camdwaitsig);
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

	InitSemaphore(&camdwaitsemaphore2);

	process=CreateNewProcTags(
		NP_Entry, (IPTR) CamdTimerProc,
		NP_Name,  (IPTR) "Camd Wait Proc",
		NP_Priority,5,
		TAG_END
	);
	if(process==NULL) return FALSE;

	while(camdwaitprocstatus==0) Delay(1);

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
	BYTE replysig;
	ULONG oldsingle=0;

	ObtainSemaphore(&camdwaitsemaphore2);

		if(imjustgoingtoreturnsoon==TRUE){			// No big point having more than one visitor to wait.
			ReleaseSemaphore(&camdwaitsemaphore2);
			return;
		}

		iswaiting=TRUE;

		/* Wait for the timer process's answer. Borrowing a semaphore only
		   waited while that process happened to own it; after the first call
		   a caller holding Forbid() could repeatedly acquire it without ever
		   letting the timer process run. */
		replysig=AllocSignal(-1);
		if(replysig==-1){
			camdwaitcallersig=SIGF_SINGLE;
			oldsingle=SetSignal(0,SIGF_SINGLE)&SIGF_SINGLE;
		}else{
			camdwaitcallersig=1L<<replysig;
		}
		camdwaitcaller=FindTask(0L);
		Signal(camdwaittask,1L<<camdwaitsig);
		Wait(camdwaitcallersig);
		if(replysig==-1){
			if(oldsingle) Signal(camdwaitcaller,SIGF_SINGLE);
		}else{
			FreeSignal(replysig);
		}

		iswaiting=FALSE;

	ReleaseSemaphore(&camdwaitsemaphore2);

	return;
}

