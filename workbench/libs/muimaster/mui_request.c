/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm LONG MUI_RequestA(register __d0 APTR app, register __d1 APTR win, register __d2 LONGBITS flags, register __a0 char *title, register __a1 char *gadgets, register __a2 char *format, register __a3 APTR params)
#else
	AROS_LH7(LONG, MUI_RequestA,

/*  SYNOPSIS */
	AROS_LHA(APTR, app, D0),
	AROS_LHA(APTR, win, D1),
	AROS_LHA(LONGBITS, flags, D2),
	AROS_LHA(char *, title, A0),
	AROS_LHA(char *, gadgets, A1),
	AROS_LHA(char *, format, A2),
	AROS_LHA(APTR, params, A3),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 7, MUIMaster)
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

    AROS_LIBFUNC_EXIT

} /* MUIA_RequestA */
