/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <stdio.h>

#include "security_intern.h"
#include "security_server.h"

/*****************************************************************************

    NAME */
	AROS_LH0(BPTR, secGetConfigDirLock,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct SecurityBase *, secBase, 20, Security)

/*  FUNCTION
              Get a Shared Lock on the Directory of the Configuration File

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

    BPTR dirLock;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    dirLock = ((BPTR)SendServerPacket(
            secBase, secSAction_ConfigDirLock,
            (SIPTR)NULL, (SIPTR)NULL, (SIPTR)NULL, (SIPTR)NULL));

    return dirLock;

    AROS_LIBFUNC_EXIT

} /* secGetConfigDirLock */

