/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Set the SoftStyle bits of the current font
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH3(ULONG, SetSoftStyle,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , style, D0),
	AROS_LHA(ULONG            , enable, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 15, Graphics)

/*  FUNCTION
	This function changes the SoftStyle of the current font in RastPort.
	Only the bits which are set in enable will be affected and the
	New value of the style will be returned, because the font may not
	support all bits.

    INPUTS
	rp - The RastPort from which the font's SoftStyle bits will altered
	style - The new font style to be set, masked by enable.
	enable - The mask of style bits to be set (1) or left untouched (0)

    RESULT
	The new style, as the font may not support all bits.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AskSoftStyle()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)


#warning TODO: Write graphics/SetSoftStyle()
    aros_print_not_implemented ("SetSoftStyle");

    return rp->Font->tf_Style;

    AROS_LIBFUNC_EXIT
} /* SetSoftStyle */
