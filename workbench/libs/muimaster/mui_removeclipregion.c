/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm VOID MUI_RemoveClipRegion(register __a0 struct MUI_RenderInfo *mri, register __a1 APTR handle)
#else
	AROS_LH2(VOID, MUI_RemoveClipRegion,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(APTR, handle, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 27, MUIMaster)
#endif
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

	sba: This function frees the region given in MUI_AddClipRegion, but this seems wrong to me.
	MUI_AddClipRegion should better duplicate the region.

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

    if (handle == (APTR)-1)
        return;

    if (w != NULL)
        l = w->WLayer;
    else
        l = mri->mri_RastPort->Layer;

    if (l == NULL)
        return;

    mri->mri_rCount--;

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        EndRefresh(w, FALSE);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        LockLayerInfo(&w->WScreen->LayerInfo);

    InstallClipRegion(l, (mri->mri_rCount > 0)
        ? mri->mri_rArray[mri->mri_rCount-1] : NULL);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        UnlockLayerInfo(&w->WScreen->LayerInfo);

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        BeginRefresh(w);

    DisposeRegion(mri->mri_rArray[mri->mri_rCount]);
    mri->mri_rArray[mri->mri_rCount] = NULL;

    AROS_LIBFUNC_EXIT

} /* MUIA_RemoveClipRegion */
