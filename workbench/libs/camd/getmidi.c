/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH2(BOOL, GetMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),
	AROS_LHA(MidiMsg *, msg, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 24, Camd)

/*  FUNCTION
		Gets a message from a midinodes buffer.

    INPUTS
		midinode - pointer to midinode
		msg - The message is removed from the internal buffer and copied into msg.

    RESULT
		TRUE if message was copied, FALSE if buffer was empty.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		WaitMidi

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;
	struct MyMidiMessage *mymsg;

	if(mymidinode->unpicked==0){
		return FALSE;
	}
	mymsg=mymidinode->in_curr_get;

	msg->mm_Time=mymsg->timestamp;
	msg->mm_Status=mymsg->m[0];
	msg->mm_Data1=mymsg->m[1];
	msg->mm_Data2=mymsg->m[2];
	msg->mm_Port=mymsg->m[3];

	mymidinode->unpicked--;

	mymidinode->in_curr_get++;
	if(mymidinode->in_curr_get==mymidinode->in_end){
		mymidinode->in_curr_get=mymidinode->in_start;
	}

	mymidinode->lastreadstatus=msg->mm_Status;

	// Hmmm, union, have to check...
	msg->mm_Msg=(msg->mm_Status<<24)|(msg->mm_Data1<<16)|(msg->mm_Data2<<8);

	return TRUE;

   AROS_LIBFUNC_EXIT
}



