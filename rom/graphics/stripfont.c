/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function StripFont()
    Lang: english
*/
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(void, StripFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont *, font, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 137, Graphics)

/*  FUNCTION
		Removes a TextFontExtension from a font.

    INPUTS
    	font	- font to remove extension from.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		ExtendFont(), ExtendFontTags()
    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct TextFontExtension *tfe;
    struct tfe_hashnode *hn;
	
    /* Valid parameter ? */
    if (font == NULL)
	return;
		
    /* Does the font have an extension ? */

    ObtainSemaphore(&PrivGBase(GfxBase)->fontsem);
    
    hn = tfe_hashlookup(font, GfxBase);
    if (NULL != hn)
    {
    	tfe = hn->ext;
	
	if (hn->font_bitmap) OOP_DisposeObject(hn->font_bitmap);
	if (hn->chunky_colorfont) FreeVec(hn->chunky_colorfont);
	
    	/* Remove the hashitem (tfe_hashdelete() has semaphore protection) */
	tfe_hashdelete(font, GfxBase);
	
	if (NULL != tfe)
	{
	    font->tf_Extension = tfe->tfe_OrigReplyPort;

            /* Font is not tagged anymore */
            font->tf_Style &= ~FSF_TAGGED;
		
	    FreeTagItems(tfe->tfe_Tags);
	    FreeMem(tfe, sizeof (struct TextFontExtension_intern));
	}
		
    }
    
    ReleaseSemaphore(&PrivGBase(GfxBase)->fontsem);

    return;	

    AROS_LIBFUNC_EXIT
} /* StripFont */
