/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS specific Input device function AddNullEvent()
    Lang: english
*/

#include <proto/exec.h>
#include "input_intern.h"

#define InputDevice ((struct inputbase *)InputBase)

/*****************************************************************************

    NAME */
#include <clib/input_protos.h>

	AROS_LH0(void, AddNullEvent,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct Device *, InputBase, 20, Input)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (InputDevice->InputTask)
    {
    	Signal(InputDevice->InputTask, SIGBREAKF_CTRL_F);
    }

    AROS_LIBFUNC_EXIT
} /* AddNullEvent */
