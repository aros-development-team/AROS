/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/graphics.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

#define MUIPEN_HIMASK 0xFFFF0000   /* ??? */

/*****************************************************************************

    NAME */
        AROS_LH2(VOID, MUI_ReleasePen,

/*  SYNOPSIS */
        AROS_LHA(struct MUI_RenderInfo *, mri, A0),
        AROS_LHA(LONG, pen, D0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 27, MUIMaster)

/*  FUNCTION
        Release the colormap allocation recorded in a MUI_ObtainPen() token.
        Existing, unallocated pens require no release and are left alone.

    INPUTS
        mri - Non-NULL rendering context used to obtain the token, with its
              original colormap still valid.
        pen - Complete token returned by MUI_ObtainPen(), or -1 to do nothing.

    RESULT
        None.

    NOTES
        Release each allocated token once. Do not mask the token with
        MUIPEN() before passing it here: doing so removes the allocation
        flag. A -1 token is ignored before mri is accessed.

    SEE ALSO
        MUI_ObtainPen(), graphics.library/ReleasePen()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (pen == -1)
        return;

    if (mri->mri_Colormap && ((pen & MUIPEN_HIMASK) == 0x10000))
    {
        ReleasePen(mri->mri_Colormap, MUIPEN(pen));
    }
    
    AROS_LIBFUNC_EXIT

} /* MUIA_ReleasePen */
