/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the attributes of a Bullet GlyphEngine
    Lang: english
*/

#define AROS_TAGRETURNTYPE ULONG

#include "alib_intern.h"

extern struct Library * BulletBase;

/*****************************************************************************

    NAME */

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/bullet.h>

	ULONG SetInfo (

/*  SYNOPSIS */
	struct GlyphEngine *glyphEngine,
	Tag                 tag1,
	...)

/*  FUNCTION
	Changes attributes of an GlyphEngine.

    INPUTS
	object - Change the attributes of this GlyphEngine
	tag1 - The first of a list of attribute/value-pairs. The last
		attribute in this list must be TAG_END or TAG_DONE.
		The value for this last attribute is not examined (ie.
		you need not specify it).

    RESULT
	Bullet Error Code

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	bullet.library/SetInfoA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    
    retval = SetInfoA (glyphEngine, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
    
} /* SetInfo */
