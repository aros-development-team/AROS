/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdlib.h>

#include "mui.h"
#include "penspec.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH3(LONG, MUI_ObtainPen,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(struct MUI_PenSpec *, spec, A1),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 22, MUIMaster)

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

    LONG retval = -1;
    struct MUI_PenSpec_intern intern;
    
    if (!spec || !mri || !mri->mri_Colormap) return -1;

    if (!zune_pen_spec_to_intern(spec, &intern))
	return -1;
    if (!zune_penspec_setup(&intern, mri))
	return -1;
    retval = intern.p_pen;
    if ((retval != -1) && (intern.p_is_allocated))
    {
	/* flag to indicate that ReleasePen() needs to be called
	   in MUI_ReleasePen() */
		       
	retval |= 0x10000;
    }

    return retval;

    AROS_LIBFUNC_EXIT

} /* MUIA_ObtainPen */
