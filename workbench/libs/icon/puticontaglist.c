/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/icon.h>

#include "icon_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH3(BOOL, PutIconTagList,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR,        path, A0),
        AROS_LHA(struct DiskObject *, icon, A1),
        AROS_LHA(struct TagItem *,    tags, A2),

/*  LOCATION */
        struct Library *, IconBase, 31, Icon)

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
    
#   warning TODO: Implement icon/PutIconTagList()
    aros_print_not_implemented("icon/PutIconTagList()");
    
    return FALSE;
    
    AROS_LIBFUNC_EXIT
} /* PutIconTagList() */
