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

#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "portnode.h"

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
