/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ExtendFont()
    Lang: english
*/
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include "graphics_intern.h"
#include "fontsupport.h"

#include <sys/types.h>


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

    struct TagItem  	     def_tags = { TAG_DONE, 0};
    struct tfe_hashnode      *hn;
    struct TextFontExtension *tfe;
    BOOL    	    	     new_hashnode = FALSE;

    if (font == NULL)
	return FALSE;

    ObtainSemaphore(&PrivGBase(GfxBase)->fontsem);
    
    /* Does the font allready have an extension ? */
    hn = tfe_hashlookup(font, GfxBase);
    if (NULL == hn)
    {
    	hn = tfe_hashnode_create(GfxBase);
	if (NULL == hn)
	{
    	    ReleaseSemaphore(&PrivGBase(GfxBase)->fontsem);	    
	    return FALSE;
	}
	new_hashnode = TRUE;
    }
    
    tfe = hn->ext;
    if (NULL != tfe)
    {
    	ReleaseSemaphore(&PrivGBase(GfxBase)->fontsem);	    
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
				
	    tfe->tfe_MatchWord		= 0; /* unused */
	    tfe->tfe_BackPtr		= font;
	    tfe->tfe_OrigReplyPort	= font->tf_Message.mn_ReplyPort;
					
	    TFE(font->tf_Extension) = tfe;
	    font->tf_Style |= FSF_TAGGED;
		
	    if (!hn->font_bitmap)
	    {
	    	hn->font_bitmap = fontbm_to_hiddbm(font, GfxBase);
	    }
	    
	    if (hn->font_bitmap)
	    {
	    	if (new_hashnode)
		{
		    tfe_hashadd(hn, font, tfe, GfxBase);
		}
		else
		{
		    hn->ext = tfe;
		}
		
		ReleaseSemaphore(&PrivGBase(GfxBase)->fontsem);
    		return TRUE;
	    }

	    
	    FreeTagItems(tfe->tfe_Tags);

	} 
	FreeMem(tfe, sizeof (struct TextFontExtension));
	
    } /* if (memory for extension allocated) */

    ReleaseSemaphore(&PrivGBase(GfxBase)->fontsem);    
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* ExtendFont */



