/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/dos.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(struct MidiDeviceData *, OpenMidiDevice,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 34, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
		May not work for AROS.

    SEE ALSO
		CloseMidiDevice

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)


	BPTR seglist;
	struct SegmentSak *myseglist;
	
	seglist=LoadSeg(name);

	if(seglist==NULL) return NULL;

	myseglist=BADDR(seglist);

	return (struct MidiDeviceData *)&myseglist->mididevicedata;

   AROS_LIBFUNC_EXIT

}



