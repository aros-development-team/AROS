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

	AROS_LH3(BOOL, Midi2Driver,

/*  SYNOPSIS */
	AROS_LHA(APTR, driverdata, A0),
	AROS_LHA(ULONG, msg, D0),
	AROS_LHA(ULONG, maxbuff, D1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 40, Camd)

/*  FUNCTION
		This is a private function, and will probably be obsolete. Please don`t use.

    INPUTS

    RESULT
    	TRUE if max(buffer,maxbuffer) was big enough to hold the message, FALSE if not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		PutMidi(), GoodPutMidi(), PutMidiMsg()

    INTERNALS

    HISTORY

	2001-07-14 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	return

	Midi2Driver_internal(
		(struct DriverData *)driverdata,
		msg,
		maxbuff
	);


   AROS_LIBFUNC_EXIT
}







