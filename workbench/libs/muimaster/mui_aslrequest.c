/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>
#include <proto/asl.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(BOOL, MUI_AslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR, requester, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 9, MUIMaster)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AslRequest(requester,tagList);

    AROS_LIBFUNC_EXIT

} /* MUIA_AslRequest */
