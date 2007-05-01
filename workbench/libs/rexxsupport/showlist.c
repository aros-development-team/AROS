/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rexx stub for AllocMem system function
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <string.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

#ifndef AROS_BSTR_ADDR
#   define AROS_BSTR_ADDR(s) (((STRPTR)BADDR(s))+1)
#endif

LONG rxsupp_showlist(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    UBYTE argc = msg->rm_Action & RXARGMASK;
    BOOL isexec;
    char delim = 0;
    struct List *execl = NULL;
    ULONG dosflags = 0L;
    UBYTE *string, *name = NULL;
    ULONG ssize;

    if (RXARG(msg, 1) == NULL || LengthArgstring(RXARG(msg, 1)) == 0)
    {
	*argstring = NULL;
	return ERR10_018;
    }
    switch (tolower(RXARG(msg, 1)[0]))
    {
    case 'a':
        isexec = FALSE;
	dosflags = LDF_READ | LDF_ASSIGNS;
        break;
      
    case 'd':
	isexec = TRUE;
	execl = &SysBase->DeviceList;
	break;
	
    case 'h':
	isexec = FALSE;
	dosflags = LDF_READ | LDF_DEVICES;
	break;
	
    case 'i':
	isexec = TRUE;
	execl = &SysBase->IntrList;
	break;
	
    case 'l':
	isexec = TRUE;
	execl = &SysBase->LibList;
	break;
	
    case 'm':
	isexec = TRUE;
	execl= &SysBase->MemList;
	break;
	
    case 'p':
	isexec = TRUE;
	execl = &SysBase->PortList;
	break;
	
    case 'r':
	isexec = TRUE;
	execl = &SysBase->ResourceList;
	break;
	
    case 's':
	isexec = TRUE;
	execl = &SysBase->SemaphoreList;
	break;
	
    case 't':
	isexec = TRUE;
	execl = &SysBase->TaskReady;
	break;
	
    case 'v':
	isexec = FALSE;
	dosflags = LDF_READ | LDF_VOLUMES;
	break;
	
    case 'w':
	isexec = TRUE;
	execl = &SysBase->TaskWait;
	break;
	
    default:
	*argstring = NULL;
	return ERR10_018;
    }
    
    if (argc < 2 || RXARG(msg, 2) == NULL)
	name = NULL;
    else
	name = RXARG(msg, 2);
    
    if (argc < 3 || RXARG(msg, 3) ==  NULL || LengthArgstring(RXARG(msg, 3)) == 0)
	delim = ' ';
    else
	delim = RXARG(msg, 3)[0];

    if (name == NULL)
    {
	ssize = 1024;
	string = AllocMem(ssize, MEMF_ANY);
	string[0] = 0;
	if (isexec)
	{
	    struct Node *n;
	    ULONG slen, totlen;
	    
	    Forbid();
	    ForeachNode(execl, n)
	    {
		slen = strlen(string);
		totlen = slen + strlen(n->ln_Name) + 2;
		if (totlen > ssize)
		{
		    ULONG oldsize = ssize;
		    UBYTE *oldstring = string;
		    
		    ssize = ((totlen/1024)+1)*1024;
		    string = AllocMem(ssize, MEMF_ANY);
		    strcpy(string, oldstring);
		    FreeMem(oldstring, oldsize);
		}
		if (slen > 0)
		{
		    string[slen] = delim;
		    string[slen+1] = 0;
		}
		strcat(string, n->ln_Name);
	    }
	    Enable();
	}
	else
	{
	    struct DosList *dosl = LockDosList(dosflags);
	    UBYTE *name;
	    ULONG slen, totlen;
	    
	    while ((dosl = NextDosEntry(dosl, dosflags)) != NULL)
	    {
		name = (STRPTR)AROS_BSTR_ADDR(dosl->dol_Name);
		slen = strlen(string);
		totlen = slen + strlen(name) + 2;
		if (totlen > ssize)
		{
		    ULONG oldsize = ssize;
		    UBYTE *oldstring = string;
		    
		    ssize = ((totlen/1024)+1)*1024;
		    string = AllocMem(ssize, MEMF_ANY);
		    strcpy(string, oldstring);
		    FreeMem(oldstring, oldsize);
		}
		if (slen > 0)
		{
		    string[slen] = delim;
		    string[slen+1] = 0;
		}
		strncat(string, name, *(UBYTE *)dosl->dol_Name);
	    }
	    UnLockDosList(dosflags);
	}
	*argstring = CreateArgstring(string, strlen(string));
	FreeMem(string, ssize);
    }
    else /* name != NULL */
    {
	BOOL found = FALSE;
	
	if (isexec)
	{
	    struct Node *n;
	    
	    Forbid();
	    ForeachNode(execl, n)
	    {
		found = strcmp(name, n->ln_Name)==0;
		if (found)
		    break;
	    }
	    Enable();
	}
	else
	{
	    struct DosList *dosl = LockDosList(dosflags);
	    
	    while(!found && (dosl = NextDosEntry(dosl, dosflags))!=NULL)
		found = strncmp(name, (STRPTR)BADDR(dosl->dol_Name)+1, *(UBYTE *)dosl->dol_Name)==0;

	    UnLockDosList(dosflags);
	}
	if (found)
	    *argstring = CreateArgstring("1",1);
	else
	    *argstring = CreateArgstring("0",1);
    }
    
    return RC_OK;
}
