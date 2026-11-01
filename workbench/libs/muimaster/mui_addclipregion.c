/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

#include "debug.h"

/*****************************************************************************

    NAME */
        AROS_LH2(APTR, MUI_AddClipRegion,

/*  SYNOPSIS */
        AROS_LHA(struct MUI_RenderInfo *, mri, A0),
        AROS_LHA(struct Region *, r, A1),

/*  LOCATION */
        struct Library *, MUIMasterBase, 30, MUIMaster)

/*  FUNCTION
        Push a region onto the rendering context's clipping stack and install
        it on the window layer, or the RastPort layer when no window exists.
        If another MUI clip is active, intersect r with that region first.

    INPUTS
        mri - Non-NULL initialized rendering context. When mri_Window is
              NULL, mri_RastPort must be valid.
        r - Region whose ownership is transferred to this call, or NULL.

    RESULT
        The previous layer clip-region pointer as a removal token, including
        NULL when no previous region exists. Returns (APTR)-1 if r or the
        target layer is NULL or the 20-entry MUI clipping stack is full.

    NOTES
        The function modifies r and disposes of it on rejection. After a
        successful push, MUI_RemoveClipRegion() disposes of it. Do not free
        or modify the region after passing it here.
        Remove successful pushes in reverse order using the same rendering
        context. Preserve the layer and context until all clips are removed.
        Refresh processing is temporarily suspended and resumed when needed.

    BUGS
        Failure of the region intersection is not checked. Removal restores
        the previous MUI stack entry, not an arbitrary pre-existing layer
        clip region represented by the returned token. Do not mix this
        stack with independently installed layer clip regions.

    SEE ALSO
        MUI_RemoveClipRegion(), MUI_AddClipping(), layers.library/InstallClipRegion()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Window *w = mri->mri_Window;
    struct Layer  *l;
    APTR result;

    if (w != NULL)
        l = w->WLayer;
    else
        l = mri->mri_RastPort->Layer;

    // if (mri->mri_rCount == MRI_RARRAY_SIZE)
    //    kprintf(" --------- mui_addclipregion clip overflow ---------------------\n");
    
    if ((l == NULL) || (r == NULL) || (mri->mri_rCount == MRI_RARRAY_SIZE))
    {
        if (r)
            DisposeRegion(r);
        return (APTR)-1;
    }
    
    if (mri->mri_rCount != 0)
        /* NOTE: ignoring the result here... */
        AndRegionRegion(mri->mri_rArray[mri->mri_rCount-1], r);

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
    {
            LockLayerInfo(&w->WScreen->LayerInfo);
        EndRefresh(w, FALSE);
    }

#if 1 /* stegerg: what's this good for? */
    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        LockLayerInfo(&w->WScreen->LayerInfo);
#endif

    result = InstallClipRegion(l, r);

#if 1 /* stegerg: what's this good for? */
    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        UnlockLayerInfo(&w->WScreen->LayerInfo);
#endif

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
    {
        BeginRefresh(w);
        UnlockLayerInfo(&w->WScreen->LayerInfo);
    }
    
    mri->mri_rArray[mri->mri_rCount++] = r;

    return result;

    AROS_LIBFUNC_EXIT

} /* MUIA_AddClipRegion */
