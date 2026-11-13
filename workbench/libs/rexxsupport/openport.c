/*
    Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc: Rexx function to create a MsgPort
*/

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <proto/alib.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <aros/debug.h>

#include <string.h>

#include "rexxsupport_intern.h"
#include "portnode.h"

/*****************************************************************************

    NAME
        OPENPORT

    SYNOPSIS
    result = OPENPORT(name)

    FUNCTION
    Creates a named Exec message port and registers its ownership with
    the REXX message service for cleanup when the script finishes.

    INPUTS
    name - Requested public message-port name.

    RESULT
    The string "1" if a port was created and the registration exchange
    completed, or "0" if the name is already in use or port creation
    fails. ERR10_013 indicates that the REXX port is unavailable;
    ERR10_003 indicates allocation failure during setup.

    NOTES
    The call waits for a reply to its resource-registration message.
    Close a successfully created port with CLOSEPORT() when finished.

    BUGS
    The implementation does not check every allocation result and
    can retain a partially created port when setup fails. It logs but
    does not propagate a failed RexxMast registration response.

    SEE ALSO
    CLOSEPORT, WAITPKT, GETPKT

*****************************************************************************/

LONG rxsupp_openport(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct PortNode *node;
    struct PortNodeData *data;
    UBYTE *name;

    data = (struct PortNodeData *)FindName(&RSBI(RexxSupportBase)->openports, ARG1(msg));
    if (data == NULL)
    {
        struct MsgPort *replyport, *rexxport, *port;
        struct RexxMsg *msg2 = NULL, *msg3;
        if (FindPort(ARG1(msg)) != NULL)
        {
            *argstring = CreateArgstring("0", 1);
            return RC_OK;
        }
        rexxport = FindPort("REXX");
        if (rexxport == NULL)
        {
            *argstring = NULL;
            return ERR10_013;
        }
        port = CreateMsgPort();
        if (port)
        {
            node = (struct PortNode *)AllocMem(sizeof(struct PortNode), MEMF_PUBLIC);
            name = AllocMem(LengthArgstring(ARG1(msg))+1, MEMF_PUBLIC);
            strcpy(name, ARG1(msg));
            node->rsrc.rr_Node.ln_Name = name;
            node->rsrc.rr_Base = RexxSupportBase;
            node->rsrc.rr_Func = -36;
            node->data.node.ln_Name = name;
            node->data.self = (struct RexxRsrc *)node;
            node->data.port = port;
            NewList(&node->data.msgs);
            /* Lock lib in memory because it can be called by rexx */
            if (IsListEmpty(&RSBI(RexxSupportBase)->openports))
                RexxSupportBase->lib_OpenCnt++;
          
            AddTail(&RSBI(RexxSupportBase)->openports, &node->data.node);
            port->mp_Node.ln_Name = name;
            port->mp_Node.ln_Pri = 0;
            AddPort(port);
          
            replyport = CreateMsgPort();
            if (replyport != NULL)
            {
                replyport->mp_Node.ln_Name = NULL;
                replyport->mp_Node.ln_Pri = 0;
                msg2 = CreateRexxMsg(replyport, NULL, NULL);
            }
            if (replyport == NULL || msg2 == NULL)
            {
                *argstring = NULL;
                return ERR10_003;
            }
            msg2->rm_Private1 = msg->rm_Private1;
            msg2->rm_Private2 = msg->rm_Private2;
            msg2->rm_Action = RXADDRSRC;
            msg2->rm_Args[0] = (IPTR)node;
            PutMsg(rexxport, (struct Message *)msg2);
            do {
                WaitPort(replyport);
                msg3 = (struct RexxMsg *)GetMsg(replyport);
            } while (msg3 != msg2);
            if (msg2->rm_Result1 != RC_OK)
              kprintf("ClosePort ADDRSRC Result1 != RC_OK\n");
            DeleteRexxMsg(msg2);
            DeleteMsgPort(replyport);
         
            *argstring = CreateArgstring("1", 1);
            return RC_OK;
        }
        else
        {
            *argstring = CreateArgstring("0", 1);
            return RC_OK;
        }
    }
    else
    {
        *argstring = CreateArgstring("0", 1);
        return RC_OK;
    }
}
