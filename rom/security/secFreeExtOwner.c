/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_memory.h"

/*****************************************************************************

    NAME */
	AROS_LH1(void, secFreeExtOwner,

/*  SYNOPSIS */
	/* void */
	AROS_LHA(struct secExtOwner *, owner, A0),

/*  LOCATION */
	struct SecurityBase *, secBase, 22, Security)

/*  FUNCTION
	Free an Extended Owner structure

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

    ULONG size;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (owner) {
        size = sizeof(struct secExtOwner)+owner->NumSecGroups*sizeof(UWORD);
        Free(owner, size);
    }

    AROS_LIBFUNC_EXIT

} /* secFreeExtOwner */

