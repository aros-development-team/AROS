/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include "workbench_intern.h"

/*****************************************************************************

    NAME */

	#include <proto/workbench.h>
	
	AROS_LH1(BOOL	, RemoveAppMenuItem,
/*  SYNOPSIS */

	AROS_LHA(struct AppMenuItem *, AppMenuItem, A0),

/*  LOCATION */
	struct WorkbenchBase *, WorkbenchBase, 13, Workbench)

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

    aros_print_not_implemented ("RemoveAppMenuItem");
#warning TODO: Write Workbench/RemoveAppMenuItem

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppMenuItem */

