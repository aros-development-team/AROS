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
        REPLY

    SYNOPSIS
    result = REPLY(message[, code])

    FUNCTION
    Removes a received message from RexxSupport's tracking list and sends
    an Exec reply to its reply port.

    INPUTS
    message - Binary pointer to a message obtained using GETPKT().
    code    - Optional intended Rexx return code for a RexxMsg.

    RESULT
    The string "1" after sending the reply. ERR10_018 is returned if
    the decoded message pointer is NULL or the parsed code is invalid.

    NOTES
    After REPLY(), do not use the message pointer; ownership has passed
    back to the original sender.

    BUGS
    When code is supplied, the implementation parses the binary message
    pointer argument instead of the second argument. It does not validate
    the pointer argument's byte length or membership in the tracked list.

    SEE ALSO
    GETPKT, GETARG, TYPEPKT

*****************************************************************************/

LONG rxsupp_reply(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct RexxMsg *msg2 = *(struct RexxMsg **)ARG1(msg);
    
    if (msg2 == NULL)
    {
        *argstring = NULL;
        return ERR10_018;
    }

    if ((msg->rm_Action & RXARGMASK) == 2 && IsRexxMsg(msg2))
    {
        char *end;
        int code = strtoul(ARG1(msg), &end, 10);
        if ((*end != 0) || code<0)
        {
            *argstring = NULL;
            return ERR10_018;
        }
        else
        {
            msg2->rm_Result1 = code;
            msg2->rm_Result2 = (IPTR)NULL;
        }
    }

    Remove((struct Node *)msg2);
    ReplyMsg((struct Message *)msg2);
    *argstring = CreateArgstring("1", 1);
    return RC_OK;
}
