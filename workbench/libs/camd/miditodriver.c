/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/



// Topic. Should status 0xf1-0xf6 messages be treated realtime?


#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"

#ifndef min
#define min(a,b) ((a)<=(b)?(a):(b))
#endif

#if defined(_AMIGA) || AROS_BIG_ENDIAN
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


__inline BYTE GetMsgLen(LONG msg){
	msg=0xff&msg>>24;

	if(msg<0x80) return 3;

	if(msg&0x80 && msg&0x40){
		if(!(msg&0x20)){
			return 1;				//0xc0 or 0xb0
		}else{
			if(msg&0x10){	//0xfx
				switch(msg){
					case 0xf0:
						return 3;		//Return error. Not the appropriate way to send sysx.
					case 0xf1:
						return 1;
					case 0xf2:
						return 2;
					case 0xf3:
						return 1;
					case 0xf4:
						return 3;
					case 0xf5:
						return 3;
					case 0xf6:
						return 0;
					case 0xf7:
						return 3;
					default:
						return 4;		//Realtime message
						break;
				}
			}
		}
	}
	return 2;
}


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
	UBYTE ret=driverdata->buffer_sx[driverdata->buffercurrsend_sx];
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
		  IncBuffer(driverdata,&driverdata->buffercurrsend);
		  driverdata->unsent--;
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
			}else{
			  IncBuffer(driverdata,&driverdata->buffercurrsend);
			  driverdata->unsent--;
			}
			return buf[BUF1];
		}

		driverdata->status=ret;
		driverdata->transmitfunc=Transmit_Datas;
		driverdata->sendpos=1;

	}

	return ret;
}


ULONG ASM Transmitter(REG(a2) struct DriverData *driverdata){
	UBYTE ret;

#ifndef __AROS__
	if((driverdata->mididevicedata->Flags&1)==0){
		return Transmitter_oldformat(driverdata);
	}
#endif

	// First of all, check if there are any realtime-messages on the realtime-buffer.
	if(driverdata->unsent_rt>0){
		ret=*driverdata->buffercurrsend_rt;
		IncBuffer_rt(driverdata,&driverdata->buffercurrsend_rt);
		driverdata->unsent_rt--;
		return ret;
	}


	if(driverdata->transmitfunc!=NULL){
		ret=(*driverdata->transmitfunc)(driverdata);
		return ret;
	}

	if(driverdata->realtimesysx==1){
		driverdata->transmitfunc=Transmit_SysEx;
		ret=Transmit_SysEx(driverdata);
		return ret;
	}

	if(driverdata->unsent!=0){
		ret=Transmit_Status(driverdata);
		return ret;
	}

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

	(*driverdata->midiportdata->ActivateXmit)(driverdata,driverdata->portnum);

	ReleaseSemaphore(&driverdata->sendsemaphore);

	return TRUE;
}




/******************************************************************************

  FUNCTION
    Returns FALSE if buffer is full or bigger than maxbuff, and does not
    send out anything.

******************************************************************************/

BOOL Midi2Driver_internal(
	struct DriverData *driverdata,
	ULONG msg,
	ULONG maxbuff
){

#ifndef __AROS__
	if((driverdata->mididevicedata->Flags&1)==0){
		return Midi2Driver_internal_oldformat(driverdata,msg,maxbuff);
	}
#endif

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

	(*driverdata->midiportdata->ActivateXmit)(driverdata,driverdata->portnum);

	ReleaseSemaphore(&driverdata->sendsemaphore);

	return TRUE;
}



BOOL SysEx2Driver(struct DriverData *driverdata,UBYTE *buffer){

#ifndef __AROS__
	if((driverdata->mididevicedata->Flags&1)==0){
		return SysEx2Driver_oldformat(driverdata,buffer);
	}
#endif

	if(buffer[1]!=0x7f){
		ObtainSemaphore(&driverdata->sendsemaphore);
		if(
			driverdata->unsent>=OUTBUFFERSIZE-2
		){
			ReleaseSemaphore(&driverdata->sendsemaphore);
			return FALSE;
		}
		*driverdata->buffercurr=0xf00000f0;
		driverdata->unsent++;
		IncBuffer(driverdata,&driverdata->buffercurr);

	}else{
		driverdata->realtimesysx=1;
		ObtainSemaphore(&driverdata->sendsemaphore);
	}

	(*driverdata->midiportdata->ActivateXmit)(driverdata,driverdata->portnum);

	ReleaseSemaphore(&driverdata->sendsemaphore);

	return TRUE;
}

