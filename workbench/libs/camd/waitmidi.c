/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/camd.h>

#include "camd_intern.h"

#undef WaitMidi

/*****************************************************************************

    NAME */

	AROS_LH2(BOOL, WaitMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),
	AROS_LHA(MidiMsg *, msg, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 25, Camd)

/*  FUNCTION
		Waits until a new message is received at the node, and
		copy the message to msg.

    INPUTS
		msg - Pointer to a midimessage where the message will be copied.

    RESULT
		Returns TRUE if a new message arrived or had arrived, FALSE, if there
		was en error on the midinode.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		GetMidi()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;
	if(mymidinode->error!=0) return FALSE;

	Wait(1L<<midinode->mi_ReceiveSigBit);

	GetMidi(midinode,msg);

	return TRUE;

   AROS_LIBFUNC_EXIT
}

