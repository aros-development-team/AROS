/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/graphics.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH2(ULONG, GetCyberIDAttr,

/*  SYNOPSIS */
	AROS_LHA(ULONG, attribute, 	D0),
	AROS_LHA(ULONG, DisplayModeID, 	D1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 17, Cybergraphics)

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
    
    return driver_GetCyberIDAttr(attribute, DisplayModeID, GfxBase);

    AROS_LIBFUNC_EXIT
} /* GetCyberIDAttr */
