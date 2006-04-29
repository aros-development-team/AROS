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

	AROS_LH7(ULONG, MovePixelArray,

/*  SYNOPSIS */
	AROS_LHA(UWORD            , SrcX, D0),
	AROS_LHA(UWORD            , SrcY, D1),
	AROS_LHA(struct RastPort *, RastPort, A1),
	AROS_LHA(UWORD            , DstX, D2),
	AROS_LHA(UWORD            , DstY, D3),
	AROS_LHA(UWORD            , SizeX, D4),
	AROS_LHA(UWORD            , SizeY, D5),

/*  LOCATION */
	struct Library *, CyberGfxBase, 22, Cybergraphics)

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
    
    return driver_MovePixelArray(SrcX, SrcY, RastPort, DstX, DstY, SizeX, SizeY, GfxBase);

    AROS_LIBFUNC_EXIT
} /* MovePixelArray */
