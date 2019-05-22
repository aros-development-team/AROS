/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_userinfo.h"
#include "security_memory.h"

/*****************************************************************************

    NAME */
	AROS_LH0(struct secUserInfo *, secAllocUserInfo,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct SecurityBase *, secBase, 9, Security)

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

    return (MAlloc(sizeof(struct secPrivUserInfo)));

    AROS_LIBFUNC_EXIT

} /* secAllocUserInfo */

