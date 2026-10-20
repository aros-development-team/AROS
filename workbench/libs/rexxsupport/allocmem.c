/*
    Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc: Rexx stub for AllocMem system function
*/

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

/*****************************************************************************

    NAME
        ALLOCMEM

    SYNOPSIS
    pointer = ALLOCMEM(size[, attributes])

    FUNCTION
    Allocates memory using Exec AllocMem(). Returns the allocated address
    as a binary pointer value in a Rexx argument string.

    INPUTS
    size       - Allocation size expressed as a decimal string.
    attributes - Optional four-byte binary Exec memory attribute mask.
                 If omitted, MEMF_PUBLIC is used. Byte 0 supplies bits
                 31-24; byte 3 supplies bits 7-0.

    RESULT
    A pointer-sized binary argument string on success. ERR10_018 is returned
    for an invalid size string or attribute length, and ERR10_003 if Exec
    allocation fails.

    NOTES
    The result is binary pointer data, not a printable decimal address.
    The caller must eventually pass the same pointer and allocation size to
    FREEMEM(). The pointer size depends on the target architecture.

    BUGS
    If memory allocation succeeds but creation of the result argument
    string fails, the function does not release the allocated memory.

    SEE ALSO
    FREEMEM, NULL, OFFSET

*****************************************************************************/

LONG rxsupp_allocmem(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char *end;
    ULONG size;
    void *mem;
    ULONG attributes = MEMF_PUBLIC;
  
    if ((msg->rm_Action & RXARGMASK) == 2)
    {
        if (LengthArgstring(ARG2(msg)) != 4)
        {
            *argstring = NULL;
            return ERR10_018;
        }
        else
        {
            /* This value has always has to be given in little endian to keep arexx
             * scripts portable */
            UBYTE *bytes = ARG2(msg);
            attributes = (ULONG)bytes[0]<<24 | (ULONG)bytes[1]<<16 | (ULONG)bytes[2]<<8 | (ULONG)bytes[3];
        }
    }
    size = strtoul(ARG1(msg), &end, 10);
    while (isspace(*end)) end++;
    if (*end != 0)
    {
        *argstring = NULL;
        return ERR10_018;
    }
    mem = AllocMem(size, attributes);
    if (mem==NULL)
    {
        *argstring = NULL;
        return ERR10_003;
    }
    else
    {
        *argstring = CreateArgstring((UBYTE *)&mem, sizeof(void *));
        return 0;
    }
}
