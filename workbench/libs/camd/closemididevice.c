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

	AROS_LH1(void, CloseMidiDevice,

/*  SYNOPSIS */
	AROS_LHA(struct MidiDeviceData *, mididevicedata, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 35, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
		May not work for AROS.

    SEE ALSO
		OpenMidiDevice

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	UBYTE *temp=(UBYTE *)mididevicedata;

//#warning Fix camd/closemididevice next line
	temp=temp-8;

	UnLoadSeg(MKBADDR(temp));

   AROS_LIBFUNC_EXIT
}

