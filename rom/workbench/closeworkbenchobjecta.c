/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Close a named named drawer displayed by Workbench. 
    Lang: english
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH2(BOOL, CloseWorkbenchObjectA,

/*  SYNOPSIS */
        AROS_LHA(STRPTR,           name, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 17, Workbench)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    /* No tags defined fot this function... */

    return NULL;

    AROS_LIBFUNC_EXIT
} /* CloseWorkbenchObjectA */

