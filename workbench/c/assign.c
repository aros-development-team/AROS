/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Assign CLI command
    Lang: english
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: assign 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    STRPTR args[2]={ NULL, NULL };
    struct RDArgs *rda;
    BPTR dir;
    LONG error=0;

    rda=ReadArgs("DEVICE/A,DIR/A",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	dir=Lock(args[1],SHARED_LOCK);
	if(dir)
	{
	    STRPTR s=args[0];
	    while(*s)
		if(*s++==':')
		    s[-1]=0;
	    AssignLock(args[0],dir);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Assign");
    return error;
}
