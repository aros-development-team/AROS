/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of graphics.library/ExtendFont()
    Lang: english
*/

#define AROS_TAGRETURNTYPE ULONG
#include <graphics/text.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	ULONG ExtendFontTags (

/*  SYNOPSIS */
	struct TextFont * font,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of graphics.library/ExtendFont().
        For information see graphics.library/ExtendFont().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics.library/ExtendFont()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = ExtendFont (font, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* ExtendFontTags */
