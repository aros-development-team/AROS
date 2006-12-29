/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rexx stub for AllocMem system function
    Lang: English
*/

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

LONG rxsupp_showdir(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    UBYTE argc = msg->rm_Action & RXARGMASK;
    UBYTE type = 0; /* 0 == all, 1 == file, 2 == dir */
    char delim = 0;
    BPTR lock;
    UBYTE *string;
    ULONG ssize;
    struct ExAllData *buffer, *exalldata;
    struct ExAllControl *exallctrl;
    BOOL done;
    
    switch (argc)
    {
    case 1:
        type = 0;
        delim = ' ';
        break;
      
    case 3:
	if (RXARG(msg,3) == NULL || LengthArgstring(RXARG(msg,3)) != 0)
	    delim = RXARG(msg,3)[0];
    case 2:
        if (RXARG(msg,2) == NULL || LengthArgstring(RXARG(msg,2)) == 0)
	    type = 0;
        else
	    switch (tolower(RXARG(msg,2)[0]))
	    {
	    case 'a':
		type = 0;
		break;

	    case 'f':
		type = 1;
		break;
	    
	    case 'd':
		type = 2;
		break;
	    
	    default:
		*argstring = NULL;
		return ERR10_018;
	    }
	if (delim == 0)
	    delim = ' ';
	break;
    }

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
	if (fib->fib_DirEntryType<0)
	{
	    FreeDosObject(DOS_FIB, fib);
	    *argstring = CreateArgstring("", 0);
	    return RC_OK;
	}
	FreeDosObject(DOS_FIB, fib);
    }
    
    buffer = AllocMem(1024, MEMF_ANY);
    ssize = 1024;
    string = AllocMem(ssize, MEMF_ANY);
    string[0] = 0;
    exallctrl = AllocDosObject(DOS_EXALLCONTROL, NULL);
    exallctrl->eac_LastKey = 0;
    do {
	done = ExAll(lock, buffer, 1024, ED_TYPE, exallctrl);
	if (exallctrl->eac_Entries>0)
	{
	    exalldata = (struct ExAllData *)buffer;
	    while (exalldata != NULL)
	    {
		if (type == 0 ||
		    (type == 1 && exalldata->ed_Type < 0) ||
		    (type == 2 && exalldata->ed_Type >= 0))
		{
		    ULONG newlen = strlen(string) + strlen(exalldata->ed_Name) + 2, len = strlen(string);
		    if (newlen > ssize)
		    {
			UBYTE *oldstring = string;
			ULONG oldsize = ssize;
			ssize = ((newlen/1024)+1)*1024;
			string = AllocMem(ssize, MEMF_ANY);
			strcpy(string, oldstring);
			FreeMem(oldstring, oldsize);
		    }
		    if (len>0)
		    {
			string[len] = delim;
			string[len+1] = 0;
		    }
		    strcat(string, exalldata->ed_Name);
		}
		exalldata = exalldata->ed_Next;
	    }
	}
    } while(!done);
    
    *argstring = CreateArgstring(string, strlen(string));
    FreeMem(buffer, 1024);
    FreeMem(string, ssize);
    FreeDosObject(DOS_EXALLCONTROL, exallctrl);
    return RC_OK;
}
