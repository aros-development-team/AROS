#include <exec/memory.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

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
