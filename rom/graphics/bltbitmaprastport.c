/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH9 (void, BltBitMapRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap   *, srcBitMap, A0),
	AROS_LHA(LONG             , xSrc, D0),
	AROS_LHA(LONG             , ySrc, D1),
	AROS_LHA(struct RastPort *, destRP, A1),
	AROS_LHA(LONG             , xDest, D2),
	AROS_LHA(LONG             , yDest, D3),
	AROS_LHA(LONG             , xSize, D4),
	AROS_LHA(LONG             , ySize, D5),
	AROS_LHA(ULONG            , minterm, D6),

/*  LOCATION */
	struct GfxBase *, GfxBase, 101, Graphics)
	    
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
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
     
    driver_BltBitMapRastPort(srcBitMap
    	, xSrc, ySrc
	, destRP
	, xDest, yDest
	, xSize, ySize
	, minterm
	, GfxBase
	
    );

    AROS_LIBFUNC_EXIT
} /* BltBitMapRastPort */
