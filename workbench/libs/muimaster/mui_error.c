/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH0(LONG, MUI_Error,

/*  SYNOPSIS */

/*  LOCATION */
	struct Library *, MUIMasterBase, 11, MUIMaster)

/*  FUNCTION
	Obsolete function. Use SetIoErr()/IoErr() instead.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MUI_SetError()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT

} /* MUIA_Error */
