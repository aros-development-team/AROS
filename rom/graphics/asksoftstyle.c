/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Get the SoftStyle bits of the current font
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH1(ULONG, AskSoftStyle,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 14, Graphics)

/*  FUNCTION
	Returns the style bits of the current font which are not instrinsic
	to the font and therefore valid to be set with SetSoftStyle().

    INPUTS
	rp - The RastPort from which the font's SoftStyle bits will be extracted

    RESULT
	The bits of the style which are algorithmically generated.
	Bits which are unsupported by the font are also set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SetSoftStyle()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)


#warning TODO: Write graphics/AskSoftStyle()
    aros_print_not_implemented ("AskSoftStyle");

    return 0L;

    AROS_LIBFUNC_EXIT
} /* AskSoftStyle */
