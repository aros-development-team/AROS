/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH2(BOOL, PutDiskObject,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR,        name, A0),
	AROS_LHA(struct DiskObject *, icon, A1),

/*  LOCATION */
	struct Library *, IconBase, 14, Icon)

/*  FUNCTION
	Writes icon to disk.
	
    INPUTS
	name - filename, ".info" will be appended.
	icon - diskobject to write
	
    RESULT
	TRUE on success, FALSE on error
	
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return PutIconTags
    (
        name, icon,
        ICONPUTA_NotifyWorkbench, TRUE,
        TAG_DONE
    );
    
    AROS_LIBFUNC_EXIT
} /* PutDiskObject() */
