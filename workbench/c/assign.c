#include <exec/memory.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

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
