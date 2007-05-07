/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Diskfont function OpenDiskFont()
    Lang: english
*/

#include <string.h>
#include <graphics/text.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include "diskfont_intern.h"

#include <aros/debug.h>


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

    SEE ALSO
    	AvailFonts()

    INTERNALS
	Internally now the WeighTAMatch function is used to find the best
	font match. In OS 3.0 and above this graphics function is made
	private to the library. So in the future maybe this function can
	be replaced by a more flexible internal mechanism.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
 	
    WORD match_weight = 0, new_match_weight;
    APTR iterator;
    BOOL bestinmemory = TRUE;
    struct TTextAttr *ttait;
    struct TextFont *tf = NULL;
    STRPTR filepart = FilePart(textAttr->ta_Name), wholename = textAttr->ta_Name;
    ULONG len = strlen(filepart);
    
    if (len>5 && strcasecmp(filepart+len-5,".font")==0)
        len -= 5;

    D(bug("OpenDiskFont(textAttr=%p)\n", textAttr));
    D(bug("Name %s YSize %ld Style 0x%lx Flags 0x%lx\n",
          textAttr->ta_Name,
          textAttr->ta_YSize,
          textAttr->ta_Style,
          textAttr->ta_Flags));

    /* Check if font is in memory, ignore path */
    textAttr->ta_Name = filepart;
    
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
	
	match_weight = WeighTAMatch(textAttr,
				    (struct TextAttr *)&tattr,
				    tattr.tta_Tags);

	D(bug("OpenDiskFont: Found font in memory weight(%d)\n", match_weight));
    }
    else
	D(bug("OpenDiskFont: No font found in memory\n"));

    textAttr->ta_Name = wholename;
    
    if (match_weight!=MAXFONTMATCHWEIGHT)
    {
	iterator = DF_IteratorInit((struct TTextAttr *)textAttr, DFB(DiskfontBase));
	if (iterator == NULL)
	    D(bug("OpenDiskFont: Error initializing Diskfont Iterator\n"));
	else
	{
	    while ((ttait = DF_IteratorGetNext(iterator, DFB(DiskfontBase)))!=NULL)
	    {
	        ULONG len2 = strlen(ttait->tta_Name) - 5;

		D(bug("OpenDiskFont: Checking font: %s(%d)\n", ttait->tta_Name, ttait->tta_YSize));

	        D(bug("len: %d len2: %d\n", len, len2));
	       
		if ((len == len2)  && (strncasecmp(ttait->tta_Name, filepart, len) == 0))
		{
		    new_match_weight = WeighTAMatch(textAttr,
						    (struct TextAttr *)ttait,
						    ttait->tta_Tags);

		    if (new_match_weight > match_weight)
		    {
			match_weight = new_match_weight;
			DF_IteratorRemember(iterator, DFB(DiskfontBase));
			bestinmemory = FALSE;
			D(bug("Better weight (%d)\n", match_weight));
		    }
		    else
			D(bug("No better weight (%d)\n", new_match_weight));
		    
		    if (match_weight==MAXFONTMATCHWEIGHT)
			break;
		}
	    }
	
	    /* Best still in memory then open this font, otherwise load from disk */
	    if (!bestinmemory)
	    {
		if (tf!=NULL)
		    CloseFont(tf);
		tf = DF_IteratorRememberOpen(iterator, DFB(DiskfontBase));
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
