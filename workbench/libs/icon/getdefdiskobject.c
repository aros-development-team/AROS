/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

/*********************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(struct DiskObject *, GetDefDiskObject,

/*  SYNOPSIS */
	AROS_LHA(LONG, type, D0),

/*  LOCATION */
	struct Library *, IconBase, 20, Icon)

/*  FUNCTION
	Gets the default icon for the supplied type of icon.

    INPUTS
	type  -  type of icon to get default diskobject for.


    RESULT
	DiskObject structure or NULL if an error occured. The error may
	be obtained by IoErr().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	PutDefDiskObject(), GetDiskObjectNew()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, IconBase)
    
    return GetIconTags(NULL, ICONGETA_GetDefaultType, type, TAG_DONE);
    
    AROS_LIBFUNC_EXIT
} /* GetDefDiskObject() */
