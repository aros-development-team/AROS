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
