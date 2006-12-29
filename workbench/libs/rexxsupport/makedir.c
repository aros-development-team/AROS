/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Make a directory
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

LONG rxsupp_makedir(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    BPTR lock;

    lock = CreateDir(ARG1(msg));
    if (lock == (BPTR)NULL)
	*argstring = CreateArgstring("0", 1);
    else
    {
	UnLock(lock);
	*argstring = CreateArgstring("1", 1);
    }
    
    return RC_OK;
}
