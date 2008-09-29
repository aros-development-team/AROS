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

	AROS_LH1(void, FlushMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 13, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
		Not tested.

    SEE ALSO
		GetMidi(), GetSysEx()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);
	ObtainSemaphore(&mymidinode->receiversemaphore);
		mymidinode->unpicked=0;
		mymidinode->in_curr=mymidinode->in_start;
		mymidinode->in_curr_get=mymidinode->in_start;
		mymidinode->error=0;
	ReleaseSemaphore(&mymidinode->receiversemaphore);
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	ObtainSemaphore(&mymidinode->sysexsemaphore2);
	ObtainSemaphore(&mymidinode->sysexsemaphore);
			mymidinode->sysex_read=mymidinode->sysex_start;
			mymidinode->sysex_write=mymidinode->sysex_write;
			mymidinode->sysex_laststart=mymidinode->sysex_write;
	ReleaseSemaphore(&mymidinode->sysexsemaphore);
	ReleaseSemaphore(&mymidinode->sysexsemaphore2);

   AROS_LIBFUNC_EXIT

}

