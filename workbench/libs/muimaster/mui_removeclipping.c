/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH2(VOID, MUI_RemoveClipping,

/*  SYNOPSIS */
        AROS_LHA(struct MUI_RenderInfo *, mri, A0),
        AROS_LHA(APTR, handle, A1),

/*  LOCATION */
        struct Library *, MUIMasterBase, 29, MUIMaster)

/*  FUNCTION
        Remove a rectangular clip by forwarding the token to
        MUI_RemoveClipRegion(). The region allocated for the clip is freed.

    INPUTS
        mri - Rendering context used for the corresponding MUI_AddClipping().
        handle - Exact token returned by MUI_AddClipping(), including NULL
                 or the no-push sentinel (APTR)-1.

    RESULT
        None.

    NOTES
        Remove nested clips in reverse order. A token other than (APTR)-1
        pops the top MUI clipping region; it does not select a stack entry.
        The rendering context must remain valid even for the sentinel.

    SEE ALSO
        MUI_AddClipping(), MUI_RemoveClipRegion()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    MUI_RemoveClipRegion(mri, handle);

    AROS_LIBFUNC_EXIT

} /* MUIA_RemoveClipping */
