/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(struct DiskObject *, GetDiskObjectNew,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
	struct Library *, IconBase, 22, Icon)

/*  FUNCTION
	Tries to open the supplied info file via GetDiskObject(). If this
	not succeeds it will try to read the default info file for
	that type of file.

    INPUTS
	name - name of the file to read an icon for.

    RESULT
	DiskObject - pointer ta diskobject struct.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetDiskObject(), GetDefDiskObject()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
        
    return GetIconTags(name, ICONGETA_FailIfUnavailable, FALSE, TAG_DONE);
    
    AROS_LIBFUNC_EXIT
} /* GetDiskObjectNew() */
