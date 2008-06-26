/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>
#include <proto/asl.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH1(VOID, MUI_FreeAslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR, requester, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 10, MUIMaster)

/*  FUNCTION
	Interface to asl.library

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	asl.library/FreeAslRequest()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeAslRequest(requester);

    AROS_LIBFUNC_EXIT

} /* MUIA_FreeAslRequestA */
