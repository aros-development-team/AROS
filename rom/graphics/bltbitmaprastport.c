/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	void BltBitMapRastPort (

/*  SYNOPSIS */
	struct BitMap   * srcBitMap,
	LONG              xSrc,
	LONG              ySrc,
	struct RastPort * destRP,
	LONG              xDest,
	LONG              yDest,
	LONG              xSize,
	LONG              ySize,
	ULONG             minterm)

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
