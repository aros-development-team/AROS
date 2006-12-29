/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get the pointer value a pointer is pointing to, often used to get
          the next element of a list
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

LONG rxsupp_next(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char *ptr, *end;
    LONG offset = 0;

    if ((msg->rm_Action & RXARGMASK) > 1)
    {
	offset = strtol(ARG2(msg), &end, 10);
	while (isspace(*end)) end++;
	if (*end != 0)
	{
	    *argstring = NULL;
	    return ERR10_018;
	}
    }
    
    if (LengthArgstring(ARG1(msg)) != sizeof(void *))
    {
	*argstring = NULL;
	return ERR10_018;
    }
    
    ptr = (char *)ARG1(msg) + offset;
    
    *argstring = CreateArgstring((UBYTE *)ptr, sizeof(void *));
    return RC_OK;
}
