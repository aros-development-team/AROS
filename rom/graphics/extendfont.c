/*
    (C) 1997 AROS - The Amiga Research OS
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
	If no extension exists, and tags are supplied,
	then it will try to build one.

    INPUTS
    	font		- font to check for an extension.
	fontTags	- tags to buil the TextFontExtesion from if it doesn't exist.
			  If a NULL pointer is given, ExtendFont will alocate default tags.
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

    struct TextFontExtension *tfe;
    struct TagItem def_tags = { TAG_DONE, 0};
    
    if (font == NULL)
	return FALSE;

	
    /* Does the font allready have an extension ? */
    
    tfe = tfe_hashlookup(font, GfxBase);
    if (tfe)
    {    
	return TRUE;
    }

    /* Try to build an extension */
    if (!fontTags)
	fontTags = &def_tags;
	    
    if ((tfe = AllocMem(sizeof (struct TextFontExtension), MEMF_ANY|MEMF_CLEAR)) != NULL)
    {
	/* We make a copy of the tagitems */
	if ((tfe->tfe_Tags = CloneTagItems(fontTags)) != NULL)
	{
	    /* Fill in the textfontextension *before* we make it public */
	    struct tfe_hashnode *hn;
				
	    tfe->tfe_MatchWord		= 0; /* unused */
	    tfe->tfe_BackPtr		= font;
	    tfe->tfe_OrigReplyPort	= font->tf_Message.mn_ReplyPort;
					
	    TFE(font->tf_Extension) = tfe;
	    font->tf_Style |= FSF_TAGGED;
		
	    hn = tfe_hashadd(tfe, font, GfxBase);
	    if (NULL != hn)
	    {
	    	if (driver_FontHIDDInit(font, GfxBase))
		{
		    return TRUE;
		}
	    }
	    FreeTagItems(fontTags);

	} 
	FreeMem(tfe, sizeof (struct TextFontExtension));
	
    } /* if (memory for extension allocated) */
    
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* ExtendFont */



