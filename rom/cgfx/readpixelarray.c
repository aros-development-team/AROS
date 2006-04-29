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

	AROS_LH10(ULONG, ReadPixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , dst		, A0),
	AROS_LHA(UWORD            , destx	, D0),
	AROS_LHA(UWORD            , desty	, D1),
	AROS_LHA(UWORD            , dstmod	, D2),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , srcx	, D3),
	AROS_LHA(UWORD            , srcy	, D4),
	AROS_LHA(UWORD            , width	, D5),
	AROS_LHA(UWORD            , height	, D6),
	AROS_LHA(UBYTE            , dstformat	, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 20, Cybergraphics)

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
	return driver_ReadPixelArray(dst
    	    , destx, desty
	    , dstmod
	    , rp
	    , srcx, srcy
	    , width, height
	    , dstformat
	    , GfxBase
        );
    }
    else return 0;
    
    AROS_LIBFUNC_EXIT
} /* ReadPixelArray */
