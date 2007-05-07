/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"



/*****************************************************************************

    NAME */

	AROS_LH3(APTR, GoodPutMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, midilink, A0),
	AROS_LHA(ULONG, msg, D0),
	AROS_LHA(ULONG, maxbuff, D1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 39, Camd)

/*  FUNCTION
		This is a private function, and will probably be obsolete. Please don`t use.

    INPUTS

    RESULT
    	NULL if success, driverdata if not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		PutMidi, PutMidiMsg, Midi2Driver

    INTERNALS

    HISTORY

	2001-07-14 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	int len=GetMsgLen(msg);
	struct Node *node;
	struct DriverData *driverdata=NULL;
	struct MyMidiCluster *mycluster=(struct MyMidiCluster *)midilink->ml_Location;
	struct MyMidiMessage2 msg2;
	struct MidiLink *midilink2;
	struct MyMidiNode *mymidinode;

	if(len==3) return NULL;	//Illegal message.

	ObtainSemaphoreShared(&mycluster->semaphore);

	if( ! (IsListEmpty(&mycluster->cluster.mcl_Receivers))){

		msg2.status=msg>>24;
		msg2.data1=0x7f&(msg>>16);
		msg2.data2=0x7f&(msg>>8);
		msg2.len=(len&3)+1;

		node=mycluster->cluster.mcl_Receivers.lh_Head;

		while(node->ln_Succ!=NULL){
			if(node->ln_Type==NT_USER-MLTYPE_NTypes){
				driverdata=Midi2Driver_internal((struct DriverData *)node,msg,maxbuff)?NULL:(struct DriverData *)node;
			}else{
				midilink2=(struct MidiLink *)node;
				mymidinode=(struct MyMidiNode *)midilink2->ml_MidiNode;
				ObtainSemaphore(&mymidinode->receiversemaphore);
					PutMidi2Link(midilink2,&msg2,*mymidinode->midinode.mi_TimeStamp);
				ReleaseSemaphore(&mymidinode->receiversemaphore);
				if(driverdata!=NULL){
					driverdata=Midi2Driver_internal(driverdata,msg,maxbuff)?NULL:driverdata;
				}
			}
			node=node->ln_Succ;
		}
	}

	ReleaseSemaphore(&mycluster->semaphore);

	return driverdata;


   AROS_LIBFUNC_EXIT
}







