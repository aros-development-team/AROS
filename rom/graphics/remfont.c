/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a font from the list of public available fonts.
    Lang: english
*/
#include <aros/debug.h>
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

    struct TextFontExtension *tfe;
    BOOL    	    	      can_remove = TRUE;
    
    ASSERT_VALID_PTR(textFont);
    
    Forbid();
    tfe = (struct TextFontExtension *)textFont->tf_Extension;
    if (tfe)
    {
    	if ((tfe->tfe_MatchWord == TFE_MATCHWORD) && (tfe->tfe_BackPtr == textFont))
	{
	    if (tfe->tfe_Flags0 & TE0F_NOREMFONT)
	    {
	    	can_remove = FALSE;
	    }
	}
	
    }
    
    if (can_remove)
    {
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

    	StripFont(textFont);

    }
        
    Permit();
    
    AROS_LIBFUNC_EXIT
    
} /* RemFont */
