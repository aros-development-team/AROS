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
#include "workbench_intern.h"


/*****************************************************************************

    NAME */
	#include <proto/wb.h>

	AROS_LH5(struct AppMenuItem *, AddAppMenuItemA,
/*  SYNOPSIS */
	AROS_LHA(ULONG			, id		, D0),
	AROS_LHA(ULONG			, userdata	, D1),
	AROS_LHA(APTR			, text		, A0),
	AROS_LHA(struct MsgPort *	, msgport	, A1),
	AROS_LHA(struct TagItem *	, taglist	, A3),

/*  LOCATION */
	struct WorkbenchBase *, WorkbenchBase, 12, Workbench)

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

    aros_print_not_implemented ("AddAppMenuItemA");
#warning TODO: Write Workbench/AddAppMenuItemA
    return NULL;

    AROS_LIBFUNC_EXIT
} /* AddAppMenuItemA */

