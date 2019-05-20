/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_groupinfo.h"
#include "security_memory.h"

/*****************************************************************************

    NAME */
	AROS_LH0(struct secGroupInfo *, secAllocGroupInfo,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct SecurityBase *, secBase, 25, Security)

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

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    return (MAlloc(sizeof(struct secPrivGroupInfo)));

    AROS_LIBFUNC_EXIT

} /* secAllocGroupInfo */

