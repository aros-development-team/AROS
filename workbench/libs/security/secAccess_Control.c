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
	AROS_LH4(BOOL, secAccess_Control,

/*  SYNOPSIS */
	/* (fs, task, owner, prot) */
	AROS_LHA(ULONG, fs, A1),
	AROS_LHA(APTR, task, A2),
	AROS_LHA(struct muExtOwner *, owner, D1),
	AROS_LHA(ULONG, prot, D3),

/*  LOCATION */
	struct Library *, SecurityBase, 33, Security)

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

    D(bug( DEBUG_NAME_STR "secAccess_Control()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secAccess_Control */

