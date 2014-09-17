/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include "felsunxi_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH3(ULONG, SetAttrsA,

/*  SYNOPSIS */
    	AROS_LHA(ULONG,            type,      D0),
    	AROS_LHA(APTR,             usbstruct, A0),
    	AROS_LHA(struct TagItem *, taglist,   D0),
	
/*  LOCATION */
	LIBBASETYPEPTR, FELSunxiBase, 6, FELSunxi)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return(0);
    AROS_LIBFUNC_EXIT

} /* SetAttrsA */
