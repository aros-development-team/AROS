#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
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
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    BPTR dir;
    STRPTR buf;
    ULONG i;
    struct FileInfoBlock *fib;
    LONG error=0;
    
    rda=ReadArgs("DIR",(ULONG *)args,NULL);
    if(rda!=NULL)
    {
	if(args[0])
	{
	    dir=Lock(args[0],ACCESS_READ);
	    if(dir)
	    {
		fib=AllocDosObject(DOS_FIB,NULL);
		if(fib!=NULL)
		{
		    if(Examine(dir,fib))
		    {
			if(fib->fib_DirEntryType>0)
			    dir=CurrentDir(dir);
			else
			{
			    SetIoErr(ERROR_DIR_NOT_FOUND);
			    error=RETURN_ERROR;
			}
		    }else
			error=RETURN_ERROR;
		    FreeDosObject(DOS_FIB,fib);
		}else
		{
		    SetIoErr(ERROR_NO_FREE_STORE);
		    error=RETURN_ERROR;
		}
		UnLock(dir);
	    }else
		error=RETURN_ERROR;
	}else
	{
	    dir=CurrentDir(0);
	    for(i=256;;i+=256)
	    {
		buf=AllocVec(i,MEMF_ANY);
		if(buf==NULL)
		{
		    SetIoErr(ERROR_NO_FREE_STORE);
		    error=RETURN_ERROR;
		    break;
		}
		if(NameFromLock(dir,buf,i))
		{
		    if(FPuts(Output(),buf)<0||
		       FPuts(Output(),"\n")<0)
			error=RETURN_ERROR;
		    FreeVec(buf);
		    break;
		}
		FreeVec(buf);
		if(IoErr()!=ERROR_LINE_TOO_LONG)
		{
		    error=RETURN_ERROR;
		    break;
		}
	    }
	    CurrentDir(dir);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"CD");
    return error;
}