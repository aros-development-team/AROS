/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Release a pen previously allocated.
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH2(void, ReleasePen,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm, A0),
	AROS_LHA(ULONG            , n , D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 158, Graphics)

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


#warning TODO: Write graphics/ReleasePen()
    aros_print_not_implemented ("ReleasePen");


    AROS_LIBFUNC_EXIT
} /* ReleasePen */
