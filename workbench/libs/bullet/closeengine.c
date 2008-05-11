/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bullet function CloseEngine()
    Lang: English
*/

#include <aros/libcall.h>
#include <proto/bullet.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, CloseEngine,

/*  SYNOPSIS */
    	AROS_LHA(struct GlyphEngine *, glyphEngine, A0),
	
/*  LOCATION */
	struct Library *, BulletBase, 6, Bullet)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    This interface is implemented in freetype2.library. 
    See /workbench/libs/freetype/src/aros.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    AROS_LIBFUNC_EXIT

} /* CloseEngine */
