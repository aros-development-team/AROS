/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics function ExtendFont()
    Lang: english
*/
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(ULONG, ExtendFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont *, font, A0),
	AROS_LHA(struct TagItem  *, fontTags, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 136, Graphics)

/*  FUNCTION
		Checks whether or not a TextFont has a TextFontExtension.
		If no extension exists, and tags are fontTags are supplied,
		then it will try to build one.

    INPUTS
    	font		- font to check for an extension.
		fontTags	- tags to buil the TextFontExtesion from if it doesn't exist.
					   May be 0.
    RESULT
    	success		- FALSE if the font has no TextFontExtension and an extension
    				  can't be built. TRUE otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	ExtendFontTags(), StripFont()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

	/* !!! defaults to FALSE !!! */
	ULONG retval = FALSE; 
	
	struct TextFontExtension *tfe;
	
	if (font == 0)
		return(FALSE);
	
	/* Does the font allready have an extension ? */
	if (TFE(font->tf_Extension)->tfe_MatchWord == TFE_COOKIE)
		retval = TRUE;
	else
	{
		/* Try to build an extension */
		if (fontTags)
		{
			if ((tfe = AllocMem(sizeof (struct TextFontExtension), MEMF_ANY|MEMF_CLEAR)) != 0)
			{
				/* We take a copy of the tagitems */
				if ((tfe->tfe_Tags = CloneTagItems(fontTags)) != 0)
				{
				
					/* Fill in the textfontextension */
				
					tfe->tfe_MatchWord		= TFE_COOKIE;
					tfe->tfe_BackPtr		= font;
					tfe->tfe_OrigReplyPort	= font->tf_Message.mn_ReplyPort;
					
					/* Singlethread to prevent race conditions */
					Forbid();
					
					font->tf_Style |= FSF_TAGGED;
					TFE(font->tf_Extension) = tfe;
					
					Permit();
					
					/* Everything went just fine */
					retval = TRUE;
				}
			}
			else
				FreeMem(tfe, sizeof (struct TextFontExtension));
		}
	}
	return (retval);

    AROS_LIBFUNC_EXIT
} /* ExtendFont */
