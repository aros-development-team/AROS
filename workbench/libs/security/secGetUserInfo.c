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
	AROS_LH2(struct muUserInfo *, secGetUserInfo,

/*  SYNOPSIS */
	/* void */
	AROS_LHA(struct muUserInfo *, info, A0),
	AROS_LHA(ULONG, keytype, D0),

/*  LOCATION */
	struct Library *, SecurityBase, 11, Security)

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

    D(bug( DEBUG_NAME_STR "secGetUserInfo()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secGetUserInfo */

