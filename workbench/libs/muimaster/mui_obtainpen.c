/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <proto/graphics.h>

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm LONG MUI_ObtainPen(register __a0 struct MUI_RenderInfo *mri, register __a1 struct MUI_PenSpec *spec, register __d0 ULONG flags)
#else
	AROS_LH3(LONG, MUI_ObtainPen,

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

    if (!spec || !mri || !mri->mri_Colormap) return -1;

    switch (spec->ps_penType)
    {
	case PST_MUI:
	    if (spec->ps_mui < 0 || spec->ps_mui >= MPEN_COUNT) return -1;
	    return mri->mri_Pens[spec->ps_mui];

	case PST_CMAP:
	    return (LONG)spec->ps_cmap;
	    break;

	case PST_RGB:
	    spec->ps_rgbColor.pixel = ObtainBestPenA(mri->mri_Colormap,
			spec->ps_rgbColor.red   << 16,
			spec->ps_rgbColor.green << 16,
			spec->ps_rgbColor.blue  << 16, NULL);
	    return spec->ps_rgbColor.pixel;
	    break;
    }
    return -1;

    AROS_LIBFUNC_EXIT

} /* MUIA_ObtainPen */
