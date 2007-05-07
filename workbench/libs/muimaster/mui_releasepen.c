/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
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
	struct Library *, MUIMasterBase, 23, MUIMaster)

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

    if (pen == -1)
        return;

    if (mri->mri_Colormap && ((pen & MUIPEN_HIMASK) == 0x10000))
    {
        ReleasePen(mri->mri_Colormap, MUIPEN(pen));
    }
    
    AROS_LIBFUNC_EXIT

} /* MUIA_ReleasePen */
