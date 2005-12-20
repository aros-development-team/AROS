/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rexx function to close a MsgPort
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

LONG rxsupp_closeport(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct PortNodeData *data;

    data = (struct PortNodeData *)FindName(&RSBI(RexxSupportBase)->openports, ARG1(msg));
    if (data != NULL)
    {
        struct MsgPort *replyport, *rexxport;
        struct RexxMsg *msg2, *msg3;
      
        rexxport = FindPort("REXX");
        replyport = CreateMsgPort();
        msg2 = CreateRexxMsg(replyport, NULL, NULL);
        if (rexxport == NULL || replyport == NULL || msg2 == NULL)
        {
	    *argstring = NULL;
	    return rexxport == NULL ? ERR10_013 : ERR10_003;
	}
        replyport->mp_Node.ln_Name = NULL;
        msg2->rm_Private1 = msg->rm_Private1;
        msg2->rm_Private2 = msg->rm_Private2;
	msg2->rm_Action = RXREMRSRC;
	msg2->rm_Args[0] = (IPTR)data->self;
	PutMsg(rexxport, (struct Message *)msg2);
	do {
	    WaitPort(replyport);
	    msg3 = (struct RexxMsg *)GetMsg(replyport);
	} while (msg3 != msg2);
        if (msg2->rm_Result1 != RC_OK)
	  kprintf("ClosePort REMRSRC Result1 != RC_OK\n");
        DeleteMsgPort(replyport);
        DeleteRexxMsg(msg2);

        portcleanup(RexxSupportBase, (struct PortNode *)data->self);
      
        *argstring = CreateArgstring("1", 1);
    }
    else
    {
        *argstring = CreateArgstring("0", 1);
    }
    return RC_OK;
}
