/*
    Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc: Rexx function to wait for a packet on a port
*/

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <proto/alib.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "portnode.h"

/*****************************************************************************

    NAME
        GETARG

    SYNOPSIS
    value = GETARG(message[, index])

    FUNCTION
    Copies one Rexx argument from a message identified by its binary
    pointer value into a new Rexx argument string.

    INPUTS
    message - Binary pointer to a RexxMsg, as returned by GETPKT().
    index   - Optional argument index; when omitted, index 0 is selected.

    RESULT
    An allocated argument string on success. ERR10_018 is returned for
    an invalid message identity or invalid parsed index.

    NOTES
    The message must remain valid until this call completes. Argument
    indices refer to the rm_Args array, beginning at zero.

    BUGS
    When index is supplied, the implementation incorrectly parses the
    binary message pointer argument as a decimal index instead of parsing
    the second argument. It also does not validate the pointer argument's
    byte length before dereferencing it.

    SEE ALSO
    GETPKT, TYPEPKT, REPLY

*****************************************************************************/

LONG rxsupp_getarg(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct RexxMsg *msg2 = *(struct RexxMsg **)ARG1(msg);
    UBYTE arg;

    if (msg2 == NULL || !IsRexxMsg(msg2))
    {
        *argstring = NULL;
        return ERR10_018;
    }

    if ((msg->rm_Action & RXARGMASK) == 1)
        arg = 0;
    else
    {
        char *end;
        int size = strtoul(ARG1(msg), &end, 10);
        if ((*end != 0) || size<0 || size>15)
        {
            *argstring = NULL;
            return ERR10_018;
        }
        else
            arg = size;
    }
  
    *argstring = CreateArgstring(RXARG(msg2, arg), LengthArgstring(RXARG(msg2, arg)));
    return RC_OK;
}
