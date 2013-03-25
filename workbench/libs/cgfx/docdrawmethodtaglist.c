/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH3(void, DoCDrawMethodTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Hook     *, hook, A0),
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(struct TagItem  *, tags, A2),

/*  LOCATION */
	struct Library *, CyberGfxBase, 26, Cybergraphics)

/*  FUNCTION
        Calls a callback hook that directly accesses a RastPort's bitmap.

    INPUTS
        hook - a callback hook. The standard hook inputs will be set as
            follows:
                object (struct RastPort *) - this function's 'rp' input.
                message (struct CDrawMsg *) - details of the area on which to
                    operate.
        rp - the RastPort to perform operations on.
        tags - not used. Must be NULL.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    driver_DoCDrawMethodTagList(hook, rp, tags, GetCGFXBase(CyberGfxBase));    

    AROS_LIBFUNC_EXIT
} /* DoCDrawMethodTagList */
