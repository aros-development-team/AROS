/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <utility/tagitem.h>

	AROS_LH1(void, FreeVisualInfo,

/*  SYNOPSIS */
	AROS_LHA(APTR, vi, A0),

/*  LOCATION */
	struct Library *, GadtoolsBase, 22, Gadtools)

/*  FUNCTION
	FreeVisualInfo() frees a visual info structure create with
	GetVisualInfo().

    INPUTS
	vi - the visual info structure to free.

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
    AROS_LIBBASE_EXT_DECL(struct GadtoolsBase *,GadtoolsBase)

    AROS_LIBFUNC_EXIT
} /* FreeVisualInfo */
