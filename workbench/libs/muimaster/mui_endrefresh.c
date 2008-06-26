/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_EndRefresh,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 29, MUIMaster)
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Window *w = mri->mri_Window;

    if (w == NULL)
        return;

    EndRefresh(w, TRUE);
    UnlockLayerInfo(&w->WScreen->LayerInfo);
    mri->mri_Flags &= ~MUIMRI_REFRESHMODE;
    return;

    AROS_LIBFUNC_EXIT

} /* MUIA_EndRefresh */
