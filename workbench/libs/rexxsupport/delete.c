/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Desc: Delete a file
*/

#include <proto/exec.h>
#include <proto/dos.h>
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
        DELETE

    SYNOPSIS
    result = DELETE(path)

    FUNCTION
    Deletes a file or filesystem entry using DOS DeleteFile().

    INPUTS
    path - Path of the entry to delete.

    RESULT
    The string "1" if DeleteFile() succeeds, or "0" if it fails.

    NOTES
    A failed deletion is represented by the returned string, not by a
    nonzero RexxSupport dispatch status.

    SEE ALSO
    MAKEDIR, RENAME

*****************************************************************************/

LONG rxsupp_delete(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    BOOL ok;

    ok = DeleteFile(ARG1(msg));
    *argstring = CreateArgstring(ok ? "1" : "0", 1);

    return RC_OK;
}
