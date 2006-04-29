/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "cybergraphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH11(LONG, WriteLUTPixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , srcRect, A0),
	AROS_LHA(UWORD            , SrcX, D0),
	AROS_LHA(UWORD            , SrcY, D1),
	AROS_LHA(UWORD            , SrcMod, D2),
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(APTR             , CTable, A2),
	AROS_LHA(UWORD            , DestX, D3),
	AROS_LHA(UWORD            , DestY, D4),
	AROS_LHA(UWORD            , SizeX, D5),
	AROS_LHA(UWORD            , SizeY, D6),
	AROS_LHA(UBYTE            , CTabFormat, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 33, Cybergraphics)

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
    
    return driver_WriteLUTPixelArray(srcRect
	, SrcX, SrcY
	, SrcMod, rp, CTable
	, DestX, DestY
	, SizeX, SizeY
	, CTabFormat
	, GfxBase);

    AROS_LIBFUNC_EXIT
} /* WriteLUTPixelArray */
