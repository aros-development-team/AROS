/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Getenv CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Getenv

    SYNOPSIS

        NAME/A

    LOCATION

        Workbench:c

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

#define ARG_TEMPLATE    "NAME/A"
#define ARG_NAME        0
#define TOTAL_ARGS      1

#define BUFFER_SIZE     256

static const char version[] = "$VER: Getenv 41.1 (03.07.1997)\n";

int __nocommandline;

int main(void)
{
	struct RDArgs * rda;
    IPTR          * args[TOTAL_ARGS] = { NULL };
    int             Return_Value;
    LONG            Var_Length;
    char            Var_Value[BUFFER_SIZE];
    IPTR            Display_Args[1];

    Return_Value = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);
    if (rda)
    {
        Var_Length = GetVar((STRPTR)args[ARG_NAME],
               &Var_Value[0],
               BUFFER_SIZE,
               GVF_GLOBAL_ONLY | LV_VAR
        );

        if (Var_Length != -1)
        {
            Display_Args[0] = (IPTR)Var_Value;
            VPrintf("%s\n", Display_Args);
        }
        else
        {
            SetIoErr(ERROR_OBJECT_NOT_FOUND);
            PrintFault(IoErr(), "Getenv #1");

            Return_Value = RETURN_WARN;
        }
	FreeArgs(rda);
    }
    else
    {
        PrintFault(IoErr(), "Getenv #2");

        Return_Value = RETURN_ERROR;
    }

    return (Return_Value);

} /* main */
