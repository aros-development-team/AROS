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

	AROS_LH7(ULONG, ExtractColor,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, RastPort, A0),
	AROS_LHA(struct BitMap   *, SingleMap, A1),
	AROS_LHA(ULONG            , Colour, D0),
	AROS_LHA(ULONG            , sX, D1),
	AROS_LHA(ULONG            , sY, D2),
	AROS_LHA(ULONG            , Width, D3),
	AROS_LHA(ULONG            , Height, D4),

/*  LOCATION */
	struct Library *, CyberGfxBase, 31, Cybergraphics)

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
    
    return driver_ExtractColor(RastPort, SingleMap, Colour, sX, sY, Width, Height, GfxBase);

    AROS_LIBFUNC_EXIT
} /* ExtractColor */
