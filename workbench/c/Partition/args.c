/*
    Copyright (C) 2004-2023, The AROS Development Team. All rights reserved.
*/

#include <dos/rdargs.h>
#include <proto/dos.h>

#include "args.h"

/*** Global Variables *******************************************************/
STATIC CONST_STRPTR TEMPLATE =
    "DEVICE,"
    "UNIT/N,"
    "SYSSIZE/K/N,"
    "SYSTYPE/K,"
    "SYSNAME/K,"
    "WORKSIZE/K/N,"
    "MAXWORK/S,"
    "WORKTYPE/K,"
    "WORKNAME/K,"
    "BOOTSIZE/K/N,"
    "BOOTTYPE/K,"
    "BOOTNAME/K,"
    "WIPE/S,"
    "FORCE/S,"
    "QUIET/S,"
    "SCHEME/K,";
STATIC CONST LONG def_unit = 0;

STATIC CONST TEXT def_sys_name[] = "DH0";
STATIC CONST TEXT def_work_name[] = "DH1";

STATIC IPTR args[COUNT] =
{
    (IPTR) "ata.device",
    (IPTR) &def_unit,
    (IPTR) NULL,
    (IPTR) NULL,
    (IPTR) def_sys_name,
    (IPTR) NULL,
    (IPTR) FALSE,
    (IPTR) NULL,
    (IPTR) def_work_name,
};
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
