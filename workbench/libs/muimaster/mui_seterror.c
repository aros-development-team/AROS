/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm LONG MUI_SetError(register __d0 LONG num)
#else
	AROS_LH1(LONG, MUI_SetError,

/*  SYNOPSIS */
	AROS_LHA(LONG, num, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 12, MUIMaster)
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

    return 0;

    AROS_LIBFUNC_EXIT

} /* MUIA_SetError */
