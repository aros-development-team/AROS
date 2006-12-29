/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: allocmem.c 15089 2002-08-04 19:48:12Z verhaegs $

    Desc: Rename a file
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

LONG rxsupp_rename(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    BOOL ok;

    ok = Rename(ARG1(msg), ARG2(msg));
    *argstring = CreateArgstring(ok ? "1" : "0", 1);
    
    return RC_OK;
}
