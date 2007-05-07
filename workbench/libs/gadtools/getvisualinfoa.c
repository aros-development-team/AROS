/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <intuition/screens.h>
#include <utility/tagitem.h>

	AROS_LH2(APTR, GetVisualInfoA,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct Library *, GadToolsBase, 21, GadTools)

/*  FUNCTION
	GetVisualInfoA() creates a visual info structure, which is needed
	by several gadtools functions. When you're done using it, you have
	to call FreeVisualInfo().

    INPUTS
	screen -  pointer to the screen to create a visual info structure for
	          (may be NULL)
	taglist - additional tags (none defined, yet)

    RESULT
	A (private) visual info structure. NULL indicates an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeVisualInfo()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VisualInfo *vi;

    if (screen == NULL)
	return NULL;

    vi = AllocVec(sizeof(struct VisualInfo), MEMF_ANY);
    if (!vi)
        return NULL;

    vi->vi_screen = screen;
    vi->vi_dri = GetScreenDrawInfo(screen);

    return vi;

    AROS_LIBFUNC_EXIT
    
} /* GetVisualInfoA */
