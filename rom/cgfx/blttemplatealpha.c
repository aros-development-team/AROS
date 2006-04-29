/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH8(void, BltTemplateAlpha,

/*  SYNOPSIS */
	AROS_LHA(APTR             , src		, A0),
	AROS_LHA(LONG             , srcx	, D0),
	AROS_LHA(LONG             , srcmod	, D1),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(LONG             , destx	, D2),
	AROS_LHA(LONG             , desty	, D3),
	AROS_LHA(LONG	          , width	, D4),
	AROS_LHA(LONG             , height	, D5),

/*  LOCATION */
	struct Library *, CyberGfxBase, 37, Cybergraphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CyberGfxBase)
    
    if (width && height)
    {
	driver_BltTemplateAlpha(src
    	    , srcx
	    , srcmod
	    , rp
	    , destx, desty
	    , width, height
	    , GfxBase
	);
    }

    AROS_LIBFUNC_EXIT
} /* BltTemplateAlpha */
