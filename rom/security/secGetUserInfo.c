/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <stdio.h>

#include "security_intern.h"
#include "security_userinfo.h"
#include "security_server.h"

/*****************************************************************************

    NAME */
	AROS_LH2(struct secUserInfo *, secGetUserInfo,

/*  SYNOPSIS */
	/* void */
	AROS_LHA(struct secUserInfo *, info, A0),
	AROS_LHA(ULONG, keytype, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 11, Security)

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

    struct secUserInfo *userInfo;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    userInfo = ((struct secUserInfo *)SendServerPacket(
            secBase, secSAction_GetUserInfo,
            (SIPTR)info, keytype, (SIPTR)NULL, (SIPTR)NULL));

    return userInfo;

    AROS_LIBFUNC_EXIT

} /* secGetUserInfo */

