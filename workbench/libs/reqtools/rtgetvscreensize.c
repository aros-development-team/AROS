/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

	struct ReqToolsBase *, ReqToolsBase, 20, ReqTools)

/*  FUNCTION
	Use this function to get the size of the visible portion of a
	screen.

	The value returned by rtGetVScreenSize() can be used for vertical
	spacing. It will be larger for interlaced and productivity screens.
	Using this number for spacing will assure your requester will look
	good on an interlaced and a non-interlaced screen.

	Current return codes are 2 for non-interlaced and 4 for interlaced.
	These values may change in the future, don't depend on them too
	much. They will in any case remain of the same magnitude.
   
    INPUTS
	screen - pointer to the screen.
	widthptr - address of an ULONG variable to hold the width.
	heightptr - address of an ULONG variable to hold the height.

    RESULT
	spacing - vertical spacing for the screen.

    NOTES
	This function is for the advanced ReqTools user.

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
