/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Remove a font from the list of public available fonts.
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <graphics/text.h>
#include <proto/graphics.h>

	AROS_LH1(void, RemFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont *, textFont, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 81, Graphics)

/*  FUNCTION
	Remove a font from the list of public available fonts. Afterwards,
	you can close it.

    INPUTS
	textFont - Remove this font.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenFont(), CloseFont(), SetFont(), AddFont()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    Remove ((struct Node *)textFont);

    AROS_LIBFUNC_EXIT
} /* RemFont */
