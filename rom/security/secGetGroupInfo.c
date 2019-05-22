/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <stdio.h>

#include "security_intern.h"
#include "security_groupinfo.h"
#include "security_server.h"

/*****************************************************************************

    NAME */
	AROS_LH2(struct secGroupInfo *, secGetGroupInfo,

/*  SYNOPSIS */
	/* (info, keytype) */
	AROS_LHA(struct secGroupInfo *, info, A0),
	AROS_LHA(ULONG, keytype, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 27, Security)

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

    struct secGroupInfo *groupInfo;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    groupInfo = ((struct secGroupInfo *)SendServerPacket(
            secBase, secSAction_GetGroupInfo,
            (SIPTR)info, keytype, (SIPTR)NULL, (SIPTR)NULL));

    return groupInfo;

    AROS_LIBFUNC_EXIT

} /* secGetGroupInfo */

