/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

    ASSERT_VALID_PTR(textFont);
    
    Forbid();
    if (!(textFont->tf_Flags & FPF_REMOVED))
    {
	textFont->tf_Flags |= FPF_REMOVED;
	Remove (&textFont->tf_Message.mn_Node);
    }
#if DEBUG
    else
    {
    	D(bug("Someone tried to remove font which is already removed!"));
    }
#endif
    Permit();
    
    AROS_LIBFUNC_EXIT
    
} /* RemFont */
