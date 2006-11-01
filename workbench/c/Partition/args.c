/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/rdargs.h>
#include <proto/dos.h>

#include "args.h"

/*** Global Variables *******************************************************/
STATIC CONST_STRPTR   TEMPLATE    = "DEVICE,UNIT/N,FORCE/S,QUIET/S";
STATIC CONST LONG     DEFAULT_UNIT = 0;
STATIC IPTR           args[COUNT] = {(IPTR) "ata.device", (IPTR) &DEFAULT_UNIT};
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
