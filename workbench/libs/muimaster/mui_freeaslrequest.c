/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/asl.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm VOID MUI_FreeAslRequest(register __a0 APTR requester)
#else
	AROS_LH1(ULONG, MUI_FreeAslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR, requester, A0),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 10, MUIMaster)
#endif
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
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

    FreeAslRequest(requester);

    AROS_LIBFUNC_EXIT

} /* MUIA_FreeAslRequestA */
