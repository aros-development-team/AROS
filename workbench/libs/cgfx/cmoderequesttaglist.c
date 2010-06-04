/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH2(ULONG, CModeRequestTagList,

/*  SYNOPSIS */
	AROS_LHA(APTR            , , A0),
	AROS_LHA(struct TagItem *, , A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 11, Cybergraphics)

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
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("CModeRequestTagList");

    AROS_LIBFUNC_EXIT
} /* CModeRequestTagList */
