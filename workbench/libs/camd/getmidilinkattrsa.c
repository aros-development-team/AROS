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

	AROS_LH2(ULONG, GetMidiLinkAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 17, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES
		If you are not the owner of the midilink, you should lock
		Camd before calling to ensure that it wont go away.
		Theres no point in locking if you know it wont go away.

    EXAMPLE

    BUGS

    SEE ALSO
		SetMidiLinkAttrsA

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct TagItem *tag;
	const struct TagItem *tstate=tags;
	ULONG *where;
	ULONG ret=0;

	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);

	while((tag=NextTagItem(&tstate))){
		ret++;
		where=(ULONG *)tag->ti_Tag;
		switch(tag->ti_Tag){
			case MIDI_Name:
				*where=(ULONG)midinode->mi_Node.ln_Name;
				break;
			case MIDI_SignalTask:
				*where=(ULONG)midinode->mi_SigTask;
				break;
			case MIDI_RecvHook:
				*where=(ULONG)midinode->mi_ReceiveHook;
				break;
			case MIDI_PartHook:
				*where=(ULONG)midinode->mi_ParticipantHook;
				break;
			case MIDI_RecvSignal:
				*where=(ULONG)midinode->mi_ReceiveSigBit;
				break;
			case MIDI_PartSignal:
				*where=(ULONG)midinode->mi_ParticipantSigBit;
				break;
			case MIDI_MsgQueue:
				*where=(ULONG)midinode->mi_MsgQueueSize;
				break;
			case MIDI_SysExSize:
				*where=(ULONG)midinode->mi_SysExQueueSize;
				break;
			case MIDI_TimeStamp:
				*where=(ULONG)midinode->mi_TimeStamp;
				break;
			case MIDI_ErrFilter:
				*where=(ULONG)midinode->mi_ErrFilter;
				break;
			case MIDI_ClientType:
				*where=(ULONG)midinode->mi_ClientType;
				break;
			case MIDI_Image:
				*where=(ULONG)midinode->mi_Image;
				break;
			default:
				ret--;
				break;
		}
	}

	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	return ret;


   AROS_LIBFUNC_EXIT
}


