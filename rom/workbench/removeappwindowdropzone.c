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

    AROS_LH2(BOOL, RemoveAppWindowDropZone,

/*  SYNOPSIS */
    AROS_LHA(struct AppWindow *		, aw      , A0),
    AROS_LHA(struct AppWindowDropZone *	, dropzone     , A1),

/*  LOCATION */
    struct WorkbenchBase *, WorkbenchBase, 20, Workbench)

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

    aros_print_not_implemented ("RemoveAppWindowDropZone");
#warning TODO: Write Workbench/RemoveAppWindowDropZone

    return NULL;

    AROS_LIBFUNC_EXIT
} /* RemoveAppWindowDropZone */

