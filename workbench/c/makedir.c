/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Makedir CLI command
    Lang: english
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: makedir 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    LONG error=0;
    BPTR lock;

    rda=ReadArgs("DIR/A",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	lock = CreateDir(args[0]);

	if (lock)
	    UnLock(lock);
	else
	{
	    VPrintf ("Cannot create %s:", (ULONG *)args);
	    error = RETURN_FAIL;
	}

	FreeArgs(rda);
    }
    else
	error=RETURN_FAIL;

    if(error)
	PrintFault(IoErr(),"MakeDir");
    return error;
}
