/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bullet function OpenEngine()
    Lang: English
*/

#include <aros/libcall.h>
#include <proto/bullet.h>

/*****************************************************************************

    NAME */

	AROS_LH0(struct GlyphEngine *, OpenEngine,

/*  SYNOPSIS */

/*  LOCATION */
	struct Library *, BulletBase, 5, Bullet)

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

} /* OpenEngine */
