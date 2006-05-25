/* 
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <exec/types.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "muimaster_intern.h"
#include "font.h"
#include "prefs.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

/* Returns a given text font, if necessary it opens the font.
 * Must be called after Area's MUIM_Setup.
 */

struct TextFont *zune_font_get(Object *obj, LONG preset)
{
    struct MUI_GlobalInfo *mgi;
    struct MUI_RenderInfo *mri;

    if ((preset <= MUIV_Font_Inherit) && (preset >= MUIV_Font_NegCount))
    {
    	CONST_STRPTR name;

	mri = muiRenderInfo(obj);
	/* font already loaded, just return it */
	if (mri->mri_Fonts[-preset])
	{
	    D(bug("zune_font_get : return mri_Fonts[%d]=%lx\n",
		  preset, mri->mri_Fonts[-preset]));
	    return mri->mri_Fonts[-preset];
	}

	mgi =  muiGlobalInfo(obj);
	/* font name given, load it */
        name = mgi->mgi_Prefs->fonts[-preset];
	D(bug("zune_font_get : preset=%d, name=%s\n", preset, name));
	if (name != NULL && name[0] != 0)
	{
	    struct TextAttr ta;
	    if ((ta.ta_Name = (char*)AllocVec(strlen(name)+10,0)))
	    {
	    	char *p;
	    	LONG size;

	    	strcpy(ta.ta_Name,name);
	    	StrToLong(FilePart(ta.ta_Name),&size);
	    	ta.ta_YSize = size;
		ta.ta_Style = 0;
		ta.ta_Flags = 0;

		if ((p = PathPart(ta.ta_Name)))
		    strcpy(p,".font");
		D(bug("zune_font_get : OpenDiskFont(%s)\n",
		      ta.ta_Name));
		mri->mri_Fonts[-preset] = OpenDiskFont(&ta);

		FreeVec(ta.ta_Name);
	    }
	    
	}
	else /* fallback to window normal font */
	{
	    if (preset != MUIV_Font_Normal && preset != MUIV_Font_Fixed) /* avoid infinite recursion */
	    {
		/* dont do this, would result in the font being closed more than once */
/*  		return (mri->mri_Fonts[-preset] = zune_font_get(obj, MUIV_Font_Normal)); */
		return zune_font_get(obj, MUIV_Font_Normal);
	    }
	}

	/* no font loaded, fallback to screen font or system font */
	if (!mri->mri_Fonts[-preset])
	{
	    if (MUIV_Font_Normal == preset)
	    {
		struct TextAttr scr_attr;
		scr_attr = *(_screen(obj)->Font);
		scr_attr.ta_Flags = 0;
		D(bug("zune_font_get : OpenDiskFont(%s) (screen font)\n", scr_attr.ta_Name));
		mri->mri_Fonts[-preset] = OpenDiskFont(&scr_attr);
	    }
	    else /* MUIV_Font_Fixed */
	    {
		struct TextAttr def_attr;
		def_attr.ta_Name = GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
		def_attr.ta_YSize = GfxBase->DefaultFont->tf_YSize;
		def_attr.ta_Style = GfxBase->DefaultFont->tf_Style;
		def_attr.ta_Flags = GfxBase->DefaultFont->tf_Flags;
		D(bug("zune_font_get : OpenDiskFont(%s) (system font)\n", def_attr.ta_Name));
		mri->mri_Fonts[-preset] = OpenDiskFont(&def_attr);
	    }
	}
	return mri->mri_Fonts[-preset];
    }
    return (struct TextFont *)preset;
}
