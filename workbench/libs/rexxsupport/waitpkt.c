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
        WAITPKT

    SYNOPSIS
    result = WAITPKT(port)

    FUNCTION
    Waits until at least one message is available at a RexxSupport-
    managed message port.

    INPUTS
    port - Name of a port created by OPENPORT().

    RESULT
    The string "1" when WaitPort() returns. Unknown port names
    produce ERR10_018.

    NOTES
    WAITPKT() does not remove a message from the port. Call GETPKT()
    afterward to retrieve it.

    SEE ALSO
    OPENPORT, GETPKT, REPLY

*****************************************************************************/

LONG rxsupp_waitpkt(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct PortNodeData *data;
    
    data = (struct PortNodeData *)FindName(&RSBI(RexxSupportBase)->openports, ARG1(msg));
    if (data != NULL)
    {
        WaitPort(data->port);
        *argstring = CreateArgstring("1", 1);
        return RC_OK;
    }
    else
    {
        *argstring = NULL;
        return ERR10_018;
    }
}
