/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>

static const char version[] = "$VER: wait 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    IPTR args[4]={ 0, 0, 0, 0 };
    struct RDArgs *rda;
    LONG error=0;
    ULONG delay = 1;
#define ERROR(a) { error=a; goto end; }

    rda=ReadArgs("time/N,SEC=SECS/S,MIN=MINS/S,UNTIL/K",args,NULL);
    if(rda==NULL)
	ERROR(RETURN_FAIL);

    if (args[0])
	delay = *((ULONG *)args[0]);

    if (args[2])
	delay *= 60L;

    if (delay > 0)
	Delay (delay * 50L);

end:
    if (rda)
        FreeArgs(rda);
    if(error)
	PrintFault(IoErr(),"Wait");
    return error;
}
