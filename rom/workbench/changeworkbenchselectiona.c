/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <workbench/workbench.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include "workbench_intern.h"

/*****************************************************************************

    NAME */

    #include <proto/workbench.h>

    AROS_LH3(BOOL, ChangeWorkbenchSelectionA,

/*  SYNOPSIS */
    AROS_LHA(STRPTR,              name, A0),
    AROS_LHA(struct Hook *,     hook, A1),
    AROS_LHA(struct TagItem *,  tags, A2),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 21, Workbench)

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

    aros_print_not_implemented ("ChangeWorkbenchSelectionA");
#warning TODO: Write Workbench/ChangeWorkbenchSelectionA

    return NULL;

    AROS_LIBFUNC_EXIT
} /* ChangeWorkbenchSelectionA */

