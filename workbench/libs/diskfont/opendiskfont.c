/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Diskfont function OpenDiskFont()
    Lang: english
*/

#ifndef TURN_OFF_DEBUG
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#endif
#  include <aros/debug.h>

#include <graphics/text.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include "diskfont_intern.h"


/*****************************************************************************

    NAME */
#include <clib/diskfont_protos.h>

	AROS_LH1(struct TextFont *, OpenDiskFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextAttr *, textAttr, A0),

/*  LOCATION */
	struct Library *, DiskfontBase, 5, Diskfont)

/*  FUNCTION
		Tries to open the font specified by textAttr. If the font has allready
		been loaded into memory, it will be opened with OpenFont(). Otherwise
		OpenDiskFont() will try to load it from disk.

    INPUTS
    	textAttr - Description of the font to load. If the textAttr->ta_Style
    			   FSF_TAGGED bit is set, it will be treated as a struct TTextAttr.
    	

    RESULT
    	Pointer to a struct TextFont on success, 0 on failure.

    NOTES

    EXAMPLE

    BUGS
    	Loadseg_AOS() which is internally used to load a font file
    	from disk, does not handle address relocation for 64 bit CPUs
    	yet. Will add a hack to support this.

    SEE ALSO
    	AvailFonts()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    diskfont_lib.fd and clib/diskfont_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,DiskfontBase)
 	
    WORD match_weight = 0, new_match_weight;
    APTR iterator;
    BOOL bestinmemory = TRUE;
    struct TTextAttr *ttait;
    struct TextFont *tf = NULL;
    ULONG len = strlen(textAttr->ta_Name)-5;
    
    D(bug("OpenDiskFont(textAttr=%p)\n", textAttr));
    D(bug("Name %s YSize %ld Style 0x%lx Flags 0x%lx\n",
          textAttr->ta_Name,
          textAttr->ta_YSize,
          textAttr->ta_Style,
          textAttr->ta_Flags));

    tf = OpenFont(textAttr);
    if (tf!=NULL)
    {
	struct TTextAttr tattr;
	
	tattr.tta_Name = tf->tf_Message.mn_Node.ln_Name;
	tattr.tta_YSize = tf->tf_YSize;
	tattr.tta_Style = tf->tf_Style;
	tattr.tta_Flags = tf->tf_Flags;
	if (ExtendFont(tf, NULL))
	{
	    tattr.tta_Tags = TFE(tf->tf_Extension)->tfe_Tags;
	    if (tattr.tta_Tags) tattr.tta_Style |= FSF_TAGGED;
	}
	else
	    tattr.tta_Tags = NULL;
	
	match_weight = WeighTAMatch((struct TTextAttr *)textAttr,
				    (struct TextAttr *)&tattr,
				    tattr.tta_Tags);
    }

    if (match_weight!=MAXFONTMATCHWEIGHT)
    {
	UWORD oldYSize;
	
	iterator = DF_IteratorInit(DFB(DiskfontBase));
	if (iterator == NULL)
	    D(bug("Error initializing Diskfont Iterator\n"));
	else
	{
	    while ((ttait = DF_IteratorGetNext(iterator, DFB(DiskfontBase)))!=NULL)
	    {
		D(bug("OpenDiskFont: Checking font: %s\n", ttait->tta_Name));
		
		if (strncasecmp(ttait->tta_Name, textAttr->ta_Name, len) == 0)
		{
		    if (IS_OUTLINE_FONT(ttait))
		    {
			/* For outline font make the YSize equal because it
			 * is scalable */
			oldYSize = ttait->tta_YSize;
			ttait->tta_YSize = textAttr->ta_YSize;
		    }
		    new_match_weight = WeighTAMatch((struct TTextAttr *)textAttr,
						    (struct TextAttr *)ttait,
						    ttait->tta_Tags);
		    if (IS_OUTLINE_FONT(ttait))
			ttait->tta_YSize = oldYSize;

		    if (new_match_weight > match_weight)
		    {
			match_weight = new_match_weight;
			DF_IteratorRemember(iterator, DFB(DiskfontBase));
			bestinmemory = FALSE;
		    }
		    if (match_weight==MAXFONTMATCHWEIGHT)
			break;
		}
	    }
	
	    /* Best still in memory then open this font, otherwise load from disk */
	    if (!bestinmemory)
	    {
		if (tf!=NULL)
		    CloseFont(tf);
		tf = DF_IteratorRememberOpen(iterator, (struct TTextAttr *)textAttr, DFB(DiskfontBase));
	    }
	    DF_IteratorFree(iterator, DFB(DiskfontBase));
	}
    }
    
    if (tf)
    {
	D(bug("extend font\n"));
	if (ExtendFont(tf, NULL))
	{
#warning CHECKME
	    TFE(tf->tf_Extension)->tfe_Flags0 |= TE0F_NOREMFONT;
	}
    }
  	
    ReturnPtr("OpenDiskFont", struct TextFont *, tf);
	
    AROS_LIBFUNC_EXIT
    
} /* OpenDiskFont */
