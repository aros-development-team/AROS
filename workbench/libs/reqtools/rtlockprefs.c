/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include "reqtools_intern.h"
#include "general.h"
#include "rtfuncs.h"

/*****************************************************************************

    NAME */

    AROS_LH0(struct ReqToolsPrefs *, rtLockPrefs,

/*  SYNOPSIS */

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 28, ReqTools)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    rtUnlockPrefs()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return RTFuncs_LockPrefs(ReqToolsBase);

    AROS_LIBFUNC_EXIT
    
} /* rtLockPrefs */
