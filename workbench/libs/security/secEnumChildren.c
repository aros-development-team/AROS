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
	AROS_LH3(LONG, secEnumChildren,

/*  SYNOPSIS */
	/* (task, kids, size) */
	AROS_LHA(struct Task *, task, A0),
	AROS_LHA(struct Task **, kids, A1),
	AROS_LHA(LONG, size, D0),

/*  LOCATION */
	struct Library *, SecurityBase, 36, Security)

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

    D(bug( DEBUG_NAME_STR "secEnumChildren()\n") );

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secEnumChildren */

