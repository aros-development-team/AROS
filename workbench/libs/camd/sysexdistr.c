/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"


/* Sysex receiving is a bit of a mess... (and not very much tested either) -ksvalast- */

int GetSysXLen(UBYTE *buffer){
	int lokke=1;

	while(buffer[lokke]<0x80){		// "!=0xf7" should allso be correct, but this one is perhaps safer.
		lokke++;
	}

	return lokke;
}

BOOL PutSysEx2Link(struct MidiLink *midilink,UBYTE data){
	UBYTE *sysex;
	struct MyMidiMessage2 msg2;

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midilink->ml_MidiNode;

	if(mymidinode->sysex_laststart==NULL && data!=0xf0){
		return TRUE;	// This midinode was created in the middle of receiving a sysex-string.
	}

	if(mymidinode->sysex_start!=NULL && (!(mymidinode->error&CMEF_SysExFull))){
		if(data==0xff){	// Error in sysex-string. Sysex is cancelled.
			mymidinode->sysex_write=mymidinode->sysex_laststart;
			ReleaseSemaphore(&mymidinode->sysexsemaphore);
			return FALSE;
		}else{
				if(data==0xf0){
					ObtainSemaphore(&mymidinode->sysexsemaphore);
					ObtainSemaphore(&mymidinode->sysexsemaphore2);
					mymidinode->sysex_laststart=mymidinode->sysex_write;
				}else{
					ObtainSemaphore(&mymidinode->sysexsemaphore2);
				}
				*mymidinode->sysex_write=data;
				mymidinode->sysex_write++;
				if(mymidinode->sysex_write==mymidinode->sysex_end){
					mymidinode->sysex_write=mymidinode->sysex_start;
				}
				if(mymidinode->sysex_write==mymidinode->sysex_read){
					mymidinode->error |= CMEF_SysExFull;
					mymidinode->sysex_write=mymidinode->sysex_laststart;
					ReleaseSemaphore(&mymidinode->sysexsemaphore2);
					ReleaseSemaphore(&mymidinode->sysexsemaphore);
					return FALSE;
				}
			ReleaseSemaphore(&mymidinode->sysexsemaphore2);
			if(data==0xf7){
				sysex=mymidinode->sysex_laststart;
				msg2.status=sysex[1];
				msg2.data1=sysex[2];
				msg2.data2=sysex[3];
				msg2.len=3;
				PutMidi2Link(midilink,&msg2,*mymidinode->midinode.mi_TimeStamp);
				ReleaseSemaphore(&mymidinode->sysexsemaphore);
				return FALSE;
			}
		}
		return TRUE;
	}else{
		return FALSE;
	}
}


