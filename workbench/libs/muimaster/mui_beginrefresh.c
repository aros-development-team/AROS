/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(BOOL, MUI_BeginRefresh,

/*  SYNOPSIS */
	AROS_LHA( struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 28, MUIMaster)

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
    struct Layer  *l;

    if ((w == NULL) || !(w->Flags & WFLG_SIMPLE_REFRESH))
        return 0;

    l = w->WLayer;

    /* doesn't need refreshing */
    if (!(l->Flags & LAYERREFRESH))
        return 0;

    /* already refreshing */
    if (mri->mri_Flags & MUIMRI_REFRESHMODE)
        return 0;

    mri->mri_Flags |= MUIMRI_REFRESHMODE;
    LockLayerInfo(&w->WScreen->LayerInfo);
    BeginRefresh(w);
    return 1;

    AROS_LIBFUNC_EXIT

} /* MUIA_BeginRefresh */
