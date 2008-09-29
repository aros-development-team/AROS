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

	AROS_LH2(void, PutSysEx,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, midilink, A0),
	AROS_LHA(UBYTE *, buffer, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 26, Camd)

/*  FUNCTION
		Distributes a SysEx message. First sends the message to the hardware
		and all midinodes connected to the midilinks cluster, then waits
		for the complete message to be sent to the hardware, if any. If
		a midinodes sysex-buffer is to small to carry the message, it will
		not be sent. If the buffer is big enough, but there is not enough
		room, a sysex-full-error will be set to the node. The message is
		sent to hardware regardless of transmit buffer size.

    INPUTS
		midilink - pointer to link.
		buffer - message to send, must start with 0xf0 and end with 0xf7.
		         No bytes higher than 0x7f are allowed in the message.
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		GetSysEx()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct Node *node;
	struct DriverData *driverdata=NULL;
	struct MyMidiCluster *mycluster=(struct MyMidiCluster *)midilink->ml_Location;
	BOOL driversuccess=TRUE;
	UBYTE *buffer2;

	ObtainSemaphoreShared(&mycluster->semaphore);

	if( ! (IsListEmpty(&midilink->ml_Location->mcl_Receivers))){

		node=midilink->ml_Location->mcl_Receivers.lh_Head;

		while(node->ln_Succ!=NULL){
			if(node->ln_Type==NT_USER-MLTYPE_NTypes){

				driverdata=(struct DriverData *)node;
				ObtainSemaphore(&driverdata->sysexsemaphore);

				driverdata->buffer_sx=buffer;
				driverdata->buffercurrsend_sx=0;
				driverdata->issending_sx=1;

				driversuccess=SysEx2Driver(driverdata,buffer);

			}else{
				buffer2=buffer;
				while(PutSysEx2Link((struct MidiLink *)node,*buffer2)==TRUE){
					buffer2++;
				}
			}
			node=node->ln_Succ;
		}

		if(driverdata!=NULL){

			if(driversuccess==FALSE){
				while(SysEx2Driver(driverdata,buffer)==FALSE) CamdWait();
			}

			// Not a very good way to wait for the data to be sent, but it should
			// hopefully quite seldom be very important.
			while(driverdata->issending_sx!=0) CamdWait();

			ReleaseSemaphore(&driverdata->sysexsemaphore);
		}
	}

	ReleaseSemaphore(&mycluster->semaphore);

   AROS_LIBFUNC_EXIT

}


