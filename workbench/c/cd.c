/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Cd CLI command
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: cd 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    BPTR dir,newdir;
    STRPTR buf;
    ULONG i;
    struct FileInfoBlock *fib;
    LONG error=0;

    rda=ReadArgs("DIR",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	if(args[0])
	{
	    dir=Lock(args[0],SHARED_LOCK);
	    if(dir)
	    {
		fib=AllocDosObject(DOS_FIB,NULL);
		if(fib!=NULL)
		{
		    if(Examine(dir,fib))
		    {
			if(fib->fib_DirEntryType>0)
			{
			    newdir=dir;
			    dir=CurrentDir(newdir);
			    for(i=256;;i+=256)
			    {
				buf=AllocVec(i,MEMF_ANY);
				if(buf==NULL)
				{
				    SetIoErr(ERROR_NO_FREE_STORE);
				    error=RETURN_ERROR;
				    break;
				}
				if(NameFromLock(newdir,buf,i))
				{
				    SetCurrentDirName(buf);
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
			}
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
