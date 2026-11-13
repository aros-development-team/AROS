/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Desc: Returns NULL pointer
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
        NULL

    SYNOPSIS
    pointer = NULL()

    FUNCTION
    Returns a binary representation of a native null pointer.

    RESULT
    An argument string containing sizeof(void *) zero-valued pointer
    bytes. This is not the printable string "0".

    SEE ALSO
    ALLOCMEM, BADDR, OFFSET

*****************************************************************************/

LONG rxsupp_null(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    void *ptr = NULL;
  
    *argstring = CreateArgstring((UBYTE *)&ptr, sizeof(void *));
    return 0;
}
