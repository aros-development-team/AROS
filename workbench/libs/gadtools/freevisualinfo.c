/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <proto/intuition.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>

	AROS_LH1(void, FreeVisualInfo,

/*  SYNOPSIS */
	AROS_LHA(APTR, vi, A0),

/*  LOCATION */
	struct Library *, GadToolsBase, 22, GadTools)

/*  FUNCTION
	FreeVisualInfo() frees a visual info structure created with
	GetVisualInfo().

    INPUTS
	vi - the visual info structure to free, may be NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetVisualInfo()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (vi)
    {
	FreeScreenDrawInfo(((struct VisualInfo *)vi)->vi_screen, ((struct VisualInfo *)vi)->vi_dri);
	FreeVec(vi);
    }
    
    AROS_LIBFUNC_EXIT
    
} /* FreeVisualInfo */
