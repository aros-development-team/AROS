/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "camd_intern.h"

#undef SysBase
#undef UtilityBase

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



void PutMidi2Link(
	struct MidiLink *midilink,
	struct MyMidiMessage2 *msg2,
	ULONG timestamp
){
	int lokke;
	ULONG type;
	struct MyMidiMessage *msg;
	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midilink->ml_MidiNode;

	if(mymidinode->error&CMEF_BufferFull){
		return;
	}

	if(msg2->status<0x80){
		type=CMB_SysEx;
	}else{
		type=MidiMsgType_status_data1(msg2->status,msg2->data1);
	}

	if(! ( (1L<<type) & midilink->ml_EventTypeMask) ){
		return;
	}

	if( (midilink->ml_ChannelMask!=(UWORD)~0) && ((1L<<type)&CMF_Channel) ){
		if( ! (
			( 1L<< (msg2->status&0xf) )
			&
			midilink->ml_ChannelMask
		) ){
			return;
		}
	}

	if(type==CMB_SysEx){
		if(midilink->ml_SysExFilter.b[0]!=0){
			if(midilink->ml_SysExFilter.b[0]&SXFM_3Byte){
				if(
					midilink->ml_SysExFilter.b[1]!=msg2->status ||
					midilink->ml_SysExFilter.b[1]!=msg2->status ||
					midilink->ml_SysExFilter.b[1]!=msg2->status
				){
					mymidinode->sysex_write=mymidinode->sysex_laststart;
					return;
				}
			}else{
				for(lokke=1;lokke<4;lokke++){
					if(midilink->ml_SysExFilter.b[lokke]==msg2->status){
						goto outofhere;
					}
				}
				mymidinode->sysex_write=mymidinode->sysex_laststart;
				return;
			}
		}
outofhere:
		msg2->data2=msg2->data1;
		msg2->data1=msg2->status;
		msg2->status=0xf0;
	}

	msg=mymidinode->in_curr;
	if(msg==NULL){					//If no buffer.
		return;
	}

	if(mymidinode->unpicked+1==mymidinode->midinode.mi_MsgQueueSize-1){
		mymidinode->error |= CMEF_BufferFull;
		return;
	}

	msg->timestamp=timestamp;

	msg->m[0]=msg2->status;
	if(msg2->len>1){
		msg->m[1]=msg2->data1;
	}else{
		msg->m[1]=0;
	}
	if(msg2->len>2){
		msg->m[2]=msg2->data2;
	}else{
		msg->m[2]=0;
	}
	msg->m[3]=midilink->ml_PortID;

	mymidinode->unpicked++;

	mymidinode->in_curr++;
	if(mymidinode->in_curr==mymidinode->in_end){
		mymidinode->in_curr=mymidinode->in_start;
	}

	if(mymidinode->midinode.mi_ReceiveSigBit!=-1){
		Signal(mymidinode->midinode.mi_SigTask,1L<<mymidinode->midinode.mi_ReceiveSigBit);
	}
	if(mymidinode->midinode.mi_ReceiveHook!=NULL){
// Topic! Should the hook carry any data? (guess so...)
		CallHookPkt(mymidinode->midinode.mi_ReceiveHook,NULL,NULL);
	}
}


/******************************************************************************

  FUNCTION
    Returns NULL if success, driverdata if not.

******************************************************************************/

struct DriverData *GoodPutMidi(
	struct MidiLink *midilink,
	ULONG msg,
	ULONG maxbuff,
	struct CamdBase *CamdBase
){
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
				driverdata=Midi2Driver((struct DriverData *)node,msg,maxbuff)?NULL:(struct DriverData *)node;
			}else{
				midilink2=(struct MidiLink *)node;
				mymidinode=(struct MyMidiNode *)midilink2->ml_MidiNode;
				ObtainSemaphore(&mymidinode->receiversemaphore);
					PutMidi2Link(midilink2,&msg2,*mymidinode->midinode.mi_TimeStamp);
				ReleaseSemaphore(&mymidinode->receiversemaphore);
				if(driverdata!=NULL){
					driverdata=Midi2Driver(driverdata,msg,maxbuff)?NULL:driverdata;
				}
			}
			node=node->ln_Succ;
		}
	}

	ReleaseSemaphore(&mycluster->semaphore);

	return driverdata;
}


