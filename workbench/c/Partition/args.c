/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/rdargs.h>
#include <proto/dos.h>

#include "args.h"

/*** Global Variables *******************************************************/
STATIC CONST_STRPTR   TEMPLATE = "FORCE/K/S,QUIET/K/S";
STATIC IPTR           args[COUNT];
STATIC struct RDArgs *rdargs;
 
/*** Functions **************************************************************/
BOOL ReadArguments(VOID)
{
    rdargs = ReadArgs(TEMPLATE, args, NULL);
    return rdargs != NULL;
}

VOID FreeArguments(VOID)
{
    FreeArgs(rdargs);
}

IPTR GetArgument(enum Argument id)
{
    if (id >= 0 && id < COUNT) return args[id];
    else                       return 0;
}
