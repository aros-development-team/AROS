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

#include <string.h>

#include "rexxsupport_intern.h"
#include "portnode.h"

/*****************************************************************************

    NAME
        GETPKT

    SYNOPSIS
    message = GETPKT(port)

    FUNCTION
    Removes the next message from a RexxSupport-managed message port and
    tracks it for a later REPLY().

    INPUTS
    port - Name of a port previously created with OPENPORT().

    RESULT
    A binary argument string containing a native message pointer.
    ERR10_018 is returned when the port is not managed by RexxSupport.

    NOTES
    Call WAITPKT() to wait until a message is present before using GETPKT().
    The result contains binary pointer data, not printable text.

    BUGS
    The implementation does not check whether GetMsg() returned NULL
    before adding the pointer to its internal message list.

    SEE ALSO
    OPENPORT, WAITPKT, GETARG, REPLY

*****************************************************************************/

LONG rxsupp_getpkt(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct PortNodeData *data;
    struct Message *msg2;
  
    data = (struct PortNodeData *)FindName(&RSBI(RexxSupportBase)->openports, ARG1(msg));
    if (data != NULL)
    {
        msg2 = GetMsg(data->port);
        AddTail(&data->msgs, (struct Node *)msg2);
        *argstring = CreateArgstring((UBYTE *)&msg2, sizeof(struct Message *));
        return RC_OK;
    }
    else
    {
        *argstring = NULL;
        return ERR10_018;
    }
}
