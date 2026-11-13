/*
    Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc: Rexx stub for FreeMem system function
*/

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

/*****************************************************************************

    NAME
        FREEMEM

    SYNOPSIS
    success = FREEMEM(pointer, size)

    FUNCTION
    Releases an Exec memory allocation using FreeMem().

    INPUTS
    pointer - Binary pointer argument string containing exactly
              sizeof(void *) bytes, as returned by ALLOCMEM().
    size    - Decimal allocation size supplied to the matching ALLOCMEM().

    RESULT
    The string "1" after FreeMem() is called. An invalid pointer-string
    length or size representation produces ERR10_018.

    NOTES
    The allocation must still be live, and size must match its original
    allocation. Do not use the address after the call.

    SEE ALSO
    ALLOCMEM

*****************************************************************************/

LONG rxsupp_freemem(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char *end;
    ULONG size;
    void *mem;
  
    *argstring = NULL;
    if (LengthArgstring(ARG1(msg)) != sizeof(void *))
        return ERR10_018;
  
    mem = *(void **)ARG1(msg);
    size = strtoul(ARG2(msg), &end, 10);
    while (isspace(*end)) end++;
    if (*end != 0)
        return ERR10_018;

    FreeMem(mem, size);
    *argstring = CreateArgstring("1", 1);
    return 0;
}
