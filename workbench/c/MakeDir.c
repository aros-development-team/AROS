/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    MakeDir CLI command.
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <stdio.h>

static const char version[] = "$VER: MakeDir 41.4 (12.11.2000)\n";

/******************************************************************************

    NAME

        MakeDir

    SYNOPSIS

        NAME/M

    LOCATION

        Workbench:C

    FUNCTION

        Create new empty directories with specified names.

    INPUTS

        NAME  --  names of the directories that should be created

    RESULT

    NOTES

        MakeDir does not create an icon for a new directory.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/


enum
{
    ARG_NAME = 0,
    NOOFARGS
};


int __nocommandline;

int main(void)
{
    IPTR           args[NOOFARGS] = { (IPTR) NULL };
    struct RDArgs *rda;
    
    LONG   error = RETURN_OK;
    BPTR   lock;
    
    rda = ReadArgs("NAME/M", args, NULL);

    if(rda != NULL)
    {
	int      i = 0;
	STRPTR  *name = (STRPTR *)args[ARG_NAME];

	if((name == NULL) || (*name == NULL))
	{
	    PutStr("No name given\n");
	    error = RETURN_FAIL;
	}
	else
	{
	    for(i = 0; name[i] != NULL; i++)
	    {
		lock = CreateDir(name[i]);

		/* The AmigaDOS semantics are quite strange here. When it is
		   impossible to create a certain directory, MakeDir goes on
		   to try to create the rest of the specified directories and
		   returns the LAST return value for the operation. */
		if(lock != NULL)
		{
		    UnLock(lock);
		    error = RETURN_OK;
		}
		else
		{
		    PutStr("Cannot create directory ");
		    PutStr(name[i]);
		    PutStr("\n");
		    error = RETURN_ERROR;
		}
	    }
	}

	FreeArgs(rda);
    }
    else
	error = RETURN_FAIL;

    if(error != RETURN_OK)
	PrintFault(IoErr(), NULL);

    return error;
}
