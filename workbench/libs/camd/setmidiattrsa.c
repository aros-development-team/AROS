/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
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
    tagList  --  pointer to an array of tags describing the player's
                 attributes or NULL.

    TAGS
    MIDI_Name (STRPTR) -- The name of the midinode; default is NULL or a pointer to a string.

    MIDI_SignalTask (struct Task *) -- Task to signal whenever a midimessage is arriving to the node;
                                       default is the task of the caller of this function. (FindTask(NULL))
       
    MIDI_RecvHook (struct Hook *)   -- Function to call whenever a midimessage is arriving to the node.
                                       You should get the midimessage as the first argument in the function,
				       however, that has not yet been implemented. Default is NULL.

    MIDI_PartHook (struct Hook *)   -- Don't really know what this one is for. Have to check amigos-autodocs.
                                       It does not currently do anything.

    MIDI_RecvSignal (BYTE)          -- Signal bit to use when signalling a task whenever a midimessage is
                                       arriving at the node, or -1 to disable signalling. Default is -1.

    MIDI_PartSignal (BYTE)          -- Signal bit to use when signalling a task when..... Default is -1.

    MIDI_MsgQueue (ULONG)           -- Number of messages the messagequeue is able to hold.

    MIDI_TimeStamp (ULONG *)        -- Pointer to an ULONG value which value is copied directly into the timestamp
                                       attribute in midimessages whenever a new message is received at the node.


     MIDI_ErrFilter (UBYTE)         -- Filters out the errors you don't want to see.


     MIDI_ClientType (UWORD)        -- What sort of application you that owns this node.

     MIDI_Image (struct Image *)    -- Pointer to an image representing this node.

     MIDI_ErrorCode (ULONG *)       -- Pointer to an ULONG which will be set if something went wrong.


    RESULT
		TRUE if everything went okey, FALSE if not. Errorcode
		is put in an ULONG pointed to by the MIDI_ErrorCode tag,
		if supplied.

    NOTES
		- If the midinode is not owned by yourself, please lock
		  camd to ensure it wont go away.

		- Allthough you are able to modify midinodes owned by
		  others, please avoid it, its normally "non of your buziness",
		  and may lead to crashes and other "unexpected" behaviors.
		  However, if you have full control of the owner of the
		  midinode (f.ex when both you and the owner belongs to the
		  same probram and you are absolutely shure you know what
		  you are doing), there is no problem.


    EXAMPLE

    BUGS

    SEE ALSO
		GetMidiAttrsA

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct TagItem *tag;
	const struct TagItem *tstate=tags;
	MidiMsg *temp;
	UBYTE *temp2;
	BOOL ret=TRUE;

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;

	ULONG *ErrorCode = (ULONG *) GetTagData(MIDI_ErrorCode, (IPTR) NULL, tags);

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
				temp=AllocVec(sizeof(MidiMsg)*(tag->ti_Data+1),MEMF_ANY | MEMF_PUBLIC);
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


