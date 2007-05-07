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

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;
	MidiMsg *mymsg;

	if(mymidinode->unpicked==0){
		return FALSE;
	}
	mymsg=mymidinode->in_curr_get;

	msg->mm_Msg=mymsg->mm_Msg;
	msg->mm_Time=mymsg->mm_Time;

	mymidinode->unpicked--;

	mymidinode->in_curr_get++;
	if(mymidinode->in_curr_get==mymidinode->in_end){
		mymidinode->in_curr_get=mymidinode->in_start;
	}

	if(msg->mm_Status==0xf0)
	  mymidinode->sysex_nextis0=FALSE;

	mymidinode->lastreadstatus=msg->mm_Status;

	return TRUE;

   AROS_LIBFUNC_EXIT
}



