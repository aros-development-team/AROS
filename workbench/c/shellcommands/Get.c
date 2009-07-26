/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        Get

    SYNOPSIS

        NAME/A

    LOCATION

        C:

    FUNCTION

        Retrieves the information stored in the given local variable.

    INPUTS

        NAME - The name of the local variable.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Get Result2

            This will retrieve the secondary return code of the last command
            that was executed.

    BUGS

    SEE ALSO

        Set, Unset

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

AROS_SH1(Get, 41.1,
AROS_SHA(STRPTR, ,NAME,/A,NULL))
{
    AROS_SHCOMMAND_INIT

    LONG            Var_Length;
    char            Var_Value[BUFFER_SIZE];
    IPTR            Display_Args[1];


    Var_Length = GetVar(SHArg(NAME), &Var_Value[0], BUFFER_SIZE,
			GVF_LOCAL_ONLY);

    if (Var_Length != -1)
    {
        Display_Args[0] = (IPTR)Var_Value;
        VPrintf("%s\n", Display_Args);
	
        return RETURN_OK;
    }

    SetIoErr(ERROR_OBJECT_NOT_FOUND);
    PrintFault(IoErr(), "Get");

    return RETURN_WARN;

    AROS_SHCOMMAND_EXIT
} /* main */
