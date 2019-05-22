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
	AROS_LH1(void, secFreeGroupInfo,

/*  SYNOPSIS */
	/* (info) */
	AROS_LHA(struct secGroupInfo *, info, A0),

/*  LOCATION */
	struct SecurityBase *, secBase, 26, Security)

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

    struct secPrivGroupInfo *_pinfo = (struct secPrivGroupInfo *)info;

    D(bug( DEBUG_NAME_STR " %s(0x%p)\n", __func__, info);)

    if (_pinfo) {
   	if (_pinfo->Pattern)
	      FreeV(_pinfo->Pattern);
 		Free(_pinfo, sizeof(struct secPrivGroupInfo));
	}

    AROS_LIBFUNC_EXIT

} /* secFreeGroupInfo */

