#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

CALLENTRY /* Before the first symbol */

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

static LONG tinymain(void);

LONG entry(struct ExecBase *sysbase)
{
    LONG error=RETURN_FAIL;
    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(DOSBase!=NULL)
    {
        error=tinymain();
        CloseLibrary((struct Library *)DOSBase);
    }
    return error;
}

static LONG tinymain(void)
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
            if(!OpenDevice(args[0],0,&iofs->IOFS,0))
            {
                iofs->IOFS.io_Command=FSA_MOUNT;
                iofs->io_Args[0]=(ULONG)args[1];
                iofs->io_Args[1]=0;
                DoIO(&iofs->IOFS);
                /*
                    Don't close the device so that it has a non-zero
                    open count as long as it is mounted. (Whoever wants
                    to dismount it must close it as well).
                    CloseDevice(&iofs->IOFS);
                */
            }
            DeleteIORequest(&iofs->IOFS);
        }
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Mount");
    return error;
}
