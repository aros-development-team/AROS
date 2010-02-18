/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/graphics.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH1(struct List *, AllocCModeListTagList,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 12, Cybergraphics)

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
    
    return driver_AllocCModeListTagList(tags, GetCGFXBase(CyberGfxBase));

    AROS_LIBFUNC_EXIT
} /* AllocCModeListTagList */
