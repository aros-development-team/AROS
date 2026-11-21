/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: The CAMD clock: milliseconds since camd.library started.
*/

#include <proto/exec.h>
#include <proto/timer.h>
#include <devices/timer.h>

#include "camd_intern.h"

/* The E-clock only counts forward, unlike the system time, which
   SetSysTime() can move. */
struct Device *TimerBase;
static struct timerequest *clockreq;
static UQUAD clockstart;
static ULONG clockfreq;

static UQUAD ReadTicks(void){
	struct EClockVal ev;

	ReadEClock(&ev);
	return ((UQUAD)ev.ev_hi<<32)|ev.ev_lo;
}

BOOL InitCamdClock(void){
	struct EClockVal ev;

	clockreq=AllocMem(sizeof(struct timerequest),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	if(clockreq==NULL) return FALSE;
	clockreq->tr_node.io_Message.mn_Length=sizeof(struct timerequest);
	if(OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)clockreq,0)!=0){
		FreeMem(clockreq,sizeof(struct timerequest));
		clockreq=NULL;
		return FALSE;
	}
	TimerBase=clockreq->tr_node.io_Device;
	clockfreq=ReadEClock(&ev);
	clockstart=((UQUAD)ev.ev_hi<<32)|ev.ev_lo;
	if(clockfreq==0) clockfreq=1;
	return TRUE;
}

void UninitCamdClock(void){
	if(clockreq!=NULL){
		CloseDevice((struct IORequest *)clockreq);
		FreeMem(clockreq,sizeof(struct timerequest));
		clockreq=NULL;
		TimerBase=NULL;
	}
}

/* Milliseconds since camd.library started, wrapping at 2^32. Callable
   from any task and from interrupts. */
ULONG CamdClock(void){
	UQUAD ticks=ReadTicks()-clockstart;

	return (ULONG)((ticks/clockfreq)*1000+((ticks%clockfreq)*1000)/clockfreq);
}
