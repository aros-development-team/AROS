/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AllocMiscResource() function.
    Lang: English
*/
#include "misc_intern.h"
#include <proto/exec.h>
#include <resources/misc.h>

/*****************************************************************************

    NAME */

	AROS_LH2(char *, AllocMiscResource,

/*  SYNOPSIS */
		 AROS_LHA(ULONG,  unitNum, D0),
		 AROS_LHA(char *, name,    A0),

/*  LOCATION */
		 APTR, MiscBase, 1, Misc)

/*  FUNCTION

    Allocates one of the miscellaneous resources.

    INPUTS

    unitNum  --  The resource to allocate
    name     --  An identifying name for you, must NOT be NULL.

    RESULT

    NULL if the allocation was successful. If the resource couln't be
    allocated, the name of the holder of the resource is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    FreeMiscResource()

    INTERNALS

    The misc.resource should probably just redirect commands to a HIDD
    in the future, to support things like multiple serial ports. 

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    char *errorStr = "Error -- invalid unit.";
    char *retval;

    if(unitNum >= MR_MAXUNIT)
	return errorStr;

    ObtainSemaphore(&GPB(MiscBase)->mb_Lock);

    retval = GPB(MiscBase)->mb_owners[unitNum];
    
    if(retval == NULL)
	GPB(MiscBase)->mb_owners[unitNum] = name;
	
    ReleaseSemaphore(&GPB(MiscBase)->mb_Lock);
    
    return retval;
    
    AROS_LIBFUNC_EXIT
} /* AllocMiscResource */
