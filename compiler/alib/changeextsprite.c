/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of graphics.library/ChangeExtSpriteA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE LONG
#include <graphics/view.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	LONG ChangeExtSprite (

/*  SYNOPSIS */
	struct ViewPort * vp,
	struct ExtSprite * oldsprite,
	struct ExtSprite * newsprite,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of graphics.library/ChangeExtSpriteA().
        For information see graphics.library/ChangeExtSpriteA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics.library/ChangeExtSpriteA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = ChangeExtSpriteA (vp, oldsprite, newsprite, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* ChangeExtSprite */
