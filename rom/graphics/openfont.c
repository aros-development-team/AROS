/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function OpenFont()
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include <graphics/text.h>

#include <string.h>

/*****************************************************************************

    NAME */
#include <graphics/text.h>
#include <proto/graphics.h>

	AROS_LH1(struct TextFont *, OpenFont,

/*  SYNOPSIS */
	AROS_LHA(const struct TextAttr *, textAttr, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 12, Graphics)

/*  FUNCTION
	Searches for a text font which best matches the specified attributes.

    INPUTS
	textAttr - pointer to a TextAttr or TTextAttr font description.

    RESULT
	Returns NULL if the font can't be found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseFont(), SetFont(), diskfont.library/OpenDiskFont()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TextFont *tf, *best_so_far = NULL;
    WORD    	    bestmatch = 0;
   
    ASSERT_VALID_PTR(textAttr);
    
    if (!textAttr->ta_Name) return NULL;
	
    /* Search for font in the fontlist */
    Forbid();
    ForeachNode(&GfxBase->TextFonts, tf)
    {
	if (0 == strcmp(tf->tf_Message.mn_Node.ln_Name, textAttr->ta_Name))
	{
	    UWORD   	    match;
	    struct TagItem  *tags = NULL;
	    struct TextAttr match_ta =
	    {
	    	tf->tf_Message.mn_Node.ln_Name,
		tf->tf_YSize,
		tf->tf_Style,
		tf->tf_Flags
	    };
	    
	    if (ExtendFont(tf, NULL))
	    {
	        tags = ((struct TextFontExtension *)tf->tf_Extension)->tfe_Tags;		
	    }
	    
	    match = WeighTAMatch(textAttr, &match_ta, tags);
	    if (match > bestmatch)
	    {
	    	bestmatch = match;
		best_so_far = tf;
	    }
	}
    }
    
    if (best_so_far) best_so_far->tf_Accessors ++;
    
    Permit();
   
    return best_so_far;

    AROS_LIBFUNC_EXIT
    
} /* OpenFont */
