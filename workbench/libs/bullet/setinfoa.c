/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bullet function SetInfoA()
    Lang: English
*/

#include <aros/libcall.h>
#include <proto/bullet.h>

/*****************************************************************************

    NAME */

	AROS_LH2(ULONG, SetInfoA,

/*  SYNOPSIS */
    	AROS_LHA(struct GlyphEngine *, glyphEngine, A0),
	AROS_LHA(struct TagItem *, tagList, A1),
	
/*  LOCATION */
	struct Library *, BulletBase, 7, Bullet)

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

} /* SetInfoA */
