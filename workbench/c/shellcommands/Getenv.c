/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Getenv CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        Getenv

    SYNOPSIS

        NAME/A

    LOCATION

        C:

    FUNCTION

        Retrieves the information stored in the given global variable.

    INPUTS

        NAME - The name of the global variable.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Getenv Kickstart

            This will retrieve the version of the Kickstart ROM.

    BUGS

    SEE ALSO

        Setenv, Unsetenv

    INTERNALS

    HISTORY

        30-Jul-1997     laguest     Corrected a few things in the source
        27-Jul-1997     laguest     Initial inclusion into the AROS tree

******************************************************************************/

#include <proto/dos.h>

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/types.h>

#include <aros/shcommands.h>

#define BUFFER_SIZE     256

AROS_SH1(Getenv, 41.1,
AROS_SHA(STRPTR, ,NAME,/A,NULL))
{
    AROS_SHCOMMAND_INIT

    LONG            Var_Length;
    char            Var_Value[BUFFER_SIZE];
    IPTR            Display_Args[1];


    Var_Length = GetVar(SHArg(NAME),
           &Var_Value[0],
           BUFFER_SIZE,
           GVF_GLOBAL_ONLY | LV_VAR
    );

    if (Var_Length != -1)
    {
        Display_Args[0] = (IPTR)Var_Value;
        VPrintf("%s\n", Display_Args);

	return RETURN_OK;
    }

    SetIoErr(ERROR_OBJECT_NOT_FOUND);
    PrintFault(IoErr(), "Getenv");

    return RETURN_WARN;

    AROS_SHCOMMAND_EXIT
} /* main */
