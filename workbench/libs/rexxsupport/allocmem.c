/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rexx stub for AllocMem system function
    Lang: English
*/

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

#warning FIXME: Second argument to AllocMem is not handled at the moment
LONG rxsupp_allocmem(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char *end;
    ULONG size;
    void *mem;
    ULONG attributes = MEMF_PUBLIC;
  
    if ((msg->rm_Action & RXARGMASK) == 2)
    {
        if (LengthArgstring(ARG2(msg)) != 4)
        {
	    *argstring = NULL;
	    return ERR10_018;
	}
        else
        {
	    /* This value has always has to be given in little endian to keep arexx
	     * scripts portable */
	    UBYTE *bytes = ARG2(msg);
	    attributes = (ULONG)bytes[0]<<24 | (ULONG)bytes[1]<<16 | (ULONG)bytes[2]<<8 | (ULONG)bytes[3];
	}
    }
    size = strtoul(ARG1(msg), &end, 10);
    while (isspace(*end)) end++;
    if (*end != 0)
    {
        *argstring = NULL;
        return ERR10_018;
    }
    mem = AllocMem(size, attributes);
    if (mem==NULL)
    {
        *argstring = NULL;
        return ERR10_003;
    }
    else
    {
        *argstring = CreateArgstring(&mem, 4);
        return 0;
    }
}
