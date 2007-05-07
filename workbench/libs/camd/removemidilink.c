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

	AROS_LH1(void, RemoveMidiLink,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, midilink, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 15, Camd)

/*  FUNCTION
		Removes and frees a midilink from the system.

    INPUTS
		midilink - pointer to midilink to remove.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);

		UnlinkMidiLink(midilink,TRUE,CamdBase);

	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	if(midilink->ml_ParserData!=NULL) FreeMem(midilink->ml_ParserData,sizeof(struct DriverData));
	FreeMem(midilink,sizeof(struct MidiLink));

   AROS_LIBFUNC_EXIT
}

