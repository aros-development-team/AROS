#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

int main (int argc, char ** argv)
{
    STRPTR args[2]={ NULL, NULL };
    struct RDArgs *rda;
    struct IOFileSys *iofs;
    struct Process *me=(struct Process *)FindTask(NULL);
    LONG error=0;

    rda=ReadArgs("FILESYS/A,DEVICE/A",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	iofs=(struct IOFileSys *)CreateIORequest(&me->pr_MsgPort,sizeof(struct IOFileSys));
	if(iofs!=NULL)
	{
	    if(!OpenDevice(args[0],0,&iofs->IOFS,0))
	    {
		iofs->IOFS.io_Command=FSA_MOUNT;
		iofs->io_Args[0]=(IPTR)args[1];
		iofs->io_Args[1]=0;
		if (DoIO(&iofs->IOFS))
		{
		    VPrintf ("Mount of %s: failed\n", (ULONG *)&args[1]);
		    error = RETURN_FAIL;
		    PrintFault(IoErr(),"Mount");
		}

		/*
		    Don't close the device so that it has a non-zero
		    open count as long as it is mounted. (Whoever wants
		    to dismount it must close it as well).
		    CloseDevice(&iofs->IOFS);
		*/
	    }
	    else
	    {
		VPrintf ("OpenDevice (%s) failed\n", (ULONG *)args);
		error = RETURN_FAIL;
		PrintFault(IoErr(),"Mount");
	    }

	    DeleteIORequest(&iofs->IOFS);
	}
	else
	    error = RETURN_FAIL;

	FreeArgs(rda);
    }
    else
    {
	error=RETURN_FAIL;
	PrintFault(IoErr(),"Mount");
    }

    return error;
}
