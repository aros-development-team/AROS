/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>
#include <exec/types.h>

	AROS_LH1(struct DiskObject *, GetDiskObject,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
	struct Library *, IconBase, 13, Icon)

/*  FUNCTION
	Opens an icon from disk.
	
    INPUTS
	name - filename without ".info" or NULL for an empty diskobject.

    RESULT
	Pointer to diskobject.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return GetIconTagList(name, NULL);
    
    AROS_LIBFUNC_EXIT
} /* GetDiskObject() */
