/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unsetenv CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        Unsetenv

    SYNOPSIS

        NAME

    LOCATION

        C:

    FUNCTION

    INPUTS

        NAME    - The name of the global variable to unset.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <utility/tagitem.h>
#include <aros/shcommands.h>

AROS_SH1(Unsetenv, 41.0,
AROS_SHA(STRPTR, ,NAME, ,NULL))
{
    AROS_SHCOMMAND_INIT


    if (SHArg(NAME) != NULL)
    {
            /* Delete the global variable from the list.
             */
        if (!DeleteVar(SHArg(NAME), GVF_GLOBAL_ONLY))
            return RETURN_FAIL;
    }
    else
    {
        /* Display a list of global variables.
         */
        BPTR lock                 = NULL;
	struct FileInfoBlock *FIB = NULL;

	if
	(
	    !(lock = Lock("ENV:", ACCESS_READ))    ||
	    !(FIB = AllocDosObject(DOS_FIB, NULL)) ||
	    (Examine(lock, FIB) == DOSFALSE)
	)
	{
	    if (FIB) FreeDosObject(DOS_FIB, FIB);
	    if (lock) UnLock(lock);

	    return RETURN_FAIL;
	}

        while (ExNext(lock, FIB))
        {
	    /* don't show dirs */
            if (FIB->fib_DirEntryType < 0)
	    {
                FPuts(Output(),FIB->fib_FileName);
		FPuts(Output(),"\n");
 	    }
	}

	FreeDosObject(DOS_FIB, FIB);
	UnLock(lock);
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
} /* main */
