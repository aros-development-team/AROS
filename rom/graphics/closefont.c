/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function CloseFont()
    Lang: english
*/
#include <aros/debug.h>
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
	Close a font.

    INPUTS
	font - font pointer from OpenFont() or OpenDiskFont()

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

    if (!textFont) return;

    ASSERT_VALID_PTR(textFont);

    Forbid();
    textFont->tf_Accessors--;
    if ((textFont->tf_Accessors == 0) && !(textFont->tf_Flags & FPF_ROMFONT))
    {
    	RemFont(textFont);
    }
    Permit();

    AROS_LIBFUNC_EXIT
    
} /* CloseFont */
