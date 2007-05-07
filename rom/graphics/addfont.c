/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Make a font public available
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <graphics/text.h>
#include <proto/graphics.h>

	AROS_LH1(void, AddFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont *, textFont, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 80, Graphics)

/*  FUNCTION
	Add a font to the list of public fonts. After that, you can
	open the font with OpenFont().

    INPUTS
	textFont - The font to add.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenFont(), RemFont(), CloseFont(), SetFont()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(textFont);
    
    textFont->tf_Message.mn_Node.ln_Type = NT_FONT;
    textFont->tf_Accessors = 0;
    textFont->tf_Flags &= ~FPF_REMOVED;
    
    Forbid();
    AddHead (&GfxBase->TextFonts, (struct Node *)textFont);
    Permit();
    
    AROS_LIBFUNC_EXIT
} /* AddFont */
