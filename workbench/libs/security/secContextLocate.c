/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include "security_intern.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	AROS_LH4(APTR, secContextLocate,

/*  SYNOPSIS */
	/* (module, id, caller, size) */
	AROS_LHA(secPluginModule *, module, A0),
	AROS_LHA(ULONG, id, D0),
	AROS_LHA(struct Task *, caller, A1),
	AROS_LHA(ULONG, size, D1),

/*  LOCATION */
	struct Library *, SecurityBase, 52, Security)

/*  FUNCTION

    INPUTS


    RESULT


    NOTES


    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, SecurityBase)

    D(bug( DEBUG_NAME_STR "secContextLocate()\n") );

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secContextLocate */

