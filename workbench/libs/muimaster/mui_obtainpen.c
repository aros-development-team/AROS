/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm LONG MUI_ObtainPen(register __a0 struct MUI_RenderInfo *mri, register __a1 struct MUI_PenSpec *spec, register __d0 ULONG flags)
#else
	AROS_LH3(ULONG, MUI_ObtainPen,

/*  SYNOPSIS */
	AROS_LHA(register __a0 struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(register __a1 struct MUI_PenSpec *, spec, A1),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 22, MUIMaster)
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

} /* MUIA_ObtainPen */
