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
	AROS_LH1(void, secFreeUserInfo,

/*  SYNOPSIS */
	/* void */
	AROS_LHA(struct secUserInfo *, info, A0),

/*  LOCATION */
	struct SecurityBase *, secBase, 10, Security)

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

    struct secPrivUserInfo *_pinfo = (struct secPrivUserInfo *)info;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (_pinfo) {
        if (_pinfo->Pub.NumSecGroups)
            Free(_pinfo->Pub.SecGroups, _pinfo->Pub.NumSecGroups*sizeof(UWORD));
        if (_pinfo->Pattern)
            FreeV(_pinfo->Pattern);
        Free(_pinfo, sizeof(struct secPrivUserInfo));
    }

    AROS_LIBFUNC_EXIT

} /* secFreeUserInfo */

