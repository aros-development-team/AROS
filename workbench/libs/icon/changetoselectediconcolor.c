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

    AROS_LH1(VOID, ChangeToSelectedIconColor,

/*  SYNOPSIS */
        AROS_LHA(struct ColorRegister *, cr, A0),

/*  LOCATION */
        struct Library *, IconBase, 33, Icon)

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
    
#   warning TODO: Implement icon/ChangeToSelectedIconColor()
    aros_print_not_implemented("icon/ChangeToSelectedIconColor()");
    
    AROS_LIBFUNC_EXIT
} /* ChangeToSelectedIconColor() */
