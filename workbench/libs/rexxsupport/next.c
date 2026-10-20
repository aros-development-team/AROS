/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Desc: Get the pointer value a pointer is pointing to, often used to get
          the next element of a list
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
        NEXT

    SYNOPSIS
    value = NEXT(pointer[, offset])

    FUNCTION
    Copies a pointer-sized sequence of bytes from the supplied argument
    string's own data buffer, starting at the specified byte offset.

    INPUTS
    pointer - Binary argument string of exactly sizeof(void *) bytes.
    offset  - Optional signed decimal byte offset; defaults to zero.

    RESULT
    A pointer-sized binary argument string. ERR10_018 is returned for
    an invalid pointer-string length or offset representation.

    BUGS
    Despite its name, the implementation does not dereference the pointer
    encoded in pointer. A nonzero offset is not checked against the input
    buffer size and can read outside the argument string.

    SEE ALSO
    OFFSET, BADDR

*****************************************************************************/

LONG rxsupp_next(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char *ptr, *end;
    LONG offset = 0;

    if ((msg->rm_Action & RXARGMASK) > 1)
    {
        offset = strtol(ARG2(msg), &end, 10);
        while (isspace(*end)) end++;
        if (*end != 0)
        {
            *argstring = NULL;
            return ERR10_018;
        }
    }
    
    if (LengthArgstring(ARG1(msg)) != sizeof(void *))
    {
        *argstring = NULL;
        return ERR10_018;
    }
    
    ptr = (char *)ARG1(msg) + offset;
    
    *argstring = CreateArgstring((UBYTE *)ptr, sizeof(void *));
    return RC_OK;
}
