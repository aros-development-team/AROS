/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: allocmem.c 15089 2002-08-04 19:48:12Z verhaegs $

    Desc: Add offset to a pointer
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

LONG rxsupp_offset(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char *ptr, *end;
    LONG offset;

    *argstring = NULL;
    
    offset = strtol(ARG2(msg), &end, 10);
    while (isspace(*end)) end++;
    if (*end != 0)
	return ERR10_018;

    
    if (LengthArgstring(ARG1(msg)) != sizeof(void *))
	return ERR10_018;

    ptr = *(char **)ARG1(msg) + offset;
    
    *argstring = CreateArgstring((UBYTE *)&ptr, sizeof(void *));
    return RC_OK;
}
