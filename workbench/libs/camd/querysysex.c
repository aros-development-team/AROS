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

	AROS_LH1(ULONG, QuerySysEx,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 28, Camd)

/*  FUNCTION
       Returns the number of bytes remaining in the current sys/ex message.

    INPUTS
       midinode - pointer to MidiNode

    RESULT
       Remaining bytes in sys/ex message.      0 is returned if the last
       message read from GetMidi() wasn't a sys/ex message.

    NOTES

    EXAMPLE

    BUGS
		Tested.

    SEE ALSO
		SkipSysEx(), GetSysEx()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created
	2005-06-30 Lyle Hazelwood fixed sum to include EOX byte
	2006-01-28 fixed wraparound bug (buffer overflow)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;
	UBYTE *sysex;
	ULONG numleft=1;

	if(mymidinode->lastreadstatus!=0xf0) return 0;

	ObtainSemaphore(&mymidinode->sysexsemaphore2);

		if(mymidinode->sysex_nextis0==TRUE){
			ReleaseSemaphore(&mymidinode->sysexsemaphore2);
			return 0;
		}

		sysex=mymidinode->sysex_read;

		while(*sysex!=0xf7){
			numleft++;
			sysex++;
			if(sysex == mymidinode->sysex_end)
                          sysex = mymidinode->sysex_start;
		}

	ReleaseSemaphore(&mymidinode->sysexsemaphore2);

	return numleft;

   AROS_LIBFUNC_EXIT
}



