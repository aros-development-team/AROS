/*
    Copyright © 2009-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <dos/rdargs.h>
#include <proto/dos.h>

#include "args.h"

/*** Global Variables *******************************************************/
STATIC CONST_STRPTR TEMPLATE = "FROM,SAVE/S";
STATIC IPTR args[COUNT];
STATIC struct RDArgs *rdargs;

/*** Functions **************************************************************/
BOOL ReadArguments(int argc, char **argv)
{
    if (argc != 0) // started from Shell
    {
        rdargs = ReadArgs(TEMPLATE, args, NULL);
        return rdargs != NULL;
    }
    return TRUE;
}

VOID FreeArguments(VOID)
{
    FreeArgs(rdargs); // safe to use with NULL
}

IPTR GetArgument(enum Argument id)
{
    if (id >= 0 && id < COUNT) return args[id];
    else return 0;
}
