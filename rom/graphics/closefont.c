/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function CloseFont()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/text.h>

/*****************************************************************************

    NAME */
#include <graphics/text.h>
#include <proto/graphics.h>

	AROS_LH1(void, CloseFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont *, textFont, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 13, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    BOOL killfont = FALSE;

    if (!textFont) return;

    ASSERT_VALID_PTR(textFont);

    Forbid();
    textFont->tf_Accessors--;
    if ((textFont->tf_Accessors == 0) && !(textFont->tf_Flags & FPF_ROMFONT))
    {
    	RemFont(textFont);
	killfont = TRUE;
    }
    Permit();

    if (!killfont) return;
 
    /* Free font data */

    /* !!! NOTE. FreeXXX functions has to match AllocXXX in
       workbench/libs/diskfont/diskfont_io.c
    */

    if (textFont->tf_Style & FSF_COLORFONT)
    {
	struct ColorFontColors *cfc;
	UWORD 	    	       i;

	for (i = 0; i < 8; i ++)
	{
	    if (CTF(textFont)->ctf_CharData[i]) FreeVec(CTF(textFont)->ctf_CharData[i]);
	}

	cfc = CTF(textFont)->ctf_ColorFontColors;
	if (cfc)
	{
	    if (cfc->cfc_ColorTable) FreeVec(cfc->cfc_ColorTable);
	    FreeVec(cfc);
	}

    }
    else
    {
	/* Not a colortextfont, only one plane */
	FreeVec(textFont->tf_CharData);
    }
    
    StripFont(textFont);

    if (textFont->tf_CharSpace) FreeVec(textFont->tf_CharSpace);
    if (textFont->tf_CharKern) FreeVec(textFont->tf_CharKern);

    /* All fonts have a tf_CharLoc allocated */    
    FreeVec(textFont->tf_CharLoc); 

    FreeVec(textFont->tf_Message.mn_Node.ln_Name);
    FreeVec(textFont);

    AROS_LIBFUNC_EXIT
    
} /* CloseFont */
