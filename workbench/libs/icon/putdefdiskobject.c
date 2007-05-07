/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(BOOL, PutDefDiskObject,

/*  SYNOPSIS */
	AROS_LHA(struct DiskObject *, icon, A0),

/*  LOCATION */
	struct Library *, IconBase, 21, Icon)

/*  FUNCTION
	Puts a new default icon for a certain type.

    INPUTS
	diskObject  - diskObject struct describing icon to put as new
		      default icon.

    RESULT
	TRUE if success, else FALSE. Error may be obtained via IoErr().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetDefDiskObject(), PutDiskObject()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return PutIconTags
    (
        NULL, NULL,
        ICONPUTA_PutDefaultType,  icon->do_Type,
        ICONPUTA_NotifyWorkbench, TRUE,
        TAG_DONE
    );
    
    AROS_LIBFUNC_EXIT
} /* PutDefDiskObject() */
