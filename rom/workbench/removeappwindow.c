/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include "workbench_intern.h"

/*****************************************************************************

    NAME */

	#include <proto/wb.h>
	
	AROS_LH1(BOOL	, RemoveAppWindow,

/*  SYNOPSIS */

	AROS_LHA(struct AppWindow *, AppWindow, A0),

/*  LOCATION */
	struct WorkbenchBase *, WorkbenchBase, 9, Workbench)

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

    aros_print_not_implemented ("RemoveAppWindow");
#warning TODO: Write Workbench/RemoveAppWindow

    return FALSE;

    AROS_LIBFUNC_EXIT

} /* RemoveAppWindow */
