/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hidd/graphics.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

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

    BOOL iscyber = FALSE;
    struct VecInfo info;
    
    if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_VEC, modeID) == sizeof(info)) {
	HIDDT_StdPixFmt stdpf;

	OOP_GetAttr((OOP_Object *)info.reserved[1], aHidd_PixFmt_StdPixFmt, &stdpf);
	if (((UWORD)-1) != hidd2cyber_pixfmt(stdpf)) {
	    	iscyber = TRUE;
	}
    }
    return iscyber;

    AROS_LIBFUNC_EXIT
} /* IsCyberModeID */
