/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <workbench/icon.h>
#include <proto/icon.h>

#include "icon_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH1(struct DiskObject *, NewDiskObject,

/*  SYNOPSIS */
        AROS_LHA(ULONG, type, D0),
        
/*  LOCATION */
        struct Library *, IconBase, 29, Icon)

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
    
#   warning TODO: Implement icon/NewDiskObject()
    aros_print_not_implemented("icon/NewDiskObject()");

    return NULL;
    
    AROS_LIBFUNC_EXIT
} /* NewDiskObject() */
