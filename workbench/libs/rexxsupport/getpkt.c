/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rexx function to wait for a packet on a port
    Lang: English
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
