/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Convert BCPL address to C pointer
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

LONG rxsupp_baddr(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    void *ptr;

    *argstring = NULL;
    
    if (LengthArgstring(ARG1(msg)) != sizeof(void *))
	return ERR10_018;
    
    ptr = BADDR(*(BPTR *)ARG1(msg));
    
    *argstring = CreateArgstring((UBYTE *)&ptr, sizeof(void *));
    return RC_OK;
}
