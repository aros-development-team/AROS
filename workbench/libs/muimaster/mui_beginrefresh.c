/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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
__asm BOOL MUI_BeginRefresh(register __a0 struct MUI_RenderInfo *mri, register __d0 ULONG flags)
#else
	AROS_LH2(ULONG, MUI_BeginRefresh,

/*  SYNOPSIS */
	AROS_LHA( struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 28, MUIMaster)
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
