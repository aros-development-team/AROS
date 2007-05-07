/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include "camd_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH1(WORD, MidiMsgType,

/*  SYNOPSIS */
	AROS_LHA(MidiMsg *, msg, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 31, Camd)

/*  FUNCTION
		Return the type of a message (see <midi/camd.h>). sysex messages
		returns -1.

    INPUTS
		msg - midimessage.

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

	if(msg->mm_Status==0xf0 || msg->mm_Status==0xf7) return -1;

	return MidiMsgType_status_data1(msg->mm_Status,msg->mm_Data1);

   AROS_LIBFUNC_EXIT
}

