/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
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

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,BulletBase)

    AROS_LIBFUNC_EXIT

} /* SetInfoA */
