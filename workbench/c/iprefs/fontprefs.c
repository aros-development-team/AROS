/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <prefs/prefhdr.h>
#include <prefs/font.h>

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

struct FileFontPrefs
{
    UBYTE   fp_Reserved[4 * 3];
    UBYTE   fp_Reserved2[2];
    UBYTE   fp_Type[2];
    UBYTE   fp_FrontPen;
    UBYTE   fp_BackPen;
    UBYTE   fp_Drawmode;
    UBYTE   fp_pad;
    UBYTE   fp_TextAttr_ta_Name[4];
    UBYTE   fp_TextAttr_ta_YSize[2];
    UBYTE   fp_TextAttr_ta_Style;
    UBYTE   fp_TextAttr_ta_Flags;
    BYTE    fp_Name[FONTNAMESIZE];
};

/*********************************************************************************************/

LONG stopchunks[] =
{
    ID_PREF, ID_FONT
};

/*********************************************************************************************/

void FontPrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;
    
    D(bug("In IPrefs:FontPrefs_Handler\n"));
    
    if ((iff = CreateIFF(filename, stopchunks, 1)))
    {
	while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
	{
	    struct ContextNode   *cn;
	    struct FileFontPrefs fontprefs;

	    cn = CurrentChunk(iff);

   	    D(bug("FontPrefs_Handler: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
		  cn->cn_Type >> 24,
		  cn->cn_Type >> 16,
		  cn->cn_Type >> 8,
		  cn->cn_Type,
		  cn->cn_ID >> 24,
		  cn->cn_ID >> 16,
		  cn->cn_ID >> 8,
		  cn->cn_ID));

	    if ((cn->cn_ID == ID_FONT) && (cn->cn_Size == sizeof(fontprefs)))
	    {
    	    	D(bug("FontPrefs_Handler: ID_FONT chunk with correct size found.\n"));

		if (ReadChunkBytes(iff, &fontprefs, sizeof(fontprefs)) == sizeof(fontprefs))
		{
		    struct TextAttr ta;
		    struct TextFont *font;
		    UWORD   	    type;
		    
    	    	    D(bug("FontPrefs_Handler: Reading of ID_FONT chunk okay.\n"));

		    type = (fontprefs.fp_Type[0] << 8) + fontprefs.fp_Type[1];

		    D(bug("Type = %d  Name = %s\n", type, fontprefs.fp_Name));
		    
		    ta.ta_Name  = fontprefs.fp_Name;
		    ta.ta_YSize = (fontprefs.fp_TextAttr_ta_YSize[0] << 8) + 
		    	    	  fontprefs.fp_TextAttr_ta_YSize[1];
		    ta.ta_Style = fontprefs.fp_TextAttr_ta_Style;
		    ta.ta_Flags = fontprefs.fp_TextAttr_ta_Flags;
		    		    
		    switch(type)
		    {
		    	case FP_SYSFONT:
			    if ((font = OpenDiskFont(&ta)))
			    {
			    	if (font->tf_Flags & FPF_PROPORTIONAL)
				{
				    D(bug("FontPrefs_Handler: FP_SYSFONT Font is proportional although it may not be!\n"));
				    CloseFont(font);
				}
				else
				{
				    Forbid();
    	    	    	    	    GfxBase->DefaultFont = font;
				    Permit();

				    D(bug("FontPrefs_Handler: Installed new system font!\n"));
				}
			    }
			    else
			    {
			    	D(bug("FontPrefs_Handler: Could not open font!\n"));
			    }
			    break;
			    
			case FP_SCREENFONT:
			    if ((font = OpenDiskFont(&ta)))
			    {
				SetDefaultScreenFont(font);
				
				D(bug("FontPrefs_Handler: Installed new system font!\n"));
			    }
			    else
			    {
			    	D(bug("FontPrefs_Handler: Could not open font!\n"));
			    }
			    break;
			
			case FP_WBFONT:
			    /* Hmm ... should IPrefs handle this, or should Workbench task
			       itself also setup a filenotification on font.prefs :-\ */
			    break;
			    
		    } /* switch(type) */
		    
		} /* if (ReadChunkBytes(iff, &fontprefs, sizeof(fontprefs)) == sizeof(fontprefs)) */
		
	    } /* if ((cn->cn_ID == ID_FONT) && (cn->cn_Size == sizeof(fontprefs))) */

	} /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */
	    
   	KillIFF(iff);
	
    } /* if ((iff = CreateIFF(filename))) */
    
    
    D(bug("In IPrefs:FontPrefs_Handler. Done with '%s'.\n", filename));
}

/*********************************************************************************************/
