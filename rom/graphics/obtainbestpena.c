/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Obtain the best pen available for a given color
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH5(LONG, ObtainBestPenA,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm  , A0),
	AROS_LHA(ULONG            , r   , D1),
	AROS_LHA(ULONG            , g   , D2),
	AROS_LHA(ULONG            , b   , D3),
	AROS_LHA(struct TagItem * , tags, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 140, Graphics)

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


#warning TODO: Write graphics/ObtainBestPenA()
    aros_print_not_implemented ("ObtainBestPenA");

    return 1;

    AROS_LIBFUNC_EXIT
} /* ObtainBestPenA */
