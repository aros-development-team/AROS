/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rexx stub for AllocMem system function
    Lang: English
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/exall.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <string.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

AROS_UFH2(void, __putChr,
	  AROS_UFHA(UBYTE, chr, D0),
	  AROS_UFHA(STRPTR *, p, A3))
{
    AROS_USERFUNC_INIT;
    
    *(*p)++ = chr;
    
    AROS_USERFUNC_EXIT;
}

void my_sprintf(struct Library *RexxSupportBase, UBYTE *buffer, UBYTE *format, ...)
{
    RawDoFmt(format, &format+1, (VOID_FUNC)__putChr, &buffer);
}

LONG rxsupp_statef(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    BPTR lock;
    UBYTE *string;
    
    lock = Lock(RXARG(msg,1), ACCESS_READ);
    if (lock == NULL)
    {
	*argstring = CreateArgstring("", 0);
	return RC_OK;
    }
    else
    {
	struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
	if (fib == NULL)
	{
	    *argstring = NULL;
	    return ERR10_003;
	}
	Examine(lock, fib);
	string = AllocMem(1024, MEMF_ANY);
	my_sprintf(RexxSupportBase, string, "%s %ld %ld %s%s%s%s%s%s%s%s %ld %ld %ld",
		  fib->fib_DirEntryType<0 ? "FILE" : "DIR",
		  fib->fib_Size,
		  fib->fib_NumBlocks,
		  fib->fib_Protection & 1<<8         ? "H" : "-",
		  fib->fib_Protection & FIBF_SCRIPT  ? "S" : "-",
		  fib->fib_Protection & FIBF_PURE    ? "P" : "-",
		  fib->fib_Protection & FIBF_ARCHIVE ? "A" : "-",
		  fib->fib_Protection & FIBF_READ    ? "-" : "R",
		  fib->fib_Protection & FIBF_WRITE   ? "-" : "W",
		  fib->fib_Protection & FIBF_EXECUTE ? "-" : "E",
		  fib->fib_Protection & FIBF_DELETE  ? "-" : "D",
		  fib->fib_Date.ds_Days,
		  fib->fib_Date.ds_Minute,
		  fib->fib_Date.ds_Tick);
	FreeDosObject(DOS_FIB, fib);
    }
    
    *argstring = CreateArgstring(string, strlen(string));
    FreeMem(string, 1024);
    return RC_OK;
}
