/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "camd_intern.h"


void PutMidi2Link(
	struct MidiLink *midilink,
	struct MyMidiMessage2 *msg2,
	ULONG timestamp
){
	ULONG type;
	MidiMsg *msg;
	MidiMsg hmsg;
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
		if(midilink->ml_SysExFilter.sxf_Mode!=0){
			if(midilink->ml_SysExFilter.sxf_Mode&SXFM_3Byte){
				if(
					midilink->ml_SysExFilter.sxf_ID1!=msg2->status ||
					midilink->ml_SysExFilter.sxf_ID1!=msg2->status ||
					midilink->ml_SysExFilter.sxf_ID1!=msg2->status
				){
					mymidinode->sysex_write=mymidinode->sysex_laststart;
					return;
				}
			}else{
			  if(midilink->ml_SysExFilter.sxf_ID1==msg2->status) goto outofhere;
			  if(midilink->ml_SysExFilter.sxf_ID2==msg2->status) goto outofhere;
			  if(midilink->ml_SysExFilter.sxf_ID3==msg2->status) goto outofhere;
			  mymidinode->sysex_write=mymidinode->sysex_laststart;
			  return;
			}
		}
outofhere:
		msg2->data2=msg2->data1;
		msg2->data1=msg2->status;
		msg2->status=0xf0;
	}


	if(mymidinode->midinode.mi_ReceiveHook!=NULL){

		/* I haven`t found any documentation about what the hooks carry. But the
		   camd in tool hook for Barsnpipes has the following proto:

	      static ULONG __asm __saveds midiinhook(register __a0 struct Hook *hook,
                                       register __a2 struct MidiLink *link,
                                       register __a1 MidiMsg *msg,
                                       register __d0 long sysexlen,
                                       register __a3 void *sysexdata);
		... So I do that. -K.Matheussen.
		*/
#if 0
		CallHookPkt(mymidinode->midinode.mi_ReceiveHook,NULL,NULL);
#endif

		hmsg.mm_Time=timestamp;

		hmsg.mm_Status=msg2->status;
		if(msg2->len>1){
			hmsg.mm_Data1=msg2->data1;
		}else{
			hmsg.mm_Data1=0;
		}
		if(msg2->len>2){
			hmsg.mm_Data2=msg2->data2;
		}else{
			hmsg.mm_Data2=0;
		}
		hmsg.mm_Port=midilink->ml_PortID;

		(*
			(
#ifdef __amigaos4__
				(ULONG (* ASM)(
						REG(a0, struct Hook*),
						REG(a2, struct MidiLink*),
						REG(a1, MidiMsg*),
						REG(d0, long),
						REG(a3, void*)
					)
#else
				 (ULONG (* ASM)(
						REG(a0) struct Hook*,
						REG(a2) struct MidiLink*,
						REG(a1) MidiMsg*,
						REG(d0) long,
						REG(a3) void*
					)
#endif
				)
				(mymidinode->midinode.mi_ReceiveHook->h_Entry)
			)
		)(
			mymidinode->midinode.mi_ReceiveHook,
			midilink,
			&hmsg,
			hmsg.mm_Status==0xf0?GetSysXLen(mymidinode->sysex_laststart):0L,
			hmsg.mm_Status==0xf0?mymidinode->sysex_laststart:NULL
		);


	}else{

		msg=mymidinode->in_curr;
		if(msg==NULL){					//If no buffer.
			return;
		}

		if(mymidinode->unpicked+1==mymidinode->midinode.mi_MsgQueueSize-1){
			mymidinode->error |= CMEF_BufferFull;
			return;
		}

		msg->mm_Time=timestamp;

		msg->mm_Status=msg2->status;
		if(msg2->len>1){
			msg->mm_Data1=msg2->data1;
		}else{
			msg->mm_Data1=0;
		}
		if(msg2->len>2){
			msg->mm_Data2=msg2->data2;
		}else{
			msg->mm_Data2=0;
		}
		msg->mm_Port=midilink->ml_PortID;

		mymidinode->unpicked++;

		mymidinode->in_curr++;
		if(mymidinode->in_curr==mymidinode->in_end){
			mymidinode->in_curr=mymidinode->in_start;
		}

		if(mymidinode->midinode.mi_ReceiveSigBit!=-1){
			Signal(mymidinode->midinode.mi_SigTask,1L<<mymidinode->midinode.mi_ReceiveSigBit);
		}

	}
}




