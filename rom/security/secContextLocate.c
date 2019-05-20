/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_plugins.h"

/*****************************************************************************

    NAME */
	AROS_LH4(APTR, secContextLocate,

/*  SYNOPSIS */
	/* (module, id, caller, size) */
	AROS_LHA(secPluginModule *, module, A0),
	AROS_LHA(ULONG, id, D0),
	AROS_LHA(struct Task *, caller, A1),
	AROS_LHA(ULONG, size, D1),

/*  LOCATION */
	struct SecurityBase *, secBase, 52, Security)

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

    APTR res = NULL;
    struct secTaskNode * node;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    node = FindContextOwner(secBase, caller);
    res = FindContext(node, module, id);
    if (res == NULL)
            res = AllocateContext(node, module, id, size);
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    return res;

    AROS_LIBFUNC_EXIT

} /* secContextLocate */
