/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
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
	struct Library *, MUIMasterBase, 26, MUIMaster)

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

    struct Window *w = mri->mri_Window;
    struct Layer  *l;
    APTR result;

    if (w != NULL)
        l = w->WLayer;
    else
        l = mri->mri_RastPort->Layer;

    // if (mri->mri_rCount == MRI_RARRAY_SIZE) kprintf(" --------- mui_addclipregion clip overflow ---------------------\n");
    
    if ((l == NULL) || (r == NULL) || (mri->mri_rCount == MRI_RARRAY_SIZE))
    {
    	if (r) DisposeRegion(r);
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
