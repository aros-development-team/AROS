#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

static LONG mount(struct IOFileSys *iofs, STRPTR filesys, STRPTR device)
{
    struct DosList *entry;
    LONG error;
    if(!OpenDevice(filesys,0,&iofs->IOFS,0))
    {
        entry=MakeDosEntry(device,DLT_DEVICE);
        if(entry!=NULL)
        {
            if(AddDosEntry(entry))
            {
                entry->dol_Unit  =iofs->IOFS.io_Unit;
                entry->dol_Device=iofs->IOFS.io_Device;
                /*
                    Neither close the device nor free the DosEntry.
                    Both will stay in the dos list as long as the
                    device is mounted.
                */
		return 0;
	    }else
	        error=IoErr();
	    FreeDosEntry(entry);
	}else
	    error=ERROR_NO_FREE_STORE;
    }else
        error=ERROR_OBJECT_NOT_FOUND;
    return error;
}

int main (int argc, char ** argv)
{
    STRPTR args[2]={ NULL, NULL };
    struct RDArgs *rda;
    struct IOFileSys *iofs;
    struct Process *me=(struct Process *)FindTask(NULL);
    LONG error=0;

    rda=ReadArgs("FILESYS/A,DEVICE/A",(ULONG *)args,NULL);
    if(rda!=NULL)
    {
	iofs=(struct IOFileSys *)CreateIORequest(&me->pr_MsgPort,sizeof(struct IOFileSys));
	if(iofs!=NULL)
	{
            SetIoErr(mount(iofs,args[0],args[1]));
            if(IoErr())
		    error = RETURN_FAIL;
	    DeleteIORequest(&iofs->IOFS);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Mount");
    return error;
}
