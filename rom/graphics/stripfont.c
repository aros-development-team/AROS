/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics function StripFont()
    Lang: english
*/
#include <proto/exec.h>
#include <proto/utility.h>
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
	
	/* Valid parameter ? */
	if (font == 0)
		return;
		
	/* Does the font have an extension ? */
	if (TFE(font->tf_Extension)->tfe_MatchWord == TFE_COOKIE)
	{
		tfe = (struct TextFontExtension*)font->tf_Extension;
		/* 
			Prevent race conditions. 
			(This may not be necessar but better to be 100% safe)
		*/
		Forbid();
		
		font->tf_Extension = 0;
		/* Font is not tagged anymore */
		font->tf_Style ^= FSF_TAGGED;
		
		Permit();
		
		FreeTagItems(tfe->tfe_Tags);
		FreeMem(tfe, sizeof (struct TextFontExtension));
		
	}
	
	return;	

    AROS_LIBFUNC_EXIT
} /* StripFont */
