/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rexx stub for FreeMem system function
    Lang: English
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
