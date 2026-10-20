/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Desc: Convert BCPL address to C pointer
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
        BADDR

    SYNOPSIS
    pointer = BADDR(bptr)

    FUNCTION
    Converts a binary BPTR value into a native C address using BADDR().

    INPUTS
    bptr - A binary argument string containing exactly sizeof(void *)
           bytes representing a BPTR value.

    RESULT
    A binary argument string containing a native pointer. ERR10_018 is
    returned when the input is not pointer-sized.

    NOTES
    The result is binary address data and is not a character string.
    The function does not verify that the resulting address is valid.

    SEE ALSO
    NULL, OFFSET

*****************************************************************************/

LONG rxsupp_baddr(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    void *ptr;

    *argstring = NULL;
    
    if (LengthArgstring(ARG1(msg)) != sizeof(void *))
        return ERR10_018;
    
    ptr = BADDR(*(BPTR *)ARG1(msg));
    
    *argstring = CreateArgstring((UBYTE *)&ptr, sizeof(void *));
    return RC_OK;
}
