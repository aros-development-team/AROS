/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm APTR MUI_AddClipRegion(register __a0 struct MUI_RenderInfo *mri, register __a1 struct Region *r)
#else
	AROS_LH2(APTR, MUI_AddClipRegion,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(struct Region *, r, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 26, MUIMaster)
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

    struct Window *w = mri->mri_Window;
    struct Layer  *l;
    APTR result;

    if (w != NULL)
        l = w->WLayer;
    else
        l = mri->mri_RastPort->Layer;

    if ((l == NULL) || (r == NULL) || (mri->mri_rCount == 10))
        return (APTR)-1;

    if (mri->mri_rCount != 0)
        /* NOTE: ignoring the result here... */
        AndRegionRegion(mri->mri_rArray[mri->mri_rCount-1], r);

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        EndRefresh(w, FALSE);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        LockLayerInfo(&w->WScreen->LayerInfo);

    result = InstallClipRegion(l, r);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        UnlockLayerInfo(&w->WScreen->LayerInfo);

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        BeginRefresh(w);

    mri->mri_rArray[mri->mri_rCount++] = r;

    return result;

    AROS_LIBFUNC_EXIT

} /* MUIA_AddClipRegion */
