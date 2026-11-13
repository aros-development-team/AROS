/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
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
        struct Library *, MUIMasterBase, 32, MUIMaster)

/*  FUNCTION
        Begin a refresh pass for a damaged simple-refresh window. Set the
        rendering context's refresh flag, lock the screen's LayerInfo, and
        call intuition.library/BeginRefresh().

    INPUTS
        mri - Non-NULL initialized rendering context. A present window must
              have a valid window layer and screen.
        flags - Ignored by this implementation; pass zero.

    RESULT
        TRUE if a refresh pass was begun. FALSE if there is no window, the
        window is not simple-refresh, its layer does not need refreshing,
        or this rendering context is already in refresh mode.

    NOTES
        Only after TRUE, complete the pass with MUI_EndRefresh() on the
        same context. The LayerInfo lock remains held between these calls.
        Do not close the window or change the context during that interval.

    SEE ALSO
        MUI_EndRefresh(), intuition.library/BeginRefresh()

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
