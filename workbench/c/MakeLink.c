/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************


    NAME

        MakeLink

    SYNOPSIS

        FROM/A, TO/A, HARD/S, FORCE/S

    LOCATION

        Workbench:C

    FUNCTION

        Create a link to a file

    INPUTS

        FROM   --  The name of the link
	TO     --  The name of the file or directory to link to
	HARD   --  If specified, the link will be a hard-link; default is
	           to create a soft-link
	FORCE  --  Allow a hard-link to point to a directory

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    04.05.2000  SDuvan  implemented

******************************************************************************/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>

const TEXT version[] = "$VER: MakeLink 41.1 (2.6.2000)\n";

enum { ARG_FROM = 0, ARG_TO, ARG_HARD, ARG_FORCE };

int __nocommandline;

int main(void)
{
    int  retval = RETURN_FAIL;
    IPTR args[] = { NULL, NULL, (IPTR)FALSE, (IPTR)FALSE };
    struct RDArgs *rda;
	
    rda = ReadArgs("FROM/A,TO/A,HARD/S,FORCE/S", args, NULL);
    
    if(rda != NULL)
    {
	BPTR  lock;

	lock = Lock((STRPTR)args[ARG_TO], SHARED_LOCK);

	if(lock != NULL)
	{
	    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

	    if(fib != NULL)
	    {
		if(Examine(lock, fib) == DOSTRUE)
		{
		    /* Directories may only be hard-linked to if FORCE is
		       specified */
		    if(fib->fib_DirEntryType >= 0 &&
		       !(BOOL)args[ARG_FORCE] && (BOOL)args[ARG_HARD])
		    {
			PutStr("Hard-links to directories require the FORCE"
			       "keyword\n");
		    }
		    else
		    {
			/* Check loops? */
			if(MakeLink((STRPTR)args[ARG_FROM],
				    (BOOL)args[ARG_HARD] ? lock :
				    (STRPTR)args[ARG_TO],
				    !(BOOL)args[ARG_HARD]) == DOSTRUE)
			    retval = RETURN_OK;
			else
			    PrintFault(IoErr(), "MakeLink");
		    }
		}   
		
		FreeDosObject(DOS_FIB, fib);
	    }
	    
	    UnLock(lock);
	}
	else
	{
	    PutStr((STRPTR)args[ARG_TO]);
	    PrintFault(IoErr(), "");
	}
    }
    else
    {
	PrintFault(IoErr(), "MakeLink");
	retval = RETURN_FAIL;
    }
    
    FreeArgs(rda);
    
    return retval;
}
