
/*
    (C) 1999 AROS - The Amiga Research OS
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

/*****************************************************************************

    NAME */

    AROS_LH0(VOID, rtUnlockPrefs,

/*  SYNOPSIS */

/*  LOCATION */

	struct Library *, RTBase, 30, ReqTools)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    rtLockPrefs()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ReleaseSemaphore(&GPB(RTBase)->rt.ReqToolsPrefs.PrefsSemaphore);

    AROS_LIBFUNC_EXIT
} /* rtUnlockPrefs */
