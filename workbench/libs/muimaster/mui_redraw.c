/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <intuition/classusr.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm VOID MUI_Redraw(register __a0 Object *obj, register __d0 ULONG flags)
#else
	AROS_LH2(VOID, MUI_Redraw,

/*  SYNOPSIS */
	AROS_LHA(Object, obj, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 17, MUIMaster)
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


#if 0
    muiRenderInfo(obj)->mri_ClipRect.x      = _left(obj);
    muiRenderInfo(obj)->mri_ClipRect.y      = _top(obj);
    muiRenderInfo(obj)->mri_ClipRect.width  = _width(obj);
    muiRenderInfo(obj)->mri_ClipRect.height = _height(obj);

    DoMethod(obj, MUIM_Draw, flags);
#endif

    AROS_LIBFUNC_EXIT

} /* MUIA_Redraw */
