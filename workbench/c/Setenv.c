  /*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Setenv CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Setenv

    SYNOPSIS

        NAME,STRING/F

    LOCATION

        Workbench:c

    FUNCTION

        Sets a global variable from the current shell. These variables can
        be accessed from any program executing at any time.

        These variables are not saved in the ENVARC: directory, hence they
        can only be used by programs during the current execution of the
        operating system.

        If no parameters are specified, the current list of global variables
        are displayed.

    INPUTS

        NAME    - The name of the global variable to set.

        STRING  - The value of the global variable NAME.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Setenv EDITOR Ed

            Any program that accesses the variable "EDITOR" will be able to
            find out the name of the text-editor the user would like to use,
            by examining the contents of the variable.

    BUGS

    SEE ALSO

        Getenv, Unsetenv

    INTERNALS

    HISTORY

        30-Jul-1997     laguest     Initial inclusion into the AROS tree
        13-Aug-1997     srittau     Minor changes

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
#include <stdio.h>

#include "shcommands.h"

AROS_SH2(Setenv, 41.1, 13.08.1997,
AROS_SHA(,NAME,     , NULL),
AROS_SHA(,STRING, /F, NULL))
{
    AROS_SHCOMMAND_INIT

    if (SHArg(NAME) != NULL || SHArg(STRING) != NULL)
    {
        /* Make sure we get to here is either arguments are
         * provided on the command line.
         */
        if (SHArg(NAME) != NULL && SHArg(STRING) != NULL)
        {
            /* Add the new global variable to the list.
             */
	     if
	     (
	         !SetVar((STRPTR)SHArg(NAME),
                         (STRPTR)SHArg(STRING),
                         -1,
                         GVF_GLOBAL_ONLY)
             )
	     {
	         SHReturn(RETURN_ERROR);
	     }
        }
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

	    SHReturn(RETURN_FAIL);
	}

        while (ExNext(lock, FIB))
        {
	    /* don't show dirs */
            if (FIB->fib_DirEntryType < 0)
	    {
                FPuts(Output(),&FIB->fib_FileName);
		FPuts(Output(),"\n");
 	    }
	}

	FreeDosObject(DOS_FIB, FIB);
	UnLock(lock);
    }

    SHReturn(RETURN_OK);

    AROS_SHCOMMAND_EXIT
} /* main */
