/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Makedir CLI command
    Lang: English
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: makedir 41.4 (12.11.2000)\n";

/* TODO:  INSERT documentation here */

/* HISTORY:  12 November 2000,  SDuvan  --  Implmented multiple target support,
                                            rewrote to live up to AmigaDOS
					    semantics */

enum
{
    ARG_NAME = 0,
    NOOFARGS
};


int main(int argc, char **argv)
{
    IPTR           args[NOOFARGS] = { NULL };
    struct RDArgs *rda;
    
    LONG   error = RETURN_OK;
    BPTR   lock;
    
    rda = ReadArgs("NAME/M", args, NULL);

    if(rda != NULL)
    {
	int      i = 0;
	STRPTR  *name = (STRPTR *)args[ARG_NAME];

	if(*name == NULL)
	{
	    printf("No name given\n");
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
		    printf("Cannot create directory %s\n", name[i]);
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
