/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/



// Topic. Should status 0xf1-0xf6 messages be treated realtime?


#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"

#undef SysBase
#undef DOSBase

#ifndef min
#define min(a,b) ((a)<=(b)?(a):(b))
#endif

#if defined(_AMIGA) || !defined(AROS_BIG_ENDIAN)
#  define BUF0 0
#  define BUF1 1
#  define BUF2 2
#  define BUF3 3
#  define TOBUF(a) (a)
#else
#  define BUF0 3
#  define BUF1 2
#  define BUF2 1
#  define BUF3 0
#  define TOBUF(a) ((a)^3)
#endif

__inline void IncBuffer(struct DriverData *data,ULONG **buffer){
	(*buffer)++;
	if(*buffer==data->bufferend){
		*buffer=data->buffer;
	}
}

__inline void IncBuffer_rt(struct DriverData *data,UBYTE **buffer_rt){
	(*buffer_rt)++;
	if(*buffer_rt==data->bufferend_rt){
		*buffer_rt=data->buffer_rt;
	}
}



/* Transmitter functions. */

ULONG Transmit_SysEx(struct DriverData *driverdata){
	ULONG ret=driverdata->buffer_sx[driverdata->buffercurrsend_sx];
	driverdata->buffercurrsend_sx++;
	if(ret==0xf7){
		driverdata->realtimesysx=0;
		driverdata->transmitfunc=NULL;
		driverdata->issending_sx=0;
	}
	return ret;
}



ULONG Transmit_Datas(struct DriverData *driverdata){
	UBYTE *buf;
	UBYTE len;
	UBYTE ret;

	buf=(UBYTE *)driverdata->buffercurrsend;
	ret=buf[TOBUF(driverdata->sendpos)];
	len=buf[BUF3];

	if(driverdata->sendpos==len){
		driverdata->transmitfunc=NULL;
		IncBuffer(driverdata,&driverdata->buffercurrsend);
		driverdata->unsent--;
	}else{
		driverdata->sendpos++;
	}

	return ret;
}



ULONG Transmit_Status(struct DriverData *driverdata){
	UBYTE *buf;
	UBYTE len;
	UBYTE ret;

	buf=(UBYTE *)driverdata->buffercurrsend;
	ret=buf[BUF0];
	len=buf[BUF3];

	if(ret>=0xf0){

		driverdata->status=0;	// (Realtime messages never come here.)

		if(ret==0xf0){
			driverdata->transmitfunc=Transmit_SysEx;
			return Transmit_SysEx(driverdata);
		}

		if(len==0){
			IncBuffer(driverdata,&driverdata->buffercurrsend);
			driverdata->unsent--;
		}else{
			driverdata->transmitfunc=Transmit_Datas;
			driverdata->sendpos=1;
		}

	}else{

		if(driverdata->status==ret){
			if(len>1){
				driverdata->transmitfunc=Transmit_Datas;
				driverdata->sendpos=2;
			}
			return buf[BUF1];
		}

		driverdata->status=ret;
		driverdata->transmitfunc=Transmit_Datas;
		driverdata->sendpos=1;

	}

	return ret;
}


#ifdef _AROS
#  define CLEARD1
#else
#  include <dos.h>
#  define CLEARD1	putreg(REG_D1,0);	// This is sas/c spesific!
#  define UNCLEARD1	putreg(REG_D1,0xff);
#endif

ULONG ASM Transmitter(REG(a2) struct DriverData *driverdata){
	UBYTE ret;


	// First of all, check if there are any realtime-messages on the realtime-buffer.
	if(driverdata->unsent_rt>0){
		ret=*driverdata->buffercurrsend_rt;
		IncBuffer_rt(driverdata,&driverdata->buffercurrsend_rt);
		driverdata->unsent_rt--;
		CLEARD1
		return ret;
	}


	if(driverdata->transmitfunc!=NULL){
		ret=(*driverdata->transmitfunc)(driverdata);
		CLEARD1
		return ret;
	}

	if(driverdata->realtimesysx==1){
		driverdata->transmitfunc=Transmit_SysEx;
		ret=Transmit_SysEx(driverdata);
		CLEARD1
		return ret;
	}

	if(driverdata->unsent!=0){
		ret=Transmit_Status(driverdata);
		CLEARD1
		return ret;
	}

#ifndef _AROS
	if(driverdata->mididevicedata->Flags&1==0){
		UNCLEARD1
		return 0xfd;		// Dummy-message.
	}
#endif

	return 0x100;

}



/* Put to buffer functions. */

BOOL Midi2Driver_rt(struct DriverData *driverdata,ULONG msg){

	ObtainSemaphore(&driverdata->sendsemaphore);

	if(
		driverdata->unsent_rt>=OUTBUFFERSIZE_RT-2
	){
		ReleaseSemaphore(&driverdata->sendsemaphore);
		return FALSE;
	}
	*driverdata->buffercurr_rt=msg>>24;
	driverdata->unsent_rt++;

	IncBuffer_rt(driverdata,&driverdata->buffercurr_rt);

	(*driverdata->midiportdata->ActivateXmit)(driverdata->portnum);

	ReleaseSemaphore(&driverdata->sendsemaphore);

	return TRUE;
}




/******************************************************************************

  FUNCTION
    Returns FALSE if buffer is full or bigger than maxbuff, and does not
    send out anything.

******************************************************************************/

BOOL Midi2Driver(
	struct DriverData *driverdata,
	ULONG msg,
	ULONG maxbuff
){

	if(msg>=0xf8000000) return Midi2Driver_rt(driverdata,msg);

	if(driverdata->unsent>=min(maxbuff,OUTBUFFERSIZE-2)){
		return FALSE;
	}

	ObtainSemaphore(&driverdata->sendsemaphore);
	if(
		driverdata->unsent>=OUTBUFFERSIZE-2
	){
		ReleaseSemaphore(&driverdata->sendsemaphore);
		return FALSE;
	}

	*driverdata->buffercurr=(msg & 0xffffff00) | GetMsgLen(msg);
	driverdata->unsent++;

	IncBuffer(driverdata,&driverdata->buffercurr);

	(*driverdata->midiportdata->ActivateXmit)(driverdata->portnum);

	ReleaseSemaphore(&driverdata->sendsemaphore);

	return TRUE;
}



BOOL SysEx2Driver(struct DriverData *driverdata,UBYTE *buffer){

	if(buffer[1]!=0x7f){
		ObtainSemaphore(&driverdata->sendsemaphore);
		if(
			driverdata->unsent==OUTBUFFERSIZE-2
		){
			ReleaseSemaphore(&driverdata->sendsemaphore);
			return FALSE;
		}
		*driverdata->buffercurr=0xf0000000;
		driverdata->unsent++;
		IncBuffer(driverdata,&driverdata->buffercurr);

	}else{
		driverdata->realtimesysx=1;
		ObtainSemaphore(&driverdata->sendsemaphore);
	}

	(*driverdata->midiportdata->ActivateXmit)(driverdata->portnum);

	ReleaseSemaphore(&driverdata->sendsemaphore);

	return TRUE;
}

