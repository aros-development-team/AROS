/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, IconBase)
    
    return GetIconTagList(name, NULL);
    
    AROS_LIBFUNC_EXIT
} /* GetDiskObject() */
