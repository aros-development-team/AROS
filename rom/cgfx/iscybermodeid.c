/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH1(BOOL, IsCyberModeID,

/*  SYNOPSIS */
	AROS_LHA(ULONG, modeID, D0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 9, Cybergraphics)

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
    
    return driver_IsCyberModeID(modeID, GfxBase);

    AROS_LIBFUNC_EXIT
} /* IsCyberModeID */
