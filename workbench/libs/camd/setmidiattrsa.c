/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/


#include <proto/utility.h>
#include <proto/exec.h>

#include "camd_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH2(BOOL, SetMidiAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 9, Camd)

/*  FUNCTION

    INPUTS
		tags - Pointer to an array of tags. Description of the
		       tags are not available yet.

    RESULT
		TRUE if everything went okey, FALSE if not. Errorcode
		is put in an ULONG pointed to by the MIDI_ErrorCode tag,
		if supplied.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		GetMidiAttrsA

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct TagItem *tag;
	const struct TagItem *tstate=tags;
	struct MyMidiMessage *temp;
	UBYTE *temp2;
	BOOL ret=TRUE;

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;

	ULONG *ErrorCode=(ULONG *)GetTagData(MIDI_ErrorCode,NULL,tags);

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);

	while((tag=NextTagItem(&tstate))){
		switch(tag->ti_Tag){
			case MIDI_Name:
				midinode->mi_Node.ln_Name=(char *)tag->ti_Data;
				break;
			case MIDI_SignalTask:
				midinode->mi_SigTask=(struct Task *)tag->ti_Data;
				break;
			case MIDI_RecvHook:
				midinode->mi_ReceiveHook=(struct Hook *)tag->ti_Data;
				break;
			case MIDI_PartHook:
				midinode->mi_ParticipantHook=(struct Hook *)tag->ti_Data;
				break;
			case MIDI_RecvSignal:
				midinode->mi_ReceiveSigBit=(BYTE)tag->ti_Data;
				break;
			case MIDI_PartSignal:
				midinode->mi_ParticipantSigBit=(BYTE)tag->ti_Data;
				break;
			case MIDI_MsgQueue:
				temp=AllocVec(sizeof(struct MyMidiMessage)*(tag->ti_Data+1),MEMF_ANY | MEMF_PUBLIC);
				if(temp==NULL){
					if(ErrorCode!=NULL){
						*ErrorCode=CME_NoMem;
					}
					ret=FALSE;
					break;
				}
				ObtainSemaphore(&mymidinode->receiversemaphore);
				midinode->mi_MsgQueueSize=(ULONG)tag->ti_Data;
				if(mymidinode->in_start!=NULL){
					FreeVec(mymidinode->in_start);
				}
				mymidinode->in_start=temp;
				mymidinode->in_end=mymidinode->in_start+midinode->mi_MsgQueueSize;
				mymidinode->unpicked=0;
				mymidinode->in_curr=mymidinode->in_start;
				mymidinode->in_curr_get=mymidinode->in_start;
				ReleaseSemaphore(&mymidinode->receiversemaphore);
				break;
			case MIDI_SysExSize:
				temp2=AllocVec(tag->ti_Data+1,MEMF_ANY|MEMF_PUBLIC);
				if(temp2==NULL){
					if(ErrorCode!=NULL){
						*ErrorCode=CME_NoMem;
					}
					ret=FALSE;
					break;
				}
				ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
				ObtainSemaphore(&mymidinode->sysexsemaphore2);
				ObtainSemaphore(&mymidinode->sysexsemaphore);
				midinode->mi_SysExQueueSize=tag->ti_Data;
				if(mymidinode->sysex_start!=NULL){
					FreeVec(mymidinode->sysex_start);
				}
				mymidinode->sysex_start=temp2;
				mymidinode->sysex_write=mymidinode->sysex_start;
				mymidinode->sysex_end=mymidinode->sysex_start+midinode->mi_SysExQueueSize;
				mymidinode->sysex_read=mymidinode->sysex_start;
				ReleaseSemaphore(&mymidinode->sysexsemaphore2);
				ReleaseSemaphore(&mymidinode->sysexsemaphore);
				ObtainSemaphore(CB(CamdBase)->CLSemaphore);
				break;
			case MIDI_TimeStamp:
				midinode->mi_TimeStamp=(ULONG *)tag->ti_Data;
				break;
			case MIDI_ErrFilter:
				midinode->mi_ErrFilter=(UBYTE)tag->ti_Data;
				break;
			case MIDI_ClientType:
				midinode->mi_ClientType=(UWORD)tag->ti_Data;
				break;
			case MIDI_Image:
				midinode->mi_Image=(struct Image *)tag->ti_Data;
				break;
			default:
				break;
		}
	}

	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	return ret;


   AROS_LIBFUNC_EXIT
}


