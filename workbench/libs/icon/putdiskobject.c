/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/arossupport.h>

#include "icon_intern.h"
#include "support.h"

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
    
    BOOL success = FALSE;
    BPTR file;
    
    if ((file = OpenIcon(name, MODE_NEWFILE)) != NULL)
    {
        success = WriteIcon(file, icon);
        CloseIcon(file);
    }
    
    return success;
    
    AROS_LIBFUNC_EXIT
} /* PutDiskObject() */
