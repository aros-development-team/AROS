
/*
    (C) 1999 - 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>

#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH3(ULONG, rtGetVScreenSize,

/*  SYNOPSIS */

	AROS_LHA(struct Screen *, screen, A0),
	AROS_LHA(ULONG *, widthptr, A1),
	AROS_LHA(ULONG *, heightptr, A2),

/*  LOCATION */

	struct Library *, RTBase, 20, ReqTools)

/*  FUNCTION
   
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    int width, height, retval;
    
    retval = GetVScreenSize(screen, &width, &height); /* general.c */
    
    *widthptr  = (ULONG)width;
    *heightptr = (ULONG)height;
    
    return (ULONG)retval;
    
    AROS_LIBFUNC_EXIT
    
} /* rtGetVScreenSize */
