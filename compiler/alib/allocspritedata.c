/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of graphics.library/AllocSpriteDataA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE struct ExtSprite *

#include <graphics/gfx.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	struct ExtSprite * AllocSpriteData (

/*  SYNOPSIS */
	struct BitMap * bitmap,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of graphics.library/AllocSpriteDataA().
        For information see graphics.library/AllocSpriteDataA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics.library/AllocSpriteDataA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = AllocSpriteDataA (bitmap, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* AllocSpriteData */
