/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <clib/alib_protos.h>
#include <intuition/classusr.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm VOID MUI_Redraw(register __a0 Object *obj, register __d0 ULONG flags)
#else
	AROS_LH2(VOID, MUI_Redraw,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 17, MUIMaster)
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

    if (!(muiAreaData(obj)->mad_Flags & MADF_CANDRAW)) return;

    muiRenderInfo(obj)->mri_ClipRect.MinX = _left(obj);
    muiRenderInfo(obj)->mri_ClipRect.MinY = _top(obj);
    muiRenderInfo(obj)->mri_ClipRect.MaxX = _right(obj);
    muiRenderInfo(obj)->mri_ClipRect.MaxY = _bottom(obj);

    DoMethod(obj, MUIM_Draw, flags);

    AROS_LIBFUNC_EXIT

} /* MUIA_Redraw */
