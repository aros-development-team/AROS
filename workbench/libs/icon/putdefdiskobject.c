/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(BOOL, PutDefDiskObject,

/*  SYNOPSIS */
	AROS_LHA(struct DiskObject *, diskObject, A0),

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

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    UBYTE definfoname[MAX_DEFICON_FILEPATH];

    GetDefIconName (diskObject->do_Type, definfoname);

    return PutDiskObject (definfoname,diskObject);
    AROS_LIBFUNC_EXIT
} /* PutDefDiskObject */
