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

	AROS_LH1(void, SkipSysEx,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 29, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		QuerySysEx(), GetSysEx()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;
	UBYTE data;

	ObtainSemaphore(&mymidinode->sysexsemaphore2);

		if(
			mymidinode->lastreadstatus!=0xf0
		){
			ReleaseSemaphore(&mymidinode->sysexsemaphore2);
			return;
		}

		mymidinode->sysex_nextis0=TRUE;

		do{
			data=*mymidinode->sysex_read;
			mymidinode->sysex_read++;
			if(mymidinode->sysex_read==mymidinode->sysex_end){
				mymidinode->sysex_read=mymidinode->sysex_start;
			}
		}while(data!=0xf7);

	ReleaseSemaphore(&mymidinode->sysexsemaphore2);

   AROS_LIBFUNC_EXIT
}



