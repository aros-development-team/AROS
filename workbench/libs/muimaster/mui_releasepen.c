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
__asm VOID MUI_ReleasePen(register __a0 struct MUI_RenderInfo *mri, register __d0 LONG pen)

#else
	AROS_LH2(VOID, MUI_ReleasePen,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(LONG, pen, D0),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 23, MUIMaster)
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

} /* MUIA_ReleasePen */
