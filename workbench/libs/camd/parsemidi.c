/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH3(void, ParseMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, midilink, A0),
	AROS_LHA(UBYTE *, buffer, A1),
	AROS_LHA(ULONG, length, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 33, Camd)

/*  FUNCTION
		Puts a midibuffer to a midilinks clusters midilinks midinodes and hardware.
		To help understand what it does, the following macro makes PutMidi
		use ParseMidi instead of calling camd.library's PutMidi function for
		small-endian cpus:

		#define PutMidi(midilink,message) ParseMidi((midilink),&(message),MidiMsgLen(message))

		(But please don't use this macro, since its not big-endian compatible,
	    and that PutMidi is faster than ParseMidi)

    INPUTS

    RESULT

    NOTES
		If its more convenient to use PutMidi and PutSysEx instead of ParseMidi,
		do that. ParseMidi is a bit heavier function to use than PutMidi and
		PutSysEx.

		MLINK_Parse must have be set when calling either AddMidiLinkA or
		SetMidiLinkAttrsA.

    EXAMPLE

    BUGS

    SEE ALSO
		PutMidi(), PutSysEx()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT


	/* The implementation of ParseMidi is really a hack. Because I thought
	   the function was supposed to do somthing else when I started implementing
	   camd.library. But when I saw that PlayMF only uses
	   ParseMidi for sending midi, and after reading PlayMF's source, it came
	   pretty clear what ParseMidi is supposed to do. I'm allso quite shure that this
	   hack should work nice (PlayMF works ok now), but the code is
		a bit ugly.

	   What it does is to simulate a hardware-driver input-stream
	   and calling some receiver-functions in MidiFromDriver.c. SysEx to
		hardware couldn't be handled that way because the buffer
		might have realtime-messages inside a sysex-message (you never
		know :), so that is treated seperately.

		-ksvalast.
	*/

	struct DriverData *driverdata=midilink->ml_ParserData;
	struct DriverData *TOdriverdata=NULL;
	UBYTE data;

	if(driverdata==NULL) return;

	driverdata->lastsysex=NULL;
	driverdata->Input_Treat=Receiver_init;

	while(length>0){
		data=*buffer;

		if(data==0xf0){
			if(TOdriverdata==NULL){
				if(midilink->ml_Location!=NULL){
					ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
						TOdriverdata=FindReceiverDriverInCluster(midilink->ml_Location);
					ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
				}
			}
			if(TOdriverdata!=NULL){
				driverdata->lastsysex=buffer;
			}
		}

		if(data>=0xf8){
			Receiver_RealTime(driverdata,data);
		}else{
			(*driverdata->Input_Treat)(driverdata,data);
		}

		if(driverdata->lastsysex!=NULL && data>=0x80 && data<0xf8 && data!=0xf0){
			if(data==0xf7){
				ObtainSemaphore(&TOdriverdata->sysexsemaphore);

					TOdriverdata->buffer_sx=driverdata->lastsysex;
					TOdriverdata->buffercurrsend_sx=0;
					TOdriverdata->issending_sx=1;

					while(SysEx2Driver(TOdriverdata,driverdata->lastsysex)==FALSE) CamdWait();
					while(TOdriverdata->issending_sx!=0) CamdWait();

				ReleaseSemaphore(&TOdriverdata->sysexsemaphore);
			}
			driverdata->lastsysex=NULL;
		}
		buffer++;
		length--;
	}

   AROS_LIBFUNC_EXIT

}

