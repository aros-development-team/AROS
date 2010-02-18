/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/graphics.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH1(ULONG, BestCModeIDTagList,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 10, Cybergraphics)

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
    
    return driver_BestCModeIDTagList(tags, GetCGFXBase(CyberGfxBase));

    AROS_LIBFUNC_EXIT
} /* BestCModeIDTagList */
