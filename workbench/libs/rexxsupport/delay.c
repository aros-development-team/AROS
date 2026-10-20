/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Desc: Rexx stub for Delay system function
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

/*****************************************************************************

    NAME
        DELAY

    SYNOPSIS
    result = DELAY(ticks)

    FUNCTION
    Suspends the calling task for the specified number of DOS ticks.

    INPUTS
    ticks - Nonempty decimal string containing only digits.

    RESULT
    The string "0" after Delay() returns. Invalid input produces
    ERR10_018.

    NOTES
    A DOS tick is the unit accepted by DOS Delay(); this function does
    not accept a duration in milliseconds.

    SEE ALSO
    FORBID, PERMIT

*****************************************************************************/

LONG rxsupp_delay(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    int pos, len;
    char *arg = ARG1(msg);
    
    len = strlen(arg);
    if (len == 0)
    {
        *argstring = NULL;
        return ERR10_018;
    }

    for (pos = 0; pos < len; pos++)
    {
        if (!isdigit(arg[pos]))
        {
            *argstring = NULL;
            return ERR10_018;
        }
    }
    
    Delay(atoi(arg));
    
    *argstring = CreateArgstring("0", 1);
    return RC_OK;
}
