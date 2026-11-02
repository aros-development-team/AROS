/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
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
        struct Library *, MUIMasterBase, 33, MUIMaster)
/*  FUNCTION
        Finish a refresh pass with intuition.library/EndRefresh(window, TRUE),
        unlock the screen's LayerInfo, and clear the context's refresh flag.

    INPUTS
        mri - Rendering context for a successful MUI_BeginRefresh() call,
              with the same window and screen still valid.
        flags - Ignored by this implementation; pass zero.

    RESULT
        None.

    NOTES
        Call exactly once after MUI_BeginRefresh() returns TRUE, in the same
        task. This function does not check whether refresh mode is active.
        A NULL mri_Window causes an early return without clearing the flag
        or releasing the lock; it is not a substitute for a matched end.

    SEE ALSO
        MUI_BeginRefresh(), intuition.library/EndRefresh()

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
