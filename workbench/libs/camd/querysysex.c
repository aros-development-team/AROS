/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
		Not tested. SysEx receiving does probably have some bugs.

    SEE ALSO
		SkipSysEx, GetSysEx

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

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
		}

	ReleaseSemaphore(&mymidinode->sysexsemaphore2);

	return numleft;

   AROS_LIBFUNC_EXIT
}



