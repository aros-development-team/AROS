/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH3(void, LoadRGB4,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),
	AROS_LHA(UWORD           *, colors, A1),
	AROS_LHA(LONG             , count, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 32, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    driver_LoadRGB4 (vp, colors, count, GfxBase);

    AROS_LIBFUNC_EXIT
} /* LoadRGB4 */
