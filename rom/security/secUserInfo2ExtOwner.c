/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_memory.h"

#include <libraries/mufs.h>

/*****************************************************************************

    NAME */
	AROS_LH1(struct secExtOwner *, secUserInfo2ExtOwner,

/*  SYNOPSIS */
	/* (info) */
	AROS_LHA(struct secUserInfo *, info, A0),

/*  LOCATION */
	struct SecurityBase *, secBase, 24, Security)

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

    struct secExtOwner *owner = NULL;
    ULONG size;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (info) {
        size = info->NumSecGroups*sizeof(UWORD);
        if ((owner = (struct secExtOwner *)MAlloc(size+sizeof(struct secExtOwner)))) {
            owner->uid = info->uid;
            owner->gid = info->gid;
            owner->NumSecGroups = info->NumSecGroups;
            CopyMem(info->SecGroups, secSecGroups(owner), size);
        }
    }
    return(owner);

    AROS_LIBFUNC_EXIT

} /* secUserInfo2ExtOwner */

